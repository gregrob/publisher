#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "debug.h"
#include "nvm_cfg.h"
#include "wifi.h"

// Two step process as defined in - https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html
// Stops bricking upon crash / power outage during update
#define ATOMIC_FS_UPDATE

/**
    ota setup.
*/
void otaSetup(void) {
    // Pointer to the RAM mirror
    const nvmCompleteStructure * ramMirrorPtr;

    // Set-up pointer to RAM mirror
    (void) nvmGetRamMirrorPointerRO(&ramMirrorPtr);

    // Port defaults to 8266
    // ArduinoOTA.setPort(8266);

    // Hostname defaults to esp8266-[ChipID]
    // ArduinoOTA.setHostname("myesp8266");
    ArduinoOTA.setHostname(getWiFiModuleDetails()->moduleHostName);

    // Authentication.
    ArduinoOTA.setPassword(ramMirrorPtr->network.otaPassword);
    
    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

    ArduinoOTA.onStart([]() {
        String type;
        String debugMessage;

        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } 
        else { // U_FS
            type = "filesystem";
        }

        // NOTE: if updating FS this would be the place to unmount FS using FS.end()
        debugMessage = (String() + "software update started, type " + type);
        debugLog(&debugMessage, info);
    });

    ArduinoOTA.onEnd([]() {
        String debugMessage = "";

        debugMessage = (String() + "software update complete");
        debugLog(&debugMessage, info);
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {        
        char percentageComplete[4];
        String debugMessage;

        // Evaluate % complete
        sprintf(percentageComplete, "%u%%", (progress / (total / 100)));

        debugMessage = (String() + "updating software: " + percentageComplete + "\r");
        debugPrint(&debugMessage, brblue);
    });

    ArduinoOTA.onError([](ota_error_t otaErrorCode) {
        String debugMessage = (String() + "software update error " + otaErrorCode + ": ");

        if (otaErrorCode == OTA_AUTH_ERROR) {
            debugMessage = debugMessage + (String() + "OTA_AUTH_ERROR - authentication failed");
        } 
        else if (otaErrorCode == OTA_BEGIN_ERROR) {
            debugMessage = debugMessage + (String() + "OTA_BEGIN_ERROR - begin failed");
        } 
        else if (otaErrorCode == OTA_CONNECT_ERROR) {
            debugMessage = debugMessage + (String() + "OTA_CONNECT_ERROR - connect failed");
        }
        else if (otaErrorCode == OTA_RECEIVE_ERROR) {
            debugMessage = debugMessage + (String() + "OTA_RECEIVE_ERROR - receive failed");
        } 
        else if (otaErrorCode == OTA_END_ERROR) {
            debugMessage = debugMessage + (String() + "OTA_END_ERROR - end failed");
        }

        debugLog(&debugMessage, error);
    });

    ArduinoOTA.begin();
}

/**
    ota loop.
*/
void otaLoop(void) {
    ArduinoOTA.handle();
}
