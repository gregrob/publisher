#include <Arduino.h>
#include <ESP8266WiFi.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"

#include "wifi.h"

#include "debug.h"
#include "credentials.h"
#include "messages_tx.h"

// WiFi settings
// WiFi login and password now through WiFiManager module
//const char* ssid = STASSID;
//const char* password = STAPSK;
const char* apPassword = WIFI_AP_PASSWORD;

// Size of a MAC address char buffer
#define WIFI_BUFFER_SIZE_MAC    (12)

// Size of a OUI char buffer
#define WIFI_BUFFER_SIZE_OUI    (6)

// Size of a NIC char buffer
#define WIFI_BUFFER_SIZE_NIC    (6)



// String size for a IP address
#define WIFI_IP_BUFFER_SIZE     (16)

// String size for a MAC address
#define WIFI_MAC_BUFFER_SIZE    (18)

// Name for the ip
#define WIFI_NAME_IP            ("ip")

// Name for the gateway
#define WIFI_NAME_GATEWAY       ("gateway")

// Name for the subnet mask
#define WIFI_NAME_SUBNET_MASK   ("mask")

// Name for the mac
#define WIFI_NAME_MAC           ("mac")

// Name for the rssi
#define WIFI_NAME_RSSI          ("rssi")


// IP address
static char ipString[WIFI_IP_BUFFER_SIZE];

// Gateway address
static char gatewayString[WIFI_IP_BUFFER_SIZE];

// Subnet mask
static char subnetMaskString[WIFI_IP_BUFFER_SIZE];

// MAC address
static char macString[WIFI_MAC_BUFFER_SIZE];

// RSSI
static long rssi;


// Wifi data for this software
static wifiData wifiDataSoftware = {WIFI_NAME_IP,           ipString,
                                    WIFI_NAME_GATEWAY,      gatewayString,
                                    WIFI_NAME_SUBNET_MASK,  subnetMaskString,
                                    WIFI_NAME_MAC,          macString,
                                    WIFI_NAME_RSSI,         &rssi
};



// Structure for all publisher modules
// The last element is always the default 
static wifiModuleDetail publisherModules[] = {{"483FDA482A64", "pub-alarm-482a64",  alarmModule},
                                              {"F4CFA2D4EA77", "pub-test-d4ea77",   testModule},
                                              {"84F3EB27BD6A", "pub-test-27bd6a",   testModule},
                                              {"000000000000", "pub-default",       testModule}
};

// Pointer to the active module in the publisher module structure
static unsigned int activeModule;

// Decives WiFi full media access control (MAC) address
static char deviceMac[WIFI_BUFFER_SIZE_MAC + 1];

// Decives WiFi orginisationally unique identifier (OUI)
static char deviceMacOui[WIFI_BUFFER_SIZE_OUI + 1];

// Decives WiFi network interface controller (NIC) specific
static char deviceMacNic[WIFI_BUFFER_SIZE_NIC + 1];

// Local function definitions
static void bufferMacString(void);
static void findCurrentModule(void);
static void callbackFailedWifiConnect (WiFiManager *myWiFiManager);
static const char* wifiStatusToString(wl_status_t status);

/**
    Buffer MAC address into a string.
    Seperate OUI and NIC parts.
*/
static void bufferMacString(void) {
    
    // Get the raw MAC address
    byte rawMAC[6];    
    WiFi.macAddress(rawMAC);

    // Pre-process the complete MAC address into a string (no : characters)
    sprintf(deviceMac, "%X%X%X%X%X%X",
        (unsigned)rawMAC[0], 
        (unsigned)rawMAC[1],
        (unsigned)rawMAC[2],
        (unsigned)rawMAC[3], 
        (unsigned)rawMAC[4],
        (unsigned)rawMAC[5]);  

    // Copy out the OUI and NIC into seperate strings
    strncpy(deviceMacOui, deviceMac, WIFI_BUFFER_SIZE_OUI);
    strncpy(deviceMacNic, deviceMac + WIFI_BUFFER_SIZE_OUI, WIFI_BUFFER_SIZE_NIC);
}

/**
    Find the current module in the table based on the MAC address.
*/
static void findCurrentModule(void) {

    // Search for the module
    // Ignore the last element in the table (i.e. -1) as this is the default
    for (activeModule = 0; activeModule < ((sizeof(publisherModules) / sizeof(publisherModules[0])) - 1); activeModule++) {
        
        // Found MAC so break out of the search
        if (strcmp(deviceMac, publisherModules[activeModule].moduleMAC) == 0) {
            break;
        }
    }
}

/**
    Callback when WiFi fails to connectg
*/
static void callbackFailedWifiConnect (WiFiManager *myWiFiManager) {
    String debugMessage;

    debugMessage = (String() + "Entering WiFi configuration mode.");
    debugLog(&debugMessage, warning);

    debugMessage = (String() + "SSID: " + myWiFiManager->getConfigPortalSSID() + ", IP: " + WiFi.softAPIP().toString());
    debugLog(&debugMessage, warning);
}

/**
    WiFi module details.
    @return        pointer to the wifi modules details.
*/
wifiModuleDetail* const getWiFiModuleDetails(void) {

    return(&publisherModules[activeModule]);
}

/**
    Returns string to explain wifi status return code.
    @param[in]     status wifi status value to check.
    @return        string explaining the wifi code.
*/
static const char* wifiStatusToString(wl_status_t status) {
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
    WiFi set-up and connect.
*/
void setupWifi() {
    String debugMessage;
    WiFiManager wifiManager;    
         
    bufferMacString();
    findCurrentModule();

    // Explicitly set mode, esp defaults to STA+AP
    WiFi.mode(WIFI_STA);
    WiFi.hostname(publisherModules[activeModule].moduleHostName);

    wifiManager.setDebugOutput(false);
    wifiManager.setAPCallback(callbackFailedWifiConnect);

    //wifiManager.setConfigPortalTimeout(180);
    wifiManager.setConfigPortalTimeout(60);

    //WiFiManagerParameter custom_text("<p>MQTT stuff</p>");
    //wifiManager.addParameter(&custom_text);
    
    //WiFiManagerParameter custom_mqtt_server("server", "mqtt server", "192.168.1.4", 40);
    //wifiManager.addParameter(&custom_mqtt_server);

    debugMessage = (String() + "Connecting to WiFi network...");
    debugLog(&debugMessage, info);

    if(false == wifiManager.autoConnect(publisherModules[activeModule].moduleHostName, apPassword)) {
        debugMessage = (String() + "WiFi connection set-up failed! Rebooting in 5s...");
        debugLog(&debugMessage, error);

        delay(5000);
        ESP.restart();
    }

    // Reaching this point means WiFi is sucessfully connected
    debugMessage = (String() + "Connected to " + WiFi.SSID() + " with IP address " + WiFi.localIP().toString());
    debugLog(&debugMessage, info);
}

/**
    WiFi check and prints status to the console.
*/
void checkWifi() {
    String debugMessage;

    debugMessage = (String() + "WiFi status: " + wifiStatusToString(WiFi.status()) + ", RSSI: " + WiFi.RSSI());
    debugLog(&debugMessage, info);

    if (WiFi.status() != WL_CONNECTED) {
        debugMessage = (String() + "WiFi connection failed! Rebooting in 5s...");
        debugLog(&debugMessage, error);

        delay(5000);
        ESP.restart();
    }
}

/**
    WiFi reset.
*/
void resetWifi() {
    String debugMessage;
    WiFiManager wifiManager;
    
    debugMessage = (String() + "WIFI resetting system to defaults! Rebooting in 5s...");
    debugLog(&debugMessage, warning);

    wifiManager.resetSettings();

    delay(5000);
    ESP.restart();
}


/**
    Transmit a wifi message.
    No processing of the message here.
*/
void wifiTransmitWifiMessage(void) {   

    // Update the IP, gateway, subnet mask, MAC, and RSSI variables before transmitting
    strcpy(ipString, WiFi.localIP().toString().c_str());
    strcpy(gatewayString, WiFi.gatewayIP().toString().c_str());
    strcpy(subnetMaskString, WiFi.subnetMask().toString().c_str());
    strcpy(macString, WiFi.macAddress().c_str());
    rssi = WiFi.RSSI();

    messsagesTxWifiMessage(&wifiDataSoftware);
}
