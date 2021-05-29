#ifndef ACCELEROMETER_HANDLER_H_
#define ACCELEROMETER_HANDLER_H_

#define kChannelNumber 3  // ax, ay, az

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"


extern TfLiteStatus SetupAccelerometer(tflite::ErrorReporter *error_reporter);
extern bool ReadAccelerometer(tflite::ErrorReporter* error_reporter,
                              uint32_t zero_point, float scale, int8_t* input,
                              int length);

#endif  // ACCELEROMETER_HANDLER_H_
