#include <stdint.h>

#include "global.h"

#define MSG_TYPE_SENSOR_DATA 1

// Payload structure from sensor node to gateway
typedef struct {
    uint8_t msg_type;
    SensorData data;
    MlState ml;
    uint32_t error_flags;
} __attribute__((packed)) SensorPayload;

typedef struct {
    char origin_mac[MAC_STR_LEN];
    SensorData data;
    MlState ml;
    uint32_t error_flags;
} __attribute__((packed)) ForwardPayload;

void espNowRxTask(void *pvParameters);
void espNowTxTask(void *pvParameters);
void initEspNowSystem();
