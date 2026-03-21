#include "global.h"
#include "hardware_manager.h"
#include "led_blinky.h"
#include "network_manager.h"
#include "web_server.h"

void setup() {
    Serial.begin(115200);
    initGlobal();
    loadConfigFromFlash();

    // Create tasks
    // Task 1: Led blinky
    xTaskCreate(ledBlinkyTask, "LedBlinkyTask", 2048, NULL, 2, NULL);
    // Task 2: RGB led

    // Task 3: Sensor reading & LCD display

    // Task 4: Web server
    xTaskCreate(networkTask, "Network_Task", 4096, NULL, 5, NULL);
    xTaskCreate(webServerTask, "WebServer_Task", 8192, NULL, 3, NULL);
    xTaskCreate(buzzerTask, "Buzzer_Task", 2048, NULL, 4, NULL);
    // Task 5: ML prediction

    // Task 6: Core IoT communication

    LOG_INFO("SETUP", "System setup complete.");
}

void loop() {}
