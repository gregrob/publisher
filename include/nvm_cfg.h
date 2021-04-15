#ifndef NVM_CFG_H
#define NVM_CFG_H

#include "nvm.h"

// Maximum length for a password
#define NVM_MAX_LENGTH_PASSWORD     (32)

// Maximum length for a user name
#define NVM_MAX_LENGTH_USER         (32)

// Maximum length for the MQTT topic
#define NVM_MAX_LENGTH_TOPIC        (32)

// Maximum address length for the home address
#define NVM_MAX_LENGTH_ADDRESS      (32)


// NVM subconfiguration index
typedef enum {
    nvmNetworkStruc  = 0,
    nvmMqttStruc     = 1,
    nvmIOStruc       = 2,
    nvmAlarmStruc    = 3,
    nvmExt1Struc     = 4, 

    nvmNumberOfTypes
} nvmSubConfigIndex;

// Network NVM structure
typedef struct __attribute__ ((packed)) {    
     char                    otaPassword[NVM_MAX_LENGTH_PASSWORD];      // OTA password
     char                    wifiAPPassword[NVM_MAX_LENGTH_PASSWORD];   // Configuraitone mode WiFi AP password

     nvmFooterCrc            footer;
} nvmSubConfigNetwork;

// MQTT NVM structure
typedef struct __attribute__ ((packed)) {    
     char                    mqttUser[NVM_MAX_LENGTH_USER];             // MQTT user name
     char                    mqttPassword[NVM_MAX_LENGTH_PASSWORD];     // MQTT password
     char                    mqttTopicRoot[NVM_MAX_LENGTH_TOPIC];       // MQTT root topic

     nvmFooterCrc            footer;
} nvmSubConfigMqtt;

// IO NVM structure
typedef struct __attribute__ ((packed)) {
     unsigned int           ledDimmingLevel;                            // LED Dimming Level
     
     nvmFooterCrc           footer;
} nvmSubConfigIO;

// Alarm NVM structure
typedef struct __attribute__ ((packed)) {
     char                    homeAddress[NVM_MAX_LENGTH_ADDRESS];       // Home address for the alarm
    
     nvmFooterCrc            footer;
} nvmSubConfigAlarm;

// Extensions 1 NVM structure
typedef struct __attribute__ ((packed)) {
     unsigned char          char1;                                      // Character 1
     unsigned char          char2;                                      // Character 2

     nvmFooterCrc           footer;
} nvmSubConfigExt1;

// Complete NVM structure
typedef struct __attribute__ ((packed)) {
    nvmSubConfigNetwork     network;             // Network settings
    nvmSubConfigMqtt        mqtt;                // MQTT settings
    nvmSubConfigIO          io;                  // IO settings
    nvmSubConfigAlarm       alarm;               // Alarm settings
    nvmSubConfigExt1        ext1;                // Extensions 1 settings
} nvmCompleteStructure;


/**
    Set-up read / write pointer to the RAM mirror
        
    @param[in]     activeNvmRamMirror pointer for the RAM mirror.
    @return        size of the RAM mirror.
*/
const uint32_t nvmGetRamMirrorPointerRW(nvmCompleteStructure ** activeNvmRamMirror);

/**
    Set-up read only pointer to the RAM mirror
        
    @param[in]     activeNvmRamMirror pointer for the RAM mirror.
    @return        size of the RAM mirror.
*/
const uint32_t nvmGetRamMirrorPointerRO(const nvmCompleteStructure ** activeNvmRamMirror);

/**
    Set-up read only pointer to the NVM configuration
        
    @param[in]     activeNvmRamMirror pointer for the NVM configuration.
    @return        size of the NVM configuration.
*/
const uint32_t nvmGetConfigPointerRO(const nvmStructureConfig ** activeNvmConfig);

/**
    Update named RAM mirror structure CRC based on the current structure contents.
  
    @param[in]     name name of the NVM structure the will have the CRC update.
*/
void nvmUpdateRamMirrorCrcByName(nvmSubConfigIndex name);

#endif
