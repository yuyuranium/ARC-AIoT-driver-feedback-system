#ifndef ACCELEROMETER_HANDLER_H_
#define ACCELEROMETER_HANDLER_H_

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"

extern TfLiteStatus SetupAccelerometer(tflite::ErrorReporter *error_reporter,
                                       int32_t input_zero_point_c,
                                       float input_scale_c,
                                       int32_t input_zero_point_p,
                                       float input_scale_p);
extern bool ReadAccelerometer(tflite::ErrorReporter *error_reporter,
                              int8_t *input_c, int8_t *input_p, int length);
extern void GetLatestData(tflite::ErrorReporter *error_reporter,
                          float *latest_data, int length);

#endif  // ACCELEROMETER_HANDLER_H_
