#ifndef ACCELEROMETER_HANDLER_H_
#define ACCELEROMETER_HANDLER_H_

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"

extern TfLiteStatus SetupAccelerometer(tflite::ErrorReporter *error_reporter,
                                       int32_t input_zero_point,
                                       float input_scale);
extern bool ReadAccelerometer(tflite::ErrorReporter *error_reporter,
                              int8_t *input, int length);

#endif  // ACCELEROMETER_HANDLER_H_
