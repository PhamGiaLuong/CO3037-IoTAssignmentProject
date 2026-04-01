#include "core_iot.h"

#include "read_sensor.h"

WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

String buildJsonPlayload() {
    SensorData data = getSensorData();
    StaticJsonDocument<256> doc;

    if (data.is_dht20_ok) {
        doc["temperature"] = serialized(String(data.current_temperature, 1));
        doc["humidity"] = serialized(String(data.current_humidity, 1));
    } else {
        doc["temperature"] = nullptr;
        doc["humidity"] = nullptr;
    }

    String pay_load;
    serializeJson(doc, pay_load);
    return pay_load;
}

void coreIotTask(void* pvParematers) {
    LOG_INFO("IOT", "Core IoT task started");
    String client_id =
        "ColdChanin_GW_" + String((uint32_t)ESP.getEfuseMac(), HEX);

    while (1) {
        GatewayConfig config = getGatewayConfig();
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
        String pay_load = buildJsonPlayload();

        const char* topic_telemetry = "esp/telemetry";

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