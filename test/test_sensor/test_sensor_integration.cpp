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
extern void testHook_simulateTxStatus(bool status);
extern void testHook_resetEspNowQueues();

// TC: IS-01
void test_error_triggers_led_semaphore(void) {
    // Drain semaphore
    xSemaphoreTake(sensor_led_sync_semaphore, 0);

    setSensorErrorFlag(SENSOR_FLAG_TEMP_WARN);

    // Check if task was awakened
    BaseType_t res = xSemaphoreTake(sensor_led_sync_semaphore, 0);
    TEST_ASSERT_EQUAL(pdTRUE, res);
}

// TC: IS-02
void test_error_triggers_lcd_semaphore(void) {
    xSemaphoreTake(lcd_sync_semaphore, 0);

    setSensorErrorFlag(SENSOR_FLAG_LCD_ERR);

    BaseType_t res = xSemaphoreTake(lcd_sync_semaphore, 0);
    TEST_ASSERT_EQUAL(pdTRUE, res);
}

// TC: IS-03
void test_save_load_config_flash_nvs(void) {
    SensorConfig writeConfig = {1500, 35.0, 15.0, 85.0, 45.0};
    setSensorConfig(writeConfig);
    saveConfigToFlash();  // Save to Flash

    // Corrupt RAM deliberately
    SensorConfig corruptConfig = {0, 0, 0, 0, 0};
    setSensorConfig(corruptConfig);

    loadConfigFromFlash();  // Restore from Flash

    SensorConfig retrieved = getSensorConfig();
    TEST_ASSERT_EQUAL_INT16(1500, retrieved.read_interval_ms);
    TEST_ASSERT_EQUAL_FLOAT(35.0, retrieved.max_temp_threshold);
}

// TC: IS-04
void test_flash_first_boot_sensor_creation(void) {
    nvs_flash_erase();
    nvs_flash_init();

    SensorConfig corrupt = {0};
    setSensorConfig(corrupt);

    loadConfigFromFlash();

    SensorConfig retrieved = getSensorConfig();
    TEST_ASSERT_EQUAL_INT16(DEFAULT_READ_INTERVAL_MS,
                            retrieved.read_interval_ms);
    TEST_ASSERT_EQUAL_FLOAT(DEFAULT_MAX_TEMP_THRESHOLD,
                            retrieved.max_temp_threshold);
}

// TC: IS-05
void test_sensor_error_triggers_all_ui_semaphores(void) {
    xSemaphoreTake(sensor_led_sync_semaphore, 0);
    xSemaphoreTake(neo_pixel_sync_semaphore, 0);
    xSemaphoreTake(lcd_sync_semaphore, 0);

    setSensorErrorFlag(SENSOR_FLAG_DHT_ERR);

    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(sensor_led_sync_semaphore, 0));
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(neo_pixel_sync_semaphore, 0));
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(lcd_sync_semaphore, 0));
}

// TC: IS-06
void test_sensor_clear_error_triggers_all_ui_semaphores(void) {
    setSensorErrorFlag(SENSOR_FLAG_HUM_HIGH);

    xSemaphoreTake(sensor_led_sync_semaphore, 0);
    xSemaphoreTake(neo_pixel_sync_semaphore, 0);
    xSemaphoreTake(lcd_sync_semaphore, 0);

    clearSensorErrorFlag(SENSOR_FLAG_HUM_HIGH);

    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(sensor_led_sync_semaphore, 0));
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(neo_pixel_sync_semaphore, 0));
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(lcd_sync_semaphore, 0));
}

// TC: IS-07
void test_tx_telemetry_skip_unpaired(void) {
    MsgPairing unpair_msg = {{MSG_PAIRING, 1}, 0x02, ""};
    uint8_t gw_mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    testHook_simulateEspNowRx(gw_mac, (uint8_t*)&unpair_msg,
                              sizeof(unpair_msg));

    xTaskCreate(espNowSensorTask, "esp_now_sn", 4096, NULL, 5,
                &current_test_task);
    vTaskDelay(pdMS_TO_TICKS(50));

    xSemaphoreGive(sensor_send_telemetry_semaphore);
    vTaskDelay(pdMS_TO_TICKS(50));

    TEST_ASSERT_EQUAL(pdFALSE,
                      xSemaphoreTake(sensor_send_telemetry_semaphore, 0));
    TEST_ASSERT_TRUE(checkSensorErrorFlag(SENSOR_FLAG_UNPAIRED));
}

// TC: IS-08
void test_tx_telemetry_success_flow(void) {
    testHook_resetEspNowQueues();
    clearSensorErrorFlag(0xFFFF);

    MsgPairing pair_msg = {{MSG_PAIRING, 2}, 0x01, "ColdRoom"};
    uint8_t gw_mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    testHook_simulateEspNowRx(gw_mac, (uint8_t*)&pair_msg, sizeof(pair_msg));

    xTaskCreate(espNowSensorTask, "esp_now_sn", 4096, NULL, 5,
                &current_test_task);
    vTaskDelay(pdMS_TO_TICKS(50));

    SensorData mock_data = {25.5f, 60.0f, true, true};
    setSensorData(mock_data);
    MlState mock_ml = {"Door Open", 0.95f};
    setMlState(mock_ml);
    xSemaphoreGive(sensor_send_telemetry_semaphore);

    vTaskDelay(pdMS_TO_TICKS(20));

    testHook_simulateTxStatus(true);
    vTaskDelay(pdMS_TO_TICKS(100));
    TEST_ASSERT_FALSE(checkSensorErrorFlag(SENSOR_FLAG_ESPNOW_DISCONN));
}

// TC: IS-09
void test_tx_failure_triggers_channel_hopping(void) {
    MsgPairing pair_msg = {{MSG_PAIRING, 3}, 0x01, "ColdRoom"};
    uint8_t gw_mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    testHook_simulateEspNowRx(gw_mac, (uint8_t*)&pair_msg, sizeof(pair_msg));

    xTaskCreate(espNowSensorTask, "esp_now_sn", 4096, NULL, 5,
                &current_test_task);
    vTaskDelay(pdMS_TO_TICKS(50));

    xSemaphoreGive(sensor_send_telemetry_semaphore);
    testHook_simulateTxStatus(false);
    for (int i = 0; i < 5; i++) {
        vTaskDelay(pdMS_TO_TICKS(100));
        testHook_simulateTxStatus(false);
    }
    TEST_ASSERT_TRUE(checkSensorErrorFlag(SENSOR_FLAG_ESPNOW_DISCONN));
}

// TC: IS-10
void test_rx_command_triggers_tx_ack(void) {
    MsgSyncConfig sync_msg;
    sync_msg.header.msg_type = MSG_SYNC_CONFIG;
    sync_msg.reading_interval = 9999;
    uint8_t gw_mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    testHook_simulateEspNowRx(gw_mac, (uint8_t*)&sync_msg, sizeof(sync_msg));

    xTaskCreate(espNowSensorTask, "esp_now_sn", 4096, NULL, 5,
                &current_test_task);
    vTaskDelay(pdMS_TO_TICKS(50));

    SensorConfig current_cfg = getSensorConfig();
    TEST_ASSERT_EQUAL_INT16(9999, current_cfg.read_interval_ms);
}

// TC: IS-11
void test_espnow_rx_ignore_unknown_msg_type(void) {
    if (gw_downlink_queue != NULL) xQueueReset(gw_downlink_queue);

    // Tạo một gói tin với msg_type lạ hoắc
    struct MockUnknownPacket {
        MsgHeader header;
        uint8_t random_payload[10];
    } mockPacket;

    mockPacket.header.msg_type = 0xFF;
    mockPacket.header.seq_num = 1;

    uint8_t random_mac[6] = {0x99, 0x88, 0x77, 0x66, 0x55, 0x44};
    testHook_simulateEspNowRx(random_mac, (uint8_t*)&mockPacket,
                              sizeof(mockPacket));
    xTaskCreate(espNowSensorTask, "esp_now_sn", 4096, NULL, 5,
                &current_test_task);
    vTaskDelay(pdMS_TO_TICKS(50));

    TEST_ASSERT_EQUAL(0, uxQueueMessagesWaiting(gw_downlink_queue));

    uint32_t flags = getSensorActiveErrorFlags();
    TEST_ASSERT_EQUAL_UINT32(0, flags & ~(SENSOR_FLAG_UNPAIRED));
}

// TC: IS-12
void test_espnow_rx_unknown_control_cmd_sends_ack(void) {
    testHook_resetEspNowQueues();
    MsgControlCmd mockPacket = {0};
    mockPacket.header.msg_type = MSG_CONTROL_CMD;
    mockPacket.cmd_code = 0x99;

    uint8_t gw_mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    testHook_simulateEspNowRx(gw_mac, (uint8_t*)&mockPacket,
                              sizeof(mockPacket));

    xTaskCreate(espNowSensorTask, "esp_now_sn", 4096, NULL, 5,
                &current_test_task);
    vTaskDelay(pdMS_TO_TICKS(50));

    testHook_simulateTxStatus(true);
    vTaskDelay(pdMS_TO_TICKS(10));

    TEST_ASSERT_EQUAL(0, uxQueueMessagesWaiting(gw_downlink_queue));
}

// TC: IS-13
void test_pairing_mac_override_on_new_request(void) {
    uint8_t mac_A[6] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
    MsgPairing pair_A = {{MSG_PAIRING, 1}, 0x01, "Old_Room"};
    testHook_simulateEspNowRx(mac_A, (uint8_t*)&pair_A, sizeof(pair_A));

    xTaskCreate(espNowSensorTask, "esp_now_sn", 4096, NULL, 5,
                &current_test_task);
    vTaskDelay(pdMS_TO_TICKS(50));

    TEST_ASSERT_FALSE(checkSensorErrorFlag(SENSOR_FLAG_UNPAIRED));

    uint8_t mac_B[6] = {0x99, 0x99, 0x99, 0x99, 0x99, 0x99};
    MsgPairing pair_B = {{MSG_PAIRING, 2}, 0x01, "New_Room"};
    testHook_simulateEspNowRx(mac_B, (uint8_t*)&pair_B, sizeof(pair_B));

    vTaskDelay(pdMS_TO_TICKS(50));
    TEST_ASSERT_FALSE(checkSensorErrorFlag(SENSOR_FLAG_UNPAIRED));
}

// TC: IS-14
void test_sync_config_zero_interval_handling(void) {
    MsgSyncConfig sync_msg;
    sync_msg.header.msg_type = MSG_SYNC_CONFIG;
    sync_msg.reading_interval = 0;
    sync_msg.max_temp = 30.0f;

    uint8_t gw_mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    testHook_simulateEspNowRx(gw_mac, (uint8_t*)&sync_msg, sizeof(sync_msg));

    xTaskCreate(espNowSensorTask, "esp_now_sn", 4096, NULL, 5,
                &current_test_task);
    vTaskDelay(pdMS_TO_TICKS(50));
    SensorConfig current_cfg = getSensorConfig();
    TEST_ASSERT_EQUAL_INT16(0, current_cfg.read_interval_ms);
}

// TC: IG-15
void test_binary_semaphore_non_stacking_behavior(void) {
    xSemaphoreTake(sensor_led_sync_semaphore, 0);

    xSemaphoreGive(sensor_led_sync_semaphore);
    xSemaphoreGive(sensor_led_sync_semaphore);
    xSemaphoreGive(sensor_led_sync_semaphore);

    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(sensor_led_sync_semaphore, 0));
    TEST_ASSERT_EQUAL(pdFALSE, xSemaphoreTake(sensor_led_sync_semaphore, 0));
}

// TC: IG-16
void test_cascading_ui_sync_on_critical_failure(void) {
    xSemaphoreTake(sensor_led_sync_semaphore, 0);
    xSemaphoreTake(neo_pixel_sync_semaphore, 0);
    xSemaphoreTake(lcd_sync_semaphore, 0);

    setSensorErrorFlag(SENSOR_FLAG_DHT_ERR);
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(sensor_led_sync_semaphore, 0));
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(neo_pixel_sync_semaphore, 0));
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(lcd_sync_semaphore, 0));
}

// TC: IG-17
void test_telemetry_force_send_sync(void) {
    testHook_resetEspNowQueues();
    clearSensorErrorFlag(0xFFFF);

    uint8_t gw_mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    MsgPairing pair_msg = {{MSG_PAIRING, 1}, 0x01, "TestRoom"};
    testHook_simulateEspNowRx(gw_mac, (uint8_t*)&pair_msg, sizeof(pair_msg));

    xTaskCreate(espNowSensorTask, "esp_now_sn", 4096, NULL, 5,
                &current_test_task);
    vTaskDelay(pdMS_TO_TICKS(50));
    MsgControlCmd cmd_pkt;
    cmd_pkt.header.msg_type = MSG_CONTROL_CMD;
    cmd_pkt.cmd_code = CMD_FORCE_READ;
    testHook_simulateEspNowRx(gw_mac, (uint8_t*)&cmd_pkt, sizeof(cmd_pkt));
    vTaskDelay(pdMS_TO_TICKS(20));

    testHook_simulateTxStatus(true);
    vTaskDelay(pdMS_TO_TICKS(100));
    TEST_ASSERT_FALSE(checkSensorErrorFlag(SENSOR_FLAG_ESPNOW_DISCONN));
}

// TC: IG-18
