#include "global.h"

void setup() {
    Serial.begin(115200);
    initGlobal();
    loadConfigFromFlash();

    // Create tasks
    // xTaskCreate();

    LOG_INFO("SETUP", "System setup complete.");
}

void loop() {}
