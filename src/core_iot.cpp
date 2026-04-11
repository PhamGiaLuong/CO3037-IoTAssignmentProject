#include "core_iot.h"

#include "read_sensor.h"

WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

String buildGatewayPayload() {
    PairedNode nodes[MAX_PAIRED_NODES];
    uint8_t count = getPairedNodesSnapshot(nodes, MAX_PAIRED_NODES);

    if (count == 0) return "{}";

    DynamicJsonDocument doc(1024);
    bool has_online_node = false;

    for (uint8_t i = 0; i < count; i++) {
        if (nodes[i].is_online) {
            has_online_node = true;

            String deviceName = String(nodes[i].node_name);
            JsonArray devArray = doc.createNestedArray(deviceName);
            JsonObject data = devArray.createNestedObject();

            data["temperature"] = serialized(
                String(nodes[i].current_data.current_temperature, 1));
            data["humidity"] =
                serialized(String(nodes[i].current_data.current_humidity, 1));

            // if (!nodes[i].current_data.is_dht20_ok) {
            //      data["error"] = "DHT_DISCONNECTED";
            // }
        }
    }

    String payload = "{}";
    if (has_online_node) {
        serializeJson(doc, payload);
    }
    return payload;
}

void coreIotTask(void* pvParematers) {
    LOG_INFO("IOT", "Core IoT task started");
    String client_id =
        "ColdChanin_GW_" + String((uint32_t)ESP.getEfuseMac(), HEX);

    while (1) {
        GatewayConfig config = getGatewayConfig();
        GatewayState gw_state = getGatewayState();
        if (checkGatewayErrorFlag(GW_FLAG_WIFI_DISCONN)) {
            LOG_WARN("IOT", "Wifi disconnected, cannot publish data");
            gw_state.is_wifi_connected = false;
            xSemaphoreGive(wifi_error_semaphore);

            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }
        gw_state.is_wifi_connected = true;

        mqtt_client.setServer(config.core_iot_server, config.core_iot_port);

        if (!mqtt_client.connected()) {
            LOG_INFO("IOT", "Connecting to MQTT broker...");
            if (mqtt_client.connect(client_id.c_str(), config.core_iot_token,
                                    NULL)) {
                LOG_INFO("IOT", "Connected MQTT");
                clearGatewayErrorFlag(GW_FLAG_COREIOT_DISCONN);
                gw_state.is_coreiot_connected = true;
            } else {
                if (!checkGatewayErrorFlag(GW_FLAG_COREIOT_DISCONN)) {
                    LOG_ERR("IOT", "MQTT connected fail RC=%d",
                            mqtt_client.state());
                    setGatewayErrorFlag(GW_FLAG_COREIOT_DISCONN);
                    gw_state.is_coreiot_connected = false;
                }
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
        }

        mqtt_client.loop();
        TickType_t wait_time = pdMS_TO_TICKS(config.send_interval_ms);
        bool is_urgen_event =
            (xSemaphoreTake(coreiot_error_semaphore, wait_time) == pdTRUE);
        String pay_load = buildGatewayPayload();

        if (pay_load == "{}") {
            continue;
        }
        const char* topic_telemetry = "v1/gateway/telemetry";

        if (mqtt_client.publish(topic_telemetry, pay_load.c_str())) {
            if (is_urgen_event) {
                LOG_WARN("IOT", "Published urgent event data: %s",
                         pay_load.c_str());
            } else {
                LOG_INFO("IOT", "Published data: %s", pay_load.c_str());
            }
            clearGatewayErrorFlag(GW_FLAG_COREIOT_DISCONN);
        } else {
            LOG_ERR("IOT", "Failed to publish data");
            setGatewayErrorFlag(GW_FLAG_COREIOT_DISCONN);
            gw_state.is_coreiot_connected = false;
        }
        setGatewayState(gw_state);
    }
}