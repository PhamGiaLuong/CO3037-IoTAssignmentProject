#include "global.h"

// PRIVATE VARIABLES
// Data storage
static SensorData sensor_data = {0.0, 0.0, false, false};
static ControlState control_state = {false, false};
static SystemConfig system_config;

// Mutexes for data protection
static SemaphoreHandle_t sensor_mutex = NULL;
static SemaphoreHandle_t control_mutex = NULL;
static SemaphoreHandle_t config_mutex = NULL;
static SemaphoreHandle_t serial_mutex = NULL;

// Preferences for ROM storage
static Preferences preferences;

// PUBLIC BINARY SEMAPHORES
SemaphoreHandle_t temp_warning_semaphore = NULL;
SemaphoreHandle_t hum_warning_semaphore = NULL;
SemaphoreHandle_t sensor_error_semaphore = NULL;
SemaphoreHandle_t wifi_error_semaphore = NULL;
SemaphoreHandle_t coreiot_error_semaphore = NULL;

// SYSTEM INITIALIZATION
void initGlobal() {
    // Initialize Mutexes
    sensor_mutex = xSemaphoreCreateMutex();
    control_mutex = xSemaphoreCreateMutex();
    config_mutex = xSemaphoreCreateMutex();
    serial_mutex = xSemaphoreCreateMutex();

    // Initialize Binary Semaphores
    temp_warning_semaphore = xSemaphoreCreateBinary();
    hum_warning_semaphore = xSemaphoreCreateBinary();
    sensor_error_semaphore = xSemaphoreCreateBinary();
    wifi_error_semaphore = xSemaphoreCreateBinary();
    coreiot_error_semaphore = xSemaphoreCreateBinary();

    LOG_INFO("GLOBAL", "Global RTOS objects initialized");
}

// Sensor Data API
SensorData getSensorData() {
    SensorData copy;
    if (xSemaphoreTake(sensor_mutex, portMAX_DELAY) == pdTRUE) {
        copy = sensor_data;
        xSemaphoreGive(sensor_mutex);
    }
    return copy;
}

void setSensorData(const SensorData& data) {
    if (xSemaphoreTake(sensor_mutex, portMAX_DELAY) == pdTRUE) {
        sensor_data = data;
        xSemaphoreGive(sensor_mutex);
    }
}

// Control State API
ControlState getControlState() {
    ControlState copy;
    if (xSemaphoreTake(control_mutex, portMAX_DELAY) == pdTRUE) {
        copy = control_state;
        xSemaphoreGive(control_mutex);
    }
    return copy;
}

void setControlState(const ControlState& state) {
    if (xSemaphoreTake(control_mutex, portMAX_DELAY) == pdTRUE) {
        control_state = state;
        xSemaphoreGive(control_mutex);
    }
}

// System Config API
SystemConfig getSystemConfig() {
    SystemConfig copy;
    if (xSemaphoreTake(config_mutex, portMAX_DELAY) == pdTRUE) {
        copy = system_config;
        xSemaphoreGive(config_mutex);
    }
    return copy;
}

void setSystemConfig(const SystemConfig& config) {
    if (xSemaphoreTake(config_mutex, portMAX_DELAY) == pdTRUE) {
        system_config = config;
        xSemaphoreGive(config_mutex);
    }
}

// ROM STORAGE FUNCTIONS
void loadConfigFromFlash() {
    preferences.begin(PREFERENCES_NAMESPACE, true);  // Read-only

    if (xSemaphoreTake(config_mutex, portMAX_DELAY) == pdTRUE) {
        system_config.ap_ssid =
            preferences.getString(AP_SSID_KEY, DEFAULT_AP_SSID_VALUE);
        system_config.ap_password =
            preferences.getString(AP_PASS_KEY, DEFAULT_AP_PASS_VALUE);
        system_config.wifi_ssid = preferences.getString(WIFI_SSID_KEY, "");
        system_config.wifi_password = preferences.getString(WIFI_PASS_KEY, "");
        system_config.core_iot_token =
            preferences.getString(CORE_IOT_TOKEN_KEY, "");
        system_config.core_iot_server =
            preferences.getString(CORE_IOT_SERVER_KEY, "");
        system_config.core_iot_port =
            preferences.getString(CORE_IOT_PORT_KEY, "");

        system_config.read_interval_ms =
            preferences.getInt(READ_INTERVAL_KEY, DEFAULT_READ_INTERVAL_MS);
        system_config.max_temp_threshold =
            preferences.getFloat(MAX_TEMP_KEY, DEFAULT_TEMP_THRESHOLD);
        system_config.max_humidity_threshold =
            preferences.getFloat(MAX_HUM_KEY, DEFAULT_HUM_THRESHOLD);

        xSemaphoreGive(config_mutex);
    }

    preferences.end();
}

void saveConfigToFlash() {
    preferences.begin(PREFERENCES_NAMESPACE, false);  // Read-Write

    if (xSemaphoreTake(config_mutex, portMAX_DELAY) == pdTRUE) {
        preferences.putString(AP_SSID_KEY, system_config.ap_ssid);
        preferences.putString(AP_PASS_KEY, system_config.ap_password);
        preferences.putString(WIFI_SSID_KEY, system_config.wifi_ssid);
        preferences.putString(WIFI_PASS_KEY, system_config.wifi_password);
        preferences.putString(CORE_IOT_TOKEN_KEY, system_config.core_iot_token);
        preferences.putString(CORE_IOT_SERVER_KEY,
                              system_config.core_iot_server);
        preferences.putString(CORE_IOT_PORT_KEY, system_config.core_iot_port);

        preferences.putInt(READ_INTERVAL_KEY, system_config.read_interval_ms);
        preferences.putFloat(MAX_TEMP_KEY, system_config.max_temp_threshold);
        preferences.putFloat(MAX_HUM_KEY, system_config.max_humidity_threshold);

        xSemaphoreGive(config_mutex);
    }

    preferences.end();
}