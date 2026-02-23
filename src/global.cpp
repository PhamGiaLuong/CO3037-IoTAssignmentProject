#include "global.h"

static float global_temperature = 0;
static float global_humidity = 0;
static SemaphoreHandle_t xMutexSensorData = NULL;

void initSensorMutex() {
    if (xMutexSensorData == NULL) {
        xMutexSensorData = xSemaphoreCreateMutex();
    }
}

void setSensorData(float temp, float humi) {
    if (xMutexSensorData != NULL) {
        if (xSemaphoreTake(xMutexSensorData, portMAX_DELAY) == pdTRUE) {
            global_temperature = temp;
            global_humidity = humi;
            xSemaphoreGive(xMutexSensorData);
        }
    }
}

// Hàm đọc an toàn (Dành cho Web, TinyML, CoreIOT)
void getSensorData(float& temp, float& humi) {
    if (xMutexSensorData != NULL) {
        if (xSemaphoreTake(xMutexSensorData, portMAX_DELAY) == pdTRUE) {
            temp = global_temperature;
            humi = global_humidity;
            xSemaphoreGive(xMutexSensorData);
        }
    } else {
        temp = 0.0;
        humi = 0.0;
    }
}