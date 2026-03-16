#ifndef __HARDWARE_MANAGER_H__
#define __HARDWARE_MANAGER_H__

#include "global.h"

#define BUZZER_PIN 2

void initHardware();
void buzzerControl(bool turn_on);

#endif  // __HARDWARE_MANAGER_H__