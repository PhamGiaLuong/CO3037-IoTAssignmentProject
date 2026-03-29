#include "global.h"
#include "hardware_manager.h"
#include "lcd_display.h"
#include "neo_pixel.h"
#include "network_manager.h"
#include "read_sensor.h"
#include "web_server.h"

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
    // Task 1: Led blinky

    // Task 2: RGB led
    xTaskCreate(neopixelTask, "NeopixelTask", 2048, NULL, 1, NULL);
    // Task 3: Sensor reading & LCD display
    xTaskCreate(readSensorTask, "SensorTask", 4096, NULL, 2, NULL);
    xTaskCreate(lcdDisplayTask, "LCDTask", 4096, NULL, 1, NULL);
    // Task 4: Web server
    xTaskCreate(networkTask, "Network_Task", 4096, NULL, 5, NULL);
    xTaskCreate(webServerTask, "WebServer_Task", 8192, NULL, 3, NULL);
    xTaskCreate(buzzerTask, "Buzzer_Task", 2048, NULL, 4, NULL);
    // Task 5: ML prediction

    // Task 6: Core IoT communication

    LOG_INFO("SETUP", "System setup complete.");
}

void loop() {}
