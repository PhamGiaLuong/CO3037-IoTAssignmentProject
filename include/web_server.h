#ifndef __WEB_SERVER_H__
#define __WEB_SERVER_H__

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WebServer.h>

#include "global.h"
#include "hardware_manager.h"

#define HTTP_PORT 80

// JSON KEYS - Network Settings
#define JSON_AP_SSID "ap_ssid"
#define JSON_AP_PASS "ap_password"
#define JSON_WIFI_SSID "wifi_ssid"
#define JSON_WIFI_PASS "wifi_password"
// JSON KEYS - Cloud Settings
#define JSON_SERVER "core_iot_server"
#define JSON_PORT "core_iot_port"
#define JSON_TOKEN "core_iot_token"
#define JSON_SEND_INTERVAL "send_interval_ms"
// JSON KEYS - Sensor Node Settings
#define JSON_READ_INTERVAL "read_interval_ms"
#define JSON_MIN_TEMP "min_temp_threshold"
#define JSON_MAX_TEMP "max_temp_threshold"
#define JSON_MIN_HUM "min_humidity_threshold"
#define JSON_MAX_HUM "max_humidity_threshold"
// JSON KEYS - Device Control
#define JSON_DEVICE "device"
#define JSON_STATE "state"

void webServerTask(void* pvParameters);

void updateNodeData(const char* mac, const SensorData& data, const MlState& ml);

#endif  // __WEB_SERVER_H__