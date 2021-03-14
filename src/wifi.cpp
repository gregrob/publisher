#include "wifi.h"
#include <ESP8266WiFi.h>
#include "credentials.h"

// wifi settings
const char* ssid = STASSID;
const char* password = STAPSK;

/**
    wifi setup.
*/
void setupWifi() {
    Serial.println("Connecting to WiFi...");
  
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    Serial.print("Connected IP address: ");
    Serial.println(WiFi.localIP());
}

/**
    Returns string to explain wifi status return code.

    @param[in]     status wifi status value to check.

    @return        string explaining the wifi code.
*/
static const char* wl_status_to_string(wl_status_t status) {
  switch (status) {
    case WL_NO_SHIELD: return "WL_NO_SHIELD";
    case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
    case WL_CONNECTED: return "WL_CONNECTED";
    case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED: return "WL_DISCONNECTED";
    default: return "UNKNOWN";
  }
}

/**
    wifi check prints the wifi status to the console.
*/
void checkWifi() {
    Serial.println("WiFi status: " + (String)wl_status_to_string(WiFi.status()) + ", RSSI: " + (String)WiFi.RSSI());

}
