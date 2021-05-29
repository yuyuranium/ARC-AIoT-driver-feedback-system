#ifndef CONSTANTS_H_
#define CONSTANTS_H_

constexpr uint8_t kMotionCount = 4;
constexpr uint8_t kIdleMotion = 0;
constexpr uint8_t kCouriseMotion = 1;
constexpr uint8_t kStartMotion = 2;
constexpr uint8_t kStopMotion = 3;

constexpr float kDetectionThresholdConfidence = 0.3;
constexpr int kPredictionHistoryLength = 50;

#endif  // CONSTANTS_H_
