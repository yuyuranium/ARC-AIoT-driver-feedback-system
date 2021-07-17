#include "accelerometer_handler.h"
#include "constants.h"
#include "hx_drv_tflm.h"
#include <cstdio>

namespace {
// Beta of LPF
const float kLPFBeta = 0.25;
// Sampling specifications
const float kSamplingPeriod = 0.04;
const uint32_t kClkRate = 400000000;
const uint32_t kSamplingCycle = kSamplingPeriod*kClkRate;
// Ring buffer size
constexpr int kRingBufferSize = 180;  // 6 * 60
// Ring buffer
int8_t ring_buffer[kRingBufferSize] = {0};
// Available data count in accelerometer FIFO
int available_count = 0;
// The head of the ring buffer
int begin_index;
// Raw data of accelerometer
float raw_ax, raw_ay, raw_az;
// Timer
uint32_t tick_now, tick_last;
// Float data of accel and jerk
float accel[3], accel_last[3], jerk[3];
// Quantization parameters
int32_t zero_point;
float scale;

}  // namespace

TfLiteStatus SetupAccelerometer(tflite::ErrorReporter *error_reporter,
                                int32_t input_zero_point, float input_scale) {
  TF_LITE_REPORT_ERROR(error_reporter, "setting up acclerometer");
  if (hx_drv_accelerometer_initial() != HX_DRV_LIB_PASS) {
    TF_LITE_REPORT_ERROR(error_reporter, "setup fail");
    return kTfLiteError;
  }

  // setting up quantization parameters
  zero_point = input_zero_point;
  scale = input_scale;
  // begin with position 0 of buffer
  begin_index = 0;
  TF_LITE_REPORT_ERROR(error_reporter, "setup done");
  return kTfLiteOk;
}

bool ReadAccelerometer(tflite::ErrorReporter *error_reporter, int8_t *input,
                       int length) {
  available_count = hx_drv_accelerometer_available_count();				

  for (int i = 0; i < available_count; ++i) {
    hx_drv_accelerometer_receive(&raw_ax, &raw_ay, &raw_az);
  }
  hx_drv_tick_get(&tick_now);  // always read the latest cc

  // to keep the same sampling rate, i.e., the same dt
  if (tick_now - tick_last < kSamplingCycle)
    return false;  // wait until we have enough long dt

  // not until we have long enough dt will the control flow goes here

  // data passes LPF first
  // LPF: Y(n) = (1-ß)*Y(n-1) + ß*X(n) = Y(n-1) - ß(Y(n-1)-X(n));
  accel[X] = accel[X] - (kLPFBeta * (accel[X] - raw_ax));
  accel[Y] = accel[Y] - (kLPFBeta * (accel[Y] - raw_ay));
  accel[Z] = accel[Z] - (kLPFBeta * (accel[Z] - raw_az));

  // calculating jerk of x, y, z
  float dt = (float)(tick_now - tick_last) / (float)kClkRate;
  jerk[X] = (accel[X] - accel_last[X]) / dt;
  jerk[Y] = (accel[Y] - accel_last[Y]) / dt;
  jerk[Z] = (accel[Z] - accel_last[Z]) / dt;

  // normalizing data
  const float norm_ax = (accel[X] - kAccelMean[X]) / kAccelStd[X];
  const float norm_ay = (accel[Y] - kAccelMean[Y]) / kAccelStd[Y];
  const float norm_az = (accel[Z] - kAccelMean[Z]) / kAccelStd[Z];
  const float norm_jx = (jerk[X] - kJerkMean[X]) / kJerkStd[X];
  const float norm_jy = (jerk[Y] - kJerkMean[Y]) / kJerkStd[Y];
  const float norm_jz = (jerk[Z] - kJerkMean[Z]) / kJerkStd[Z];

  // quantization of data
  const int8_t quant_ax = ((int8_t)(norm_ax / scale)) + (int8_t)zero_point;
  const int8_t quant_ay = ((int8_t)(norm_ay / scale)) + (int8_t)zero_point;
  const int8_t quant_az = ((int8_t)(norm_az / scale)) + (int8_t)zero_point;
  const int8_t quant_jx = ((int8_t)(norm_jx / scale)) + (int8_t)zero_point;
  const int8_t quant_jy = ((int8_t)(norm_jy / scale)) + (int8_t)zero_point;
  const int8_t quant_jz = ((int8_t)(norm_jz / scale)) + (int8_t)zero_point;

  ring_buffer[begin_index++] = quant_ax;
  ring_buffer[begin_index++] = quant_ay;
  ring_buffer[begin_index++] = quant_az;
  ring_buffer[begin_index++] = quant_jx;
  ring_buffer[begin_index++] = quant_jy;
  ring_buffer[begin_index++] = quant_jz;

  // If reach end of buffer, return to 0 position
  if (begin_index >= kRingBufferSize) begin_index = 0;

  // store the current data for next cycle
  tick_last = tick_now;
  accel_last[X] = accel[X];
  accel_last[Y] = accel[Y];
  accel_last[Z] = accel[Z];

  for (int i = 0; i < length; ++i) {
    int ring_index = begin_index - length + i;
    if (ring_index < 0)
      ring_index += kRingBufferSize;
    // copy stored data to the input tensor
    input[i] = ring_buffer[ring_index];
  }

  return true;
}
