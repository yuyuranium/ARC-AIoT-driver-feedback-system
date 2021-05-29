#include "accelerometer_handler.h"

#include "hx_drv_tflm.h"

#include <cstdio>

namespace {
// Ring buffer size
constexpr int kRingBufferSize = 90;  // 3 * 30
// Ring buffer
int8_t save_data[kRingBufferSize] = {0};
// Available data count in accelerometer FIFO
int available_count = 0;

int begin_index;
}  // namespace

TfLiteStatus SetupAccelerometer(tflite::ErrorReporter* error_reporter) {
  TF_LITE_REPORT_ERROR(error_reporter, "setting up acclerometer");
  if (hx_drv_accelerometer_initial() != HX_DRV_LIB_PASS) {
    TF_LITE_REPORT_ERROR(error_reporter, "setup fail");
    return kTfLiteError;
  }

  TF_LITE_REPORT_ERROR(error_reporter, "setup done");

  begin_index = 0;

  return kTfLiteOk;
}

bool ReadAccelerometer(tflite::ErrorReporter* error_reporter,
                       uint32_t zero_point, float scale, int8_t* input,
                       int length) {
  float x, y, z;

  available_count = hx_drv_accelerometer_available_count();				
  for (int i = 0; i < available_count; ++i) {
    hx_drv_accelerometer_receive(&x, &y, &z);
    x *= 32.0f;  // Scale that we set at labeling time
    y *= 32.0f;
    z *= 32.0f;
    save_data[begin_index++] = ((int8_t)(x / scale)) + (int8_t)zero_point;
    save_data[begin_index++] = ((int8_t)(y / scale)) + (int8_t)zero_point;
    save_data[begin_index++] = ((int8_t)(z / scale)) + (int8_t)zero_point;
    if (begin_index == kRingBufferSize)
      begin_index = 0;
  }

  for (int i = 0; i < length; ++i) {
    int ring_index = begin_index - length + i;
    if (ring_index < 0)
      ring_index += kRingBufferSize;
    input[i] = save_data[ring_index];
  }

  return true;
}
