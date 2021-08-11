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
  0.9569233989958942, 0.013474283870207656, -0.013631698874467501 };
constexpr float kJerkMean[3] = {
  0.0005011723938845438, 0.001576811732740949, 0.008756530947382475 };
constexpr float kAccelStd[3] = {
  0.03043855586863038, 0.0975338627323963, 0.08715879145312343 };
constexpr float kJerkStd[3] = {
  0.6788996939235906, 0.5475977123303466, 0.6220835815551232 };

// Confidence and length of history for motion detection
constexpr float kDetectionConfidenceThreshold = 0.72;
constexpr int kDetectionHistoryLength = 18;
constexpr int kTransitionMotinoAccumulatorThreshold = 9;
constexpr int kTransitionHistoryLength = 10;
constexpr int kDiffBufferPadding = 9;

// The length of diff buffer depends on previous parameters of models' or state
// machine's
constexpr int kErrorBufferLength = kSteps + kDetectionHistoryLength / 2
    + kTransitionMotinoAccumulatorThreshold * 2 + kDiffBufferPadding;

// Number of states
constexpr uint8_t kStates = 16;

#endif  // CONSTANTS_H_
