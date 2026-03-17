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
        SensorData s_data = getSensorData();
        SystemConfig s_config = getSystemConfig();
        MlState m_state = getMlState();

        StaticJsonDocument<JSON_DOC_SIZE> doc;

        doc[JSON_TEMP] = s_data.current_temperature;
        doc[JSON_HUM] = s_data.current_humidity;
        doc[JSON_DHT_STATUS] = s_data.is_dht20_ok ? "OK" : "ERROR";
        doc[JSON_LCD_STATUS] = s_data.is_lcd_ok ? "OK" : "ERROR";
        doc[JSON_MIN_TEMP] = s_config.min_temp_threshold;
        doc[JSON_MAX_TEMP] = s_config.max_temp_threshold;
        doc[JSON_MIN_HUM] = s_config.min_humidity_threshold;
        doc[JSON_MAX_HUM] = s_config.max_humidity_threshold;
        doc[JSON_ML_PREDICTION] = m_state.prediction;
        doc[JSON_ML_CONFIDENCE] = m_state.confidence * 100.0;

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

        doc[JSON_DEVICE1] = c_state.is_device1_on ? "ON" : "OFF";
        doc[JSON_DEVICE2] = c_state.is_device2_on ? "ON" : "OFF";

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

        int device_id = doc[JSON_DEVICE];
        String state_cmd = doc[JSON_STATE];
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

        doc[JSON_AP_SSID] = config.ap_ssid;
        doc[JSON_AP_PASS] = config.ap_password;
        doc[JSON_WIFI_SSID] = config.wifi_ssid;
        doc[JSON_WIFI_PASS] = config.wifi_password;
        doc[JSON_SERVER] = config.core_iot_server;
        doc[JSON_PORT] = config.core_iot_port;
        doc[JSON_TOKEN] = config.core_iot_token;
        doc[JSON_SEND_INTERVAL] = config.send_interval_ms;
        doc[JSON_READ_INTERVAL] = config.read_interval_ms;
        doc[JSON_MIN_TEMP] = config.min_temp_threshold;
        doc[JSON_MAX_TEMP] = config.max_temp_threshold;
        doc[JSON_MIN_HUM] = config.min_humidity_threshold;
        doc[JSON_MAX_HUM] = config.max_humidity_threshold;

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
        if (doc.containsKey(JSON_AP_SSID)) {
            String ssid = doc[JSON_AP_SSID].as<String>();
            strlcpy(config.ap_ssid, ssid.c_str(), MAX_SSID_LEN);
        }
        if (doc.containsKey(JSON_AP_PASS)) {
            String pass = doc[JSON_AP_PASS].as<String>();
            strlcpy(config.ap_password, pass.c_str(), MAX_PASS_LEN);
        }

        LOG_INFO("WEBSERVER", "New AP SSID: %s, Password: %s", config.ap_ssid,
                 config.ap_password);
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
        if (doc.containsKey(JSON_WIFI_SSID)) {
            String ssid = doc[JSON_WIFI_SSID].as<String>();
            strlcpy(config.wifi_ssid, ssid.c_str(), MAX_SSID_LEN);
        }
        if (doc.containsKey(JSON_WIFI_PASS)) {
            String pass = doc[JSON_WIFI_PASS].as<String>();
            strlcpy(config.wifi_password, pass.c_str(), MAX_PASS_LEN);
        }

        setSystemConfig(config);
        saveConfigToFlash();
        LOG_INFO("WEBSERVER", "WiFi Settings updated");
        server.send(
            200, "application/json",
            "{\"status\":\"success\", \"message\":\"WiFi Settings Saved!\"}");
    });

    // Core IoT settings
    server.on("/api/settings/cloud", HTTP_POST, []() {
        if (!server.hasArg("plain"))
            return server.send(400, "text/plain", "Missing JSON");

        StaticJsonDocument<JSON_DOC_SIZE> doc;
        deserializeJson(doc, server.arg("plain"));

        SystemConfig config = getSystemConfig();

        if (doc.containsKey(JSON_SERVER)) {
            String server_url = doc[JSON_SERVER].as<String>();
            strlcpy(config.core_iot_server, server_url.c_str(), MAX_SERVER_LEN);
        }
        if (doc.containsKey(JSON_PORT)) config.core_iot_port = doc[JSON_PORT];
        if (doc.containsKey(JSON_TOKEN)) {
            String token = doc[JSON_TOKEN].as<String>();
            strlcpy(config.core_iot_token, token.c_str(), MAX_TOKEN_LEN);
        }
        if (doc.containsKey(JSON_SEND_INTERVAL))
            config.send_interval_ms = doc[JSON_SEND_INTERVAL];

        setSystemConfig(config);
        saveConfigToFlash();
        LOG_INFO("WEBSERVER", "Cloud settings updated");
        server.send(200, "application/json",
                    "{\"status\":\"success\", \"message\":\"Cloud settings "
                    "updated!\"}");
    });

    // Sensor Thresholds API
    server.on("/api/settings/sensors", HTTP_POST, []() {
        if (!server.hasArg("plain"))
            return server.send(400, "text/plain", "Missing JSON");

        StaticJsonDocument<JSON_DOC_SIZE> doc;
        deserializeJson(doc, server.arg("plain"));

        SystemConfig config = getSystemConfig();

        if (doc.containsKey(JSON_READ_INTERVAL))
            config.read_interval_ms = doc[JSON_READ_INTERVAL];
        if (doc.containsKey(JSON_MIN_TEMP))
            config.min_temp_threshold = doc[JSON_MIN_TEMP];
        if (doc.containsKey(JSON_MAX_TEMP))
            config.max_temp_threshold = doc[JSON_MAX_TEMP];
        if (doc.containsKey(JSON_MIN_HUM))
            config.min_humidity_threshold = doc[JSON_MIN_HUM];
        if (doc.containsKey(JSON_MAX_HUM))
            config.max_humidity_threshold = doc[JSON_MAX_HUM];

        setSystemConfig(config);
        saveConfigToFlash();
        LOG_INFO("WEBSERVER", "Sensor thresholds updated");
        server.send(200, "application/json",
                    "{\"status\":\"success\", \"message\":\"Sensor thresholds "
                    "updated!\"}");
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