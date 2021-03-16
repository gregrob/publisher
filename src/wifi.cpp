#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "debug.h"
#include "credentials.h"
#include "wifi.h"

// wifi settings
const char* ssid = STASSID;
const char* password = STAPSK;

/**
    wifi setup.
*/
void setupWifi() {
    String debugMessage;
    
    debugMessage = (String() + "connecting to wifi network " + ssid);
    debugLog(&debugMessage, info);
  
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        debugMessage = (String() + "connection failed! rebooting in 5s...");
        debugLog(&debugMessage, error);
        delay(5000);
        ESP.restart();
    }

    debugMessage = (String() + "connected to wifi with ip address " + WiFi.localIP().toString());
    debugLog(&debugMessage, info);
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
    String debugMessage;

    debugMessage = (String() + "wifi status: " + wl_status_to_string(WiFi.status()) + ", rssi: " + WiFi.RSSI());
    debugLog(&debugMessage, info);
}
