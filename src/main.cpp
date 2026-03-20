#include "global.h"
#include "lcdDisplayTask.h"
#include "neopixelTask.h"
#include "readSensorTask.h"

void setup() {
    Serial.begin(115200);
    delay(2000);
    Wire.begin(SENSOR_SDA_PIN, SENSOR_SCL_PIN);
    strip.begin();
    strip.show();
    delay(500);
    initGlobal();
    loadConfigFromFlash();

    // Create tasks
    xTaskCreate(readSensorTask, "SensorTask", 4096, NULL, 2, NULL);
    xTaskCreate(neopixelTask, "NeopixelTask", 2048, NULL, 1, NULL);
    xTaskCreate(lcdDisplayTask, "LCDTask", 4096, NULL, 1, NULL);

    LOG_INFO("SETUP", "System setup complete.");
}

void loop() {}
