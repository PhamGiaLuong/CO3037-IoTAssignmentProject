#include "Adafruit_NeoPixel.h"
#include "global.h"

#define NEOPIXEL_NUM_LEDS 2
#define NEOPIXEL_PIN 45
extern Adafruit_NeoPixel strip;
void neopixelTask(void *pvParameters);