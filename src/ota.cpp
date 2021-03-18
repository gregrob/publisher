#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "debug.h"
#include "credentials.h"

// Two step process as defined in - https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html
// Stops bricking upon crash / power outage during update
#define ATOMIC_FS_UPDATE

/**
    ota setup.
*/
void otaSetup(void) {
    // Port defaults to 8266
    // ArduinoOTA.setPort(8266);

    // Hostname defaults to esp8266-[ChipID]
    // ArduinoOTA.setHostname("myesp8266");

    // No authentication by default
    // ArduinoOTA.setPassword("admin");

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



/// TODODODODODOO
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
        }
    });

    ArduinoOTA.begin();
}

/**
    ota loop.
*/
void otaLoop(void) {
    ArduinoOTA.handle();
}