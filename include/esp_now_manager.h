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

void initEspNow();
void espNowSensorTask(void* pvParameters);
void espNowGatewayTask(void* pvParameters);

#endif  // __ESP_NOW_MANAGER_H__