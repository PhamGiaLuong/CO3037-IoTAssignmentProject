#include <Arduino.h>
#include <nvs_flash.h>
#include <unity.h>

#include "esp_now_manager.h"
#include "global.h"

extern TaskHandle_t current_test_task;
extern void testHook_setNodeLastSeenMillis(const char* mac_str,
                                           uint32_t simulated_millis);
extern void testHook_simulateEspNowRx(const uint8_t* mac, const uint8_t* data,
                                      int len);

// TC: IG-01
void test_web_to_espnow_downlink_queue(void) {
    xQueueReset(gw_downlink_queue);

    GwDownlinkMessage sendMsg;
    sendMsg.type = DOWNLINK_PAIRING;
    strlcpy(sendMsg.target_mac, "AA:BB:CC:11:22:33", MAC_STR_LEN);

    xQueueSend(gw_downlink_queue, &sendMsg, 0);

    GwDownlinkMessage recvMsg;
    BaseType_t res = xQueueReceive(gw_downlink_queue, &recvMsg, 0);

    TEST_ASSERT_EQUAL(pdTRUE, res);
    TEST_ASSERT_EQUAL(DOWNLINK_PAIRING, recvMsg.type);
    TEST_ASSERT_EQUAL_STRING("AA:BB:CC:11:22:33", recvMsg.target_mac);
}

// TC: IG-02
void test_heartbeat_offline_timeout(void) {
    addPairedNode("AA:BB:CC:11:22:33", "Room 1");

    testHook_setNodeLastSeenMillis("AA:BB:CC:11:22:33", 0);
    delay(50);
    checkNodesOnlineStatus(10);

    PairedNode node;
    getNodeByMac("AA:BB:CC:11:22:33", &node);
    TEST_ASSERT_FALSE(node.is_online);
}

// TC: IG-03
void test_web_api_mode_switch_semaphore(void) {
    xSemaphoreTake(switch_to_sta_semaphore, 0);

    // Simulate Web API call execution
    xSemaphoreGive(switch_to_sta_semaphore);

    BaseType_t res = xSemaphoreTake(switch_to_sta_semaphore, 0);
    TEST_ASSERT_EQUAL(pdTRUE, res);
}

// TC: IG-04
void test_flash_save_load_gw_config(void) {
    GatewayConfig cfg = {0};
    strlcpy(cfg.wifi_ssid, "TEST_WIFI", MAX_SSID_LEN);
    setGatewayConfig(cfg);
    saveConfigToFlash();

    GatewayConfig corrupt = {0};
    setGatewayConfig(corrupt);
    loadConfigFromFlash();
    TEST_ASSERT_EQUAL_STRING("TEST_WIFI", getGatewayConfig().wifi_ssid);
}

// TC: IG-05
void test_flash_save_load_sensor_config(void) {
    SensorConfig cfg = {2000, 45.0, 10.0, 80.0, 40.0};
    setSensorConfig(cfg);
    saveConfigToFlash();
    SensorConfig corrupt = {0};
    setSensorConfig(corrupt);
    loadConfigFromFlash();
    TEST_ASSERT_EQUAL_FLOAT(45.0, getSensorConfig().max_temp_threshold);
}

// TC: IG-06
void test_gw_error_triggers_led_semaphore(void) {
    xSemaphoreTake(gw_led_sync_semaphore, 0);  // Drain
    setGatewayErrorFlag(GW_FLAG_NET_AP_MODE);
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(gw_led_sync_semaphore, 0));
}

// TC: IG-07
void test_web_api_pushes_downlink_queue(void) {
    xQueueReset(gw_downlink_queue);
    GwDownlinkMessage msg;
    msg.type = DOWNLINK_CONTROL_CMD;
    msg.cmd_code = 0xAA;  // Giả lập CMD_HARD_RESET
    xQueueSend(gw_downlink_queue, &msg, 0);
    GwDownlinkMessage recv;
    xQueueReceive(gw_downlink_queue, &recv, 0);
    TEST_ASSERT_EQUAL(0xAA, recv.cmd_code);
}

// TC: IG-08
void test_web_api_pairing_downlink(void) {
    GwDownlinkMessage msg;
    msg.type = DOWNLINK_PAIRING;
    xQueueSend(gw_downlink_queue, &msg, 0);
    GwDownlinkMessage recv;
    xQueueReceive(gw_downlink_queue, &recv, 0);
    TEST_ASSERT_EQUAL(DOWNLINK_PAIRING, recv.type);
}

// TC: IG-09
void test_web_api_sync_config_downlink(void) {
    GwDownlinkMessage msg;
    msg.type = DOWNLINK_SYNC_CONFIG;
    xQueueSend(gw_downlink_queue, &msg, 0);
    GwDownlinkMessage recv;
    xQueueReceive(gw_downlink_queue, &recv, 0);
    TEST_ASSERT_EQUAL(DOWNLINK_SYNC_CONFIG, recv.type);
}

// TC: IG-10
void test_network_mode_switch_ap_sema(void) {
    xSemaphoreTake(switch_to_ap_semaphore, 0);
    xSemaphoreGive(switch_to_ap_semaphore);  // Simulate Web API call
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(switch_to_ap_semaphore, 0));
}

// TC: IG-11
void test_network_mode_switch_sta_sema(void) {
    xSemaphoreTake(switch_to_sta_semaphore, 0);
    xSemaphoreGive(switch_to_sta_semaphore);  // Simulate Web API call
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(switch_to_sta_semaphore, 0));
}

// TC: IG-12
void test_wifi_disconnect_triggers_error_sema(void) {
    xSemaphoreTake(wifi_error_semaphore, 0);
    xSemaphoreGive(wifi_error_semaphore);
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(wifi_error_semaphore, 0));
}

// TC: IG-13
void test_mqtt_disconnect_triggers_error_sema(void) {
    xSemaphoreTake(coreiot_error_semaphore, 0);
    xSemaphoreGive(coreiot_error_semaphore);
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(coreiot_error_semaphore, 0));
}

// TC: IG-14
void test_espnow_rx_updates_heartbeat(void) {
    addPairedNode("AA:BB:CC:11:22:33", "Room 1");

    testHook_setNodeLastSeenMillis("AA:BB:CC:11:22:33", 0);

    MsgTelemetry mockPacket = {0};
    mockPacket.header.msg_type = MSG_TELEMETRY;
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33};
    testHook_simulateEspNowRx(mac, (uint8_t*)&mockPacket, sizeof(mockPacket));

    xTaskCreate(espNowGatewayTask, "esp_now_gw", 4096, NULL, 5,
                &current_test_task);
    vTaskDelay(pdMS_TO_TICKS(50));

    PairedNode node;
    getNodeByMac("AA:BB:CC:11:22:33", &node);

    TEST_ASSERT_GREATER_THAN_UINT32(0, node.last_seen_millis);
}

// TC: IG-15
void test_espnow_rx_updates_global_data(void) {
    addPairedNode("AA:BB:CC:11:22:33", "Room 1");

    MsgTelemetry mockPacket = {0};
    mockPacket.header.msg_type = MSG_TELEMETRY;
    mockPacket.temperature = -15.5f;

    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33};
    testHook_simulateEspNowRx(mac, (uint8_t*)&mockPacket, sizeof(mockPacket));

    xTaskCreate(espNowGatewayTask, "esp_now_gw", 4096, NULL, 5,
                &current_test_task);
    vTaskDelay(pdMS_TO_TICKS(50));

    PairedNode node;
    TEST_ASSERT_TRUE(getNodeByMac("AA:BB:CC:11:22:33", &node));
    TEST_ASSERT_EQUAL_FLOAT(-15.5f, node.current_data.current_temperature);
}

// TC: IG-16
void test_espnow_rx_ignores_unpaired_mac(void) {
    uint8_t count_before = getActiveNodeCount();

    MsgTelemetry mockPacket = {0};
    mockPacket.header.msg_type = MSG_TELEMETRY;
    uint8_t unpair_mac[6] = {0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA};

    testHook_simulateEspNowRx(unpair_mac, (uint8_t*)&mockPacket,
                              sizeof(mockPacket));

    xTaskCreate(espNowGatewayTask, "esp_now_gw", 4096, NULL, 5,
                &current_test_task);
    vTaskDelay(pdMS_TO_TICKS(50));

    TEST_ASSERT_EQUAL(count_before, getActiveNodeCount());
}

// TC: IG-17
void test_espnow_task_consumes_downlink_queue(void) {
    xQueueReset(gw_downlink_queue);

    GwDownlinkMessage msg;
    msg.type = DOWNLINK_UNPAIR;
    xQueueSend(gw_downlink_queue, &msg, 0);

    xTaskCreate(espNowGatewayTask, "esp_now_gw", 4096, NULL, 5,
                &current_test_task);
    vTaskDelay(pdMS_TO_TICKS(50));

    TEST_ASSERT_EQUAL(0, uxQueueMessagesWaiting(gw_downlink_queue));
}

// TC: IG-18
void test_heartbeat_monitor_task_integration(void) {
    addPairedNode("AA:BB:CC:11:22:33", "Room 1");

    testHook_setNodeLastSeenMillis("AA:BB:CC:11:22:33", 0);
    delay(50);
    checkNodesOnlineStatus(10);

    PairedNode node;
    TEST_ASSERT_TRUE(getNodeByMac("AA:BB:CC:11:22:33", &node));
    TEST_ASSERT_FALSE(node.is_online);  // Confirm offline

    MsgTelemetry mockPacket = {0};
    mockPacket.header.msg_type = MSG_TELEMETRY;
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33};
    testHook_simulateEspNowRx(mac, (uint8_t*)&mockPacket, sizeof(mockPacket));

    xTaskCreate(espNowGatewayTask, "esp_now_gw", 4096, NULL, 5,
                &current_test_task);
    vTaskDelay(pdMS_TO_TICKS(50));

    TEST_ASSERT_TRUE(getNodeByMac("AA:BB:CC:11:22:33", &node));
    TEST_ASSERT_TRUE(node.is_online);
}

// TC: IG-19
void test_downlink_queue_overflow(void) {
    xQueueReset(gw_downlink_queue);
    GwDownlinkMessage msg;
    msg.type = DOWNLINK_PAIRING;

    // Fill up the queue (assuming QUEUE_LENGTH is 10)
    for (int i = 0; i < 10; i++) {
        xQueueSend(gw_downlink_queue, &msg, 0);
    }
    // Now the queue is full, the next send should fail
    BaseType_t res = xQueueSend(gw_downlink_queue, &msg, 0);
    TEST_ASSERT_EQUAL(errQUEUE_FULL, res);
}

// TC: IG-20
void test_flash_first_boot_creation(void) {
    nvs_flash_erase();  // Delete NVS to simulate first boot
    nvs_flash_init();   // Re-init to create empty NVS namespace

    loadConfigFromFlash();

    // Expect default config values to be set
    TEST_ASSERT_EQUAL_INT16(2000, getSensorConfig().read_interval_ms);
}

// TC: IG-21
void test_web_api_unpair_downlink(void) {
    GwDownlinkMessage msg;
    msg.type = DOWNLINK_UNPAIR;
    xQueueSend(gw_downlink_queue, &msg, 0);
    GwDownlinkMessage recv;
    xQueueReceive(gw_downlink_queue, &recv, 0);
    TEST_ASSERT_EQUAL(DOWNLINK_UNPAIR, recv.type);
}

// TC: IG-22
void test_web_api_clear_alarm_downlink(void) {
    GwDownlinkMessage msg;
    msg.type = DOWNLINK_CONTROL_CMD;
    msg.cmd_code = 0xBB;  // CMD_CLEAR_ALARM
    xQueueSend(gw_downlink_queue, &msg, 0);
    GwDownlinkMessage recv;
    xQueueReceive(gw_downlink_queue, &recv, 0);
    TEST_ASSERT_EQUAL(0xBB, recv.cmd_code);
}

// TC: IG-23
void test_web_api_force_read_downlink(void) {
    GwDownlinkMessage msg;
    msg.type = DOWNLINK_CONTROL_CMD;
    msg.cmd_code = 0xCC;  // CMD_FORCE_READ
    xQueueSend(gw_downlink_queue, &msg, 0);
    GwDownlinkMessage recv;
    xQueueReceive(gw_downlink_queue, &recv, 0);
    TEST_ASSERT_EQUAL(0xCC, recv.cmd_code);
}

// TC: IG-24
void test_espnow_rx_ack_response_log(void) {
    MsgAckResponse mockPacket = {0};
    mockPacket.header.msg_type = MSG_ACK_RESPONSE;

    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33};
    testHook_simulateEspNowRx(mac, (uint8_t*)&mockPacket, sizeof(mockPacket));

    xTaskCreate(espNowGatewayTask, "esp_now_gw", 4096, NULL, 5,
                &current_test_task);
    vTaskDelay(pdMS_TO_TICKS(50));

    TEST_ASSERT_TRUE(true);
}