#include <Arduino.h>
#include <unity.h>

#include "esp_now_manager.h"
#include "global.h"

TaskHandle_t current_test_task = NULL;
extern void testHook_resetRxQueue();

// Unit Tests declarations
extern void test_add_paired_node_success(void);
extern void test_remove_paired_node(void);
extern void test_update_node_telemetry_data(void);
extern void test_gateway_config_encapsulation(void);
extern void test_gateway_state_encapsulation(void);
extern void test_control_state_encapsulation(void);
extern void test_set_clear_gw_error_flag(void);
extern void test_multiple_gw_error_flags(void);
extern void test_active_node_count_increment(void);
extern void test_active_node_count_decrement(void);
extern void test_add_duplicate_mac_node(void);
extern void test_get_node_by_mac_not_found(void);
extern void test_get_paired_nodes_snapshot(void);
extern void test_update_node_config_success(void);
extern void test_update_node_data_ml_success(void);
extern void test_mac_str_to_bytes_conversion(void);
extern void test_bytes_to_mac_str_conversion(void);
extern void test_prediction_to_id_mapping(void);
extern void test_heartbeat_update_valid_mac(void);
extern void test_heartbeat_check_timeout(void);
extern void test_msg_header_struct_packing(void);
extern void test_add_node_exceeds_max_limit(void);
extern void test_remove_node_not_found(void);
extern void test_update_node_config_not_found(void);
extern void test_update_node_data_ml_not_found(void);
extern void test_clear_all_gateway_error_flags(void);
extern void test_ml_prediction_unknown_mapping(void);

// Integration Tests declarations
extern void test_web_to_espnow_downlink_queue(void);
extern void test_heartbeat_offline_timeout(void);
extern void test_web_api_mode_switch_semaphore(void);
extern void test_flash_save_load_gw_config(void);
extern void test_flash_save_load_sensor_config(void);
extern void test_gw_error_triggers_led_semaphore(void);
extern void test_web_api_pushes_downlink_queue(void);
extern void test_web_api_pairing_downlink(void);
extern void test_web_api_sync_config_downlink(void);
extern void test_network_mode_switch_ap_sema(void);
extern void test_network_mode_switch_sta_sema(void);
extern void test_wifi_disconnect_triggers_error_sema(void);
extern void test_mqtt_disconnect_triggers_error_sema(void);
extern void test_espnow_rx_updates_heartbeat(void);
extern void test_espnow_rx_updates_global_data(void);
extern void test_espnow_rx_ignores_unpaired_mac(void);
extern void test_espnow_task_consumes_downlink_queue(void);
extern void test_heartbeat_monitor_task_integration(void);
extern void test_downlink_queue_overflow(void);
extern void test_flash_first_boot_creation(void);
extern void test_web_api_unpair_downlink(void);
extern void test_web_api_clear_alarm_downlink(void);
extern void test_web_api_force_read_downlink(void);
extern void test_espnow_rx_ack_response_log(void);

void setUp(void) {
    if (gw_downlink_queue != NULL) xQueueReset(gw_downlink_queue);
    testHook_resetRxQueue();
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
    RUN_TEST(test_add_paired_node_success);
    RUN_TEST(test_remove_paired_node);
    RUN_TEST(test_update_node_telemetry_data);
    RUN_TEST(test_gateway_config_encapsulation);
    RUN_TEST(test_gateway_state_encapsulation);
    RUN_TEST(test_control_state_encapsulation);
    RUN_TEST(test_set_clear_gw_error_flag);
    RUN_TEST(test_multiple_gw_error_flags);
    RUN_TEST(test_active_node_count_increment);
    RUN_TEST(test_active_node_count_decrement);
    RUN_TEST(test_add_duplicate_mac_node);
    RUN_TEST(test_get_node_by_mac_not_found);
    RUN_TEST(test_get_paired_nodes_snapshot);
    RUN_TEST(test_update_node_config_success);
    RUN_TEST(test_update_node_data_ml_success);
    RUN_TEST(test_mac_str_to_bytes_conversion);
    RUN_TEST(test_bytes_to_mac_str_conversion);
    RUN_TEST(test_prediction_to_id_mapping);
    RUN_TEST(test_heartbeat_update_valid_mac);
    RUN_TEST(test_heartbeat_check_timeout);
    RUN_TEST(test_msg_header_struct_packing);
    RUN_TEST(test_add_node_exceeds_max_limit);
    RUN_TEST(test_remove_node_not_found);
    RUN_TEST(test_update_node_config_not_found);
    RUN_TEST(test_update_node_data_ml_not_found);
    RUN_TEST(test_clear_all_gateway_error_flags);
    RUN_TEST(test_ml_prediction_unknown_mapping);

    // Integration Test
    RUN_TEST(test_web_to_espnow_downlink_queue);
    RUN_TEST(test_heartbeat_offline_timeout);
    RUN_TEST(test_web_api_mode_switch_semaphore);
    RUN_TEST(test_flash_save_load_gw_config);
    RUN_TEST(test_flash_save_load_sensor_config);
    RUN_TEST(test_gw_error_triggers_led_semaphore);
    RUN_TEST(test_web_api_pushes_downlink_queue);
    RUN_TEST(test_web_api_pairing_downlink);
    RUN_TEST(test_web_api_sync_config_downlink);
    RUN_TEST(test_network_mode_switch_ap_sema);
    RUN_TEST(test_network_mode_switch_sta_sema);
    RUN_TEST(test_wifi_disconnect_triggers_error_sema);
    RUN_TEST(test_mqtt_disconnect_triggers_error_sema);
    RUN_TEST(test_espnow_rx_updates_heartbeat);
    RUN_TEST(test_espnow_rx_updates_global_data);
    RUN_TEST(test_espnow_rx_ignores_unpaired_mac);
    RUN_TEST(test_espnow_task_consumes_downlink_queue);
    RUN_TEST(test_heartbeat_monitor_task_integration);
    RUN_TEST(test_downlink_queue_overflow);
    RUN_TEST(test_flash_first_boot_creation);
    RUN_TEST(test_web_api_unpair_downlink);
    RUN_TEST(test_web_api_clear_alarm_downlink);
    RUN_TEST(test_web_api_force_read_downlink);
    RUN_TEST(test_espnow_rx_ack_response_log);

    UNITY_END();
}

void loop() {}