#include "web_server.h"

// MACROS & CONSTANTS
#define JSON_DOC_SIZE 512

static WebServer server(HTTP_PORT);

// INTERNAL HELPER FUNCTIONS PROTOTYPES
static void setupStaticFiles();
static void setupDashboardApi();
static void setupControlApi();
static void setupSettingsApi();
static void setupNodeApi();
static void setupModeApi();
static void initMockData();

static bool isValidMacAddress(const char* mac) {
    if (mac == nullptr || strlen(mac) != 17) return false;

    for (int i = 0; i < 17; i++) {
        if (i % 3 == 2) {
            if (mac[i] != ':' && mac[i] != '-') return false;
        } else {
            if (!isxdigit(mac[i])) return false;
        }
    }
    return true;
}

// Main Web Server Task
void webServerTask(void* pvParameters) {
    if (!LittleFS.begin(true)) {
        LOG_ERR(
            "WEBSERVER",
            "Failed to mount LittleFS. Did you upload the filesystem image?");
    } else {
        LOG_INFO("WEBSERVER", "LittleFS mounted successfully.");
    }

    // initMockData();
    // Register all routes and API endpoints
    setupStaticFiles();
    setupDashboardApi();
    setupControlApi();
    setupSettingsApi();
    setupNodeApi();
    setupModeApi();

    // Start Web Server
    server.begin();
    LOG_INFO("WEBSERVER", "Started listening on port %d", HTTP_PORT);

    while (1) {
        server.handleClient();
        vTaskDelay(pdMS_TO_TICKS(3));
    }
}

// API IMPLEMENTATIONS
// Module: Static File Serving & SPA Routing
static void setupStaticFiles() {
    server.on("/", HTTP_GET, []() {
        File file = LittleFS.open("/index.html", "r");
        if (!file) {
            server.send(404, "text/plain",
                        "Error: index.html not found in SPIFFS/LittleFS.");
            return;
        }
        server.streamFile(file, "text/html");
        file.close();
    });

    server.on("/favicon.ico", HTTP_GET, []() { server.send(204); });

    auto captivePortalRedirect = []() {
        String redirectUrl = "http://" + WiFi.softAPIP().toString() + "/";
        server.sendHeader("Location", redirectUrl, true);
        server.send(302, "text/plain", "");
    };
    // Block OS health check URLs to prevent captive portal bypass
    server.on("/generate204", HTTP_GET, captivePortalRedirect);
    server.on("/generate_204", HTTP_GET, captivePortalRedirect);
    server.on("/chat", HTTP_GET, captivePortalRedirect);
    server.on("/gen_204", HTTP_GET, captivePortalRedirect);
    server.on("/getHttpDnsServerList", HTTP_GET, captivePortalRedirect);
    server.on("/getDNList", HTTP_GET, captivePortalRedirect);
    server.on("/hotspot-detect.html", HTTP_GET,
              captivePortalRedirect);  // iOS / macOS
    server.on("/library/test/success.html", HTTP_GET,
              captivePortalRedirect);                         // iOS
    server.on("/ncsi.txt", HTTP_GET, captivePortalRedirect);  // Windows
    server.on("/cname.aspx", HTTP_GET, captivePortalRedirect);
    server.on("/connecttest.txt", HTTP_GET, captivePortalRedirect);

    // Redirect all unknown requests to the home page
    server.onNotFound([captivePortalRedirect]() {
        if (server.method() == HTTP_OPTIONS) {
            server.send(200);  // Handle CORS preflight requests
        } else {
            LOG_WARN("WEBSERVER", "Unknown URI requested: %s (%s)",
                     server.uri().c_str(),
                     server.method() == HTTP_GET ? "GET" : "OTHER");
            captivePortalRedirect();
        }
    });
}

// Module: Dashboard Sensor Data
static void setupDashboardApi() {
    server.on("/api/data", HTTP_GET, []() {
        GatewayState gw_state = getGatewayState();
        DynamicJsonDocument doc(8192);

        JsonObject gw_obj = doc.createNestedObject("gateway");
        gw_obj["status"] =
            gw_state.is_coreiot_connected
                ? "Online"
                : (gw_state.is_wifi_connected ? "Online (No MQTT)" : "Offline");
        gw_obj["espnow"] = "Active";

        JsonArray nodes_arr = doc.createNestedArray("nodes");

        PairedNode snapshot_nodes[MAX_PAIRED_NODES];
        uint8_t count =
            getPairedNodesSnapshot(snapshot_nodes, MAX_PAIRED_NODES);

        for (uint8_t i = 0; i < count; i++) {
            JsonObject n = nodes_arr.createNestedObject();
            n["mac"] = snapshot_nodes[i].mac_address;
            n["name"] = snapshot_nodes[i].node_name;
            n["is_online"] = snapshot_nodes[i].is_online;
            n["temp"] =
                String(snapshot_nodes[i].current_data.current_temperature, 1);
            n["hum"] =
                String(snapshot_nodes[i].current_data.current_humidity, 1);
            n["prediction"] = snapshot_nodes[i].current_ml_state.prediction;

            JsonObject periph = n.createNestedObject("periph");
            periph["dht"] =
                snapshot_nodes[i].current_data.is_dht20_ok ? "OK" : "ERR";
            periph["lcd"] =
                snapshot_nodes[i].current_data.is_lcd_ok ? "OK" : "ERR";

            JsonObject conf = n.createNestedObject("config");
            conf["t_min"] = snapshot_nodes[i].current_config.min_temp_threshold;
            conf["t_max"] = snapshot_nodes[i].current_config.max_temp_threshold;
            conf["h_min"] =
                snapshot_nodes[i].current_config.min_humidity_threshold;
            conf["h_max"] =
                snapshot_nodes[i].current_config.max_humidity_threshold;
        }

        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });
}

// Module: Relay Controls
static void setupControlApi() {
    server.on("/api/control", HTTP_GET, []() {
        ControlState c_state = getControlState();
        StaticJsonDocument<256> doc;

        doc["device1"] = c_state.is_device1_on ? "ON" : "OFF";
        doc["device2"] = c_state.is_device2_on ? "ON" : "OFF";

        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });

    server.on("/api/control", HTTP_POST, []() {
        if (!server.hasArg("plain"))
            return server.send(400, "text/plain", "Missing JSON body");

        StaticJsonDocument<256> doc;
        if (deserializeJson(doc, server.arg("plain")))
            return server.send(400, "text/plain", "Invalid JSON format");

        int device_id = doc[JSON_DEVICE];
        String state_cmd = doc[JSON_STATE];
        String message = "Invalid device";

        ControlState c_state = getControlState();
        if (device_id == 1) {
            c_state.is_device1_on = (state_cmd == "ON");
            setControlState(c_state);
            message = "Buzzer turned " + state_cmd;
            LOG_INFO("WEBSERVER", "Device 1 set to %s", state_cmd.c_str());
        } else if (device_id == 2) {
            c_state.is_device2_on = (state_cmd == "ON");
            setControlState(c_state);
            message = "Fan turned " + state_cmd;
            LOG_INFO("WEBSERVER", "Device 2 set to %s", state_cmd.c_str());
        } else {
            return server.send(400, "application/json",
                               "{\"status\":\"error\"}");
        }

        server.send(
            200, "application/json",
            "{\"status\":\"success\", \"message\":\"" + message + "\"}");
    });
}
// Module: System Configurations
static void setupSettingsApi() {
    // Gateway settings (Network, Cloud)
    server.on("/api/settings", HTTP_GET, []() {
        GatewayConfig config = getGatewayConfig();
        StaticJsonDocument<512> doc;

        doc[JSON_AP_SSID] = config.ap_ssid;
        doc[JSON_AP_PASS] = config.ap_password;
        doc[JSON_WIFI_SSID] = config.wifi_ssid;
        doc[JSON_WIFI_PASS] = config.wifi_password;
        doc[JSON_SERVER] = config.core_iot_server;
        doc[JSON_PORT] = config.core_iot_port;
        doc[JSON_TOKEN] = config.core_iot_token;
        doc[JSON_SEND_INTERVAL] = config.send_interval_ms;

        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });

    server.on("/api/settings/ap", HTTP_POST, []() {
        if (!server.hasArg("plain"))
            return server.send(400, "text/plain", "Missing JSON");
        StaticJsonDocument<256> doc;
        deserializeJson(doc, server.arg("plain"));

        GatewayConfig config = getGatewayConfig();
        if (doc.containsKey(JSON_AP_SSID))
            strlcpy(config.ap_ssid, doc[JSON_AP_SSID].as<const char*>(),
                    MAX_SSID_LEN);
        if (doc.containsKey(JSON_AP_PASS))
            strlcpy(config.ap_password, doc[JSON_AP_PASS].as<const char*>(),
                    MAX_PASS_LEN);

        setGatewayConfig(config);
        saveConfigToFlash();
        LOG_INFO("WEBSERVER", "AP Settings updated");
        server.send(200, "application/json", "{\"status\":\"success\"}");
    });

    server.on("/api/settings/wifi", HTTP_POST, []() {
        if (!server.hasArg("plain"))
            return server.send(400, "text/plain", "Missing JSON");
        StaticJsonDocument<256> doc;
        deserializeJson(doc, server.arg("plain"));

        GatewayConfig config = getGatewayConfig();
        if (doc.containsKey(JSON_WIFI_SSID))
            strlcpy(config.wifi_ssid, doc[JSON_WIFI_SSID].as<String>().c_str(),
                    MAX_SSID_LEN);
        if (doc.containsKey(JSON_WIFI_PASS))
            strlcpy(config.wifi_password,
                    doc[JSON_WIFI_PASS].as<String>().c_str(), MAX_PASS_LEN);

        setGatewayConfig(config);
        saveConfigToFlash();
        LOG_INFO("WEBSERVER", "WiFi Settings updated");
        server.send(200, "application/json", "{\"status\":\"success\"}");
    });

    server.on("/api/settings/cloud", HTTP_POST, []() {
        if (!server.hasArg("plain"))
            return server.send(400, "text/plain", "Missing JSON");
        StaticJsonDocument<512> doc;
        deserializeJson(doc, server.arg("plain"));

        GatewayConfig config = getGatewayConfig();
        if (doc.containsKey(JSON_SERVER))
            strlcpy(config.core_iot_server,
                    doc[JSON_SERVER].as<String>().c_str(), MAX_SERVER_LEN);
        if (doc.containsKey(JSON_PORT)) config.core_iot_port = doc[JSON_PORT];
        if (doc.containsKey(JSON_TOKEN))
            strlcpy(config.core_iot_token, doc[JSON_TOKEN].as<String>().c_str(),
                    MAX_TOKEN_LEN);
        if (doc.containsKey(JSON_SEND_INTERVAL))
            config.send_interval_ms = doc[JSON_SEND_INTERVAL];

        setGatewayConfig(config);
        saveConfigToFlash();
        LOG_INFO("WEBSERVER", "Cloud Settings updated");
        server.send(200, "application/json", "{\"status\":\"success\"}");
    });
}

static void setupNodeApi() {
    // Get specific node config
    server.on("/api/nodes/config", HTTP_GET, []() {
        if (!server.hasArg("mac"))
            return server.send(400, "application/json",
                               "{\"error\":\"Missing MAC\"}");

        String mac = server.arg("mac");
        PairedNode target_node;

        if (!getNodeByMac(mac.c_str(), &target_node)) {
            return server.send(404, "application/json",
                               "{\"unconfigured\":true}");
        }

        StaticJsonDocument<256> doc;
        doc[JSON_READ_INTERVAL] = target_node.current_config.read_interval_ms;
        doc[JSON_MIN_TEMP] = target_node.current_config.min_temp_threshold;
        doc[JSON_MAX_TEMP] = target_node.current_config.max_temp_threshold;
        doc[JSON_MIN_HUM] = target_node.current_config.min_humidity_threshold;
        doc[JSON_MAX_HUM] = target_node.current_config.max_humidity_threshold;

        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });

    // Specific Node Config Update
    server.on("/api/settings/sensors", HTTP_POST, []() {
        if (!server.hasArg("plain"))
            return server.send(400, "text/plain", "Missing JSON");

        StaticJsonDocument<512> doc;
        if (deserializeJson(doc, server.arg("plain"))) {
            return server.send(400, "application/json",
                               "{\"error\":\"Invalid JSON\"}");
        }

        if (!doc.containsKey("target_mac"))
            return server.send(400, "application/json",
                               "{\"error\":\"Missing target_mac\"}");

        const char* mac = doc["target_mac"];
        PairedNode target_node;

        if (!getNodeByMac(mac, &target_node)) {
            return server.send(404, "application/json",
                               "{\"error\":\"Node not found\"}");
        }

        SensorConfig updated_config = target_node.current_config;

        if (doc.containsKey(JSON_READ_INTERVAL))
            updated_config.read_interval_ms = doc[JSON_READ_INTERVAL];
        if (doc.containsKey(JSON_MIN_TEMP))
            updated_config.min_temp_threshold = doc[JSON_MIN_TEMP];
        if (doc.containsKey(JSON_MAX_TEMP))
            updated_config.max_temp_threshold = doc[JSON_MAX_TEMP];
        if (doc.containsKey(JSON_MIN_HUM))
            updated_config.min_humidity_threshold = doc[JSON_MIN_HUM];
        if (doc.containsKey(JSON_MAX_HUM))
            updated_config.max_humidity_threshold = doc[JSON_MAX_HUM];

        // Update local config
        if (updateNodeConfig(mac, updated_config)) {
            saveConfigToFlash();
            LOG_INFO("WEBSERVER", "Updated config for Node %s", mac);

            // Send Sync Config command via ESP-NOW to the specific Node
            GwDownlinkMessage cmd_msg;
            cmd_msg.type = DOWNLINK_SYNC_CONFIG;
            strlcpy(cmd_msg.target_mac, mac, MAC_STR_LEN);
            if (xQueueSend(gw_downlink_queue, &cmd_msg, pdMS_TO_TICKS(100)) !=
                pdPASS) {
                LOG_ERR("WEBSERVER", "Queue fulled, command is rejected!");
            }

            server.send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            server.send(500, "application/json",
                        "{\"error\":\"Failed to update config\"}");
        }
    });

    // Pairing
    server.on("/api/nodes/pair", HTTP_POST, []() {
        if (!server.hasArg("plain"))
            return server.send(400, "text/plain", "Missing JSON");

        StaticJsonDocument<256> doc;
        deserializeJson(doc, server.arg("plain"));

        const char* mac = doc["mac"];
        const char* name = doc["name"];

        if (!isValidMacAddress(mac)) {
            LOG_ERR("WEBSERVER", "Invalid MAC Address format rejected: %s",
                    mac ? mac : "null");
            return server.send(400, "application/json",
                               "{\"error\":\"Invalid MAC Address format\"}");
        }

        if (name == nullptr || strlen(name) == 0 || strlen(name) > 31) {
            LOG_ERR("WEBSERVER", "Invalid Node Name rejected.");
            return server.send(400, "application/json",
                               "{\"error\":\"Invalid Room Name\"}");
        }

        // Add to paired list
        if (addPairedNode(mac, name)) {
            saveConfigToFlash();
            LOG_INFO("WEBSERVER", "Paired new Node: %s [%s]", name, mac);

            // Send Pairing command via ESP-NOW to the new Node
            GwDownlinkMessage cmd_msg;
            cmd_msg.type = DOWNLINK_PAIRING;
            strlcpy(cmd_msg.target_mac, mac, MAC_STR_LEN);
            strlcpy(cmd_msg.room_name, name, 32);
            if (xQueueSend(gw_downlink_queue, &cmd_msg, pdMS_TO_TICKS(100)) !=
                pdPASS) {
                LOG_ERR("WEBSERVER", "Queue fulled, command is rejected!");
            }

            server.send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            server.send(
                400, "application/json",
                "{\"error\":\"Failed to add node (List Full or Exists)\"}");
        }
    });

    // Unpairing
    server.on("/api/nodes/unpair", HTTP_POST, []() {
        if (!server.hasArg("plain"))
            return server.send(400, "text/plain", "Missing JSON");
        StaticJsonDocument<128> doc;
        deserializeJson(doc, server.arg("plain"));

        const char* mac = doc["mac"];

        // Delete from paired list
        if (removePairedNode(mac)) {
            saveConfigToFlash();
            LOG_INFO("WEBSERVER", "Unpaired Node: %s", mac);

            // Send Unpairing command via ESP-NOW to the Node
            GwDownlinkMessage cmd_msg;
            cmd_msg.type = DOWNLINK_UNPAIR;
            strlcpy(cmd_msg.target_mac, mac, MAC_STR_LEN);
            if (xQueueSend(gw_downlink_queue, &cmd_msg, pdMS_TO_TICKS(100)) !=
                pdPASS) {
                LOG_ERR("WEBSERVER", "Queue fulled, command is rejected!");
            }

            server.send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            server.send(404, "application/json",
                        "{\"error\":\"Node not found\"}");
        }
    });

    // Hardware Reset Command
    server.on("/api/nodes/reset", HTTP_POST, []() {
        if (!server.hasArg("plain"))
            return server.send(400, "text/plain", "Missing JSON");
        StaticJsonDocument<128> doc;
        deserializeJson(doc, server.arg("plain"));

        const char* mac = doc["mac"];

        // Create and send Reset command via ESP-NOW to the Node
        GwDownlinkMessage cmd_msg;
        cmd_msg.type = DOWNLINK_CONTROL_CMD;
        strlcpy(cmd_msg.target_mac, mac, MAC_STR_LEN);
        cmd_msg.cmd_code = 0xAA;  // CMD_HARD_RESET
        cmd_msg.cmd_param = 0;

        // Use non-blocking send to avoid potential deadlocks if the queue is
        // full
        if (xQueueSend(gw_downlink_queue, &cmd_msg, pdMS_TO_TICKS(100)) ==
            pdPASS) {
            LOG_INFO("WEBSERVER", "Queued RESET command for Node: %s", mac);
            server.send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            LOG_ERR("WEBSERVER", "Queue full! Drop RESET command.");
            server.send(503, "application/json", "{\"status\":\"busy\"}");
        }
    });
}

// Module: Network Mode Switching
static void setupModeApi() {
    server.on("/api/mode/wifi", HTTP_POST, []() {
        LOG_INFO("WEBSERVER",
                 "Triggered Switch to WiFi Mode via Web Dashboard");
        // Signal Network Manager task using FreeRTOS Semaphore
        xSemaphoreGive(switch_to_sta_semaphore);
        server.send(200, "application/json",
                    "{\"status\":\"switching\", \"message\":\"Switching to "
                    "WiFi Mode...\"}");
    });
}

static void initMockData() {
    addPairedNode("AA:BB:CC:DD:EE:11", "Cold Room 01");
    addPairedNode("AA:BB:CC:DD:EE:22", "Cold Room 02");

    SensorData d1 = {4.5, 85.0, true, true};
    MlState ml1 = {"Normal", 98.5};
    updateNodeDataAndMl("AA:BB:CC:DD:EE:11", d1, ml1);

    SensorData d2 = {12.0, 70.0, true, false};
    MlState ml2 = {"Anomaly", 85.2};
    updateNodeDataAndMl("AA:BB:CC:DD:EE:22", d2, ml2);
}