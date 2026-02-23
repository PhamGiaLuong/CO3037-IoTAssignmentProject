#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include <WiFi.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

void initSensorMutex();
void setSensorData(float temp, float humi);
void getSensorData(float& temp, float& humi);

#endif