#ifndef MOTION_PREDICTOR_H_
#define MOTION_PREDICTOR_H_

#include "tensorflow/lite/micro/micro_error_reporter.h"

extern int SetDetectionThreshold(tflite::ErrorReporter* error_reporter,
                                 float confidence, uint32_t zero_point,
                                 float scale);
extern int8_t PredictMotion(tflite::ErrorReporter* error_reporter, int8_t *output);

#endif // MOTION_PREDICTOR_H_
