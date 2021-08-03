#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define X 0
#define Y 1
#define Z 2

// Motion parameters
constexpr uint8_t kMotions = 6;
constexpr uint8_t kIdel = 0;
constexpr uint8_t kCruise = 1;
constexpr uint8_t kAccel = 2;
constexpr uint8_t kBrake = 3;
constexpr uint8_t kLeft = 4;
constexpr uint8_t kRight = 5;

// Consts obtained from training phase
constexpr int kSteps = 30;
constexpr float kAccelMean[3] = {
  0.9572375414426996, 0.012870309508895035, -0.0278956020587236 };
constexpr float kJerkMean[3] = {
  0.0004580301914796934, 0.0007405162553078224, 0.012318304740119934 };
constexpr float kAccelStd[3] = {
  0.03648254431044691, 0.10694366078089648, 0.09455331861316113 };
constexpr float kJerkStd[3] = {
  0.792294015927371, 0.6213015301790688, 0.7008286966902062 };

// Confidence and length of history for motion detection
constexpr float kDetectionConfidenceThreshold = 0.75;
constexpr int kDetectionHistoryLength = 9;
constexpr int kTransitionMotinoAccumulatorThreshold = 7;
constexpr int kTransitionHistoryLength = 10;
constexpr int kDiffBufferPadding = 9;

// The length of diff buffer depends on previous parameters of models' or state
// machine's
constexpr int kErrorBufferLength = kSteps + kDetectionHistoryLength / 2
    + kTransitionMotinoAccumulatorThreshold * 2 + kDiffBufferPadding;

// Number of states
constexpr uint8_t kStates = 16;

#endif  // CONSTANTS_H_
