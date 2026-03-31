#include <Arduino.h>
#include <unity.h>

#include "esp_now_manager.h"
#include "global.h"

extern void testHook_setNodeLastSeenMillis(const char* mac_str,
                                           uint32_t simulated_millis);
extern void testHook_macStrToBytes(const char* mac_str, uint8_t* mac_bytes);
extern void testHook_bytesToMacStr(const uint8_t* mac_bytes, char* mac_str);
extern uint8_t testHook_mapPredictionToId(const char* pred);

// TC: UG-01
void test_add_paired_node_success(void) {
    bool result = addPairedNode("AA:BB:CC:11:22:33", "Room 1");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, getActiveNodeCount());

    PairedNode node;
    getNodeByMac("AA:BB:CC:11:22:33", &node);
    TEST_ASSERT_EQUAL_FLOAT(
        0.0, node.current_data.current_temperature);  // Init data should be 0
}

// TC: UG-02
void test_remove_paired_node(void) {
    addPairedNode("AA:BB:CC:11:22:33", "Room 1");

    bool result = removePairedNode("AA:BB:CC:11:22:33");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, getActiveNodeCount());

    PairedNode node;
    TEST_ASSERT_FALSE(getNodeByMac("AA:BB:CC:11:22:33", &node));
}

// TC: UG-03
void test_update_node_telemetry_data(void) {
    addPairedNode("AA:BB:CC:11:22:33", "Room 1");

    SensorData newData = {25.0, 60.0, true, true};
    MlState newMl = {"Normal", 0.99};
    updateNodeDataAndMl("AA:BB:CC:11:22:33", newData, newMl);

    PairedNode node;
    getNodeByMac("AA:BB:CC:11:22:33", &node);
    TEST_ASSERT_EQUAL_FLOAT(25.0, node.current_data.current_temperature);
    TEST_ASSERT_EQUAL_STRING("Normal", node.current_ml_state.prediction);
}

// TC: UG-04
void test_gateway_config_encapsulation(void) {
    GatewayConfig mock = {0};
    strlcpy(mock.wifi_ssid, "TEST_SSID", MAX_SSID_LEN);
    strlcpy(mock.wifi_password, "12345678", MAX_PASS_LEN);
    strlcpy(mock.core_iot_token, "TOKEN_ABC", MAX_TOKEN_LEN);

    setGatewayConfig(mock);
    TEST_ASSERT_EQUAL_STRING("TEST_SSID", getGatewayConfig().wifi_ssid);
}

// TC: UG-05
void test_gateway_state_encapsulation(void) {
    GatewayState mock = {0};  // active_error_flags = 0
    mock.is_ap_mode = true;

    setGatewayState(mock);
    TEST_ASSERT_TRUE(getGatewayState().is_ap_mode);
}

// TC: UG-06
void test_control_state_encapsulation(void) {
    ControlState mock = {true, false};  // dev1_on, dev2_on
    setControlState(mock);
    TEST_ASSERT_TRUE(getControlState().is_device1_on);
    TEST_ASSERT_FALSE(getControlState().is_device2_on);
}

// TC: UG-07
void test_set_clear_gw_error_flag(void) {
    clearGatewayErrorFlag(0xFFFF);
    setGatewayErrorFlag(GW_FLAG_WIFI_DISCONN);
    TEST_ASSERT_TRUE(checkGatewayErrorFlag(GW_FLAG_WIFI_DISCONN));
}

// TC: UG-08
void test_multiple_gw_error_flags(void) {
    clearGatewayErrorFlag(0xFFFF);
    setGatewayErrorFlag(GW_FLAG_WIFI_DISCONN);
    setGatewayErrorFlag(GW_FLAG_COREIOT_DISCONN);
    uint16_t expected = GW_FLAG_WIFI_DISCONN | GW_FLAG_COREIOT_DISCONN;
    TEST_ASSERT_EQUAL(expected, getGatewayActiveErrorFlags());
}

// TC: UG-09
void test_active_node_count_increment(void) {
    addPairedNode("AA:BB:CC:11:22:33", "R1");
    addPairedNode("AA:BB:CC:11:22:34", "R2");
    TEST_ASSERT_EQUAL(2, getActiveNodeCount());
}

// TC: UG-10
void test_active_node_count_decrement(void) {
    addPairedNode("AA:BB:CC:11:22:33", "R1");
    removePairedNode("AA:BB:CC:11:22:33");
    TEST_ASSERT_EQUAL(0, getActiveNodeCount());
}

// TC: UG-11
void test_add_duplicate_mac_node(void) {
    addPairedNode("AA:BB:CC:11:22:33", "R1");
    TEST_ASSERT_FALSE(addPairedNode("AA:BB:CC:11:22:33", "R2"));
}

// TC: UG-12
void test_get_node_by_mac_not_found(void) {
    PairedNode node;
    TEST_ASSERT_FALSE(getNodeByMac("FF:FF:FF:FF:FF:FF", &node));
}

// TC: UG-13
void test_get_paired_nodes_snapshot(void) {
    addPairedNode("AA:BB:CC:11:22:33", "R1");
    PairedNode arr[2];
    TEST_ASSERT_EQUAL(1, getPairedNodesSnapshot(arr, 2));
}

// TC: UG-14
void test_update_node_config_success(void) {
    addPairedNode("AA:BB:CC:11:22:33", "R1");
    SensorConfig cfg = {2000, 20.0, 10.0, 80.0, 40.0};
    updateNodeConfig("AA:BB:CC:11:22:33", cfg);
    PairedNode node;
    getNodeByMac("AA:BB:CC:11:22:33", &node);
    TEST_ASSERT_EQUAL_FLOAT(20.0, node.current_config.max_temp_threshold);
}

// TC: UG-15
void test_update_node_data_ml_success(void) {
    addPairedNode("AA:BB:CC:11:22:33", "R1");
    SensorData data = {25.0, 60.0, true, true};
    MlState ml = {"Normal", 0.99};
    updateNodeDataAndMl("AA:BB:CC:11:22:33", data, ml);
    PairedNode node;
    getNodeByMac("AA:BB:CC:11:22:33", &node);
    TEST_ASSERT_EQUAL_FLOAT(25.0, node.current_data.current_temperature);
}

// TC: UG-16
void test_mac_str_to_bytes_conversion(void) {
    uint8_t mac[6];
    testHook_macStrToBytes("AA:BB:CC:11:22:33", mac);
    uint8_t exp[6] = {0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(exp, mac, 6);
}

// TC: UG-17
void test_bytes_to_mac_str_conversion(void) {
    uint8_t mac[6] = {0xFF, 0xEE, 0xDD, 0x11, 0x22, 0x33};
    char str[18];
    testHook_bytesToMacStr(mac, str);
    TEST_ASSERT_EQUAL_STRING("FF:EE:DD:11:22:33", str);
}

// TC: UG-18
void test_prediction_to_id_mapping(void) {
    TEST_ASSERT_EQUAL(1, testHook_mapPredictionToId("Door Open"));
}

// TC: UG-19
void test_heartbeat_update_valid_mac(void) {
    addPairedNode("AA:BB:CC:11:22:33", "R1");
    uint32_t t1 = millis();
    delay(50);
    updateNodeHeartbeat("AA:BB:CC:11:22:33");
    PairedNode node;
    getNodeByMac("AA:BB:CC:11:22:33", &node);
    TEST_ASSERT_GREATER_OR_EQUAL(t1 + 50, node.last_seen_millis);
}

// TC: UG-20
void test_heartbeat_check_timeout(void) {
    addPairedNode("AA:BB:CC:11:22:33", "Room 1");

    testHook_setNodeLastSeenMillis("AA:BB:CC:11:22:33", 0);
    delay(50);

    checkNodesOnlineStatus(10);

    PairedNode node;
    getNodeByMac("AA:BB:CC:11:22:33", &node);
    TEST_ASSERT_FALSE(node.is_online);
}

// TC: UG-21
void test_msg_header_struct_packing(void) {
    TEST_ASSERT_EQUAL(2, sizeof(MsgHeader));
}

// TC: UG-22
void test_add_node_exceeds_max_limit(void) {
    for (int i = 0; i < 10; i++) {  // Max limit 10
        char mac[20];
        sprintf(mac, "AA:BB:CC:11:22:%02X", i);
        addPairedNode(mac, "R");
    }
    TEST_ASSERT_FALSE(addPairedNode("AA:BB:CC:11:22:FF", "RX"));
    TEST_ASSERT_EQUAL(10, getActiveNodeCount());
}

// TC: UG-23
void test_remove_node_not_found(void) {
    TEST_ASSERT_FALSE(removePairedNode("00:11:22:33:44:55"));
}

// TC: UG-24
void test_update_node_config_not_found(void) {
    SensorConfig cfg;
    TEST_ASSERT_FALSE(updateNodeConfig("00:11:22:33:44:55", cfg));
}

// TC: UG-25
void test_update_node_data_ml_not_found(void) {
    SensorData d;
    MlState m;
    TEST_ASSERT_FALSE(updateNodeDataAndMl("00:11:22:33", d, m));
}

// TC: UG-26
void test_clear_all_gateway_error_flags(void) {
    setGatewayErrorFlag(GW_FLAG_WIFI_DISCONN);
    clearGatewayErrorFlag(0xFFFF);
    TEST_ASSERT_EQUAL(0, getGatewayActiveErrorFlags());
}

// TC: UG-27
void test_ml_prediction_unknown_mapping(void) {
    TEST_ASSERT_EQUAL(0, testHook_mapPredictionToId("Random String"));
}