#include <Arduino.h>
#include <ESP8266WiFi.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"

#include "wifi.h"

#include "utils.h"
#include "debug.h"
#include "nvm_cfg.h"
#include "outputs_cfg.h"
#include "reset_ctrl.h"
#include "messages_tx.h"


// Size of a MAC address char buffer
#define WIFI_BUFFER_SIZE_MAC    (12)

// Size of a OUI char buffer
#define WIFI_BUFFER_SIZE_OUI    (6)

// Size of a NIC char buffer
#define WIFI_BUFFER_SIZE_NIC    (6)

// Size of a SSID char buffer
#define WIFI_BUFFER_SIZE_SSID   (33)

// String size for a IP address
#define WIFI_IP_BUFFER_SIZE     (16)

// String size for a MAC address
#define WIFI_MAC_BUFFER_SIZE    (18)

// Name for the ssid
#define WIFI_NAME_SSID          ("ssid")

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

// HTTP definitions for the configuration portal
#define HTTP_PARAM_HEADING_START   "<br /><br /><br /> <b>"
#define HTTP_PARAM_HEADING_END     "</b>"
#define HTTP_PARAM_TEXT_1_START    "<br /> <small>"
#define HTTP_PARAM_TEXT_N_START    "<br />" HTTP_PARAM_TEXT_1_START
#define HTTP_PARAM_TEXT_END        "</small>"

#define HTTP_TEXT_NVM_VERSION      "NVM Data Version"
#define HTTP_TEXT_NVM_ERROR_CNT    "NVM Error Counter"

#define HTTP_TEXT_NETWORK_OTA_PWD  "OTA Password"
#define HTTP_TEXT_NETWORK_AP_PWD   "WiFi AP Password"

#define HTTP_TEXT_MQTT_SERVER      "MQTT Server"
#define HTTP_TEXT_MQTT_USER        "MQTT User"
#define HTTP_TEXT_MQTT_PWD         "MQTT Password"
#define HTTP_TEXT_MQTT_TOPIC       "MQTT Root Topic"

#define HTTP_TEXT_IO_RUN_LED       "Run Mode LED Brightness"
#define HTTP_TEXT_IO_CFG_LED       "Config Mode LED Brightness"
#define HTTP_TEXT_IO_RESET_SW      "Enable Reset Switch"

#define HTTP_TEXT_ALARM_ADDRESS    "Home Address"

// HTTP constant strings for the configuration portal
const char* httpTextHeadingNvm      = HTTP_PARAM_HEADING_START "NVM Settings"             HTTP_PARAM_HEADING_END;
const char* httpTextNvmVersion      = HTTP_PARAM_TEXT_1_START  HTTP_TEXT_NVM_VERSION      HTTP_PARAM_TEXT_END;
const char* httpTextNvmErrorCnt     = HTTP_PARAM_TEXT_N_START  HTTP_TEXT_NVM_ERROR_CNT    HTTP_PARAM_TEXT_END;

const char* httpTextHeadingNetwork  = HTTP_PARAM_HEADING_START "Network Settings"         HTTP_PARAM_HEADING_END;
const char* httpTextOtaPassword     = HTTP_PARAM_TEXT_1_START  HTTP_TEXT_NETWORK_OTA_PWD  HTTP_PARAM_TEXT_END;
const char* httpTextWifiApPassword  = HTTP_PARAM_TEXT_N_START  HTTP_TEXT_NETWORK_AP_PWD   HTTP_PARAM_TEXT_END;

const char* httpTextHeadingMqtt     = HTTP_PARAM_HEADING_START "MQTT Settings"            HTTP_PARAM_HEADING_END;
const char* httpTextMqttServer      = HTTP_PARAM_TEXT_1_START  HTTP_TEXT_MQTT_SERVER      HTTP_PARAM_TEXT_END;
const char* httpTextMqttUser        = HTTP_PARAM_TEXT_N_START  HTTP_TEXT_MQTT_USER        HTTP_PARAM_TEXT_END;
const char* httpTextMqttPassword    = HTTP_PARAM_TEXT_N_START  HTTP_TEXT_MQTT_PWD         HTTP_PARAM_TEXT_END;
const char* httpTextMqttTopicRoot   = HTTP_PARAM_TEXT_N_START  HTTP_TEXT_MQTT_TOPIC       HTTP_PARAM_TEXT_END;

const char* httpTextHeadingIO       = HTTP_PARAM_HEADING_START "IO Settings"              HTTP_PARAM_HEADING_END;
const char* httpTextRunLedBright    = HTTP_PARAM_TEXT_1_START  HTTP_TEXT_IO_RUN_LED       HTTP_PARAM_TEXT_END;
const char* httpTextCfgLedBright    = HTTP_PARAM_TEXT_N_START  HTTP_TEXT_IO_CFG_LED       HTTP_PARAM_TEXT_END;
const char* httpTextCfgResetSw      = HTTP_PARAM_TEXT_N_START  HTTP_TEXT_IO_RESET_SW      HTTP_PARAM_TEXT_END;

const char* httpTextHeadingAlarm    = HTTP_PARAM_HEADING_START "Alarm Settings"           HTTP_PARAM_HEADING_END;
const char* httpTextHomeAddress     = HTTP_PARAM_TEXT_1_START  HTTP_TEXT_ALARM_ADDRESS    HTTP_PARAM_TEXT_END;

// SSID connected
static char ssidString[WIFI_BUFFER_SIZE_SSID];

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
static wifiData wifiDataSoftware = {WIFI_NAME_SSID,         ssidString,
                                    WIFI_NAME_IP,           ipString,
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

// Configuration save
static bool configSave = false;

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
    Callback when WiFi fails to connect.
*/
static void callbackFailedWifiConnect(WiFiManager *myWiFiManager) {
    
    // Pointer to the RAM mirror
    const nvmCompleteStructure * ramMirrorPtr;

    // Set-up pointer to RAM mirror
    (void) nvmGetRamMirrorPointerRO(&ramMirrorPtr);

    // Debug message
    String debugMessage;

    debugMessage = (String() + "Entering WiFi configuration mode.");
    debugLog(&debugMessage, warning);

    debugMessage = (String() + "SSID: " + myWiFiManager->getConfigPortalSSID() + ", IP: " + WiFi.softAPIP().toString());
    debugLog(&debugMessage, warning);

    // Turn on the config mode LED
    outputsSetOutputByName(configMode, {direct, ramMirrorPtr->io.ledBrightnessConfigMode, 0, 0, 0});
}

/**
    Callback when configuraiton needs to be saved.
*/
static void callbackConfigUpdated(void) {
    configSave = true;
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

    // Pointer to the RAM mirror
    nvmCompleteStructure * ramMirrorPtr;

    // Set-up pointer to RAM mirror
    (void) nvmGetRamMirrorPointerRW(&ramMirrorPtr);

    // String storage for NVM version integer (text entry field)
    char nvmVersionString[STRNLEN_INT(NVM_MAX_VERSION) + 1];

    // String storage for errorCounter integer (text entry field)
    char nvmErrorCounterString[STRNLEN_INT(NVM_MAX_ERROR) + 1];

    // String storage for run LED dimming integer (text entry field)
    char ledRunDimmingString[STRNLEN_INT(PWMRANGE) + 1];

    // String storage for config LED dimming integer (text entry field)
    char ledCfgDimmingString[STRNLEN_INT(PWMRANGE) + 1];

    // String storage for reset switch config (text entry field)
    char resetSwCfgString[STRNLEN_INT(1) + 1];
    
    bufferMacString();
    findCurrentModule();

    // Explicitly set mode, esp defaults to STA+AP
    WiFi.mode(WIFI_STA);
    WiFi.hostname(publisherModules[activeModule].moduleHostName);

    // Setup debug output and call backs for the wifiManager
    wifiManager.setDebugOutput(false);
    wifiManager.setAPCallback(callbackFailedWifiConnect);
    wifiManager.setSaveConfigCallback(callbackConfigUpdated);

    // Setup the config portal timeout (standard is 180s but have chosen 60s)
    wifiManager.setConfigPortalTimeout(60);

    // Save parameters even if connection is unsuccessful
    wifiManager.setBreakAfterConfig(true);

    // Nvm configs
    WiFiManagerParameter textHeadingNvm(httpTextHeadingNvm);
    WiFiManagerParameter textNvmVersion(httpTextNvmVersion);
    sprintf(nvmVersionString, "%d", ramMirrorPtr->nvm.core.version);
    WiFiManagerParameter fieldNvmVersion("nvmVersion", HTTP_TEXT_NVM_ERROR_CNT, nvmVersionString, STRNLEN_INT(NVM_MAX_VERSION));
    WiFiManagerParameter textNvmErrorCnt(httpTextNvmErrorCnt);
    sprintf(nvmErrorCounterString, "%d", ramMirrorPtr->nvm.core.errorCounter);
    WiFiManagerParameter fieldNvmErrorCnt("errorCounter", HTTP_TEXT_NVM_ERROR_CNT, nvmErrorCounterString, STRNLEN_INT(NVM_MAX_ERROR));

    wifiManager.addParameter(&textHeadingNvm);
    wifiManager.addParameter(&textNvmVersion);
    wifiManager.addParameter(&fieldNvmVersion);
    wifiManager.addParameter(&textNvmErrorCnt);
    wifiManager.addParameter(&fieldNvmErrorCnt);

    // Network configs
    WiFiManagerParameter textHeadingNetwork(httpTextHeadingNetwork);
    WiFiManagerParameter textOtaPassword(httpTextOtaPassword);
    WiFiManagerParameter fieldOtaPassword("otaPassword", HTTP_TEXT_NETWORK_OTA_PWD, ramMirrorPtr->network.otaPassword, NVM_MAX_LENGTH_PASSWORD);
    WiFiManagerParameter textWifiApPassword(httpTextWifiApPassword);
    WiFiManagerParameter fieldWifiApPassword("wifiAPPassword", HTTP_TEXT_NETWORK_AP_PWD, ramMirrorPtr->network.wifiAPPassword, NVM_MAX_LENGTH_PASSWORD);
    
    wifiManager.addParameter(&textHeadingNetwork);
    wifiManager.addParameter(&textOtaPassword);
    wifiManager.addParameter(&fieldOtaPassword);
    wifiManager.addParameter(&textWifiApPassword);
    wifiManager.addParameter(&fieldWifiApPassword);

    // MQTT configs
    WiFiManagerParameter textHeadingMqtt(httpTextHeadingMqtt);
    WiFiManagerParameter textMqttServer(httpTextMqttServer);
    WiFiManagerParameter fieldMqttServer("mqttServer", HTTP_TEXT_MQTT_SERVER, ramMirrorPtr->mqtt.mqttServer, NVM_MAX_LENGTH_URL);
    WiFiManagerParameter textMqttUser(httpTextMqttUser);
    WiFiManagerParameter fieldMqttUser("mqttUser", HTTP_TEXT_MQTT_USER, ramMirrorPtr->mqtt.mqttUser, NVM_MAX_LENGTH_USER);
    WiFiManagerParameter textMqttPassword(httpTextMqttPassword);
    WiFiManagerParameter fieldMqttPassword("mqttPassword", HTTP_TEXT_MQTT_PWD, ramMirrorPtr->mqtt.mqttPassword, NVM_MAX_LENGTH_PASSWORD);
    WiFiManagerParameter textMqttTopicRoot(httpTextMqttTopicRoot);
    WiFiManagerParameter fieldMqttTopicRoot("mqttTopicRoot", HTTP_TEXT_MQTT_TOPIC, ramMirrorPtr->mqtt.mqttTopicRoot, NVM_MAX_LENGTH_TOPIC);

    wifiManager.addParameter(&textHeadingMqtt);
    wifiManager.addParameter(&textMqttServer);
    wifiManager.addParameter(&fieldMqttServer);
    wifiManager.addParameter(&textMqttUser);
    wifiManager.addParameter(&fieldMqttUser);
    wifiManager.addParameter(&textMqttPassword);
    wifiManager.addParameter(&fieldMqttPassword);
    wifiManager.addParameter(&textMqttTopicRoot);
    wifiManager.addParameter(&fieldMqttTopicRoot);

    // IO configs
    WiFiManagerParameter textHeadingIO(httpTextHeadingIO);
    WiFiManagerParameter textTextRunLedBright(httpTextRunLedBright);
    sprintf(ledRunDimmingString, "%d", ramMirrorPtr->io.ledBrightnessRunMode);
    WiFiManagerParameter fieldRunLedBright("ledBrightnessRunMode", HTTP_TEXT_IO_RUN_LED, ledRunDimmingString, STRNLEN_INT(PWMRANGE));
    WiFiManagerParameter textCfgLedBright(httpTextCfgLedBright);
    sprintf(ledCfgDimmingString, "%d", ramMirrorPtr->io.ledBrightnessConfigMode);
    WiFiManagerParameter fieldCfgLedBright("ledBrightnessConfigMode", HTTP_TEXT_IO_CFG_LED, ledCfgDimmingString, STRNLEN_INT(PWMRANGE));
    WiFiManagerParameter textCfgResetSw(httpTextCfgResetSw);
    sprintf(resetSwCfgString, "%d", ramMirrorPtr->io.resetSwitchEnabled);
    WiFiManagerParameter fieldCfgResetSw("resetSwitchEnabled", HTTP_TEXT_IO_RESET_SW, resetSwCfgString, STRNLEN_INT(1));

    wifiManager.addParameter(&textHeadingIO);
    wifiManager.addParameter(&textTextRunLedBright);
    wifiManager.addParameter(&fieldRunLedBright);
    wifiManager.addParameter(&textCfgLedBright);
    wifiManager.addParameter(&fieldCfgLedBright);
    wifiManager.addParameter(&textCfgResetSw);
    wifiManager.addParameter(&fieldCfgResetSw);

    // Alarm configs
    WiFiManagerParameter textHeadingAlarm(httpTextHeadingAlarm);
    WiFiManagerParameter textHomeAddress(httpTextHomeAddress);
    WiFiManagerParameter fieldHomeAddress("homeAddress", HTTP_TEXT_ALARM_ADDRESS, ramMirrorPtr->alarm.homeAddress, NVM_MAX_LENGTH_ADDRESS);

    wifiManager.addParameter(&textHeadingAlarm);
    wifiManager.addParameter(&textHomeAddress);
    wifiManager.addParameter(&fieldHomeAddress);
    
    // Debug message
    debugMessage = (String() + "Connecting to WiFi network...");
    debugLog(&debugMessage, info);

    // Try auto connect
    bool connectionStatus = wifiManager.autoConnect(publisherModules[activeModule].moduleHostName, ramMirrorPtr->network.wifiAPPassword);

    // Turn off the config mode LED
    outputsSetOutputByName(configMode, {direct, 0, 0, 0, 0});

    // If there is data to update
    if(configSave == true) {

        // NVM configs
        ramMirrorPtr->nvm.core.version = (uint16_t) atoi(fieldNvmVersion.getValue());
        ramMirrorPtr->nvm.core.errorCounter = (uint16_t) atoi(fieldNvmErrorCnt.getValue());
        nvmUpdateRamMirrorCrcByName(nvmNvmStruc);

        // Network configs
        strcpy(ramMirrorPtr->network.otaPassword, fieldOtaPassword.getValue());
        strcpy(ramMirrorPtr->network.wifiAPPassword, fieldWifiApPassword.getValue());
        nvmUpdateRamMirrorCrcByName(nvmNetworkStruc);

        // MQTT configs
        strcpy(ramMirrorPtr->mqtt.mqttServer, fieldMqttServer.getValue());
        strcpy(ramMirrorPtr->mqtt.mqttUser, fieldMqttUser.getValue());
        strcpy(ramMirrorPtr->mqtt.mqttPassword, fieldMqttPassword.getValue());
        strcpy(ramMirrorPtr->mqtt.mqttTopicRoot, fieldMqttTopicRoot.getValue());
        nvmUpdateRamMirrorCrcByName(nvmMqttStruc);

        // IO configs
        ramMirrorPtr->io.ledBrightnessConfigMode = (uint16_t) atoi(fieldCfgLedBright.getValue());
        ramMirrorPtr->io.ledBrightnessRunMode = (uint16_t) atoi(fieldRunLedBright.getValue());
        ramMirrorPtr->io.resetSwitchEnabled = (uint8_t) atoi(fieldCfgResetSw.getValue());
        nvmUpdateRamMirrorCrcByName(nvmIOStruc);

        // Alarm configs
        strcpy(ramMirrorPtr->alarm.homeAddress, fieldHomeAddress.getValue());
        nvmUpdateRamMirrorCrcByName(nvmAlarmStruc);
        
        nvmComittRamMirror();
        
        // Debug message
        debugMessage = (String() + "Configuraiton data saved.  Rebooting in 5s...");
        debugLog(&debugMessage, info);

        // Reboot so modules initialise from NVM (!!! could avoid reboot by doing a re-init here !!!)
        delay(5000);
        restCtrlImmediateHandle(rstTypeReset);
    }

    // If the auto connect failed, reset
    if(connectionStatus == false) {
        debugMessage = (String() + "WiFi connection set-up failed! Rebooting in 5s...");
        debugLog(&debugMessage, error);

        // Reboot
        delay(5000);
        restCtrlImmediateHandle(rstTypeReset);
    }

    // Reaching this point means WiFi is sucessfully connected
    debugMessage = (String() + "Connected to " + WiFi.SSID() + " with IP address " + WiFi.localIP().toString());
    debugLog(&debugMessage, info);

    // Blink the WiFi running LED
    outputsSetOutputByName(runMode, {flash, ramMirrorPtr->io.ledBrightnessRunMode, 1, 0, 19});
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
    Reset wifi settings.
*/
void wifiReset() {
    WiFiManager wifiManager;
    
    wifiManager.resetSettings();
}


/**
    Transmit a wifi message.
    No processing of the message here.
*/
void wifiTransmitWifiMessage(void) {   

    // Update the IP, gateway, subnet mask, MAC, and RSSI variables before transmitting
    strcpy(ssidString, WiFi.SSID().c_str());
    strcpy(ipString, WiFi.localIP().toString().c_str());
    strcpy(gatewayString, WiFi.gatewayIP().toString().c_str());
    strcpy(subnetMaskString, WiFi.subnetMask().toString().c_str());
    strcpy(macString, WiFi.macAddress().c_str());
    rssi = WiFi.RSSI();

    messsagesTxWifiMessage(&wifiDataSoftware);
}
