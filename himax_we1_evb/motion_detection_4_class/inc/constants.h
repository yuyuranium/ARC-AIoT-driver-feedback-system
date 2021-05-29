#ifndef CONSTANTS_H_
#define CONSTANTS_H_

// The expected accelerometer data sample frequency
// const float kTargetHz = 25;

// What gestures are supported.
// constexpr int kGestureCount = 4;
// constexpr int kWingGesture = 0;
// constexpr int kRingGesture = 1;
// constexpr int kSlopeGesture = 2;
// constexpr int kNoGesture = 3;

constexpr uint8_t kMotionCount = 5;
constexpr uint8_t kIdleMotion = 0;
constexpr uint8_t kCouriseMotion = 1;
constexpr uint8_t kStartMotion = 2;
constexpr uint8_t kStopMotion = 3;
constexpr uint8_t kUnknownMotion = 4;


// These control the sensitivity of the detection algorithm. If you're seeing
// too many false positives or not enough true positives, you can try tweaking
// these thresholds. Often, increasing the size of the training set will give
// more robust results though, so consider retraining if you are seeing poor
// predictions.
constexpr float kDetectionThreshold = 0.8f;
constexpr int kPredictionHistoryLength = 5;
// constexpr int kPredictionSuppressionDuration = 25;

#endif  // CONSTANTS_H_
