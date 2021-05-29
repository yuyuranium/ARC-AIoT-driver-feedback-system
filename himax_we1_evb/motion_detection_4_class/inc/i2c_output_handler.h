#ifndef I2C_OUTPUT_HANDLER_H_
#define I2C_OUTPUT_HANDLER_H_

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"

extern TfLiteStatus SetupI2c(tflite::ErrorReporter* error_reporter);
extern bool I2cHandleOutput(tflite::ErrorReporter* error_reporter,
                            int8_t *output, int length);

#endif
