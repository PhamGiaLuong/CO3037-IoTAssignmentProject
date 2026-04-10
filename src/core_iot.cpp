#include "core_iot.h"

#include "read_sensor.h"

WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

const char* wifi_ssid = "Computer Virus";
const char* wifi_password = "12346789";

String buildJsonPlayload(String deviceName) {
    // SensorData data = getSensorData();
    float mock_temp = random(100, 400) / 10.0;

    float mock_hum = random(200, 900) / 10.0;
    SensorData data = {mock_temp, mock_hum, true, true};
    String payload = "{\"" + deviceName +
                     "\":[{\"temperature\":" + String(mock_temp, 1) +
                     ",\"humidity\":" + String(mock_hum, 1) + "}]}";
    return payload;
    // cho gateway
    // StaticJsonDocument<1024> doc;

    // // Thiết bị 1: ESP32_001
    // JsonArray dev1 = doc["Sensor 01"].to<JsonArray>();
    // JsonObject data1 = dev1.createNestedObject();
    // // data1["ts"] = 1712750000000; // Thêm ts nếu bạn có NTP
    // JsonObject val1 = data1["values"].to<JsonObject>();
    // val1["temperature"] = 22.5;
    // val1["humidity"] = 55;

    // // Thiết bị 2: ESP32_002
    // JsonArray dev2 = doc["Sensor 02"].to<JsonArray>();
    // JsonObject data2 = dev2.createNestedObject();
    // JsonObject val2 = data2["values"].to<JsonObject>();
    // val2["temperature"] = 30.5;
    // val2["humidity"] = 80;

    // // Thiết bị 3: ESP32_003
    // JsonArray dev3 = doc["Sensor 03"].to<JsonArray>();
    // JsonObject data3 = dev3.createNestedObject();
    // JsonObject val3 = data3["values"].to<JsonObject>();
    // val3["temperature"] = 10.5;
    // val3["humidity"] = 20;

    // String payload;
    // serializeJson(doc, payload);
    // return payload;
}

void coreIotTask(void* pvParematers) {
    LOG_INFO("IOT", "Core IoT task started");
    Serial.print("Connecting to WiFi...");
    WiFi.begin(wifi_ssid, wifi_password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
    }
    String client_id =
        "ColdChanin_GW_" + String((uint32_t)ESP.getEfuseMac(), HEX);

    while (1) {
        GatewayConfig config = getGatewayConfig();
        strlcpy(config.core_iot_server, "app.coreiot.io", MAX_SERVER_LEN);
        config.core_iot_port = 1883;
        strlcpy(config.core_iot_token, "coreiot_gateway", MAX_TOKEN_LEN);
        if (checkGatewayErrorFlag(GW_FLAG_WIFI_DISCONN)) {
            LOG_WARN("IOT", "Wifi disconnected, cannot publish data");
            xSemaphoreGive(wifi_error_semaphore);

            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        mqtt_client.setServer(config.core_iot_server, config.core_iot_port);

        if (!mqtt_client.connected()) {
            LOG_INFO("IOT", "Connecting to MQTT broker...");
            if (mqtt_client.connect(client_id.c_str(), config.core_iot_token,
                                    NULL)) {
                LOG_INFO("IOT", "Connected MQTT");
                clearGatewayErrorFlag(GW_FLAG_COREIOT_DISCONN);
            } else {
                if (!checkGatewayErrorFlag(GW_FLAG_COREIOT_DISCONN)) {
                    LOG_ERR("IOT", "MQTT connected fail RC=%d",
                            mqtt_client.state());
                    setGatewayErrorFlag(GW_FLAG_COREIOT_DISCONN);
                }
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
        }

        mqtt_client.loop();
        TickType_t wait_time = pdMS_TO_TICKS(config.send_interval_ms);
        bool is_urgen_event =
            (xSemaphoreTake(coreiot_error_semaphore, wait_time) == pdTRUE);
        String pay_load = buildJsonPlayload("Sensor 04");

        const char* topic_telemetry = "v1/gateway/telemetry";

        if (mqtt_client.publish(topic_telemetry, pay_load.c_str())) {
            if (is_urgen_event) {
                LOG_WARN("IOT", "Published urgent event data: %s",
                         pay_load.c_str());
            } else {
                LOG_INFO("IOT", "Published data: %s", pay_load.c_str());
            }
        } else {
            LOG_ERR("IOT", "Failed to publish data");
        }
    }
}