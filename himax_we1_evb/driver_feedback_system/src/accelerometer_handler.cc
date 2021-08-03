#include "accelerometer_handler.h"
#include "constants.h"
#include "hx_drv_tflm.h"
#include <cmath>

namespace {
// Beta of LPF
constexpr float kLPFBeta = 0.25;
// Sampling parameters
constexpr float kSamplingPeriod = 0.025;
constexpr uint32_t kClkRate = 400000000;
constexpr uint32_t kSamplingCycle = kSamplingPeriod*kClkRate;
// Normalization parameters
float accel_mean[3] = {
  0.9487810621465815, 0.03946548768500278, 0.06886678821127502 };
float jerk_mean[3] = {
  -0.00326175719059994, 0.001484422197827616, 0.009953398211114467 };
float accel_std[3] = {
  0.1313167093450731, 0.11926362940605395, 0.17273308090512682 };
float jerk_std[3] = {
  2.3153819130934132, 1.803095985733507, 2.2968154997810615 };
uint32_t calibration_count = 0;
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
float raw_ax, raw_ay, raw_az;
// Timer
uint32_t tick_now, tick_last;
// Float data of accel and jerk
float accel[3] = {0.0}, accel_last[3] = {0.0}, jerk[3] = {0.0};
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

void UpdateMeanAndStd(tflite::ErrorReporter *error_reporter, float *mean, float *std) {
  constexpr uint32_t kBatchSize = 90;
  float accel_buf[3][kBatchSize], jerk_buf[3][kBatchSize];

  // Firstly, collect "kBatchSize" of data with the same LPF and sampling rate
  for (uint32_t i = 0; i < kBatchSize; ++i) {
    // To keep the same sampling rate, i.e., the same dt
    do {
      available_count = hx_drv_accelerometer_available_count();				
      for (int j = 0; j < available_count; ++j) {
        hx_drv_accelerometer_receive(&raw_ax, &raw_ay, &raw_az);
      }
      hx_drv_tick_get(&tick_now);  // always read the latest cc
    } while (tick_now - tick_last < kSamplingCycle);  // Non-blocking delay

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

    accel_buf[X][i] = accel[X];
    accel_buf[Y][i] = accel[Y];
    accel_buf[Z][i] = accel[Z];
    jerk_buf[X][i]= jerk[X];
    jerk_buf[Y][i]= jerk[Y];
    jerk_buf[Z][i]= jerk[Z];

    // Store the current data for next cycle
    tick_last = tick_now;
    accel_last[X] = accel[X];
    accel_last[Y] = accel[Y];
    accel_last[Z] = accel[Z];
  }

  // Secondly, calculate mean and std of the calibration batch 
  float batch_accel_mean[3] = {0.0}, batch_jerk_mean[3] = {0.0};
  float batch_accel_var[3] = {0.0}, batch_jerk_var[3] = {0.0};

  // Do it for each axis respectively
  for (int i = 0; i < 3; ++i) {
    // Calculating means
    for (uint32_t j = 0; j < kBatchSize; ++j) {
      batch_accel_mean[i] += accel_buf[i][j];
      batch_jerk_mean[i] += jerk_buf[i][j];
    }
    batch_accel_mean[i] /= kBatchSize;
    batch_jerk_mean[i] /= kBatchSize;

    // Calculating variance of each axis 
    for (uint32_t j = 0; j < kBatchSize; ++j) {
      batch_accel_var[i] += powf(accel_buf[i][j] - batch_accel_mean[i], 2);
      batch_jerk_var[i] += powf(jerk_buf[i][j] - batch_jerk_mean[i], 2);
    }
    batch_accel_var[i] = batch_accel_var[i] / kBatchSize;
    batch_jerk_var[i] = batch_jerk_var[i] / kBatchSize;
  }

  // Update the current mean and std
  // For mean, it is the linear combination of the new one and the old one
  uint32_t total_count = calibration_count + kBatchSize;
  for (int i = 0; i < 3; ++i) {
    accel_std[i] = sqrtf(
        (calibration_count * powf(accel_std[i], 2)
        + kBatchSize * batch_accel_var[i]
        + calibration_count * kBatchSize * powf(
            accel_mean[i] - batch_accel_mean[i], 2) / total_count)
        / total_count);
    jerk_std[i] = sqrtf(
        (calibration_count * powf(jerk_std[i], 2)
        + kBatchSize * batch_jerk_var[i]
        + calibration_count * kBatchSize * powf(
            jerk_mean[i] - batch_jerk_mean[i], 2) / total_count)
        / total_count);
    accel_mean[i] = (calibration_count * accel_mean[i]
        + kBatchSize * batch_accel_mean[i]) / total_count;
    jerk_mean[i] = (calibration_count * jerk_mean[i]
        + kBatchSize * batch_jerk_mean[i]) / total_count;
  }
  calibration_count = total_count;
  mean[0] = accel_mean[0];
  mean[1] = accel_mean[1];
  mean[2] = accel_mean[2];
  mean[3] = jerk_mean[0];
  mean[4] = jerk_mean[1];
  mean[5] = jerk_mean[2];
  std[0] = accel_std[0];
  std[1] = accel_std[1];
  std[2] = accel_std[2];
  std[3] = jerk_std[0];
  std[4] = jerk_std[1];
  std[5] = jerk_std[2];
}

bool ReadAccelerometer(int8_t *input_c, int8_t *input_p, int length) {
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

void GetLatestData(float *latest_data, int length) {
  for (int i = 0; i < length; ++i) {
    int ring_index = begin_index - length + i;
    if (ring_index < 0)
      ring_index += kRingBufferSize;
    // Copy and convert stored data to the input array
    latest_data[i] = (ring_buffer_p[ring_index] - zero_point_p) * scale_p;
  }
}

void GetLatestAccel(float *latest_accel) {
  latest_accel[X] = (accel[X] - kAccelMean[X]) / kAccelStd[X];
  latest_accel[Y] = (accel[Y] - kAccelMean[Y]) / kAccelStd[Y];
  latest_accel[Z] = (accel[Z] - kAccelMean[Z]) / kAccelStd[Z];
}
