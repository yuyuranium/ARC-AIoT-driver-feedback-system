#include "main_functions.h"
#include "hx_drv_tflm.h"

#include "accelerometer_handler.h"
#include "model.h"
#include "constants.h"
#include "motion_predictor.h"
#include "i2c_output_handler.h"

#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

// Globals, used for compatibility with Arduino-style sketches.
namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor *model_input = nullptr;
TfLiteTensor *model_output = nullptr;
int input_length;

// Create an area of memory to use for input, output, and intermediate arrays.
// The size of this will depend on the model you're using, and may need to be
// determined by experimentation.
constexpr int kTensorArenaSize = 90 * 1024;
#if (defined(__GNUC__) || defined(__GNUG__)) && !defined (__CCAC__)
alignas(16) static uint8_t tensor_arena[kTensorArenaSize] __attribute__((section(".tensor_arena")));
#else
#pragma Bss(".tensor_arena")
static uint8_t tensor_arena[kTensorArenaSize];
#pragma Bss()
#endif // if defined (_GNUC_) && !defined (_CCAC_)

// To transmit floating point numbers through I2C, decoupling ieee754 format to
// four bytes using union
typedef union {
  float f_val;
  uint8_t bytes[sizeof(float)];
} floatData;

}  // namespace
  
char buf[100];

// The name of this function is important for Arduino compatibility.
void setup() {
  // Set up logging. Google style is to avoid globals or statics because of
  // lifetime uncertainty, but since this has a trivial destructor it's okay.
  static tflite::MicroErrorReporter micro_error_reporter;  // NOLINT
  error_reporter = &micro_error_reporter;

  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  model = tflite::GetModel(classifier_tflite);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  // Pull in only the operation implementations we need.
  // This relies on a complete list of all the ops needed by this graph.
  // An easier approach is to just use the AllOpsResolver, but this will
  // incur some penalty in code space for op implementations that are not
  // needed by this graph.
  static tflite::MicroMutableOpResolver<10> micro_op_resolver;  // NOLINT
  micro_op_resolver.AddConv2D();
  micro_op_resolver.AddMaxPool2D();
  micro_op_resolver.AddFullyConnected();
  micro_op_resolver.AddConcatenation();
  micro_op_resolver.AddMul();
  micro_op_resolver.AddAdd();
  micro_op_resolver.AddMean();
  micro_op_resolver.AddReshape();
  micro_op_resolver.AddRelu();
  micro_op_resolver.AddSoftmax();

  // Build an interpreter to run the model with.
  static tflite::MicroInterpreter static_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
    return;
  }
  TF_LITE_REPORT_ERROR(error_reporter, "here");

  // Obtain pointer to the model's input/output tensor.
  model_input = interpreter->input(0);
  model_output = interpreter->output(0);

  input_length = model_input->bytes / sizeof(int8_t);

  int detection_threshold = SetDetectionThreshold(
      error_reporter, kDetectionThresholdConfidence,
      model_output->params.zero_point, model_output->params.scale);

  // Initialize accelerometer and i2c bus.
  TfLiteStatus accel_setup_status = SetupAccelerometer(
      error_reporter,
      model_input->params.zero_point, model_input->params.scale);
  if (accel_setup_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "accel set up failed\n");
  }

  TfLiteStatus i2c_setup_status = SetupI2C(error_reporter);
  if (i2c_setup_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "i2c set up failed\n");
  }

  sprintf(buf, "Starting inferencing with quantized detection threshold: %d",
          detection_threshold);
  TF_LITE_REPORT_ERROR(error_reporter, buf);
}

void loop() {
  // Attempt to read new data from the accelerometer.
  bool got_data = ReadAccelerometer(error_reporter,
                                    model_input->data.int8,
                                    input_length);
  // If there was no new data, wait until next time.
  if (!got_data) return;

  // Run inference, and report any error.
  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed\n");
    return;
  }

  int8_t index = PredictMotion(error_reporter, model_output->data.int8);
  sprintf(buf, "Predict: %d", index);
  TF_LITE_REPORT_ERROR(error_reporter, buf);

  // The following code is to display the real value of inference output.
  // for (int i = 0; i < 6; ++i) {
  //   float o = (model_output->data.int8[i] - model_output->params.zero_point) * model_output->params.scale;
  //   int t = o * 1000;
  //   int int_part = t / 1000;
  //   int frac_part = t % 1000;
  //   sprintf(buf, "[%d]: %d.%d", i, int_part, frac_part);
  //   TF_LITE_REPORT_ERROR(error_reporter, buf);
  // }
  // TF_LITE_REPORT_ERROR(error_reporter, "\r");

  // int8_t data_buf[kMotionCount * sizeof(float)];
  // for (int i = 0; i < 6; ++i) {
  //   floatData ff;
  //   ff.f_val = (model_output->data.int8[i] - model_output->params.zero_point) *
  //       model_output->params.scale;
  //   data_buf[i * 4 + 0] = ff.bytes[0];
  //   data_buf[i * 4 + 1] = ff.bytes[1];
  //   data_buf[i * 4 + 2] = ff.bytes[2];
  //   data_buf[i * 4 + 3] = ff.bytes[3];
  // }

  TfLiteStatus transmit_status = I2CSendOutput(error_reporter, &index, 1);
  if (transmit_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "Cannot transmit\n");
  }
}
