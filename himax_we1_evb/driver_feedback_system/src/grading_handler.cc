#include "grading_handler.h"
#include "constants.h"
#include "hx_drv_tflm.h"
#include <cmath>

namespace {
// Number of states that will be graded: Start-off, Brake, Left, Right, Cruise
// and Accel, and there are 6 of them
constexpr int kGradingStates = 6;
// The index of the strong state of start-off is 9, followed by the other states
constexpr int kGradingStateBegin = 9;
// Normalization parameters, for each state, there are data of the three axes
constexpr float kMseMeans[kGradingStates][3] = {
  { 0.04211, 0.00629, 0.01044 },
  { 0.10482, 0.01128, 0.01645 },
  { 0.28978, 0.04377, 0.03605 },
  { 0.17091, 0.02996, 0.02291 },
  { 0.34318, 0.03561, 0.03958 },
  { 0.15541, 0.02058, 0.02193 }
};   
constexpr float kMseStds[kGradingStates][3] = {
  { 0.02771, 0.0034,  0.00441 },
  { 0.13765, 0.01129, 0.01694 },
  { 0.55187, 0.05608, 0.05854 },
  { 0.10318, 0.01645, 0.01783 },
  { 0.58518, 0.06067, 0.04802 },
  { 0.13231, 0.04209, 0.01592 }
};
// The scale that make data vary if in +- 2 std and saturate if out of it
constexpr float kMseScale = 5 / 2;
// To convert a float to a fix point, the max of float is 5, and 5 fractional
// bits is sufficient for the precision
constexpr uint8_t kFractionalBits = 5;
constexpr float kQuantScale = (float)(1 << kFractionalBits);
}  // namespace

// The modified sigmoid function that is reversed and scaled by 5
inline float SigmoidNx5(const float x) {
  return 5 / (1 + exp(x));
}

inline uint8_t FixPoint(float f) {
  return (uint8_t)(round(f * kQuantScale));
}

void Grade(uint8_t *grading, float *mse, int8_t state) {
  int8_t grading_state = state - kGradingStateBegin;
  // For each axis, calculate the star of it
  for (int i = 0; i < 3; ++i) {
    float norm_mse =
        (mse[i] - kMseMeans[grading_state][i]) / kMseStds[grading_state][i];
    float star = SigmoidNx5(kMseScale * norm_mse);
    grading[i] = FixPoint(star);
  }
}
