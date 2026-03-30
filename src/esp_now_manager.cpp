#include "esp_now_manager.h"

// PROTOCOL DATA STRUCTURES
// Use packed structures to prevent memory padding issues over ESP-NOW
#pragma pack(push, 1)

struct MsgHeader {
    uint8_t msg_type;
    uint8_t seq_num;
};

struct MsgTelemetry {
    MsgHeader header;
    float temperature;
    float humidity;
    uint8_t ml_prediction;
    float ml_confidence;
    uint32_t error_flags;
};

struct MsgSyncConfig {
    MsgHeader header;
    float min_temp;
    float max_temp;
    float min_hum;
    float max_hum;
    uint32_t reading_interval;
};

struct MsgPairing {
    MsgHeader header;
    uint8_t pair_state;  // 0x01: Pair, 0x02: Unpair
    char room_name[32];
};

struct MsgControlCmd {
    MsgHeader header;
    uint8_t cmd_code;
    uint32_t cmd_param;
};

struct MsgAckResponse {
    MsgHeader header;
    uint8_t msg_type_replied;
    uint8_t status_code;  // 0: Success, 1: Fail
};

#pragma pack(pop)

// INTERNAL TYPES & GLOBALS
struct RxPacket {
    uint8_t mac_addr[6];
    uint8_t data[250];
    int len;
};

static QueueHandle_t rx_queue = NULL;
static QueueHandle_t tx_status_queue = NULL;

// Global counter for Sequence Number
static uint8_t global_seq_num = 0;

// Gateway MAC and Pairing Status (Used by Sensor Node)
static uint8_t gateway_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static bool is_sensor_paired = false;

// Track last received seq_num to prevent duplicates
static uint8_t last_rx_seq_num[MAX_PAIRED_NODES] = {0};

// Helper macros
#define GET_SEQ_NUM() (++global_seq_num)

// HELPER FUNCTIONS
static void macStrToBytes(const char* mac_str, uint8_t* mac_bytes) {
    sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac_bytes[0],
           &mac_bytes[1], &mac_bytes[2], &mac_bytes[3], &mac_bytes[4],
           &mac_bytes[5]);
}

static void bytesToMacStr(const uint8_t* mac_bytes, char* mac_str) {
    snprintf(mac_str, MAC_STR_LEN, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac_bytes[0], mac_bytes[1], mac_bytes[2], mac_bytes[3],
             mac_bytes[4], mac_bytes[5]);
}

// Map string prediction to uint8_t for Telemetry Payload
static uint8_t mapPredictionToId(const char* pred) {
    if (strstr(pred, "Door") != NULL) return 1;
    if (strstr(pred, "Fault") != NULL) return 2;
    return 0;  // Normal
}

// ESP-NOW CALLBACKS
// Callbacks MUST NOT block. Only push to Queue.
static void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
    bool tx_success = (status == ESP_NOW_SEND_SUCCESS);
    xQueueSend(tx_status_queue, &tx_success, 0);
}

static void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData,
                       int len) {
    RxPacket packet;
    memcpy(packet.mac_addr, mac, 6);
    memcpy(packet.data, incomingData, len);
    packet.len = len;
    xQueueSend(rx_queue, &packet, 0);
}

// INITIALIZATION
void initEspNow() {
    if (esp_now_init() != ESP_OK) {
        LOG_ERR("ESPNOW", "Error initializing ESP-NOW");
        setGatewayErrorFlag(GW_FLAG_ESPNOW_ERR);
        return;
    }

    // Register callbacks
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    // Create Queues
    rx_queue = xQueueCreate(20, sizeof(RxPacket));
    tx_status_queue = xQueueCreate(5, sizeof(bool));

    LOG_INFO("ESPNOW", "ESP-NOW Initialized successfully");
}

// SENSOR NODE LOGIC TASK
void espNowSensorTask(void* pvParameters) {
    LOG_INFO("ESPNOW_SN", "Sensor ESP-NOW Task Started");

    Preferences prefs;
    prefs.begin(PREFERENCES_NAMESPACE, false);

    // Startup: Check if already paired
    size_t mac_len = prefs.getBytes("gw_mac", gateway_mac, 6);
    is_sensor_paired = (mac_len == 6);

    if (!is_sensor_paired) {
        setSensorErrorFlag(SENSOR_FLAG_UNPAIRED);
    } else {
        clearSensorErrorFlag(SENSOR_FLAG_UNPAIRED);
    }

    uint8_t current_channel = prefs.getUChar("esp_channel", 1);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);

    if (is_sensor_paired) {
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, gateway_mac, 6);
        peerInfo.channel = current_channel;
        peerInfo.encrypt = false;
        esp_now_add_peer(&peerInfo);

        char mac_str[MAC_STR_LEN];
        bytesToMacStr(gateway_mac, mac_str);
        LOG_INFO("ESPNOW_SN", "Loaded Gateway MAC: %s", mac_str);
    } else {
        LOG_INFO("ESPNOW_SN",
                 "Not paired. Waiting for pairing via Gateway WebServer...");
    }

    while (1) {
        // Check for incoming commands from Gateway
        RxPacket rx_pkt;
        if (xQueueReceive(rx_queue, &rx_pkt, pdMS_TO_TICKS(10)) == pdTRUE) {
            MsgHeader* header = (MsgHeader*)rx_pkt.data;

            // Handle Pairing Message (Can be received even if not paired yet)
            if (header->msg_type == MSG_PAIRING) {
                MsgPairing* pair_msg = (MsgPairing*)rx_pkt.data;

                if (pair_msg->pair_state == 0x01) {  // Pair Request
                    memcpy(gateway_mac, rx_pkt.mac_addr, 6);
                    prefs.putBytes("gw_mac", gateway_mac, 6);

                    if (esp_now_is_peer_exist(gateway_mac)) {
                        esp_now_del_peer(gateway_mac);
                    }

                    esp_now_peer_info_t peerInfo = {};
                    memcpy(peerInfo.peer_addr, gateway_mac, 6);
                    peerInfo.channel = current_channel;
                    peerInfo.encrypt = false;
                    esp_now_add_peer(&peerInfo);

                    is_sensor_paired = true;
                    clearSensorErrorFlag(SENSOR_FLAG_UNPAIRED);

                    LOG_INFO("ESPNOW_SN", "Paired successfully! Room Name: %s",
                             pair_msg->room_name);
                } else if (pair_msg->pair_state == 0x02) {  // Unpair Request
                    if (esp_now_is_peer_exist(gateway_mac)) {
                        esp_now_del_peer(gateway_mac);
                    }
                    memset(gateway_mac, 0xFF, 6);
                    prefs.remove("gw_mac");
                    is_sensor_paired = false;
                    setSensorErrorFlag(SENSOR_FLAG_UNPAIRED);

                    LOG_INFO("ESPNOW_SN", "Unpaired from Gateway.");
                }

                // Send ACK directly to the sender's MAC
                MsgAckResponse ack = {
                    {MSG_ACK_RESPONSE, GET_SEQ_NUM()}, MSG_PAIRING, 0};
                esp_now_send(rx_pkt.mac_addr, (uint8_t*)&ack, sizeof(ack));
            }
            // Handle Sync Config
            else if (header->msg_type == MSG_SYNC_CONFIG) {
                MsgSyncConfig* cfg = (MsgSyncConfig*)rx_pkt.data;
                SensorConfig new_cfg = {(int16_t)cfg->reading_interval,
                                        cfg->max_temp, cfg->min_temp,
                                        cfg->max_hum, cfg->min_hum};
                setSensorConfig(new_cfg);
                saveConfigToFlash();
                LOG_INFO("ESPNOW_SN", "Received new config from Gateway");

                MsgAckResponse ack = {
                    {MSG_ACK_RESPONSE, GET_SEQ_NUM()}, MSG_SYNC_CONFIG, 0};
                esp_now_send(gateway_mac, (uint8_t*)&ack, sizeof(ack));
            }
            // Handle Control Commands
            else if (header->msg_type == MSG_CONTROL_CMD) {
                MsgControlCmd* cmd = (MsgControlCmd*)rx_pkt.data;
                if (cmd->cmd_code == CMD_HARD_RESET) {
                    LOG_WARN("ESPNOW_SN", "Hardware Reset Command Received!");
                    ESP.restart();
                } else if (cmd->cmd_code == CMD_CLEAR_ALARM) {
                    clearSensorErrorFlag(0xFFFF);  // Clear all
                } else if (cmd->cmd_code == CMD_FORCE_READ) {
                    xSemaphoreGive(sensor_send_telemetry_semaphore);
                }
                MsgAckResponse ack = {
                    {MSG_ACK_RESPONSE, GET_SEQ_NUM()}, MSG_CONTROL_CMD, 0};
                esp_now_send(gateway_mac, (uint8_t*)&ack, sizeof(ack));
            }
        }

        // Check if Sensor Task wants to send Telemetry
        if (xSemaphoreTake(sensor_send_telemetry_semaphore,
                           pdMS_TO_TICKS(10)) == pdTRUE) {
            if (!is_sensor_paired) {
                LOG_WARN("ESPNOW_SN", "Not paired. Cannot send telemetry.");
                continue;  // Skip sending if not paired
            }

            SensorData data = getSensorData();
            MlState ml = getMlState();

            MsgTelemetry tx_pkt;
            tx_pkt.header.msg_type = MSG_TELEMETRY;
            tx_pkt.header.seq_num = GET_SEQ_NUM();
            tx_pkt.temperature = data.current_temperature;
            tx_pkt.humidity = data.current_humidity;
            tx_pkt.ml_prediction = mapPredictionToId(ml.prediction);
            tx_pkt.ml_confidence = ml.confidence;
            tx_pkt.error_flags = getSensorActiveErrorFlags();

            // Clear TX queue before sending
            xQueueReset(tx_status_queue);
            esp_err_t result =
                esp_now_send(gateway_mac, (uint8_t*)&tx_pkt, sizeof(tx_pkt));

            bool tx_success = false;
            if (result == ESP_OK) {
                xQueueReceive(tx_status_queue, &tx_success, pdMS_TO_TICKS(500));
            }

            // CHANNEL HOPPING LOGIC IF FAIL
            if (!tx_success) {
                LOG_WARN("ESPNOW_SN",
                         "Delivery Fail. Start Channel Hopping...");
                setSensorErrorFlag(SENSOR_FLAG_ESPNOW_DISCONN);
                bool delivered = false;

                for (int ch = 1; ch <= 13; ch++) {
                    esp_now_peer_info_t peerInfo = {};
                    memcpy(peerInfo.peer_addr, gateway_mac, 6);
                    peerInfo.channel = ch;
                    peerInfo.encrypt = false;
                    esp_now_mod_peer(&peerInfo);

                    esp_wifi_set_promiscuous(true);
                    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
                    esp_wifi_set_promiscuous(false);

                    xQueueReset(tx_status_queue);
                    esp_now_send(gateway_mac, (uint8_t*)&tx_pkt,
                                 sizeof(tx_pkt));

                    if (xQueueReceive(tx_status_queue, &tx_success,
                                      pdMS_TO_TICKS(200)) == pdTRUE) {
                        if (tx_success) {
                            LOG_INFO("ESPNOW_SN", "Found Gateway at Channel %d",
                                     ch);
                            prefs.putUChar("esp_channel", ch);
                            clearSensorErrorFlag(SENSOR_FLAG_ESPNOW_DISCONN);
                            delivered = true;
                            break;
                        }
                    }
                }
                if (!delivered)
                    LOG_ERR("ESPNOW_SN",
                            "Failed to find Gateway on all channels");
            } else {
                clearSensorErrorFlag(SENSOR_FLAG_ESPNOW_DISCONN);
                // LOG_INFO("ESPNOW_SN", "Telemetry sent successfully.");
            }
        }
    }
}

// GATEWAY NODE LOGIC TASK
void espNowGatewayTask(void* pvParameters) {
    LOG_INFO("ESPNOW_GW", "Gateway ESP-NOW Task Started");

    uint32_t last_heartbeat_check = millis();

    while (1) {
        // Process Downlink Queue (Commands from WebServer)
        GwDownlinkMessage downlink_cmd;
        if (xQueueReceive(gw_downlink_queue, &downlink_cmd,
                          pdMS_TO_TICKS(10)) == pdTRUE) {
            uint8_t target_mac[6];
            macStrToBytes(downlink_cmd.target_mac, target_mac);

            // Add peer temporarily if not exists
            if (!esp_now_is_peer_exist(target_mac)) {
                esp_now_peer_info_t peerInfo = {};
                memcpy(peerInfo.peer_addr, target_mac, 6);
                peerInfo.channel = 0;  // Use current channel
                peerInfo.encrypt = false;
                esp_now_add_peer(&peerInfo);
            }

            if (downlink_cmd.type == DOWNLINK_PAIRING) {
                MsgPairing pkt = {{MSG_PAIRING, GET_SEQ_NUM()}, 0x01, ""};
                strlcpy(pkt.room_name, downlink_cmd.room_name, 32);
                esp_now_send(target_mac, (uint8_t*)&pkt, sizeof(pkt));
                LOG_INFO("ESPNOW_GW", "Sent PAIRING Request to %s",
                         downlink_cmd.target_mac);
            } else if (downlink_cmd.type == DOWNLINK_UNPAIR) {
                MsgPairing pkt = {{MSG_PAIRING, GET_SEQ_NUM()}, 0x02, ""};
                esp_now_send(target_mac, (uint8_t*)&pkt, sizeof(pkt));
                LOG_INFO("ESPNOW_GW", "Sent UNPAIR Request to %s",
                         downlink_cmd.target_mac);
            } else if (downlink_cmd.type == DOWNLINK_SYNC_CONFIG) {
                PairedNode node;
                if (getNodeByMac(downlink_cmd.target_mac, &node)) {
                    MsgSyncConfig pkt = {
                        {MSG_SYNC_CONFIG, GET_SEQ_NUM()},
                        node.current_config.min_temp_threshold,
                        node.current_config.max_temp_threshold,
                        node.current_config.min_humidity_threshold,
                        node.current_config.max_humidity_threshold,
                        (uint32_t)node.current_config.read_interval_ms};
                    esp_now_send(target_mac, (uint8_t*)&pkt, sizeof(pkt));
                    LOG_INFO("ESPNOW_GW", "Sent SYNC_CONFIG to %s",
                             downlink_cmd.target_mac);
                }
            } else if (downlink_cmd.type == DOWNLINK_CONTROL_CMD) {
                MsgControlCmd pkt = {{MSG_CONTROL_CMD, GET_SEQ_NUM()},
                                     downlink_cmd.cmd_code,
                                     downlink_cmd.cmd_param};
                esp_now_send(target_mac, (uint8_t*)&pkt, sizeof(pkt));
                LOG_INFO("ESPNOW_GW", "Sent CONTROL_CMD to %s",
                         downlink_cmd.target_mac);
            }
        }

        // Process Uplink (Telemetry/ACKs from Sensors)
        RxPacket rx_pkt;
        if (xQueueReceive(rx_queue, &rx_pkt, pdMS_TO_TICKS(10)) == pdTRUE) {
            char mac_str[MAC_STR_LEN];
            bytesToMacStr(rx_pkt.mac_addr, mac_str);

            MsgHeader* header = (MsgHeader*)rx_pkt.data;

            // Heartbeat check & duplicate filter
            updateNodeHeartbeat(mac_str);

            if (header->msg_type == MSG_TELEMETRY) {
                // Strict Pairing Check: Only process telemetry if Node is in
                // the local Paired List
                PairedNode temp;
                if (getNodeByMac(mac_str, &temp)) {
                    MsgTelemetry* tel = (MsgTelemetry*)rx_pkt.data;

                    SensorData newData = {
                        tel->temperature, tel->humidity,
                        !(tel->error_flags & SENSOR_FLAG_DHT_ERR),
                        !(tel->error_flags & SENSOR_FLAG_LCD_ERR)};

                    MlState newMl;
                    newMl.confidence = tel->ml_confidence;
                    if (tel->ml_prediction == 0)
                        strlcpy(newMl.prediction, "Normal", 32);
                    else if (tel->ml_prediction == 1)
                        strlcpy(newMl.prediction, "Door Open", 32);
                    else
                        strlcpy(newMl.prediction, "Fault", 32);

                    updateNodeDataAndMl(mac_str, newData, newMl);
                } else {
                    LOG_WARN("ESPNOW_GW",
                             "Ignored telemetry from UNPAIRED node: %s",
                             mac_str);
                }
            } else if (header->msg_type == MSG_ACK_RESPONSE) {
                MsgAckResponse* ack = (MsgAckResponse*)rx_pkt.data;
                LOG_INFO("ESPNOW_GW",
                         "Received ACK from %s for MsgType: 0x%02X", mac_str,
                         ack->msg_type_replied);
            }
        }

        // Heartbeat Monitor (Run every 10s)
        if (millis() - last_heartbeat_check > 10000) {
            last_heartbeat_check = millis();
            // Timeout = 30s (Offline threshold)
            extern void checkNodesOnlineStatus(uint32_t timeout_ms);
            checkNodesOnlineStatus(30000);
        }
    }
}