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
  { 0.15541, 0.02058, 0.02193 },
  { 0.13482, 0.01428, 0.01945 },
  { 0.28978, 0.04377, 0.03605 },
  { 0.26091, 0.04196, 0.03491 },
  { 0.54518, 0.03581, 0.06978 },
  { 0.32841, 0.02058, 0.04193 }
};   
constexpr float kMseStds[kGradingStates][3] = {
  { 0.13211, 0.04009, 0.01352 },
  { 0.11705, 0.00809, 0.01104 },
  { 0.55237, 0.05288, 0.05854 },
  { 0.55237, 0.05288, 0.05854 },
  { 0.88898, 0.05897, 0.08782 },
  { 0.43201, 0.08009, 0.06552 }
};
// The scale that make data vary if in +- 1 std and saturate if out of it
constexpr float kMseScale = 5 / 2;
// To convert a float to a fix point, the max of float is 1, and 8 fractional
// bits is sufficient for the precision
constexpr uint8_t kFractionalBits = 8;
constexpr float kQuantScale = (float)(1 << kFractionalBits);
}  // namespace

// The modified sigmoid function that is reversed and scaled by 5
inline float SigmoidNx(const float x) {
  return 1 / (1 + exp(x));
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
    float y = SigmoidNx(kMseScale * norm_mse - (float)0.8473);
    grading[i] = FixPoint(y);
  }
}
