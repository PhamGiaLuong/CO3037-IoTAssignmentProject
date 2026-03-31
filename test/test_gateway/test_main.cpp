#include <Arduino.h>
#include <unity.h>

#include "global.h"

// Unit Tests declarations
extern void test_add_paired_node_success(void);
extern void test_remove_paired_node(void);
extern void test_update_node_telemetry_data(void);

// Integration Tests declarations
extern void test_web_to_espnow_downlink_queue(void);
extern void test_heartbeat_offline_timeout(void);
extern void test_web_api_mode_switch_semaphore(void);

void setUp(void) { initGlobal(); }

void tearDown(void) {
    // Clean up nodes after each test
    removePairedNode("AA:BB:CC:11:22:33");
}

void setup() {
    delay(2000);
    UNITY_BEGIN();

    // Unit Test
    RUN_TEST(test_add_paired_node_success);
    RUN_TEST(test_remove_paired_node);
    RUN_TEST(test_update_node_telemetry_data);

    // Integration Test
    RUN_TEST(test_web_to_espnow_downlink_queue);
    RUN_TEST(test_heartbeat_offline_timeout);
    RUN_TEST(test_web_api_mode_switch_semaphore);

    UNITY_END();
}

void loop() {}