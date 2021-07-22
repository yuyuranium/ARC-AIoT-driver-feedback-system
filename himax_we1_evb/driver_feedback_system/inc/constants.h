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
constexpr float kAccelMean[3] = {
  0.9487810621465815, 0.03946548768500278, 0.06886678821127502 };
constexpr float kJerkMean[3] = {
  -0.00326175719059994, 0.001484422197827616, 0.009953398211114467 };
constexpr float kAccelStd[3] = {
  0.1313167093450731, 0.11926362940605395, 0.17273308090512682 };
constexpr float kJerkStd[3] = {
  2.3153819130934132, 1.803095985733507, 2.2968154997810615 };

// Confidence and length of history for motion detection
constexpr float kDetectionConfidenceThreshold = 0.78;
constexpr int kDetectionHistoryLength = 15;
constexpr int kTransitionMotinoAccumulatorThreshold = 10;
constexpr int kTransitionHistoryLength = 10;

// Number of states
constexpr uint8_t kStates = 16;

// Names of each state
constexpr char *kStateNames[kStates] = {
  "initial",      //  0
  "w-idle",       //  1
  "w-start-off",  //  2
  "w-brake",      //  3
  "w-left",       //  4
  "w-right",      //  5
  "w-cruise",     //  6
  "w-accel",      //  7
  "s-idle",       //  8
  "s-start-off",  //  9
  "s-brake",      // 10
  "s-left",       // 11
  "s-right",      // 12
  "s-cruise",     // 13
  "s-accel",      // 14
  "pending"       // 15
};

#endif  // CONSTANTS_H_
