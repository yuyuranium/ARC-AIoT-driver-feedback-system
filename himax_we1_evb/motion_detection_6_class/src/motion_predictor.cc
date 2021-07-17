#include "hx_drv_tflm.h"
#include "motion_predictor.h"
#include "constants.h"
#include <cstdio>

namespace {
// State for the averaging algorithm we're using.
int8_t prediction_history[kMotionCount][kPredictionHistoryLength] = {0};
int prediction_history_index = 0;
int detectionThreshold = 0;
int last_prediction = 0;
}  // namespace

int SetDetectionThreshold(tflite::ErrorReporter* error_reporter,
                          float confidence, uint32_t zero_point, float scale) {
  detectionThreshold = (int)(confidence / scale) + (int)zero_point;
  return detectionThreshold;
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

  if (max_predict_score > detectionThreshold) {
    last_prediction = max_predict_index;
    return max_predict_index;
  } else {
    return last_prediction;
  }
}
