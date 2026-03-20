#ifndef __WEB_SERVER_H__
#define __WEB_SERVER_H__

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WebServer.h>

#include "global.h"
#include "hardware_manager.h"

// JSON KEYS
// Dashboard Keys
#define JSON_TEMP "temperature"
#define JSON_HUM "humidity"
#define JSON_DHT_STATUS "dht_status"
#define JSON_LCD_STATUS "lcd_status"
// Data Threshold
#define JSON_MIN_TEMP "min_temp_threshold"
#define JSON_MAX_TEMP "max_temp_threshold"
#define JSON_MIN_HUM "min_humidity_threshold"
#define JSON_MAX_HUM "max_humidity_threshold"
// ML
#define JSON_ML_PREDICTION "ml_prediction"
#define JSON_ML_CONFIDENCE "ml_confidence"
// Device Control
#define JSON_DEVICE "device"
#define JSON_STATE "state"
#define JSON_DEVICE1 "device1"
#define JSON_DEVICE2 "device2"
// Network Settings
#define JSON_AP_SSID "ap_ssid"
#define JSON_AP_PASS "ap_password"
#define JSON_WIFI_SSID "wifi_ssid"
#define JSON_WIFI_PASS "wifi_password"
// Cloud Settings
#define JSON_SERVER "core_iot_server"
#define JSON_PORT "core_iot_port"
#define JSON_TOKEN "core_iot_token"
// Interval Keys
#define JSON_SEND_INTERVAL "send_interval_ms"
#define JSON_READ_INTERVAL "read_interval_ms"

void webServerTask(void* pvParameters);

#endif  // __WEB_SERVER_H__