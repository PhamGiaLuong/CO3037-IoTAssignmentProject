#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

// MACROS & CONSTANTS
#define DEFAULT_READ_INTERVAL_MS 2000
#define DEFAULT_TEMP_THRESHOLD 35.0
#define DEFAULT_HUM_THRESHOLD 80.0
#define DEFAULT_AP_SSID_VALUE "ESP32_AP"
#define DEFAULT_AP_PASS_VALUE "11335588"
#define PREFERENCES_NAMESPACE "iot_config"
#define AP_SSID_KEY "ap_ssid"
#define AP_PASS_KEY "ap_pass"
#define WIFI_SSID_KEY "wifi_ssid"
#define WIFI_PASS_KEY "wifi_pass"
#define CORE_IOT_TOKEN_KEY "core_iot_token"
#define CORE_IOT_SERVER_KEY "core_iot_server"
#define CORE_IOT_PORT_KEY "core_iot_port"
#define READ_INTERVAL_KEY "read_interval"
#define MAX_TEMP_KEY "max_temp"
#define MAX_HUM_KEY "max_hum"

// STRUCTS
struct SensorData {
    float current_temperature;
    float current_humidity;
    bool is_dht20_ok;
    bool is_lcd_ok;
};

struct ControlState {
    bool is_device1_on;
    bool is_device2_on;
};

struct SystemConfig {
    String ap_ssid;
    String ap_password;
    String wifi_ssid;
    String wifi_password;
    String core_iot_token;
    String core_iot_server;
    String core_iot_port;
    int read_interval_ms;
    float max_temp_threshold;
    float max_humidity_threshold;
};

// BINARY SEMAPHORES (Event Signaling)
extern SemaphoreHandle_t temp_warning_semaphore;
extern SemaphoreHandle_t hum_warning_semaphore;
extern SemaphoreHandle_t sensor_error_semaphore;
extern SemaphoreHandle_t wifi_error_semaphore;
extern SemaphoreHandle_t coreiot_error_semaphore;

// THREAD-SAFE LOGGING MACROS
// Syntax: LOG_INFO("MODULE_NAME", "Message format", variables...);
extern SemaphoreHandle_t serial_mutex;
#define LOG_INFO(tag, format, ...)                                           \
    do {                                                                     \
        if (xSemaphoreTake(serial_mutex, portMAX_DELAY) == pdTRUE) {         \
            Serial.printf("[INFO] [%s] " format "\r\n", tag, ##__VA_ARGS__); \
            xSemaphoreGive(serial_mutex);                                    \
        }                                                                    \
    } while (0)

#define LOG_WARN(tag, format, ...)                                           \
    do {                                                                     \
        if (xSemaphoreTake(serial_mutex, portMAX_DELAY) == pdTRUE) {         \
            Serial.printf("[WARN] [%s] " format "\r\n", tag, ##__VA_ARGS__); \
            xSemaphoreGive(serial_mutex);                                    \
        }                                                                    \
    } while (0)

#define LOG_ERR(tag, format, ...)                                            \
    do {                                                                     \
        if (xSemaphoreTake(serial_mutex, portMAX_DELAY) == pdTRUE) {         \
            Serial.printf("[ERR]  [%s] " format "\r\n", tag, ##__VA_ARGS__); \
            xSemaphoreGive(serial_mutex);                                    \
        }                                                                    \
    } while (0)

// SYSTEM APIs
void initGlobal();
void loadConfigFromFlash();
void saveConfigToFlash();

// Sensor Data API
SensorData getSensorData();
void setSensorData(const SensorData& data);

// Control State API
ControlState getControlState();
void setControlState(const ControlState& state);

// System Config API
SystemConfig getSystemConfig();
void setSystemConfig(const SystemConfig& config);

#endif