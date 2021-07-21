#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define X 0
#define Y 1
#define Z 2

constexpr uint8_t kMotionCount = 6;
constexpr uint8_t kIdel = 0;
constexpr uint8_t kCruise = 1;
constexpr uint8_t kAccel = 2;
constexpr uint8_t kBrake = 3;
constexpr uint8_t kLeft = 4;
constexpr uint8_t kRight = 5;

// consts obtained from training phase
constexpr float kAccelMean[3] = {
    0.9487810621465815, 0.03946548768500278, 0.06886678821127502 };
constexpr float kJerkMean[3] = {
    -0.00326175719059994, 0.001484422197827616, 0.009953398211114467 };
constexpr float kAccelStd[3] = {
    0.1313167093450731, 0.11926362940605395, 0.17273308090512682 };
constexpr float kJerkStd[3] = {
    2.3153819130934132, 1.803095985733507, 2.2968154997810615 };

constexpr float kDetectionThresholdConfidence = 0.75;
constexpr int kPredictionHistoryLength = 10;

#endif  // CONSTANTS_H_
