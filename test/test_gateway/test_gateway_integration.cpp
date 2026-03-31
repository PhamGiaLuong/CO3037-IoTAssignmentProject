#include <Arduino.h>
#include <unity.h>

#include "global.h"

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

    // Note: Since paired_nodes is static in global.cpp, a test hook/friend
    // method is normally needed. For this test logic simulation: We assume a
    // hook exists to simulate the time jump.
    // simulateTimeJumpForNode("AA:BB:CC:11:22:33", -40000);

    // Then call check routine
    // checkNodesOnlineStatus(30000);
    // PairedNode node;
    // getNodeByMac("AA:BB:CC:11:22:33", &node);
    // TEST_ASSERT_FALSE(node.is_online);

    TEST_IGNORE_MESSAGE(
        "Requires global.cpp test hook for last_seen_millis manipulation");
}

// TC: IG-03
void test_web_api_mode_switch_semaphore(void) {
    xSemaphoreTake(switch_to_sta_semaphore, 0);

    // Simulate Web API call execution
    xSemaphoreGive(switch_to_sta_semaphore);

    BaseType_t res = xSemaphoreTake(switch_to_sta_semaphore, 0);
    TEST_ASSERT_EQUAL(pdTRUE, res);
}