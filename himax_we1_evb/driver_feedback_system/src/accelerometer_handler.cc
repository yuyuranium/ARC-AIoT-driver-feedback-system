#include "accelerometer_handler.h"
#include "constants.h"
#include "hx_drv_tflm.h"
#include <cstdio>

namespace {
// Beta of LPF
const float kLPFBeta = 0.25;
// Sampling parameters
const float kSamplingPeriod = 0.04;
const uint32_t kClkRate = 400000000;
const uint32_t kSamplingCycle = kSamplingPeriod*kClkRate;
// Ring buffer size
constexpr int kRingBufferSize = 180;  // 6 * 60
// Ring buffers for both models
int8_t ring_buffer_c[kRingBufferSize] = {0};
int8_t ring_buffer_p[kRingBufferSize] = {0};
// Available data count in accelerometer FIFO
int available_count = 0;
// The head of the ring buffer
int begin_index;
// Raw data of accelerometer
float raw_ax, raw_ay, raw_az; // Timer
uint32_t tick_now, tick_last;
// Float data of accel and jerk
float accel[3], accel_last[3], jerk[3];
// Quantization parameters of both models' input
int32_t zero_point_c;
int32_t zero_point_p;
float scale_c;
float scale_p;
}  // namespace

TfLiteStatus SetupAccelerometer(tflite::ErrorReporter *error_reporter,
                                int32_t input_zero_point_c,
                                float input_scale_c,
                                int32_t input_zero_point_p,
                                float input_scale_p) {
  TF_LITE_REPORT_ERROR(error_reporter, "setting up acclerometer");
  if (hx_drv_accelerometer_initial() != HX_DRV_LIB_PASS) {
    TF_LITE_REPORT_ERROR(error_reporter, "setup fail");
    return kTfLiteError;
  }

  // Setting up quantization parameters
  zero_point_c = input_zero_point_c;
  scale_c = input_scale_c;
  zero_point_p = input_zero_point_p;
  scale_p = input_scale_p;
  // Begin with position 0 of buffer
  begin_index = 0;
  TF_LITE_REPORT_ERROR(error_reporter, "setup done");
  return kTfLiteOk;
}

bool ReadAccelerometer(tflite::ErrorReporter *error_reporter,
                       int8_t *input_c, int8_t *input_p, int length) {
  available_count = hx_drv_accelerometer_available_count();				

  for (int i = 0; i < available_count; ++i) {
    hx_drv_accelerometer_receive(&raw_ax, &raw_ay, &raw_az);
  }
  hx_drv_tick_get(&tick_now);  // always read the latest cc

  // To keep the same sampling rate, i.e., the same dt
  if (tick_now - tick_last < kSamplingCycle)
    return false;  // wait until we have enough long dt

  // Not until we have long enough dt will the control flow goes here

  // Data passes LPF first
  // LPF: Y(n) = (1-ß)*Y(n-1) + ß*X(n) = Y(n-1) - ß(Y(n-1)-X(n));
  accel[X] = accel[X] - (kLPFBeta * (accel[X] - raw_ax));
  accel[Y] = accel[Y] - (kLPFBeta * (accel[Y] - raw_ay));
  accel[Z] = accel[Z] - (kLPFBeta * (accel[Z] - raw_az));

  // Calculating jerk of x, y, z
  const float dt = (float)(tick_now - tick_last) / (float)kClkRate;
  jerk[X] = (accel[X] - accel_last[X]) / dt;
  jerk[Y] = (accel[Y] - accel_last[Y]) / dt;
  jerk[Z] = (accel[Z] - accel_last[Z]) / dt;

  // Normalizing data
  const float norm_ax = (accel[X] - kAccelMean[X]) / kAccelStd[X];
  const float norm_ay = (accel[Y] - kAccelMean[Y]) / kAccelStd[Y];
  const float norm_az = (accel[Z] - kAccelMean[Z]) / kAccelStd[Z];
  const float norm_jx = (jerk[X] - kJerkMean[X]) / kJerkStd[X];
  const float norm_jy = (jerk[Y] - kJerkMean[Y]) / kJerkStd[Y];
  const float norm_jz = (jerk[Z] - kJerkMean[Z]) / kJerkStd[Z];

  // Quantization of data
  const int8_t quant_ax_c = (int8_t)(norm_ax / scale_c) + (int8_t)zero_point_c;
  const int8_t quant_ay_c = (int8_t)(norm_ay / scale_c) + (int8_t)zero_point_c;
  const int8_t quant_az_c = (int8_t)(norm_az / scale_c) + (int8_t)zero_point_c;
  const int8_t quant_jx_c = (int8_t)(norm_jx / scale_c) + (int8_t)zero_point_c;
  const int8_t quant_jy_c = (int8_t)(norm_jy / scale_c) + (int8_t)zero_point_c;
  const int8_t quant_jz_c = (int8_t)(norm_jz / scale_c) + (int8_t)zero_point_c;

  const int8_t quant_ax_p = (int8_t)(norm_ax / scale_p) + (int8_t)zero_point_p;
  const int8_t quant_ay_p = (int8_t)(norm_ay / scale_p) + (int8_t)zero_point_p;
  const int8_t quant_az_p = (int8_t)(norm_az / scale_p) + (int8_t)zero_point_p;
  const int8_t quant_jx_p = (int8_t)(norm_jx / scale_p) + (int8_t)zero_point_p;
  const int8_t quant_jy_p = (int8_t)(norm_jy / scale_p) + (int8_t)zero_point_p;
  const int8_t quant_jz_p = (int8_t)(norm_jz / scale_p) + (int8_t)zero_point_p;
  
  ring_buffer_p[begin_index] = quant_ax_p;
  ring_buffer_c[begin_index++] = quant_ax_c;
  ring_buffer_p[begin_index] = quant_ay_p;
  ring_buffer_c[begin_index++] = quant_ay_c;
  ring_buffer_p[begin_index] = quant_az_p;
  ring_buffer_c[begin_index++] = quant_az_c;
  ring_buffer_p[begin_index] = quant_jx_p;
  ring_buffer_c[begin_index++] = quant_jx_c;
  ring_buffer_p[begin_index] = quant_jy_p;
  ring_buffer_c[begin_index++] = quant_jy_c;
  ring_buffer_p[begin_index] = quant_jz_p;
  ring_buffer_c[begin_index++] = quant_jz_c;

  // If reach end of buffer, return to 0 position
  if (begin_index >= kRingBufferSize) begin_index = 0;

  // Store the current data for next cycle
  tick_last = tick_now;
  accel_last[X] = accel[X];
  accel_last[Y] = accel[Y];
  accel_last[Z] = accel[Z];

  for (int i = 0; i < length; ++i) {
    int ring_index = begin_index - length + i;
    if (ring_index < 0)
      ring_index += kRingBufferSize;
    // Copy stored data to the input tensor
    input_c[i] = ring_buffer_c[ring_index];
    input_p[i] = ring_buffer_p[ring_index];
  }

  return true;
}

void GetLatestData(tflite::ErrorReporter *error_reporter, float *latest_data,
                   int length) {
  for (int i = 0; i < length; ++i) {
    int ring_index = begin_index - length + i;
    if (ring_index < 0)
      ring_index += kRingBufferSize;
    // Copy and convert stored data to the input array
    latest_data[i] = (ring_buffer_p[ring_index] - zero_point_p) * scale_p;
  }
}
