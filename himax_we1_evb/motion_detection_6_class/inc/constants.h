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
    0.9510178671889282, 0.030960616493194554, 0.0779531734518597 };
constexpr float kJerkMean[3] = {
    -0.005992084180475412, 0.0008366459020875214, 0.00811919606520344 };
constexpr float kAccelStd[3] = {
    0.12887788435917533, 0.11934031413396946, 0.17276545673112512 };
constexpr float kJerkStd[3] = {
    2.3096330606078035, 1.8162722841544086, 2.333676180306296 };

constexpr float kDetectionThresholdConfidence = 0.6;
constexpr int kPredictionHistoryLength = 20;

#endif  // CONSTANTS_H_
