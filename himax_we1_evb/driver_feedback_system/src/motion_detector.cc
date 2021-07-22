#include "hx_drv_tflm.h"
#include "motion_detector.h"
#include "constants.h"
#include <cstdio>

namespace {
// State for the averaging algorithm we're using.
int8_t detection_history[kMotions][kDetectionHistoryLength] = {0};
int detection_history_index = 0;
int detection_threshold = 0;
int last_detection = 0;
}  // namespace

int SetDetectionThreshold(tflite::ErrorReporter* error_reporter,
                          float confidence, uint32_t zero_point, float scale) {
  detection_threshold = (int)(confidence / scale) + (int)zero_point;
  return detection_threshold;
}

int8_t DetectMotion(tflite::ErrorReporter* error_reporter, int8_t *output) {
  for (int i = 0; i < kMotions; ++i) {
    detection_history[i][detection_history_index] = output[i];
  }
  // Figure out which slot to put the next detections into.
  ++detection_history_index;
  if (detection_history_index >= kDetectionHistoryLength) {
    detection_history_index = 0;
  }

  // Average the last n detections for each gesture, and find which has the
  // highest score.
  int max_detection_index = -1;
  int8_t max_detection_score = 0.0;
  for (int i = 0; i < kMotions; i++) {
    int detection_sum = 0;
    for (int j = 0; j < kDetectionHistoryLength; ++j) {
      detection_sum += (int)detection_history[i][j];
    }
    const int detection_average = detection_sum / kDetectionHistoryLength;
    if ((max_detection_index == -1) || (detection_average
        > max_detection_score)) {
      max_detection_index = i;
      max_detection_score = detection_average;
    }
  }

  if (max_detection_score > detection_threshold) {
    last_detection = max_detection_index;
    return max_detection_index;
  } else {
    return -1;
  }
}
