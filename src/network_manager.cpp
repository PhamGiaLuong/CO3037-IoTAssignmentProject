#include "network_manager.h"

// MACROS & CONSTANTS
#define BOOT_BUTTON_PIN 0
#define WIFI_TIMEOUT_MS 10000
#define DNS_PORT 53

static DNSServer dns_server;

// State Machine for Network Control Flow
typedef enum {
    STATE_INIT,
    STATE_STA_CONNECTING,
    STATE_STA_CONNECTED,
    STATE_AP_MODE
} NetworkState;

static NetworkState current_state = STATE_INIT;

void startStaMode(char* ssid = nullptr, char* password = nullptr) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    LOG_INFO("NETWORK", "Starting STA Mode. Connecting to %s", ssid);
}

void startApMode() {
    SystemConfig config = getSystemConfig();
    WiFi.mode(WIFI_AP);
    WiFi.softAP(config.ap_ssid, config.ap_password);

    // Start DNS Server for Captive Portal (Auto-redirect)
    dns_server.start(DNS_PORT, "*", WiFi.softAPIP());

    // Update System State
    SystemState state = getSystemState();
    state.is_ap_mode = true;
    state.is_wifi_connected = false;
    setSystemState(state);

    setSystemErrorFlag(EVENT_NET_AP_MODE);

    LOG_INFO("NETWORK", "Starting AP Mode. SSID: %s, IP: %s", config.ap_ssid,
             WiFi.softAPIP().toString().c_str());
}

void networkTask(void* pvParameters) {
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
    uint32_t start_connect_time = 0;

    while (1) {
        // Handle physical Boot Button Press -> Force AP Mode
        if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
            vTaskDelay(pdMS_TO_TICKS(50));  // Debounce button
            if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
                xSemaphoreGive(switch_to_ap_semaphore);
                // Wait until button is released
                while (digitalRead(BOOT_BUTTON_PIN) == LOW) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }
        }

        // Network State Machine
        switch (current_state) {
            case STATE_INIT: {
                SystemConfig config = getSystemConfig();
                // Check if WiFi config is saved (Length > 0)
                if (strlen(config.wifi_ssid) > 0) {
                    current_state = STATE_STA_CONNECTING;
                    startStaMode();
                    start_connect_time = millis();
                } else {
                    current_state = STATE_AP_MODE;
                    startApMode();
                }
                break;
            }
            case STATE_STA_CONNECTING: {
                if (WiFi.status() == WL_CONNECTED) {
                    current_state = STATE_STA_CONNECTED;

                    SystemState state = getSystemState();
                    state.is_wifi_connected = true;
                    state.is_ap_mode = false;
                    setSystemState(state);

                    clearSystemErrorFlag(EVENT_NET_AP_MODE);
                    clearSystemErrorFlag(EVENT_WIFI_DISCONN);
                    LOG_INFO("NETWORK", "STA Connected! IP: %s",
                             WiFi.localIP().toString().c_str());
                } else if (millis() - start_connect_time > WIFI_TIMEOUT_MS) {
                    // Check 10s timeout condition
                    LOG_WARN("NETWORK",
                             "STA Connection timeout! Fallback to AP Mode.");
                    setSystemErrorFlag(EVENT_WIFI_DISCONN);
                    current_state = STATE_AP_MODE;
                    startApMode();
                }
                break;
            }
            case STATE_STA_CONNECTED: {
                if (WiFi.status() != WL_CONNECTED) {
                    LOG_WARN("NETWORK",
                             "STA Disconnected! Attempting to reconnect...");
                    setSystemErrorFlag(EVENT_WIFI_DISCONN);

                    SystemState state = getSystemState();
                    state.is_wifi_connected = false;
                    setSystemState(state);

                    current_state = STATE_STA_CONNECTING;
                    start_connect_time = millis();
                    WiFi.reconnect();
                }

                // Wait for explicit signal from WebServer or Button to switch
                // to AP
                if (xSemaphoreTake(switch_to_ap_semaphore, 0) == pdTRUE) {
                    LOG_INFO("NETWORK", "User request to switch to AP Mode.");
                    WiFi.disconnect();
                    current_state = STATE_AP_MODE;
                    startApMode();
                }
                break;
            }
            case STATE_AP_MODE: {
                // Process DNS requests for Captive Portal
                dns_server.processNextRequest();

                // Wait for explicit signal from WebServer to switch to STA
                if (xSemaphoreTake(switch_to_sta_semaphore, 0) == pdTRUE) {
                    SystemConfig config = getSystemConfig();
                    if (strlen(config.wifi_ssid) == 0) {
                        LOG_ERR(
                            "NETWORK",
                            "WiFi SSID is empty! Cannot switch to STA mode.");
                        break;
                    }
                    LOG_INFO("NETWORK", "User request to switch to STA Mode.");
                    dns_server.stop();
                    current_state = STATE_STA_CONNECTING;
                    startStaMode(config.wifi_ssid, config.wifi_password);
                    start_connect_time = millis();
                }
                break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
