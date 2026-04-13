#ifndef __LED_BLINKY_H__
#define __LED_BLINKY_H__

#include "global.h"

#define SENSOR_LED_PIN 48
#define GATEWAY_LED_PIN 48

void sensorLedBlinkyTask(void* pvParameters);
void gatewayLedBlinkyTask(void* pvParameters);

#endif  // __LED_BLINKY_H__