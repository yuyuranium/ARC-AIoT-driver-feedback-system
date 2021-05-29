#ifndef MOTION_PREDICTOR_H_
#define MOTION_PREDICTOR_H_

extern void SetDetectionThreshold(tflite::ErrorReporter* error_reporter, float thresh, uint32_t zero_point, float scale);
extern int8_t PredictMotion(tflite::ErrorReporter* error_reporter, int8_t *output);

#endif
