#ifndef STATE_MACHINE_HANDLER_H_
#define STATE_MACHINE_HANDLER_H_

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"

extern TfLiteStatus SetupStateMachine(tflite::ErrorReporter *error_reporter);
extern uint8_t StateTransition(int8_t motion);

#endif  // STATE_MACHINE_HANDLER_H_
