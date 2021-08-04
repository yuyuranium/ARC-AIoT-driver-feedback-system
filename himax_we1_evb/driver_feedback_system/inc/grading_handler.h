#ifndef GRADING_HANDLER_H_
#define GRADING_HANDLER_H_

#include "tensorflow/lite/c/common.h"

extern void Grade(uint8_t *grading, float *mses, int8_t state);

#endif  // GRADING_HANDLER_H_
