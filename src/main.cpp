#include "global.h"
#include "network_manager.h"

void setup() {
    Serial.begin(115200);
    initGlobal();
    loadConfigFromFlash();

    // Create tasks
    // xTaskCreate();
    xTaskCreate(networkTask, "Network_Task", 4096, NULL, 5, NULL);

    LOG_INFO("SETUP", "System setup complete.");
}

void loop() {}
