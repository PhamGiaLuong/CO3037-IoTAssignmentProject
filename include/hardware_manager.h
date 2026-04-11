#ifndef __HARDWARE_MANAGER_H__
#define __HARDWARE_MANAGER_H__

#include "global.h"

#define BUZZER_PIN 2
#define FAN_PIN 5

void buzzerTask(void* pvParameters);
void fanTask(void* pvParameters);

#endif  // __HARDWARE_MANAGER_H__