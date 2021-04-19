#ifndef WIFI_H
#define WIFI_H

// Structure for wifi data
typedef struct {
    const char*   ssidName;
    const char*   ssidDataPtr;
    const char*   ipAddressName;
    const char*   ipAddressDataPtr;
    const char*   gatewayAddressName;
    const char*   gatewayAddressDataPtr;
    const char*   subnetMaskName;
    const char*   subnetMaskDataPtr;
    const char*   macAddressName;
    const char*   macAddressDataPtr;
    const char*   rssiName;
    const long*   rssiDataPtr;
} wifiData;

// Host types
enum wifiHostTypes {
    alarmModule = 0,
    tankModule  = 1, 
    testModule  = 2
};

// Structure for storing module host details
typedef struct {
  const char*           moduleMAC;
  const char*           moduleHostName;
  const wifiHostTypes   moduleHostType;
} wifiModuleDetail;

/**
    WiFi module details.
    @return        pointer to the wifi modules details.
*/
wifiModuleDetail* const getWiFiModuleDetails(void);

/**
    WiFi set-up and connect.
*/
void setupWifi(void);

/**
    WiFi check and prints status to the console.
*/
void checkWifi(void);

/**
    Reset wifi settings.
*/
void wifiReset(void);

/**
    Returns true when WiFi is connected.
*/
bool wifiIsConnected(void);

/**
    Transmit a wifi message.
    No processing of the message here.
*/
void wifiTransmitWifiMessage(void);

#endif
