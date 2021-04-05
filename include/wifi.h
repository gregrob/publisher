#ifndef WIFI_H
#define WIFI_H

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
void setupWifi();

/**
    WiFi check and prints status to the console.
*/
void checkWifi();

#endif