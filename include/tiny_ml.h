#ifndef __TINY_ML_H__
#define __TINY_ML_H__

#include <Arduino.h>
#include <TensorFlowLite_ESP32.h>

#include "TinyML.h"
#include "global.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

void tinyMlTask(void* pvParameters);
void evaluateModelAccuracy();

#endif  // __TINY_ML_H__