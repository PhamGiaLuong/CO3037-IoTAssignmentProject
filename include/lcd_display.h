#include "LiquidCrystal_I2C.h"
#include "Wire.h"
#include "global.h"

#define SENSOR_SDA_PIN 11
#define SENSOR_SCL_PIN 12

void lcdDisplayTask(void* pvParameters);
