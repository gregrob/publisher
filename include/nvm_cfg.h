#ifndef NVM_CFG_H
#define NVM_CFG_H


// Maximum length for a password
#define NVM_MAX_LENGTH_PASSWORD     (32)

// Maximum length for a user name
#define NVM_MAX_LENGTH_USER         (32)

// Maximum length for the MQTT topic
#define NVM_MAX_LENGTH_TOPIC        (32)

// Maximum address length for the home address
#define NVM_MAX_LENGTH_ADDRESS      (64)


// Base NVM structure
typedef struct __attribute__ ((packed)) {
     char                    mqttUser[NVM_MAX_LENGTH_USER];             // MQTT user name
     char                    mqttPassword[NVM_MAX_LENGTH_PASSWORD];     // MQTT password
     char                    mqttTopicRoot[NVM_MAX_LENGTH_TOPIC];       // MQTT root topic
     
     char                    otaPassword[NVM_MAX_LENGTH_PASSWORD];      // OTA password
     char                    wifiAPPassword[NVM_MAX_LENGTH_PASSWORD];   // Configuraitone mode WiFi AP password

     char                    homeAddress[NVM_MAX_LENGTH_ADDRESS];       // Home address for the alarm
    
     nvmFooterCrc            footer;
} nvmSubConfigBase;

// Extensions 1 NVM structure
typedef struct __attribute__ ((packed)) {
     unsigned int           ledDimmingLevel;     // LED Dimming Level
     
     nvmFooterCrc           footer;
} nvmSubConfigExt1;

// Extensions 2 NVM structure
typedef struct __attribute__ ((packed)) {
     unsigned char          char1;               // Character 1
     unsigned char          char2;               // Character 2

     nvmFooterCrc           footer;
} nvmSubConfigExt2;

// Complete NVM structure
typedef struct __attribute__ ((packed)) {
    nvmSubConfigBase        base;                // Base NVM structure
    nvmSubConfigExt1        ext1;                // Extensions 1
    nvmSubConfigExt2        ext2;                // Extensions 2
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


#endif
