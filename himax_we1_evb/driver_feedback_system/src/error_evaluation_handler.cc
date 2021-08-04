#include "error_evaluation_handler.h"
#include "constants.h"
#include "hx_drv_tflm.h"

#define SQ(x) ((x) * (x))  // Square of a given number

namespace {
// The shift buffer that stores the previous difference of prediction and actual
// value, using ring buffer structure
float shift_error_buffer[3][kErrorBufferLength] = {0.0};
// The head of the shift buffer
int begin_index = 0;
// Prediction from last cycle, used to compare with the actual value we obtain
// this cycle
float predicted_value[3] = {0.0};
// Store the state information to know what to do now
int8_t current_state = 0;
// The accumulated error and number of accumulated of a given strong state
float accumulated_error_sq[3] = {0.0};
int n_accumulated = 0.0;
}  // namespace

bool EvaluateMSE(float *mse, int8_t state,
                 float *prediction, float *actual) {
  // Update shift error buffer with latest data
  // Calculate the difference between previous prediction and the actual value
  shift_error_buffer[X][begin_index] = actual[X] - predicted_value[X]; 
  shift_error_buffer[Y][begin_index] = actual[Y] - predicted_value[Y]; 
  shift_error_buffer[Z][begin_index] = actual[Z] - predicted_value[Z]; 
  predicted_value[X] = prediction[X];
  predicted_value[Y] = prediction[Y];
  predicted_value[Z] = prediction[Z];

  if (++begin_index == kErrorBufferLength) {
    begin_index = 0;
  }

  // If state is changed, then calculate the average mse of the previous state
  // or the state lasts over 2 minutes, for the concern of buffer overflow,
  // also calculate the mse
  if ((state != current_state && n_accumulated > 0) || n_accumulated > 4800) {
    mse[X] = accumulated_error_sq[X] / (float)n_accumulated;
    mse[Y] = accumulated_error_sq[Y] / (float)n_accumulated;
    mse[Z] = accumulated_error_sq[Z] / (float)n_accumulated;
    accumulated_error_sq[X] = 0.0;
    accumulated_error_sq[Y] = 0.0;
    accumulated_error_sq[Z] = 0.0;
    current_state = state;

    // State interval less than 1 second is not worth trusting
    if (n_accumulated < 40) return false;

    n_accumulated = 0;
    return true;
  }

  // Calculate the index of oldest data who is the next of begin index
  int tail_index = begin_index + 1;
  if (tail_index == kDetectionHistoryLength) {
    tail_index = 0;
  }

  switch (state) {
    // For start off (9), accelerate (14) and brake (10)
    // Evaluate only the directions of back and forth, which is Z direction
    // UPDATE: Evaluate the direction of up and down as well
    case 9:
    case 10:
    case 14:
      accumulated_error_sq[X] += SQ(shift_error_buffer[X][tail_index]);
      accumulated_error_sq[Y] += SQ(shift_error_buffer[Y][tail_index]);
      accumulated_error_sq[Z] += SQ(shift_error_buffer[Z][tail_index]);
      ++n_accumulated;
      break;
    // For left turn (11) and right turn (12)
    // Evaluate directions of left and right only, which is Y direction
    // UPDATE: Evaluate the direction of up and down as well
    case 11:
    case 12:
      accumulated_error_sq[X] += SQ(shift_error_buffer[X][tail_index]);
      accumulated_error_sq[Y] += SQ(shift_error_buffer[Y][tail_index]);
      accumulated_error_sq[Z] += SQ(shift_error_buffer[Z][tail_index]);
      ++n_accumulated;
      break;
    // For cruise (13)
    // Evaluate all directions
    case 13:
      accumulated_error_sq[X] += SQ(shift_error_buffer[X][tail_index]);
      accumulated_error_sq[Y] += SQ(shift_error_buffer[Y][tail_index]);
      accumulated_error_sq[Z] += SQ(shift_error_buffer[Z][tail_index]);
      ++n_accumulated;
      break;
    default:
      accumulated_error_sq[X] = 0.0;
      accumulated_error_sq[Y] = 0.0;
      accumulated_error_sq[Z] = 0.0;
      n_accumulated = 0;
      break;
  }

  current_state = state;
  return false;
}
