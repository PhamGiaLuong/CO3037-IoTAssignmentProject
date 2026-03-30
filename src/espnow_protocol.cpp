#include "espnow_protocol.h"

#include <WiFi.h>
#include <esp_now.h>

// MAC address for gateway
uint8_t mac_address[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

QueueHandle_t espnow_rx_queue;

// SUB-GATEWAY RECEIVER

void rxDataCallback(const uint8_t *mac, const uint8_t *incoming_data, int len) {
    if (len == sizeof(SensorPayload) &&
        incoming_data[0] == MSG_TYPE_SENSOR_DATA) {
        SensorPayload rx_data;
        memcpy(&rx_data, incoming_data, sizeof(rx_data));
        xQueueSendFromISR(espnow_rx_queue, &rx_data, NULL);
    }
}

// Handle data received from sensor nodes, forward to web server and core IoT
void espNowRxTask(void *pvParameters) {
    LOG_INFO("ESPNOW", "ESP-NOW RX task started");
    SensorPayload rx_data;
    while (1) {
        if (xQueueReceive(espnow_rx_queue, &rx_data, portMAX_DELAY) == pdTRUE) {
            // ex mac
            char mac_str[] = "AA:BB:CC:DD:EE:FF";
            LOG_INFO("ESPNOW_RX",
                     "Received data from sensor node: Temp=%.1fC, Hum=%.1f%%, "
                     "Errors=0x%02X",
                     rx_data.data.current_temperature,
                     rx_data.data.current_humidity, rx_data.error_flags);

            addPairedNode(mac_str, "SensorNode1");
            updateNodeDataAndMl(mac_str, rx_data.data, rx_data.ml);

            if (rx_data.error_flags > 0) {
                LOG_WARN("ESPNOW_RX",
                         "Sensor node reported error flags: 0x%02X",
                         rx_data.error_flags);
                xSemaphoreGive(gw_led_sync_semaphore);
            }
        }
    }
}

// SENSOR NODE TRANSMITTER

void txDataCallback(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        LOG_INFO("ESPNOW_TX", "Data sent successfully to gateway");
    } else {
        LOG_ERR("ESPNOW_TX", "Failed to send data to gateway");
    }
}

void espNowTxTask(void *pvParameters) {
    LOG_INFO("ESPNOW_TX", "ESP-NOW TX task started");

    SensorConfig config = getSensorConfig();
    TickType_t send_interval = pdMS_TO_TICKS(config.read_interval_ms);

    while (1) {
        // 1. Đóng gói dữ liệu
        SensorPayload tx_payload;
        tx_payload.msg_type = MSG_TYPE_SENSOR_DATA;
        tx_payload.data = getSensorData();  // Lấy từ SensorTask
        tx_payload.ml = getMlState();       // Lấy từ AI Task
        tx_payload.error_flags = getSensorActiveErrorFlags();  //

        // 2. Bắn qua ESP-NOW
        esp_err_t result = esp_now_send(mac_address, (uint8_t *)&tx_payload,
                                        sizeof(tx_payload));

        if (result == ESP_OK) {
            LOG_INFO("ESPNOW_TX", "Sent payload successfully: Temp=%.1f",
                     tx_payload.data.current_temperature);
        } else {
            LOG_ERR("ESPNOW_TX", "Failed to send ESP-NOW");
        }

        vTaskDelay(send_interval);
    }
}

// Initialization function for ESP-NOW
void initEspNowSystem() {
    // Bắt buộc chuyển WiFi sang Station Mode
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        LOG_ERR("ESPNOW", "Failed to initialize ESP-NOW");
        return;
    }

    // Khởi tạo hàng đợi chứa tối đa 10 gói tin
    espnow_rx_queue = xQueueCreate(10, sizeof(SensorPayload));

    // Đăng ký các Callback
    esp_now_register_recv_cb(rxDataCallback);
    esp_now_register_send_cb(txDataCallback);

    // Đăng ký điểm đến (Peer)
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac_address, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        LOG_ERR("ESPNOW", "Failed to add peer");
        return;
    }

    LOG_INFO("ESPNOW", "ESP-NOW is ready");
}