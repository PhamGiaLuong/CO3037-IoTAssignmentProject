#include "global.h"

// PRIVATE VARIABLES
// Shared Data
static SensorData sensor_data = {0.0, 0.0, false, false};
static MlState ml_state = {"Waiting for Data", 0.0};
static PairedNode paired_nodes[MAX_PAIRED_NODES];
static uint8_t active_node_count = 0;

// Sensor Node Data
static SensorConfig sensor_config;
static SensorState sensor_state = {0};

// Gateway Node Data
static GatewayConfig gateway_config;
static GatewayState gateway_state = {0, false, false, false};
static ControlState control_state = {false, false};

// Mutexes for data protection
SemaphoreHandle_t serial_mutex = NULL;
static SemaphoreHandle_t node_list_mutex = NULL;
static SemaphoreHandle_t sensor_data_mutex = NULL;
static SemaphoreHandle_t ml_state_mutex = NULL;

static SemaphoreHandle_t sensor_config_mutex = NULL;
static SemaphoreHandle_t sensor_state_mutex = NULL;

static SemaphoreHandle_t gw_config_mutex = NULL;
static SemaphoreHandle_t gw_state_mutex = NULL;
static SemaphoreHandle_t control_state_mutex = NULL;

// Preferences for ROM storage
static Preferences preferences;

// PUBLIC BINARY SEMAPHORES
// Sensor
SemaphoreHandle_t sensor_led_sync_semaphore = NULL;
SemaphoreHandle_t neo_pixel_sync_semaphore = NULL;
SemaphoreHandle_t lcd_sync_semaphore = NULL;
// Gateway
SemaphoreHandle_t gw_led_sync_semaphore = NULL;
SemaphoreHandle_t switch_to_ap_semaphore = NULL;
SemaphoreHandle_t switch_to_sta_semaphore = NULL;
SemaphoreHandle_t wifi_error_semaphore = NULL;
SemaphoreHandle_t coreiot_error_semaphore = NULL;
SemaphoreHandle_t sensor_send_telemetry_semaphore = NULL;
QueueHandle_t gw_downlink_queue = NULL;

// SYSTEM INITIALIZATION
void initGlobal() {
    // Initialize Mutexes
    serial_mutex = xSemaphoreCreateMutex();
    sensor_data_mutex = xSemaphoreCreateMutex();
    ml_state_mutex = xSemaphoreCreateMutex();
    sensor_config_mutex = xSemaphoreCreateMutex();
    sensor_state_mutex = xSemaphoreCreateMutex();
    gw_config_mutex = xSemaphoreCreateMutex();
    gw_state_mutex = xSemaphoreCreateMutex();
    control_state_mutex = xSemaphoreCreateMutex();
    node_list_mutex = xSemaphoreCreateMutex();

    // Initialize Semaphores
    sensor_led_sync_semaphore = xSemaphoreCreateBinary();
    neo_pixel_sync_semaphore = xSemaphoreCreateBinary();
    lcd_sync_semaphore = xSemaphoreCreateBinary();

    gw_led_sync_semaphore = xSemaphoreCreateBinary();
    switch_to_ap_semaphore = xSemaphoreCreateBinary();
    switch_to_sta_semaphore = xSemaphoreCreateBinary();
    wifi_error_semaphore = xSemaphoreCreateBinary();
    coreiot_error_semaphore = xSemaphoreCreateBinary();
    sensor_send_telemetry_semaphore = xSemaphoreCreateBinary();

    gw_downlink_queue = xQueueCreate(10, sizeof(GwDownlinkMessage));

    LOG_INFO("GLOBAL", "Global RTOS objects initialized cleanly");
}

// SHARED DATA APIs
SensorData getSensorData() {
    SensorData copy;
    if (xSemaphoreTake(sensor_data_mutex, portMAX_DELAY) == pdTRUE) {
        copy = sensor_data;
        xSemaphoreGive(sensor_data_mutex);
    }
    return copy;
}

void setSensorData(const SensorData& data) {
    if (xSemaphoreTake(sensor_data_mutex, portMAX_DELAY) == pdTRUE) {
        sensor_data = data;
        xSemaphoreGive(sensor_data_mutex);
    }
}

MlState getMlState() {
    MlState copy;
    if (xSemaphoreTake(ml_state_mutex, portMAX_DELAY) == pdTRUE) {
        copy = ml_state;
        xSemaphoreGive(ml_state_mutex);
    }
    return copy;
}

void setMlState(const MlState& state) {
    if (xSemaphoreTake(ml_state_mutex, portMAX_DELAY) == pdTRUE) {
        ml_state = state;
        xSemaphoreGive(ml_state_mutex);
    }
}

// NODE MANAGEMENT FUNCTIONS
static int8_t findNodeIndexByMac_Internal(const char* mac) {
    for (uint8_t i = 0; i < MAX_PAIRED_NODES; i++) {
        if (paired_nodes[i].is_active &&
            strcasecmp(paired_nodes[i].mac_address, mac) == 0) {
            return i;
        }
    }
    return -1;
}

bool addPairedNode(const char* mac, const char* name) {
    bool success = false;
    if (xSemaphoreTake(node_list_mutex, portMAX_DELAY) == pdTRUE) {
        if (findNodeIndexByMac_Internal(mac) == -1) {
            for (uint8_t i = 0; i < MAX_PAIRED_NODES; i++) {
                if (!paired_nodes[i].is_active) {
                    strlcpy(paired_nodes[i].mac_address, mac, MAC_STR_LEN);
                    strlcpy(paired_nodes[i].node_name, name, 32);
                    paired_nodes[i].is_active = true;
                    paired_nodes[i].is_online = true;
                    paired_nodes[i].last_seen_millis = millis();

                    paired_nodes[i].current_config.read_interval_ms =
                        DEFAULT_READ_INTERVAL_MS;
                    paired_nodes[i].current_config.min_temp_threshold =
                        DEFAULT_MIN_TEMP_THRESHOLD;
                    paired_nodes[i].current_config.max_temp_threshold =
                        DEFAULT_MAX_TEMP_THRESHOLD;
                    paired_nodes[i].current_config.min_humidity_threshold =
                        DEFAULT_MIN_HUM_THRESHOLD;
                    paired_nodes[i].current_config.max_humidity_threshold =
                        DEFAULT_MAX_HUM_THRESHOLD;

                    paired_nodes[i].current_data = {0.0, 0.0, false, false};
                    strlcpy(paired_nodes[i].current_ml_state.prediction,
                            "Unknown", 32);
                    paired_nodes[i].current_ml_state.confidence = 0.0;

                    active_node_count++;
                    success = true;
                    break;
                }
            }
        }
        xSemaphoreGive(node_list_mutex);
    }
    return success;
}

bool removePairedNode(const char* mac) {
    bool success = false;
    if (xSemaphoreTake(node_list_mutex, portMAX_DELAY) == pdTRUE) {
        int8_t idx = findNodeIndexByMac_Internal(mac);
        if (idx != -1) {
            paired_nodes[idx].is_active = false;
            active_node_count--;
            success = true;
        }
        xSemaphoreGive(node_list_mutex);
    }
    return success;
}

void updateNodeHeartbeat(const char* mac) {
    if (xSemaphoreTake(node_list_mutex, portMAX_DELAY) == pdTRUE) {
        int8_t idx = findNodeIndexByMac_Internal(mac);
        if (idx != -1) {
            paired_nodes[idx].last_seen_millis = millis();
            paired_nodes[idx].is_online = true;
        }
        xSemaphoreGive(node_list_mutex);
    }
}

void checkNodesOnlineStatus(uint32_t timeout_ms) {
    uint32_t current_time = millis();
    if (xSemaphoreTake(node_list_mutex, portMAX_DELAY) == pdTRUE) {
        for (uint8_t i = 0; i < MAX_PAIRED_NODES; i++) {
            if (paired_nodes[i].is_active && paired_nodes[i].is_online) {
                // If the node hasn't been seen within the timeout, mark it
                // offline
                if ((current_time - paired_nodes[i].last_seen_millis) >
                    timeout_ms) {
                    paired_nodes[i].is_online = false;
                    LOG_WARN("HEARTBEAT", "Node %s is OFFLINE (Timeout)",
                             paired_nodes[i].mac_address);
                }
            }
        }
        xSemaphoreGive(node_list_mutex);
    }
}

uint8_t getActiveNodeCount() {
    uint8_t count = 0;
    if (xSemaphoreTake(node_list_mutex, portMAX_DELAY) == pdTRUE) {
        count = active_node_count;
        xSemaphoreGive(node_list_mutex);
    }
    return count;
}

uint8_t getPairedNodesSnapshot(PairedNode* out_array, uint8_t max_size) {
    uint8_t copied_count = 0;
    if (out_array == nullptr || max_size == 0) return 0;

    if (xSemaphoreTake(node_list_mutex, portMAX_DELAY) == pdTRUE) {
        for (uint8_t i = 0; i < MAX_PAIRED_NODES && copied_count < max_size;
             i++) {
            if (paired_nodes[i].is_active) {
                out_array[copied_count] = paired_nodes[i];
                copied_count++;
            }
        }
        xSemaphoreGive(node_list_mutex);
    }
    return copied_count;
}

bool getNodeByMac(const char* mac, PairedNode* out_node) {
    bool success = false;
    if (out_node == nullptr) return false;

    if (xSemaphoreTake(node_list_mutex, portMAX_DELAY) == pdTRUE) {
        int8_t idx = findNodeIndexByMac_Internal(mac);
        if (idx != -1) {
            *out_node = paired_nodes[idx];
            success = true;
        }
        xSemaphoreGive(node_list_mutex);
    }
    return success;
}

bool updateNodeConfig(const char* mac, const SensorConfig& new_config) {
    bool success = false;
    if (xSemaphoreTake(node_list_mutex, portMAX_DELAY) == pdTRUE) {
        int8_t idx = findNodeIndexByMac_Internal(mac);
        if (idx != -1) {
            paired_nodes[idx].current_config = new_config;
            success = true;
        }
        xSemaphoreGive(node_list_mutex);
    }
    return success;
}

bool updateNodeDataAndMl(const char* mac, const SensorData& new_data,
                         const MlState& new_ml) {
    bool success = false;
    if (xSemaphoreTake(node_list_mutex, portMAX_DELAY) == pdTRUE) {
        int8_t idx = findNodeIndexByMac_Internal(mac);
        if (idx != -1) {
            paired_nodes[idx].current_data = new_data;
            paired_nodes[idx].current_ml_state = new_ml;
            success = true;
        }
        xSemaphoreGive(node_list_mutex);
    }
    return success;
}

// SENSOR NODE APIs
SensorConfig getSensorConfig() {
    SensorConfig copy;
    if (xSemaphoreTake(sensor_config_mutex, portMAX_DELAY) == pdTRUE) {
        copy = sensor_config;
        xSemaphoreGive(sensor_config_mutex);
    }
    return copy;
}

void setSensorConfig(const SensorConfig& config) {
    if (xSemaphoreTake(sensor_config_mutex, portMAX_DELAY) == pdTRUE) {
        sensor_config = config;
        xSemaphoreGive(sensor_config_mutex);
    }
}

SensorState getSensorState() {
    SensorState copy;
    if (xSemaphoreTake(sensor_state_mutex, portMAX_DELAY) == pdTRUE) {
        copy = sensor_state;
        xSemaphoreGive(sensor_state_mutex);
    }
    return copy;
}

void setSensorErrorFlag(uint16_t flag) {
    if (xSemaphoreTake(sensor_state_mutex, portMAX_DELAY) == pdTRUE) {
        if ((sensor_state.active_error_flags & flag) != flag) {
            sensor_state.active_error_flags |= flag;
            if (sensor_led_sync_semaphore != NULL)
                xSemaphoreGive(sensor_led_sync_semaphore);
            if (lcd_sync_semaphore != NULL) xSemaphoreGive(lcd_sync_semaphore);
        }
        xSemaphoreGive(sensor_state_mutex);
    }
}

void clearSensorErrorFlag(uint16_t flag) {
    if (xSemaphoreTake(sensor_state_mutex, portMAX_DELAY) == pdTRUE) {
        if ((sensor_state.active_error_flags & flag) != 0) {
            sensor_state.active_error_flags &= ~flag;
            if (sensor_led_sync_semaphore != NULL)
                xSemaphoreGive(sensor_led_sync_semaphore);
            if (lcd_sync_semaphore != NULL) xSemaphoreGive(lcd_sync_semaphore);
        }
        xSemaphoreGive(sensor_state_mutex);
    }
}

bool checkSensorErrorFlag(uint16_t flag) {
    bool is_set = false;
    if (xSemaphoreTake(sensor_state_mutex, portMAX_DELAY) == pdTRUE) {
        is_set = (sensor_state.active_error_flags & flag) != 0;
        xSemaphoreGive(sensor_state_mutex);
    }
    return is_set;
}

uint32_t getSensorActiveErrorFlags() {
    uint32_t flags = 0;
    if (xSemaphoreTake(sensor_state_mutex, portMAX_DELAY) == pdTRUE) {
        flags = sensor_state.active_error_flags;
        xSemaphoreGive(sensor_state_mutex);
    }
    return flags;
}

// GATEWAY NODE APIs
GatewayConfig getGatewayConfig() {
    GatewayConfig copy;
    if (xSemaphoreTake(gw_config_mutex, portMAX_DELAY) == pdTRUE) {
        copy = gateway_config;
        xSemaphoreGive(gw_config_mutex);
    }
    return copy;
}

void setGatewayConfig(const GatewayConfig& config) {
    if (xSemaphoreTake(gw_config_mutex, portMAX_DELAY) == pdTRUE) {
        gateway_config = config;
        xSemaphoreGive(gw_config_mutex);
    }
}

GatewayState getGatewayState() {
    GatewayState copy;
    if (xSemaphoreTake(gw_state_mutex, portMAX_DELAY) == pdTRUE) {
        copy = gateway_state;
        xSemaphoreGive(gw_state_mutex);
    }
    return copy;
}

void setGatewayState(const GatewayState& state) {
    if (xSemaphoreTake(gw_state_mutex, portMAX_DELAY) == pdTRUE) {
        gateway_state = state;
        xSemaphoreGive(gw_state_mutex);
    }
}

void setGatewayErrorFlag(uint16_t flag) {
    if (xSemaphoreTake(gw_state_mutex, portMAX_DELAY) == pdTRUE) {
        if ((gateway_state.active_error_flags & flag) != flag) {
            gateway_state.active_error_flags |= flag;
            if (gw_led_sync_semaphore != NULL)
                xSemaphoreGive(gw_led_sync_semaphore);
        }
        xSemaphoreGive(gw_state_mutex);
    }
}

void clearGatewayErrorFlag(uint16_t flag) {
    if (xSemaphoreTake(gw_state_mutex, portMAX_DELAY) == pdTRUE) {
        if ((gateway_state.active_error_flags & flag) != 0) {
            gateway_state.active_error_flags &= ~flag;
            if (gw_led_sync_semaphore != NULL)
                xSemaphoreGive(gw_led_sync_semaphore);
        }
        xSemaphoreGive(gw_state_mutex);
    }
}

bool checkGatewayErrorFlag(uint16_t flag) {
    bool is_set = false;
    if (xSemaphoreTake(gw_state_mutex, portMAX_DELAY) == pdTRUE) {
        is_set = (gateway_state.active_error_flags & flag) != 0;
        xSemaphoreGive(gw_state_mutex);
    }
    return is_set;
}

uint32_t getGatewayActiveErrorFlags() {
    uint32_t flags = 0;
    if (xSemaphoreTake(gw_state_mutex, portMAX_DELAY) == pdTRUE) {
        flags = gateway_state.active_error_flags;
        xSemaphoreGive(gw_state_mutex);
    }
    return flags;
}

ControlState getControlState() {
    ControlState copy;
    if (xSemaphoreTake(control_state_mutex, portMAX_DELAY) == pdTRUE) {
        copy = control_state;
        xSemaphoreGive(control_state_mutex);
    }
    return copy;
}

void setControlState(const ControlState& state) {
    if (xSemaphoreTake(control_state_mutex, portMAX_DELAY) == pdTRUE) {
        control_state = state;
        xSemaphoreGive(control_state_mutex);
    }
}

// ROM STORAGE FUNCTIONS
void loadConfigFromFlash() {
    bool isNamespaceExist = preferences.begin(PREFERENCES_NAMESPACE, true);

#ifdef GATEWAY_NODE
    // Load Gateway Config
    if (xSemaphoreTake(gw_config_mutex, portMAX_DELAY) == pdTRUE) {
        size_t len_ap_ssid = preferences.getString(
            AP_SSID_KEY, gateway_config.ap_ssid, MAX_SSID_LEN);
        if (len_ap_ssid == 0)
            strlcpy(gateway_config.ap_ssid, DEFAULT_AP_SSID_VALUE,
                    MAX_SSID_LEN);

        size_t len_ap_pass = preferences.getString(
            AP_PASS_KEY, gateway_config.ap_password, MAX_PASS_LEN);
        if (len_ap_pass == 0)
            strlcpy(gateway_config.ap_password, DEFAULT_AP_PASS_VALUE,
                    MAX_PASS_LEN);

        size_t len_wifi_ssid = preferences.getString(
            WIFI_SSID_KEY, gateway_config.wifi_ssid, MAX_SSID_LEN);
        if (len_wifi_ssid == 0) gateway_config.wifi_ssid[0] = '\0';

        size_t len_wifi_pass = preferences.getString(
            WIFI_PASS_KEY, gateway_config.wifi_password, MAX_PASS_LEN);
        if (len_wifi_pass == 0) gateway_config.wifi_password[0] = '\0';

        size_t len_token = preferences.getString(
            CORE_IOT_TOKEN_KEY, gateway_config.core_iot_token, MAX_TOKEN_LEN);
        if (len_token == 0) gateway_config.core_iot_token[0] = '\0';

        size_t len_server = preferences.getString(
            CORE_IOT_SERVER_KEY, gateway_config.core_iot_server,
            MAX_SERVER_LEN);
        if (len_server == 0)
            strlcpy(gateway_config.core_iot_server, DEFAULT_CORE_IOT_SERVER,
                    MAX_SERVER_LEN);

        gateway_config.core_iot_port =
            preferences.getShort(CORE_IOT_PORT_KEY, DEFAULT_CORE_IOT_PORT);
        gateway_config.send_interval_ms =
            preferences.getShort(SEND_INTERVAL_KEY, DEFAULT_SEND_INTERVAL_MS);

        xSemaphoreGive(gw_config_mutex);
    }
#endif

#ifdef SENSOR_NODE
    // Load Sensor Config
    if (xSemaphoreTake(sensor_config_mutex, portMAX_DELAY) == pdTRUE) {
        sensor_config.read_interval_ms =
            preferences.getShort(READ_INTERVAL_KEY, DEFAULT_READ_INTERVAL_MS);
        sensor_config.max_temp_threshold =
            preferences.getFloat(MAX_TEMP_KEY, DEFAULT_MAX_TEMP_THRESHOLD);
        sensor_config.min_temp_threshold =
            preferences.getFloat(MIN_TEMP_KEY, DEFAULT_MIN_TEMP_THRESHOLD);
        sensor_config.max_humidity_threshold =
            preferences.getFloat(MAX_HUM_KEY, DEFAULT_MAX_HUM_THRESHOLD);
        sensor_config.min_humidity_threshold =
            preferences.getFloat(MIN_HUM_KEY, DEFAULT_MIN_HUM_THRESHOLD);

        xSemaphoreGive(sensor_config_mutex);
    }
#endif
    preferences.end();

    if (!isNamespaceExist) {
        LOG_INFO("GLOBAL",
                 "First boot detected. Creating Preferences namespace...");
        saveConfigToFlash();
    }
}

void saveConfigToFlash() {
    preferences.begin(PREFERENCES_NAMESPACE, false);
#ifdef GATEWAY_NODE
    if (xSemaphoreTake(gw_config_mutex, portMAX_DELAY) == pdTRUE) {
        preferences.putString(AP_SSID_KEY, gateway_config.ap_ssid);
        preferences.putString(AP_PASS_KEY, gateway_config.ap_password);
        preferences.putString(WIFI_SSID_KEY, gateway_config.wifi_ssid);
        preferences.putString(WIFI_PASS_KEY, gateway_config.wifi_password);
        preferences.putString(CORE_IOT_TOKEN_KEY,
                              gateway_config.core_iot_token);
        preferences.putString(CORE_IOT_SERVER_KEY,
                              gateway_config.core_iot_server);
        preferences.putShort(CORE_IOT_PORT_KEY, gateway_config.core_iot_port);
        preferences.putShort(SEND_INTERVAL_KEY,
                             gateway_config.send_interval_ms);
        xSemaphoreGive(gw_config_mutex);
    }

#endif

#ifdef SENSOR_NODE
    if (xSemaphoreTake(sensor_config_mutex, portMAX_DELAY) == pdTRUE) {
        preferences.putShort(READ_INTERVAL_KEY, sensor_config.read_interval_ms);
        preferences.putFloat(MAX_TEMP_KEY, sensor_config.max_temp_threshold);
        preferences.putFloat(MIN_TEMP_KEY, sensor_config.min_temp_threshold);
        preferences.putFloat(MAX_HUM_KEY, sensor_config.max_humidity_threshold);
        preferences.putFloat(MIN_HUM_KEY, sensor_config.min_humidity_threshold);
        xSemaphoreGive(sensor_config_mutex);
    }
#endif
    preferences.end();
}

#ifdef PIO_UNIT_TESTING
void testHook_setNodeLastSeenMillis(const char* mac_str,
                                    uint32_t simulated_millis) {
    for (uint8_t i = 0; i < MAX_PAIRED_NODES; i++) {
        if (paired_nodes[i].is_active &&
            strcasecmp(paired_nodes[i].mac_address, mac_str) == 0) {
            paired_nodes[i].last_seen_millis = simulated_millis;
            break;
        }
    }
}
#endif