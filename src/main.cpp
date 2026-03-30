#include "esp_now_manager.h"
#include "global.h"
#include "hardware_manager.h"
#include "led_blinky.h"

#ifdef SENSOR_NODE
#include "lcd_display.h"
#include "neo_pixel.h"
#include "read_sensor.h"
#include "tiny_ml.h"
#endif

#ifdef GATEWAY_NODE
#include "core_iot.h"
#include "network_manager.h"
#include "web_server.h"
#endif

void setup() {
    Serial.begin(115200);

    initGlobal();
    loadConfigFromFlash();
    initEspNow();

#ifdef SENSOR_NODE
    LOG_INFO("MAIN", "Booting SENSOR NODE...");
    delay(2000);
    Wire.begin(SENSOR_SDA_PIN, SENSOR_SCL_PIN);
    strip.begin();
    strip.show();
    delay(500);

    xTaskCreate(sensorLedBlinkyTask, "Led_SN", 2048, NULL, 1, NULL);
    xTaskCreate(neopixelTask, "NeoPixel", 2048, NULL, 2, NULL);
    xTaskCreate(readSensorTask, "Sensor", 4096, NULL, 3, NULL);
    xTaskCreate(lcdDisplayTask, "LCD", 4096, NULL, 2, NULL);
    xTaskCreate(tinyMlTask, "TinyML", 4096, NULL, 2, NULL);
    xTaskCreate(espNowSensorTask, "EspNow_SN", 4096, NULL, 4, NULL);
#endif

#ifdef GATEWAY_NODE
    LOG_INFO("MAIN", "Booting GATEWAY NODE...");

    xTaskCreate(gatewayLedBlinkyTask, "Led_GW", 2048, NULL, 1, NULL);
    xTaskCreate(networkTask, "Network", 4096, NULL, 4, NULL);
    xTaskCreate(webServerTask, "WebServer", 8192, NULL, 3, NULL);
    xTaskCreate(buzzerTask, "Buzzer_Task", 2048, NULL, 4, NULL);
    xTaskCreate(coreIotTask, "CoreIoT", 4096, NULL, 2, NULL);
    xTaskCreate(espNowGatewayTask, "EspNow_GW", 4096, NULL, 4, NULL);
#endif
}

void loop() {}
