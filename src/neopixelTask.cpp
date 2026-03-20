#include "neopixelTask.h"

#define COLOR_OFF strip.Color(0, 0, 0)
#define COLOR_RED strip.Color(255, 0, 0)
#define COLOR_GREEN strip.Color(0, 255, 0)
#define COLOR_BLUE strip.Color(0, 0, 255)
#define COLOR_YELLOW strip.Color(255, 255, 0)

Adafruit_NeoPixel strip(NEOPIXEL_NUM_LEDS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setNeopixelColor(uint32_t color) {
    for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, color);
    }
    strip.show();
}

void neopixelTask(void *pvParameters) {
    LOG_INFO("NEOPIXEL", "Neopixel task started");
    bool neo_state = false;

    while (1) {
        uint32_t error_flag = getActiveErrorFlags();
        if (error_flag & EVENT_SENSOR_ERROR) {
            neo_state = !neo_state;
            setNeopixelColor(neo_state ? COLOR_RED : COLOR_OFF);
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        } else if (error_flag & EVENT_HUM_LOW) {
            setNeopixelColor(COLOR_YELLOW);
        } else if (error_flag & EVENT_HUM_HIGH) {
            setNeopixelColor(COLOR_BLUE);
        } else {
            setNeopixelColor(COLOR_GREEN);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
