#include "web_server.h"

// MACROS & CONSTANTS
#define JSON_DOC_SIZE 512
#define HTTP_PORT 80

static WebServer server(HTTP_PORT);

// INTERNAL HELPER FUNCTIONS PROTOTYPES
static void setupStaticFiles();
static void setupDashboardApi();
static void setupControlApi();
static void setupSettingsApi();
static void setupModeApi();

// Main Web Server Task
void webServerTask(void* pvParameters) {
    if (!LittleFS.begin(true)) {
        LOG_ERR(
            "WEBSERVER",
            "Failed to mount LittleFS. Did you upload the filesystem image?");
    } else {
        LOG_INFO("WEBSERVER", "LittleFS mounted successfully.");
    }

    // Register all routes and API endpoints
    setupStaticFiles();
    setupDashboardApi();
    setupControlApi();
    setupSettingsApi();
    setupModeApi();

    // Start Web Server
    server.begin();
    LOG_INFO("WEBSERVER", "Started listening on port %d", HTTP_PORT);

    while (1) {
        server.handleClient();
        vTaskDelay(pdMS_TO_TICKS(20));
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
    server.on("/generate204", HTTP_GET, captivePortalRedirect);   // Android
    server.on("/generate_204", HTTP_GET, captivePortalRedirect);  // Android
    server.on("/chat", HTTP_GET, captivePortalRedirect);          // Android
    server.on("/gen_204", HTTP_GET, captivePortalRedirect);       // Android
    server.on("/hotspot-detect.html", HTTP_GET,
              captivePortalRedirect);  // iOS / macOS
    server.on("/library/test/success.html", HTTP_GET,
              captivePortalRedirect);                                // iOS
    server.on("/ncsi.txt", HTTP_GET, captivePortalRedirect);         // Windows
    server.on("/cname.aspx", HTTP_GET, captivePortalRedirect);       // Windows
    server.on("/connecttest.txt", HTTP_GET, captivePortalRedirect);  // Windows

    // Redirect all unknown requests to the home page
    server.onNotFound([captivePortalRedirect]() {
        if (server.method() == HTTP_OPTIONS) {
            server.send(200);  // Handle CORS preflight requests
        } else {
            LOG_WARN("WEBSERVER", "Unknown URI requested: %s",
                     server.uri().c_str());
            captivePortalRedirect();
        }
    });
}

// Module: Dashboard Sensor Data
static void setupDashboardApi() {
    server.on("/api/data", HTTP_GET, []() {
        SensorData s_data = getSensorData();
        SystemConfig s_config = getSystemConfig();

        StaticJsonDocument<JSON_DOC_SIZE> doc;

        doc["temperature"] = s_data.current_temperature;
        doc["humidity"] = s_data.current_humidity;
        doc["dht_status"] = s_data.is_dht20_ok ? "OK" : "ERROR";
        doc["lcd_status"] = s_data.is_lcd_ok ? "OK" : "ERROR";
        doc["min_temp_threshold"] = s_config.min_temp_threshold;
        doc["max_temp_threshold"] = s_config.max_temp_threshold;
        doc["min_humidity_threshold"] = s_config.min_humidity_threshold;
        doc["max_humidity_threshold"] = s_config.max_humidity_threshold;
        // TODO: Update ML prediction based on Task 5 results
        doc["ml_prediction"] = "Normal";

        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });
}

// Module: Relay Controls
static void setupControlApi() {
    // Read current device states
    server.on("/api/control", HTTP_GET, []() {
        ControlState c_state = getControlState();
        StaticJsonDocument<JSON_DOC_SIZE> doc;

        doc["device1"] = c_state.is_device1_on ? "ON" : "OFF";
        doc["device2"] = c_state.is_device2_on ? "ON" : "OFF";

        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });

    // Update device states
    server.on("/api/control", HTTP_POST, []() {
        if (!server.hasArg("plain")) {
            return server.send(400, "text/plain", "Missing JSON body");
        }

        StaticJsonDocument<JSON_DOC_SIZE> doc;
        if (deserializeJson(doc, server.arg("plain"))) {
            return server.send(400, "text/plain", "Invalid JSON format");
        }

        int device_id = doc["device"];
        String state_cmd = doc["state"];
        String message = "Invalid device or state command";

        ControlState c_state = getControlState();
        if (device_id == 1) {
            c_state.is_device1_on = (state_cmd == "ON");
            setControlState(c_state);
            message = "Buzzer turned " + state_cmd;
        } else {
            server.send(
                400, "application/json",
                "{\"status\":\"error\", \"message\":\"" + message + "\"}");
            LOG_WARN("WEBSERVER", "Invalid device ID in control API: %d",
                     device_id);
            return;
        }
        LOG_INFO("WEBSERVER", "Device %d set to %s via Web", device_id,
                 state_cmd.c_str());

        server.send(
            200, "application/json",
            "{\"status\":\"success\", \"message\":\"" + message + "\"}");
    });
}

// Module: System Configurations
static void setupSettingsApi() {
    // Get all current settings
    server.on("/api/settings", HTTP_GET, []() {
        SystemConfig config = getSystemConfig();
        StaticJsonDocument<JSON_DOC_SIZE> doc;

        doc["ap_ssid"] = config.ap_ssid;
        doc["ap_password"] = config.ap_password;
        doc["wifi_ssid"] = config.wifi_ssid;
        doc["wifi_password"] = config.wifi_password;
        doc["core_iot_token"] = config.core_iot_token;
        doc["read_interval_ms"] = config.read_interval_ms;
        doc["min_temp_threshold"] = config.min_temp_threshold;
        doc["max_temp_threshold"] = config.max_temp_threshold;
        doc["min_humidity_threshold"] = config.min_humidity_threshold;
        doc["max_humidity_threshold"] = config.max_humidity_threshold;

        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });

    // Access Point settings
    server.on("/api/settings/ap", HTTP_POST, []() {
        if (!server.hasArg("plain"))
            return server.send(400, "text/plain", "Missing JSON");

        StaticJsonDocument<JSON_DOC_SIZE> doc;
        deserializeJson(doc, server.arg("plain"));

        SystemConfig config = getSystemConfig();
        if (doc.containsKey("ap_ssid"))
            strlcpy(config.ap_ssid, doc["ap_ssid"] | "", MAX_SSID_LEN);
        if (doc.containsKey("ap_password"))
            strlcpy(config.ap_password, doc["ap_password"] | "", MAX_PASS_LEN);

        setSystemConfig(config);
        saveConfigToFlash();
        LOG_INFO("WEBSERVER", "AP Settings updated");
        server.send(
            200, "application/json",
            "{\"status\":\"success\", \"message\":\"AP Settings Saved!\"}");
    });

    // WiFi Station settings
    server.on("/api/settings/wifi", HTTP_POST, []() {
        if (!server.hasArg("plain"))
            return server.send(400, "text/plain", "Missing JSON");

        StaticJsonDocument<JSON_DOC_SIZE> doc;
        deserializeJson(doc, server.arg("plain"));

        SystemConfig config = getSystemConfig();
        if (doc.containsKey("wifi_ssid"))
            strlcpy(config.wifi_ssid, doc["wifi_ssid"] | "", MAX_SSID_LEN);
        if (doc.containsKey("wifi_password"))
            strlcpy(config.wifi_password, doc["wifi_password"] | "",
                    MAX_PASS_LEN);

        setSystemConfig(config);
        saveConfigToFlash();
        LOG_INFO("WEBSERVER", "WiFi Settings updated");
        server.send(
            200, "application/json",
            "{\"status\":\"success\", \"message\":\"WiFi Settings Saved!\"}");
    });

    // Cloud & Sensor thresholds
    server.on("/api/settings/cloud", HTTP_POST, []() {
        if (!server.hasArg("plain"))
            return server.send(400, "text/plain", "Missing JSON");

        StaticJsonDocument<JSON_DOC_SIZE> doc;
        deserializeJson(doc, server.arg("plain"));

        SystemConfig config = getSystemConfig();
        if (doc.containsKey("core_iot_token"))
            strlcpy(config.core_iot_token, doc["core_iot_token"] | "",
                    MAX_TOKEN_LEN);
        if (doc.containsKey("read_interval_ms"))
            config.read_interval_ms = doc["read_interval_ms"];
        if (doc.containsKey("min_temp_threshold"))
            config.min_temp_threshold = doc["min_temp_threshold"];
        if (doc.containsKey("max_temp_threshold"))
            config.max_temp_threshold = doc["max_temp_threshold"];
        if (doc.containsKey("min_humidity_threshold"))
            config.min_humidity_threshold = doc["min_humidity_threshold"];
        if (doc.containsKey("max_humidity_threshold"))
            config.max_humidity_threshold = doc["max_humidity_threshold"];

        setSystemConfig(config);
        saveConfigToFlash();
        LOG_INFO("WEBSERVER", "Cloud & Sensor thresholds updated");
        server.send(200, "application/json",
                    "{\"status\":\"success\", \"message\":\"Cloud & Sensor "
                    "thresholds updated!\"}");
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