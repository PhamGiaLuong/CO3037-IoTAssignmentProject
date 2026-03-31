#ifndef __ESP_NOW_MANAGER_H__
#define __ESP_NOW_MANAGER_H__

#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "global.h"

// Message Types
#define MSG_TELEMETRY 0x01
#define MSG_SYNC_CONFIG 0x02
#define MSG_PAIRING 0x03
#define MSG_CONTROL_CMD 0x04
#define MSG_ACK_RESPONSE 0x05

// Command Codes for Control Message
#define CMD_HARD_RESET 0xAA
#define CMD_CLEAR_ALARM 0xBB
#define CMD_FORCE_READ 0xCC

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

struct RxPacket {
    uint8_t mac_addr[6];
    uint8_t data[250];
    int len;
};

void initEspNow();
void espNowSensorTask(void* pvParameters);
void espNowGatewayTask(void* pvParameters);

#endif  // __ESP_NOW_MANAGER_H__