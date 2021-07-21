#include "main_functions.h"
#include "hx_drv_tflm.h"

#include "accelerometer_handler.h"
#include "classifier_model.h"
#include "predictor_model.h"
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
// Error reporter for displaying message when debugging
tflite::ErrorReporter *error_reporter = nullptr;
// The classifier model
const tflite::Model *classifier = nullptr;
tflite::MicroInterpreter *classifier_interpreter = nullptr;
TfLiteTensor *classifier_input = nullptr;
TfLiteTensor *classifier_output = nullptr;
// The predictor model
const tflite::Model *predictor = nullptr;
tflite::MicroInterpreter *predictor_interpreter = nullptr;
TfLiteTensor *predictor_input = nullptr;
TfLiteTensor *predictor_output = nullptr;
// Input length of two model (should be the same)
int input_length;
// Create an area of memory to use for input, output, and intermediate arrays.
// The size of this will depend on the model you're using, and may need to be
// determined by experimentation.
constexpr int kTensorArenaSize = 90 * 1024;
#if (defined(__GNUC__) || defined(__GNUG__)) && !defined (__CCAC__)
alignas(16) static uint8_t tensor_arena_c[kTensorArenaSize] __attribute__((section(".tensor_arena_c")));
alignas(16) static uint8_t tensor_arena_p[kTensorArenaSize] __attribute__((section(".tensor_arena_p")));
#else
#pragma Bss(".tensor_arena_c")
static uint8_t tensor_arena_c[kTensorArenaSize];
#pragma Bss()
#pragma Bss(".tensor_arena_p")
static uint8_t tensor_arena_p[kTensorArenaSize];
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
  classifier = tflite::GetModel(classifier_tflite);
  if (classifier->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         classifier->version(), TFLITE_SCHEMA_VERSION);
    return;
  }
  predictor = tflite::GetModel(predictor_tflite);
  if (predictor->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         predictor->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  // Pull in only the operation implementations we need.
  // This relies on a complete list of all the ops needed by this graph.
  // An easier approach is to just use the AllOpsResolver, but this will
  // incur some penalty in code space for op implementations that are not
  // needed by this graph.
  static tflite::MicroMutableOpResolver<10> classifier_op_resolver;  // NOLINT
  classifier_op_resolver.AddConv2D();
  classifier_op_resolver.AddMaxPool2D();
  classifier_op_resolver.AddFullyConnected();
  classifier_op_resolver.AddConcatenation();
  classifier_op_resolver.AddMul();
  classifier_op_resolver.AddAdd();
  classifier_op_resolver.AddMean();
  classifier_op_resolver.AddReshape();
  classifier_op_resolver.AddRelu();
  classifier_op_resolver.AddSoftmax();

  static tflite::MicroMutableOpResolver<4> predictor_op_resolver;  // NOLINT
  predictor_op_resolver.AddReshape();
  predictor_op_resolver.AddConv2D();
  predictor_op_resolver.AddMaxPool2D();
  predictor_op_resolver.AddFullyConnected();

  // Build two interpreters to run the model with.
  static tflite::MicroInterpreter static_classifier_interpreter(
      classifier, classifier_op_resolver, tensor_arena_c, kTensorArenaSize,
      error_reporter);
  classifier_interpreter = &static_classifier_interpreter;
  static tflite::MicroInterpreter static_predictor_interpreter(
      predictor, predictor_op_resolver, tensor_arena_p, kTensorArenaSize,
      error_reporter);
  predictor_interpreter = &static_predictor_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  if (classifier_interpreter->AllocateTensors() != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
    return;
  }
  if (predictor_interpreter->AllocateTensors() != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
    return;
  }

  // Obtain pointer to the model's input/output tensor.
  classifier_input = classifier_interpreter->input(0);
  classifier_output = classifier_interpreter->output(0);
  predictor_input = predictor_interpreter->input(0);
  predictor_output = predictor_interpreter->output(0);

  // Obtain # of bytes of input tensor
  input_length = classifier_input->bytes / sizeof(int8_t);

  // Initialize detection with confidence threshold
  int quant_detection_threshold = SetDetectionThreshold(
      error_reporter, kDetectionThresholdConfidence,
      classifier_output->params.zero_point, classifier_output->params.scale);

  // Initialize accelerometer
  TfLiteStatus accel_setup_status = SetupAccelerometer(error_reporter,
      classifier_input->params.zero_point, classifier_input->params.scale,
      predictor_input->params.zero_point, predictor_input->params.scale);
  if (accel_setup_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "accel set up failed\n");
  }

  // Initialize i2c bus.
  TfLiteStatus i2c_setup_status = SetupI2C(error_reporter);
  if (i2c_setup_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "i2c set up failed\n");
  }
  
  // TODO Run the state machine

  TF_LITE_REPORT_ERROR(
      error_reporter,
      "Starting inferencing with quantized detection threshold: %d",
      quant_detection_threshold);
}

void loop() {
  // Attempt to read new data from the accelerometer.
  bool got_data = ReadAccelerometer(error_reporter,
                                    classifier_input->data.int8,
                                    predictor_input->data.int8,
                                    input_length);
  // If there was no new data, wait until next time.
  if (!got_data) return;

  // Run inference on classifier model, and report any error.
  TfLiteStatus invoke_status_c = classifier_interpreter->Invoke();
  if (invoke_status_c != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed\n");
    return;
  }

  // Run inference on predictor model, and report any error.
  TfLiteStatus invoke_status_p = predictor_interpreter->Invoke();
  if (invoke_status_p != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed\n");
    return;
  }

  int8_t index = PredictMotion(error_reporter, classifier_output->data.int8);
  TF_LITE_REPORT_ERROR(error_reporter, "Predict: %d", index);

  // TODO Run the state machine

  // Retrieve data from prediction result
  float origin[6];
  GetLatestData(error_reporter, origin, 6);

  int8_t data_buf[25];  // prediction ax ay az and origin ax ay az and class
  for (int i = 0; i < 3; ++i) {
    floatData ff;
    ff.f_val =
        (predictor_output->data.int8[i] - predictor_output->params.zero_point) *
        predictor_output->params.scale;  // retrieve the real value of output
    data_buf[i * 4 + 0] = ff.bytes[0];
    data_buf[i * 4 + 1] = ff.bytes[1];
    data_buf[i * 4 + 2] = ff.bytes[2];
    data_buf[i * 4 + 3] = ff.bytes[3];
  }

  for (int i = 0; i < 3; ++i) {  // put only ax ay and az
    floatData ff;
    ff.f_val = origin[i];
    data_buf[12 + i * 4] = ff.bytes[0];
    data_buf[13 + i * 4] = ff.bytes[1];
    data_buf[14 + i * 4] = ff.bytes[2];
    data_buf[15 + i * 4] = ff.bytes[3];
  }

  data_buf[24] = index;  // put classification result on the last byte

  TfLiteStatus transmit_status = I2CSendOutput(error_reporter, data_buf, 25);
  if (transmit_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "Cannot transmit\n");
  }
}
