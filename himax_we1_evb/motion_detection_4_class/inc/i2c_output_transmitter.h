#ifndef I2C_OUTPUT_TRANSMITTER_H_
#define I2C_OUTPUT_TRANSMITTER_H_

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"

#define ARDUINO 0x12

extern TfLiteStatus SetupI2c(tflite::ErrorReporter *error_reporter);
extern bool I2cTransmitOutput(int8_t *output_data);

#endif
