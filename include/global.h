#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Wire.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

// MACROS & CONSTANTS
#define DEFAULT_READ_INTERVAL_MS 2000
#define DEFAULT_SEND_INTERVAL_MS 5000
#define DEFAULT_MAX_HUM_THRESHOLD 80.0
#define DEFAULT_MIN_HUM_THRESHOLD 40.0
#define DEFAULT_MAX_TEMP_THRESHOLD 35.0
#define DEFAULT_MIN_TEMP_THRESHOLD 20.0
#define DEFAULT_CORE_IOT_PORT 1883
#define DEFAULT_CORE_IOT_SERVER "app.coreiot.io"
#define DEFAULT_AP_SSID_VALUE "ESP32_IoT_AP"
#define DEFAULT_AP_PASS_VALUE "11335588"
#define PREFERENCES_NAMESPACE "co3037_ns"
#define AP_SSID_KEY "ap_ssid"
#define AP_PASS_KEY "ap_pass"
#define WIFI_SSID_KEY "wifi_ssid"
#define WIFI_PASS_KEY "wifi_pass"
#define CORE_IOT_TOKEN_KEY "0mpjdux3sh40nhu4qrpb"
#define CORE_IOT_SERVER_KEY "c_iot_server"
#define CORE_IOT_PORT_KEY "c_iot_port"
#define READ_INTERVAL_KEY "interval"
#define SEND_INTERVAL_KEY "send_int"
#define MAX_TEMP_KEY "max_temp"
#define MIN_TEMP_KEY "min_temp"
#define MAX_HUM_KEY "max_hum"
#define MIN_HUM_KEY "min_hum"
#define MAX_SSID_LEN 32
#define MAX_PASS_LEN 64
#define MAX_TOKEN_LEN 64
#define MAX_SERVER_LEN 64
#define MAX_PAIRED_NODES 10
#define MAC_STR_LEN 18

// Sensor Node Flags (Task 1, 2, 3)
#define SENSOR_FLAG_TEMP_WARN (1 << 0)
#define SENSOR_FLAG_HUM_WARN (1 << 1)
#define SENSOR_FLAG_DHT_ERR (1 << 2)
#define SENSOR_FLAG_LCD_ERR (1 << 3)

// Gateway Node Flags (Task 1, 4, 6)
#define GW_FLAG_NET_AP_MODE (1 << 0)
#define GW_FLAG_WIFI_DISCONN (1 << 1)
#define GW_FLAG_COREIOT_DISCONN (1 << 2)

// STRUCTS
struct SensorData {
    float current_temperature;
    float current_humidity;
    bool is_dht20_ok;
    bool is_lcd_ok;
};

struct MlState {
    char prediction[32];
    float confidence;
};

struct SensorConfig {
    int16_t read_interval_ms;
    float max_temp_threshold;
    float min_temp_threshold;
    float max_humidity_threshold;
    float min_humidity_threshold;
};

struct SensorState {
    uint16_t active_error_flags;
};

struct GatewayConfig {
    char ap_ssid[MAX_SSID_LEN];
    char ap_password[MAX_PASS_LEN];
    char wifi_ssid[MAX_SSID_LEN];
    char wifi_password[MAX_PASS_LEN];
    char core_iot_token[MAX_TOKEN_LEN];
    char core_iot_server[MAX_SERVER_LEN];
    int16_t core_iot_port;
    int16_t send_interval_ms;
};

struct GatewayState {
    uint16_t active_error_flags;
    bool is_ap_mode;
    bool is_wifi_connected;
    bool is_coreiot_connected;
};

struct ControlState {
    bool is_device1_on;
    bool is_device2_on;
};

struct PairedNode {
    char mac_address[MAC_STR_LEN];
    char node_name[32];
    bool is_active;

    SensorData current_data;
    MlState current_ml_state;
    SensorConfig current_config;
};

// BINARY SEMAPHORES (Event Signaling)
extern SemaphoreHandle_t gw_led_sync_semaphore;
extern SemaphoreHandle_t wifi_error_semaphore;
extern SemaphoreHandle_t coreiot_error_semaphore;
extern SemaphoreHandle_t switch_to_ap_semaphore;
extern SemaphoreHandle_t switch_to_sta_semaphore;

extern SemaphoreHandle_t sensor_led_sync_semaphore;
extern SemaphoreHandle_t neo_pixel_sync_semaphore;
extern SemaphoreHandle_t lcd_sync_semaphore;

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

// --- Shared Data APIs ---
SensorData getSensorData();
void setSensorData(const SensorData& data);

MlState getMlState();
void setMlState(const MlState& state);

// Sensor Node APIs
SensorConfig getSensorConfig();
void setSensorConfig(const SensorConfig& config);

SensorState getSensorState();
void setSensorErrorFlag(uint16_t flag);
void clearSensorErrorFlag(uint16_t flag);
bool checkSensorErrorFlag(uint16_t flag);
uint32_t getSensorActiveErrorFlags();

// Gateway Node APIs
GatewayConfig getGatewayConfig();
void setGatewayConfig(const GatewayConfig& config);

bool addPairedNode(const char* mac, const char* name);
bool removePairedNode(const char* mac);
uint8_t getActiveNodeCount();
uint8_t getPairedNodesSnapshot(PairedNode* out_array, uint8_t max_size);
bool getNodeByMac(const char* mac, PairedNode* out_node);
bool updateNodeConfig(const char* mac, const SensorConfig& new_config);
bool updateNodeDataAndMl(const char* mac, const SensorData& new_data,
                         const MlState& new_ml);

GatewayState getGatewayState();
void setGatewayState(const GatewayState& state);
void setGatewayErrorFlag(uint16_t flag);
void clearGatewayErrorFlag(uint16_t flag);
bool checkGatewayErrorFlag(uint16_t flag);
uint32_t getGatewayActiveErrorFlags();

ControlState getControlState();
void setControlState(const ControlState& state);

#endif