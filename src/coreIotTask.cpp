#include "coreIotTask.h"

WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

String buildJsonPlayload() {
    SensorData data = getSensorData();
    uint32_t flags = getActiveErrorFlags();
    StaticJsonDocument<256> doc;

    if (data.is_dht20_ok) {
        doc["temperature"] = serialized(String(data.current_temperature, 1));
        doc["humidity"] = serialized(String(data.current_humidity, 1));
    } else {
        doc["temperature"] = nullptr;
        doc["humidity"] = nullptr;
    }

    if (flags & EVENT_SENSOR_ERROR) {
        doc["status"] = "SENSOR_FAULT";
    } else if (flags & (EVENT_TEMP_HIGH | EVENT_TEMP_LOW)) {
        doc["status"] = "CRITICAL_TEMP";
    } else if (flags & (EVENT_HUM_HIGH | EVENT_HUM_LOW)) {
        doc["status"] = "HUMIDITY_WARN";
    } else {
        doc["status"] = "NORMAL";
    }

    String pay_load;
    serializeJson(doc, pay_load);
    return pay_load;
}

void coreIotTask(void* pvParematers) {
    LOG_INFO("Iot", "Core IoT task started");
    String client_id =
        "ColdChanin_GW_" + String((uint32_t)ESP.getEfuseMac(), HEX);

    while (1) {
        SystemConfig config = getSystemConfig();
        if (checkSystemErrorFlag(EVENT_WIFI_DISCONN)) {
            LOG_WARN("Iot", "Wifi disconnected, cannot publish data");

            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        mqtt_client.setServer(config.core_iot_server, config.core_iot_port);

        if (!mqtt_client.connected()) {
            LOG_INFO("Iot", "Connecting to MQTT broker...");
            if (mqtt_client.connect(client_id.c_str(), config.core_iot_token,
                                    NULL)) {
                LOG_INFO("Iot", "Connected MQTT");
                clearSystemErrorFlag(EVENT_COREIOT_DISCONN);
            } else {
                if (!checkSystemErrorFlag(EVENT_COREIOT_DISCONN)) {
                    LOG_ERR("Iot", "MQTT connected fail RC=%d",
                            mqtt_client.state());
                    setSystemErrorFlag(EVENT_COREIOT_DISCONN);
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

        const char* topic_telemetry = "devices/node01/telemetry";

        if (mqtt_client.publish(topic_telemetry, pay_load.c_str())) {
            if (is_urgen_event) {
                LOG_WARN("Iot", "Published urgent event data: %s",
                         pay_load.c_str());
            } else {
                LOG_INFO("Iot", "Published data: %s", pay_load.c_str());
            }
        } else {
            LOG_ERR("Iot", "Failed to publish data");
        }
    }
}