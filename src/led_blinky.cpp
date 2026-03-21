#include "led_blinky.h"

// TIMING CONFIGURATIONS
#define NORMAL_ON_MS 500
#define NORMAL_OFF_MS 2500

#define WARNING_ON_MS 500
#define WARNING_OFF_MS 500

#define CRITICAL_ON_MS 250
#define CRITICAL_OFF_MS 250

// EVENT MASKS FOR SEVERITY LEVELS
#define CRITICAL_MASK                                            \
    (EVENT_SENSOR_ERROR | EVENT_LCD_ERROR | EVENT_WIFI_DISCONN | \
     EVENT_COREIOT_DISCONN)
#define WARNING_MASK (EVENT_TEMP_WARNING | EVENT_NET_AP_MODE)

enum LedLevel { LEVEL_NORMAL, LEVEL_WARNING, LEVEL_CRITICAL };

void ledBlinkyTask(void* pvParameters) {
    LOG_INFO("LED_BLINKY", "LED blinky task started");

    pinMode(BLINKY_LED_PIN, OUTPUT);
    digitalWrite(BLINKY_LED_PIN, LOW);

    LedLevel current_level = LEVEL_NORMAL;
    bool is_led_on = false;
    TickType_t block_time = pdMS_TO_TICKS(NORMAL_OFF_MS);

    while (1) {
        if (xSemaphoreTake(led_sync_semaphore, block_time) == pdTRUE) {
            uint32_t current_flags = getActiveErrorFlags();
            LedLevel new_level = LEVEL_NORMAL;

            if (current_flags & CRITICAL_MASK) {
                new_level = LEVEL_CRITICAL;
            } else if (current_flags & WARNING_MASK) {
                new_level = LEVEL_WARNING;
            }

            if (new_level != current_level) {
                current_level = new_level;
                is_led_on = true;
                digitalWrite(BLINKY_LED_PIN, HIGH);

                if (current_level == LEVEL_CRITICAL) {
                    block_time = pdMS_TO_TICKS(CRITICAL_ON_MS);
                    LOG_WARN("LED_BLINKY", "System level changed to CRITICAL");
                } else if (current_level == LEVEL_WARNING) {
                    block_time = pdMS_TO_TICKS(WARNING_ON_MS);
                    LOG_INFO("LED_BLINKY", "System level changed to WARNING");
                } else {
                    block_time = pdMS_TO_TICKS(NORMAL_ON_MS);
                    LOG_INFO("LED_BLINKY", "System level restored to NORMAL");
                }
            }
        } else {
            is_led_on = !is_led_on;
            digitalWrite(BLINKY_LED_PIN, is_led_on ? HIGH : LOW);

            if (current_level == LEVEL_CRITICAL) {
                block_time =
                    pdMS_TO_TICKS(is_led_on ? CRITICAL_ON_MS : CRITICAL_OFF_MS);
            } else if (current_level == LEVEL_WARNING) {
                block_time =
                    pdMS_TO_TICKS(is_led_on ? WARNING_ON_MS : WARNING_OFF_MS);
            } else {
                block_time =
                    pdMS_TO_TICKS(is_led_on ? NORMAL_ON_MS : NORMAL_OFF_MS);
            }
            // LOG_INFO("LED_BLINKY", "LED toggled %s. Next toggle in %d ms",
            //  is_led_on ? "ON" : "OFF", block_time / portTICK_PERIOD_MS);
        }
    }
}