#include <Arduino.h>
#include <unity.h>

#include "esp_now_manager.h"
#include "global.h"

TaskHandle_t current_test_task = NULL;
extern void testHook_resetRxQueue();

// Unit Tests declarations
extern void test_set_and_clear_sensor_error_flag(void);
extern void test_sensor_config_read_write(void);
extern void test_ml_state_encapsulation(void);
extern void test_sensor_data_read_write(void);
extern void test_multiple_error_flags_logic(void);
extern void test_multiple_error_flags_logic_2(void);
extern void test_clear_non_existent_flag(void);
extern void test_sensor_config_boundary_values(void);
// extern void test_lcd_i2c_connection(void);
extern void test_mac_str_to_bytes_conversion(void);
extern void test_bytes_to_mac_str_conversion(void);
extern void test_map_prediction_to_id(void);
extern void test_add_node_exceeds_max_limit(void);
extern void test_remove_node_not_found(void);
extern void test_update_node_config_not_found(void);
extern void test_update_node_data_ml_not_found(void);

// Integration Tests declarations
extern void test_error_triggers_led_semaphore(void);
extern void test_error_triggers_lcd_semaphore(void);
extern void test_save_load_config_flash_nvs(void);
extern void test_flash_first_boot_sensor_creation(void);
extern void test_sensor_error_triggers_all_ui_semaphores(void);
extern void test_sensor_clear_error_triggers_all_ui_semaphores(void);
extern void test_tx_telemetry_skip_unpaired(void);
extern void test_tx_telemetry_success_flow(void);
extern void test_tx_failure_triggers_channel_hopping(void);
extern void test_rx_command_triggers_tx_ack(void);
extern void test_espnow_rx_ignore_unknown_msg_type(void);
extern void test_espnow_rx_unknown_control_cmd_sends_ack(void);
extern void test_pairing_mac_override_on_new_request(void);
extern void test_sync_config_zero_interval_handling(void);
extern void test_binary_semaphore_non_stacking_behavior(void);
extern void test_cascading_ui_sync_on_critical_failure(void);
extern void test_telemetry_force_send_sync(void);

void setUp(void) {
    if (gw_downlink_queue != NULL) xQueueReset(gw_downlink_queue);
    testHook_resetRxQueue();
    clearSensorErrorFlag(0xFFFF);

    xSemaphoreTake(sensor_led_sync_semaphore, 0);
    xSemaphoreTake(lcd_sync_semaphore, 0);
    xSemaphoreTake(neo_pixel_sync_semaphore, 0);
}

void tearDown(void) {
    // Clean up nodes after each test
    PairedNode nodes[10];
    uint8_t count = getPairedNodesSnapshot(nodes, 10);
    for (uint8_t i = 0; i < count; i++) {
        removePairedNode(nodes[i].mac_address);
    }

    if (current_test_task != NULL) {
        vTaskDelete(current_test_task);
        current_test_task = NULL;
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }

    delay(2000);

    initGlobal();
    WiFi.mode(WIFI_STA);
    initEspNow();
    UNITY_BEGIN();

    // Unit Test
    RUN_TEST(test_set_and_clear_sensor_error_flag);
    RUN_TEST(test_sensor_config_read_write);
    RUN_TEST(test_ml_state_encapsulation);
    RUN_TEST(test_sensor_data_read_write);
    RUN_TEST(test_multiple_error_flags_logic);
    RUN_TEST(test_multiple_error_flags_logic_2);
    RUN_TEST(test_clear_non_existent_flag);
    RUN_TEST(test_sensor_config_boundary_values);
    // RUN_TEST(test_lcd_i2c_connection);
    RUN_TEST(test_mac_str_to_bytes_conversion);
    RUN_TEST(test_bytes_to_mac_str_conversion);
    RUN_TEST(test_map_prediction_to_id);
    RUN_TEST(test_add_node_exceeds_max_limit);
    RUN_TEST(test_remove_node_not_found);
    RUN_TEST(test_update_node_config_not_found);
    RUN_TEST(test_update_node_data_ml_not_found);

    // Integration Test
    RUN_TEST(test_error_triggers_led_semaphore);
    RUN_TEST(test_error_triggers_lcd_semaphore);
    RUN_TEST(test_save_load_config_flash_nvs);
    RUN_TEST(test_flash_first_boot_sensor_creation);
    RUN_TEST(test_sensor_error_triggers_all_ui_semaphores);
    RUN_TEST(test_sensor_clear_error_triggers_all_ui_semaphores);
    RUN_TEST(test_tx_telemetry_skip_unpaired);
    RUN_TEST(test_tx_telemetry_success_flow);
    RUN_TEST(test_tx_failure_triggers_channel_hopping);
    RUN_TEST(test_rx_command_triggers_tx_ack);
    RUN_TEST(test_espnow_rx_ignore_unknown_msg_type);
    RUN_TEST(test_espnow_rx_unknown_control_cmd_sends_ack);
    RUN_TEST(test_pairing_mac_override_on_new_request);
    RUN_TEST(test_sync_config_zero_interval_handling);
    RUN_TEST(test_binary_semaphore_non_stacking_behavior);
    RUN_TEST(test_cascading_ui_sync_on_critical_failure);
    RUN_TEST(test_telemetry_force_send_sync);

    UNITY_END();
}

void loop() {}