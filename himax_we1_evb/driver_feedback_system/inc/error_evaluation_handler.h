#ifndef EVALUATION_HANDLER_H_
#define EVALUATION_HANDLER_H_

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"

extern TfLiteStatus SetupEvaluationParameters(
    tflite::ErrorReporter *error_reporter);
extern bool EvaluateMSE(float *mse, int8_t state,
                        float *prediction, float *actual);

#endif  // EVALUATION_HANDLER_H_

