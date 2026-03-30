#include "DHT20.h"
#include "Wire.h"
#include "global.h"

enum ERRORS_SENSOR {
    EVENT_TEMP_HIGH,
    EVENT_TEMP_LOW,
    EVENT_HUM_HIGH,
    EVENT_HUM_LOW,
    EVENT_TEMP_NORMAL,
    EVENT_HUM_NORMAL
};

static uint8_t temp_error = EVENT_TEMP_NORMAL;
static uint8_t hum_error = EVENT_HUM_NORMAL;

void readSensorTask(void *pvParameters);