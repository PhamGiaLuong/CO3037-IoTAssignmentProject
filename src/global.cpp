#include "global.h"

// PRIVATE VARIABLES
// Data storage
static SensorData sensor_data = {0.0, 0.0, false, false};
static ControlState control_state = {false, false};
static SystemConfig system_config;
static SystemState system_state = {0, false, false, false};
static MlState ml_state = {"Waiting for Data", 0.0};

// Mutexes for data protection
static SemaphoreHandle_t sensor_mutex = NULL;
static SemaphoreHandle_t control_mutex = NULL;
static SemaphoreHandle_t config_mutex = NULL;
static SemaphoreHandle_t state_mutex = NULL;
static SemaphoreHandle_t ml_mutex = NULL;
SemaphoreHandle_t serial_mutex = NULL;

// Preferences for ROM storage
static Preferences preferences;

// PUBLIC BINARY SEMAPHORES
SemaphoreHandle_t temp_warning_semaphore = NULL;
SemaphoreHandle_t hum_warning_semaphore = NULL;
SemaphoreHandle_t sensor_error_semaphore = NULL;
SemaphoreHandle_t wifi_error_semaphore = NULL;
SemaphoreHandle_t coreiot_error_semaphore = NULL;
SemaphoreHandle_t lcd_sync_semaphore = NULL;
SemaphoreHandle_t switch_to_ap_semaphore = NULL;
SemaphoreHandle_t switch_to_sta_semaphore = NULL;

// SYSTEM INITIALIZATION
void initGlobal() {
    // Initialize Mutexes
    sensor_mutex = xSemaphoreCreateMutex();
    control_mutex = xSemaphoreCreateMutex();
    config_mutex = xSemaphoreCreateMutex();
    serial_mutex = xSemaphoreCreateMutex();
    state_mutex = xSemaphoreCreateMutex();
    ml_mutex = xSemaphoreCreateMutex();

    // Initialize Binary Semaphores
    temp_warning_semaphore = xSemaphoreCreateBinary();
    hum_warning_semaphore = xSemaphoreCreateBinary();
    sensor_error_semaphore = xSemaphoreCreateBinary();
    wifi_error_semaphore = xSemaphoreCreateBinary();
    coreiot_error_semaphore = xSemaphoreCreateBinary();
    lcd_sync_semaphore = xSemaphoreCreateBinary();
    switch_to_ap_semaphore = xSemaphoreCreateBinary();
    switch_to_sta_semaphore = xSemaphoreCreateBinary();

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

// System State API
SystemState getSystemState() {
    SystemState copy;
    if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
        copy = system_state;
        xSemaphoreGive(state_mutex);
    }
    return copy;
}

void setSystemState(const SystemState& state) {
    if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
        system_state = state;
        xSemaphoreGive(state_mutex);
    }
}

// ML State API
MlState getMlState() {
    MlState temp;
    if (xSemaphoreTake(ml_mutex, portMAX_DELAY) == pdTRUE) {
        temp = ml_state;
        xSemaphoreGive(ml_mutex);
    }
    return temp;
}

void setMlState(const MlState& state) {
    if (xSemaphoreTake(ml_mutex, portMAX_DELAY) == pdTRUE) {
        ml_state = state;
        xSemaphoreGive(ml_mutex);
    }
}

// Event Management API
void setSystemErrorFlag(uint16_t flag) {
    if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
        system_state.active_error_flags |= flag;
        xSemaphoreGive(state_mutex);
    }
}

void clearSystemErrorFlag(uint16_t flag) {
    if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
        system_state.active_error_flags &= ~flag;
        xSemaphoreGive(state_mutex);
    }
}

bool checkSystemErrorFlag(uint16_t flag) {
    bool is_set = false;
    if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
        is_set = (system_state.active_error_flags & flag) != 0;
        xSemaphoreGive(state_mutex);
    }
    return is_set;
}

uint32_t getActiveErrorFlags() {
    uint32_t flags = 0;
    if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
        flags = system_state.active_error_flags;
        xSemaphoreGive(state_mutex);
    }
    return flags;
}

// ROM STORAGE FUNCTIONS
void loadConfigFromFlash() {
    bool isNamespaceExist =
        preferences.begin(PREFERENCES_NAMESPACE, true);  // Read-only

    if (xSemaphoreTake(config_mutex, portMAX_DELAY) == pdTRUE) {
        // Access Point
        size_t len_ap_ssid = preferences.getString(
            AP_SSID_KEY, system_config.ap_ssid, MAX_SSID_LEN);
        if (len_ap_ssid == 0) {
            strlcpy(system_config.ap_ssid, DEFAULT_AP_SSID_VALUE, MAX_SSID_LEN);
        }
        size_t len_ap_pass = preferences.getString(
            AP_PASS_KEY, system_config.ap_password, MAX_PASS_LEN);
        if (len_ap_pass == 0) {
            strlcpy(system_config.ap_password, DEFAULT_AP_PASS_VALUE,
                    MAX_PASS_LEN);
        }
        // WiFi Station
        size_t len_wifi_ssid = preferences.getString(
            WIFI_SSID_KEY, system_config.wifi_ssid, MAX_SSID_LEN);
        if (len_wifi_ssid == 0) system_config.wifi_ssid[0] = '\0';
        size_t len_wifi_pass = preferences.getString(
            WIFI_PASS_KEY, system_config.wifi_password, MAX_PASS_LEN);
        if (len_wifi_pass == 0) system_config.wifi_password[0] = '\0';
        // Core IoT
        size_t len_token = preferences.getString(
            CORE_IOT_TOKEN_KEY, system_config.core_iot_token, MAX_TOKEN_LEN);
        if (len_token == 0) system_config.core_iot_token[0] = '\0';
        size_t len_server = preferences.getString(
            CORE_IOT_SERVER_KEY, system_config.core_iot_server, MAX_SERVER_LEN);
        if (len_server == 0)
            strlcpy(system_config.core_iot_server, DEFAULT_CORE_IOT_SERVER,
                    MAX_SERVER_LEN);
        system_config.core_iot_port =
            preferences.getShort(CORE_IOT_PORT_KEY, DEFAULT_CORE_IOT_PORT);
        system_config.send_interval_ms =
            preferences.getShort(SEND_INTERVAL_KEY, DEFAULT_SEND_INTERVAL_MS);
        // Thresholds and intervals
        system_config.read_interval_ms =
            preferences.getShort(READ_INTERVAL_KEY, DEFAULT_READ_INTERVAL_MS);
        system_config.max_temp_threshold =
            preferences.getFloat(MAX_TEMP_KEY, DEFAULT_MAX_TEMP_THRESHOLD);
        system_config.min_temp_threshold =
            preferences.getFloat(MIN_TEMP_KEY, DEFAULT_MIN_TEMP_THRESHOLD);
        system_config.max_humidity_threshold =
            preferences.getFloat(MAX_HUM_KEY, DEFAULT_MAX_HUM_THRESHOLD);
        system_config.min_humidity_threshold =
            preferences.getFloat(MIN_HUM_KEY, DEFAULT_MIN_HUM_THRESHOLD);

        xSemaphoreGive(config_mutex);
    }
    preferences.end();
    if (!isNamespaceExist) {
        LOG_INFO("GLOBAL",
                 "First boot detected. Creating Preferences namespace...");
        saveConfigToFlash();
    }
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
        preferences.putShort(CORE_IOT_PORT_KEY, system_config.core_iot_port);
        preferences.putShort(SEND_INTERVAL_KEY, system_config.send_interval_ms);
        preferences.putShort(READ_INTERVAL_KEY, system_config.read_interval_ms);
        preferences.putFloat(MAX_TEMP_KEY, system_config.max_temp_threshold);
        preferences.putFloat(MIN_TEMP_KEY, system_config.min_temp_threshold);
        preferences.putFloat(MAX_HUM_KEY, system_config.max_humidity_threshold);
        preferences.putFloat(MIN_HUM_KEY, system_config.min_humidity_threshold);

        xSemaphoreGive(config_mutex);
    }

    preferences.end();
}