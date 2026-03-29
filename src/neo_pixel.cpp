#include "neo_pixel.h"

#include "read_sensor.h"

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
    bool neo_state = true;
    TickType_t wait_time = portMAX_DELAY;
    while (1) {
        bool is_new_event =
            (xSemaphoreTake(neo_pixel_sync_semaphore, wait_time) == pdTRUE);

        uint32_t error_flag = getSensorActiveErrorFlags();

        if (is_new_event) {
            neo_state = true;
        } else {
            neo_state = !neo_state;
        }

        if (error_flag & SENSOR_FLAG_DHT_ERR) {
            setNeopixelColor(neo_state ? COLOR_RED : COLOR_OFF);
            wait_time = pdMS_TO_TICKS(500);
        } else if (hum_error == EVENT_HUM_LOW) {
            setNeopixelColor(COLOR_YELLOW);
            wait_time = pdMS_TO_TICKS(1000);
        } else if (hum_error == EVENT_HUM_HIGH) {
            setNeopixelColor(COLOR_BLUE);
            wait_time = pdMS_TO_TICKS(1000);
        } else {
            setNeopixelColor(COLOR_GREEN);
            wait_time = portMAX_DELAY;
        }
    }
}
