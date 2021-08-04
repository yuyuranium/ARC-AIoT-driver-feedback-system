#include "state_machine_handler.h"
#include "constants.h"
#include "hx_drv_tflm.h"

namespace {
// The transition table
constexpr int8_t kTransitionTable[kStates][kMotions] = {
  { 1, 15, 15, 15, 15, 15 },  //  0: "initial",
  { 8,  6,  2,  3,  4,  5 },  //  1: "w-idle",
  { 1,  6,  9,  3,  4,  5 },  //  2: "w-start-off",
  { 1,  6,  7, 10,  4,  5 },  //  3: "w-brake",
  { 1,  6,  7,  3, 11,  5 },  //  4: "w-left",
  { 1,  6,  7,  3,  4, 12 },  //  5: "w-right",
  { 1, 13,  7,  3,  4,  5 },  //  6: "w-cruise",
  { 1,  6, 14,  3,  4,  5 },  //  7: "w-accel",
  { 8,  1,  2,  1,  1,  1 },  //  8: "s-idle",
  { 2,  6,  9,  2,  2,  2 },  //  9: "s-start-off",
  { 1,  3,  3, 10,  3,  3 },  // 10: "s-brake",
  { 4,  4,  4,  4, 11,  4 },  // 11: "s-left",
  { 5,  5,  5,  5,  5, 12 },  // 12: "s-right",
  { 6, 13,  6,  3,  6,  6 },  // 13: "s-cruise",
  { 7,  6, 14,  7,  7,  7 },  // 14: "s-accel",
  { 1, 15, 15, 15, 15, 15 },  // 15: "pending"
};
// Current state of the vehicle
int8_t state = 0;
// The history of transitions using ring buffer structure
uint8_t transition_history[kTransitionHistoryLength] = {0};
// The head to the transition history
int begin_index = 0;
// The buffer that stores the detected motion, if full, it triggers transition
int8_t current_motion = -1;
// The index to put the latest motion in
int8_t motion_accumulator = 0;
}  // namespace

int8_t StateTransition(int8_t motion) {
  // Not doing anything when detection is unknown
  if (motion == -1) return state;

  // To check if the upcoming detection is consistent with the buffered ones
  if (motion != current_motion) {
    // Clean the buffer and update the current motion
    current_motion = motion;
    motion_accumulator = 1;
  } else if (++motion_accumulator >= kTransitionMotinoAccumulatorThreshold) {
    // Trigger transition after accumulating reaches the threshold
    state = kTransitionTable[state][motion];
    transition_history[begin_index++] = state;
    if (begin_index == kTransitionHistoryLength) {
      begin_index = 0;
    }
    motion_accumulator = 0;  // Reset the accumulator
  }
  current_motion = motion;

  return state;
}
