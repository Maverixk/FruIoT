#include "tinyml.h"
#include "model.h"
#include "sensors/sensors.h"

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

// Scaler parameters
const float MEAN_RAW   = 462.75464684;
const float STD_RAW    = 198.68084281; // Square root of 3.94740773e+04

const float MEAN_RATIO = 0.54427414;
const float STD_RATIO  = 0.30030085;   // Square root of 9.01806035e-02

const float MEAN_TEMP  = 24.53048327;
const float STD_TEMP   = 0.95095342;   // Square root of 9.04312406e-01

const float MEAN_HUM   = 96.22732342;
const float STD_HUM    = 5.83833446;   // Square root of 3.40861493e+01     

// TFLite globals
namespace {
    tflite::ErrorReporter* error_reporter = nullptr;
    const tflite::Model* model = nullptr;
    tflite::MicroInterpreter* interpreter = nullptr;
    TfLiteTensor* input = nullptr;
    TfLiteTensor* output = nullptr;

    // Use RTC_DATA_ATTR if you want the memory to survive deep sleep!
    RTC_DATA_ATTR uint8_t tensor_arena[4 * 1024]; 
    RTC_DATA_ATTR bool model_initialized = false;
}

bool init_model() {
    // If waking from deep sleep, and memory was kept in RTC, skip initialization!
    if (model_initialized) return true;

    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(fruiot_model_quantized_tflite); 
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        return false;
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, sizeof(tensor_arena), error_reporter);
    interpreter = &static_interpreter;

    if (interpreter->AllocateTensors() != kTfLiteOk) {
        return false;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);
    
    model_initialized = true;
    return true;
}

int predict_status(sensors::SensorData data) {
    if (!model_initialized) return -1;

    // Scale
    float scaled_raw   = (data.mq135Raw - MEAN_RAW) / STD_RAW;
    float scaled_ratio = (data.mq135Ratio - MEAN_RATIO) / STD_RATIO;
    float scaled_temp  = (data.temperatureC - MEAN_TEMP) / STD_TEMP;
    float scaled_hum   = (data.humidityPct - MEAN_HUM) / STD_HUM;

    // Quantize
    input->data.int8[0] = scaled_raw   / input->params.scale + input->params.zero_point;
    input->data.int8[1] = scaled_ratio / input->params.scale + input->params.zero_point;
    input->data.int8[2] = scaled_temp  / input->params.scale + input->params.zero_point;
    input->data.int8[3] = scaled_hum   / input->params.scale + input->params.zero_point;

    // Predict
    if (interpreter->Invoke() != kTfLiteOk) return -1;

    // Dequantize
    float prob_unripe = (output->data.int8[0] - output->params.zero_point) * output->params.scale;
    float prob_mature = (output->data.int8[1] - output->params.zero_point) * output->params.scale;
    float prob_ruined = (output->data.int8[2] - output->params.zero_point) * output->params.scale;

    // Take highest probability class
    if (prob_unripe > prob_mature && prob_unripe > prob_ruined) return 0;
    if (prob_mature > prob_unripe && prob_mature > prob_ruined) return 1;
    return 2;
}