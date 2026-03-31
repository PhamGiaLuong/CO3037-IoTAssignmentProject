#include <Arduino.h>
#include <unity.h>

#include "global.h"

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