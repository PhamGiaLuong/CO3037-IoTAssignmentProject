#include "global.h"
#include "lcdDisplayTask.h"
#include "neopixelTask.h"
#include "readSensorTask.h"

void setup() {
    Serial.begin(115200);
    Wire.begin();
    initGlobal();
    loadConfigFromFlash();

    // Create tasks
    xTaskCreate(readSensorTask, "SensorTask", 4096, NULL, 2, NULL);
    xTaskCreate(neopixelTask, "NeopixelTask", 2048, NULL, 1, NULL);
    xTaskCreate(lcdDisplayTask, "LCDTask", 2048, NULL, 1, NULL);

    LOG_INFO("SETUP", "System setup complete.");
}

void loop() {}
