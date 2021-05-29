#include "hx_drv_tflm.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"

#include "constants.h"
#include "motion_predictor.h"
#include <cstdio>

namespace {
// State for the averaging algorithm we're using.
int8_t prediction_history[kMotionCount][kPredictionHistoryLength] = {0};
int prediction_history_index = 0;
int kDetectionThreshold = 0;
char buf[80];
}  // namespace

void SetDetectionThreshold(tflite::ErrorReporter* error_reporter, float thresh, uint32_t zero_point, float scale) {
  kDetectionThreshold = (int)(thresh / scale) + (int)zero_point;
  sprintf(buf, "thresh: %d", kDetectionThreshold);
  TF_LITE_REPORT_ERROR(error_reporter, buf);
}

int8_t PredictMotion(tflite::ErrorReporter* error_reporter, int8_t *output) {
  for (int i = 0; i < kMotionCount; ++i) {
    prediction_history[i][prediction_history_index] = output[i];
  }
  // Figure out which slot to put the next predictions into.
  ++prediction_history_index;
  if (prediction_history_index >= kPredictionHistoryLength) {
    prediction_history_index = 0;
  }

  // Average the last n predictions for each gesture, and find which has the
  // highest score.
  int max_predict_index = -1;
  int8_t max_predict_score = 0.0f;
  for (int i = 0; i < kMotionCount; i++) {
    int prediction_sum = 0;
    for (int j = 0; j < kPredictionHistoryLength; ++j) {
      prediction_sum += (int)prediction_history[i][j];
    }
    const int prediction_average = prediction_sum / kPredictionHistoryLength;
    if ((max_predict_index == -1) || (prediction_average > max_predict_score)) {
      max_predict_index = i;
      max_predict_score = prediction_average;
    }
  }
  sprintf(buf, "max: %d %d", max_predict_index, max_predict_score);
  TF_LITE_REPORT_ERROR(error_reporter, buf);


  if (max_predict_score > kDetectionThreshold) {
    return max_predict_index;
  } else {
    return kUnknownMotion;
  }
}
