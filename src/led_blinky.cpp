#include "led_blinky.h"

// TIMING CONFIGURATIONS
#define NORMAL_ON_MS 500
#define NORMAL_OFF_MS 2500

#define WARNING_ON_MS 500
#define WARNING_OFF_MS 500

#define CRITICAL_ON_MS 250
#define CRITICAL_OFF_MS 250

// EVENT MASKS FOR SEVERITY LEVELS
#define SENSOR_CRITICAL_MASK (SENSOR_FLAG_DHT_ERR | SENSOR_FLAG_LCD_ERR)
#define SENSOR_WARNING_MASK (SENSOR_FLAG_TEMP_WARN | SENSOR_FLAG_HUM_WARN)

#define GW_CRITICAL_MASK (GW_FLAG_WIFI_DISCONN | GW_FLAG_COREIOT_DISCONN)
#define GW_WARNING_MASK (GW_FLAG_NET_AP_MODE)

enum LedLevel { LEVEL_NORMAL, LEVEL_WARNING, LEVEL_CRITICAL };

// Sensor node: warning based on DHT20 sensor status, LCD status, temperature
// and humidity thresholds
void sensorLedBlinkyTask(void* pvParameters) {
    LOG_INFO("LED_SENSOR", "Sensor LED blinky task started");

    pinMode(SENSOR_LED_PIN, OUTPUT);
    digitalWrite(SENSOR_LED_PIN, LOW);

    LedLevel current_level = LEVEL_NORMAL;
    bool is_led_on = false;
    TickType_t block_time = pdMS_TO_TICKS(NORMAL_OFF_MS);

    while (1) {
        if (xSemaphoreTake(sensor_led_sync_semaphore, block_time) == pdTRUE) {
            uint32_t current_flags = getSensorActiveErrorFlags();
            LedLevel new_level = LEVEL_NORMAL;

            if (current_flags & SENSOR_CRITICAL_MASK) {
                new_level = LEVEL_CRITICAL;
            } else if (current_flags & SENSOR_WARNING_MASK) {
                new_level = LEVEL_WARNING;
            }

            if (new_level != current_level) {
                current_level = new_level;
                is_led_on = true;
                digitalWrite(SENSOR_LED_PIN, HIGH);

                if (current_level == LEVEL_CRITICAL) {
                    block_time = pdMS_TO_TICKS(CRITICAL_ON_MS);
                    LOG_WARN("LED_SENSOR", "System level changed to CRITICAL");
                } else if (current_level == LEVEL_WARNING) {
                    block_time = pdMS_TO_TICKS(WARNING_ON_MS);
                    LOG_INFO("LED_SENSOR", "System level changed to WARNING");
                } else {
                    block_time = pdMS_TO_TICKS(NORMAL_ON_MS);
                    LOG_INFO("LED_SENSOR", "System level restored to NORMAL");
                }
            }
        } else {
            is_led_on = !is_led_on;
            digitalWrite(SENSOR_LED_PIN, is_led_on ? HIGH : LOW);

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
        }
    }
}

// Gateway node: warning base on Network status (AP mode, WiFi disconnected,
// Core IoT disconnected)
void gatewayLedBlinkyTask(void* pvParameters) {
    LOG_INFO("LED_GATEWAY", "Gateway LED blinky task started");

    pinMode(GATEWAY_LED_PIN, OUTPUT);
    digitalWrite(GATEWAY_LED_PIN, LOW);

    LedLevel current_level = LEVEL_NORMAL;
    bool is_led_on = false;
    TickType_t block_time = pdMS_TO_TICKS(NORMAL_OFF_MS);

    while (1) {
        if (xSemaphoreTake(gw_led_sync_semaphore, block_time) == pdTRUE) {
            uint32_t current_flags = getGatewayActiveErrorFlags();
            LedLevel new_level = LEVEL_NORMAL;

            if (current_flags & GW_CRITICAL_MASK) {
                new_level = LEVEL_CRITICAL;
            } else if (current_flags & GW_WARNING_MASK) {
                new_level = LEVEL_WARNING;
            }

            if (new_level != current_level) {
                current_level = new_level;
                is_led_on = true;
                digitalWrite(GATEWAY_LED_PIN, HIGH);

                if (current_level == LEVEL_CRITICAL) {
                    block_time = pdMS_TO_TICKS(CRITICAL_ON_MS);
                    LOG_WARN("LED_GATEWAY",
                             "Network level changed to CRITICAL");
                } else if (current_level == LEVEL_WARNING) {
                    block_time = pdMS_TO_TICKS(WARNING_ON_MS);
                    LOG_INFO("LED_GATEWAY",
                             "Network level changed to WARNING (AP Mode)");
                } else {
                    block_time = pdMS_TO_TICKS(NORMAL_ON_MS);
                    LOG_INFO("LED_GATEWAY", "Network level restored to NORMAL");
                }
            }
        } else {
            is_led_on = !is_led_on;
            digitalWrite(GATEWAY_LED_PIN, is_led_on ? HIGH : LOW);

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
        }
    }
}