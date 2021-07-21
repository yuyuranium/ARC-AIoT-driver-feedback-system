#include "state_machine_handler.h"
#include "constants.h"
#include "hx_drv_tflm.h"

namespace {
const uint8_t kStates = 16;
const uint8_t kTransitionTable[kStates][kMotions] = {

};




// Beta of LPF
const float kLPFBeta = 0.25;
// Sampling parameters
const float kSamplingPeriod = 0.04;
const uint32_t kClkRate = 400000000;
const uint32_t kSamplingCycle = kSamplingPeriod*kClkRate;
// Ring buffer size
constexpr int kRingBufferSize = 180;  // 6 * 60
// Ring buffers for both models
int8_t ring_buffer_c[kRingBufferSize] = {0};
int8_t ring_buffer_p[kRingBufferSize] = {0};
// Available data count in accelerometer FIFO
int available_count = 0;
// The head of the ring buffer
int begin_index;
// Raw data of accelerometer
float raw_ax, raw_ay, raw_az; // Timer
uint32_t tick_now, tick_last;
// Float data of accel and jerk
float accel[3], accel_last[3], jerk[3];
// Quantization parameters of both models' input
int32_t zero_point_c;
int32_t zero_point_p;
float scale_c;
float scale_p;
}  // namespace
