#include "i2c_output_handler.h"

#include "hx_drv_tflm.h"

namespace {
// Address of arduino
constexpr uint8_t kArduinoAddr = 0x12;
}  // namespace

TfLiteStatus SetupI2c(tflite::ErrorReporter* error_reporter) {
  if (hx_drv_share_switch(SHARE_MODE_I2CM) != HX_DRV_LIB_PASS) {
    TF_LITE_REPORT_ERROR(error_reporter, "setup fail");
    return kTfLiteError;
  }
  
  TF_LITE_REPORT_ERROR(error_reporter, "setup done");
  return kTfLiteOk;
}

bool I2cHandleOutput(tflite::ErrorReporter* error_reporter, int8_t *output,
                     int length) {
  if (hx_drv_i2cm_set_data(kArduinoAddr, NULL, 0, (uint8_t *)output, length)
      != HX_DRV_LIB_PASS) {
    return false;
  }

  return true;
}
