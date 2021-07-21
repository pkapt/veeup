
#include <TensorFlowLite.h>
#include "mbed.h"
#define RMT1_PIN 11
#define RMT1_PIN 12
#include "main_functions.h"
#include "image_provider.h"
#include "model_settings.h"
#include "person_detect_model_data.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"
#include <Arduino_OV767X.h>

// Globals, used for compatibility with Arduino-style sketches.
namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;

// In order to use optimized tensorflow lite kernels, a signed int8_t quantized
// model is preferred over the legacy unsigned model format. This means that
// throughout this project, input images must be converted from unisgned to
// signed format. The easiest and quickest way to convert from unsigned to
// signed 8-bit integers is to subtract 128 from the unsigned value to get a
// signed value.

// An area of memory to use for input, output, and intermediate arrays.
constexpr int kTensorArenaSize = 136 * 1024;
static uint8_t tensor_arena[kTensorArenaSize];
} 
int numPowerPose1Classifications = 0;
int numPowerPose2Classifications = 0;


//how many frames you must hold the pose
const int numSamples = 3;

// The name of this function is important for Arduino compatibility.
void setup() {

  // Set up logging. Google style is to avoid globals or statics because of
  // lifetime uncertainty, but since this has a trivial destructor it's okay.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  model = tflite::GetModel(g_person_detect_model_data);
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
  //
  // tflite::AllOpsResolver resolver;
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroMutableOpResolver<6> micro_op_resolver;
  micro_op_resolver.AddAveragePool2D();
  micro_op_resolver.AddConv2D();
  micro_op_resolver.AddDepthwiseConv2D();
  micro_op_resolver.AddReshape();
  micro_op_resolver.AddSoftmax();
  micro_op_resolver.AddFullyConnected();

  // Build an interpreter to run the model with.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroInterpreter static_interpreter(
    model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
    return;
  }

  // Get information about the memory area to use for the model's input.
  input = interpreter->input(0);
  pinMode(RMT1_PIN, OUTPUT);
  digitalWrite(RMT1_PIN, HIGH);
//  pinMode(RMT2_PIN, OUTPUT);
//  digitalWrite(RMT2_PIN, HIGH);
}

void loop() {

  // Get image from provider.
  if (kTfLiteOk != GetImage(error_reporter, kNumCols, kNumRows, kNumChannels,
                            input->data.int8)) {
    TF_LITE_REPORT_ERROR(error_reporter, "Image capture failed.");
  }

  // Run the model on this input and make sure it succeeds.
  if (kTfLiteOk != interpreter->Invoke()) {
    TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed.");
  }

  TfLiteTensor* output = interpreter->output(0);

  // Process the inference results.
  int8_t person_score = output->data.uint8[kPersonIndex];
  int8_t no_person_score = output->data.uint8[kNotAPersonIndex];
  for (int i = 0; i < kCategoryCount; i++) {
    int8_t curr_category_score = output->data.uint8[i];
    const char* currCategory = kCategoryLabels[i];
    TF_LITE_REPORT_ERROR(error_reporter, "%s : %d", currCategory, curr_category_score);
  }
  int8_t class1_score = output->data.uint8[0];
  int8_t class2_score = output->data.uint8[1];
  int8_t class3_score = output->data.uint8[2];


  if (class1_score < class2_score) {
    numPowerPose1Classifications++;
  }
//  if (max([class1_score,class2_score,class3_score])==class2_score) {
//    numPowerPose1Classifications++;
//  }
  digitalWrite(RMT1_PIN, LOW);


  //If we've captured numSamples pictures in the right pose, 
  //turn off the alarm for another 24 hours. 
  if (numPowerPose1Classifications > numSamples) {
    digitalWrite(RMT1_PIN, HIGH);
    Serial.print("Classification 1");
    numPowerPoseClassifications = 0;
  }
    //If we've captured numSamples pictures in the right pose, 
//  //turn off the alarm for another 24 hours. 
//  if (numPowerPose2Classifications > numSamples) {
//    digitalWrite(RMT2_PIN, HIGH);
//    Serial.print("Classification 2 ");
//    numPowerPoseClassifications = 0;
//  }


}
