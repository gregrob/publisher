#include <Arduino.h>

#include "nvm_cfg.h"


// NVM ROM default for the CRC footer in every NVM structure
static const nvmFooterCrc nvmFooterCrcDefault = {NVM_BUFFER_DEFAULT, NVM_CRC_DEFAULT};

// NVM ROM defaults for nvmSubConfigNvm - DO NOT CHANGE THIS ONE
static const nvmSubConfigNvm nvmSubConfigNvmDefault = {{NVM_DATA_ERRORS_DEFAULT, NVM_DATA_VERSION_DEFAULT}, nvmFooterCrcDefault};

// NVM ROM defaults for nvmSubConfigNetwork
static const nvmSubConfigNetwork nvmSubConfigNetworkDefault = {"password", "password", nvmFooterCrcDefault};

// NVM ROM defaults for nvmSubConfigMqtt
static const nvmSubConfigMqtt nvmSubConfigMqttDefault = {"swarm.max.lan", "testuser", "password", "publisher", nvmFooterCrcDefault};

// NVM ROM defaults for nvmSubConfigIO
static const nvmSubConfigIO nvmSubConfigIODefault = {PWMRANGE, nvmFooterCrcDefault};

// NVM ROM defaults for nvmSubConfigAlarm
static const nvmSubConfigAlarm nvmSubConfigAlarmDefault = {"Home Address", nvmFooterCrcDefault};

// NVM ROM defaults for nvmSubConfigExt1
static const nvmSubConfigExt1 nvmSubConfigExt1Default = {'G', 'R', nvmFooterCrcDefault};

// NVM RAM mirror
static nvmCompleteStructure nvmRamMirror;

// NVM RAM mirror size (in bytes)
static const uint32_t nvmRamMirrorSizeBytes = (sizeof(nvmRamMirror) / sizeof(uint8_t));

// NVM configuration
static const nvmStructureConfig nvmConfig[] = { // Memory configuration for nvmSubConfigNvm
                                                {(uint8_t * const) & nvmRamMirror.nvm, 
                                                (const uint8_t * const) & nvmSubConfigNvmDefault,
                                                (uint8_t * const) & nvmRamMirror.nvm.footer.crc,
                                                ((sizeof(nvmRamMirror.nvm) / sizeof(uint8_t)) - NVM_CRC_SIZE_BYTES),
                                                true},
        
                                                // Memory configuration for nvmSubConfigNetwork
                                                {(uint8_t * const) & nvmRamMirror.network, 
                                                (const uint8_t * const) & nvmSubConfigNetworkDefault,
                                                (uint8_t * const) & nvmRamMirror.network.footer.crc,
                                                ((sizeof(nvmRamMirror.network) / sizeof(uint8_t)) - NVM_CRC_SIZE_BYTES),
                                                true},

                                                // Memory configuration for nvmSubConfigMqtt
                                                {(uint8_t * const) & nvmRamMirror.mqtt, 
                                                (const uint8_t * const) & nvmSubConfigMqttDefault,
                                                (uint8_t * const) & nvmRamMirror.mqtt.footer.crc,
                                                ((sizeof(nvmRamMirror.mqtt) / sizeof(uint8_t)) - NVM_CRC_SIZE_BYTES),
                                                true},

                                                // Memory configuration for nvmSubConfigIO
                                                {(uint8_t * const) & nvmRamMirror.io,
                                                (const uint8_t * const) & nvmSubConfigIODefault,
                                                (uint8_t * const) & nvmRamMirror.io.footer.crc,
                                                ((sizeof(nvmRamMirror.io) / sizeof(uint8_t)) - NVM_CRC_SIZE_BYTES),
                                                true},

                                                // Memory configuration for nvmSubConfigAlarm
                                                {(uint8_t * const) & nvmRamMirror.alarm, 
                                                (const uint8_t * const) & nvmSubConfigAlarmDefault,
                                                (uint8_t * const) & nvmRamMirror.alarm.footer.crc,
                                                ((sizeof(nvmRamMirror.alarm) / sizeof(uint8_t)) - NVM_CRC_SIZE_BYTES),
                                                true},

                                                // Memory configuration for nvmSubConfigExt1
                                                {(uint8_t * const) & nvmRamMirror.ext1,
                                                (const uint8_t * const) & nvmSubConfigExt1Default,
                                                (uint8_t * const) & nvmRamMirror.ext1.footer.crc,
                                                ((sizeof(nvmRamMirror.ext1) / sizeof(uint8_t)) - NVM_CRC_SIZE_BYTES),
                                                false}
};

// NVM configuration size (in elements)
static const uint32_t nvmConfigSizeElements = (sizeof(nvmConfig) / sizeof(nvmConfig[0]));

// Make sure that NVM configuraiton structure is the same size as the index enum
static_assert(nvmSubConfigIndex::nvmNumberOfTypes == nvmConfigSizeElements, "Mismatch number of elements between <enum nvmSubConfigIndex> and <nvmConfig>.");

/**
    Set-up read / write pointer to the RAM mirror
        
    @param[in]     activeNvmRamMirror pointer for the RAM mirror.
    @return        size of the RAM mirror.
*/
const uint32_t nvmGetRamMirrorPointerRW(nvmCompleteStructure ** activeNvmRamMirror) {
    *activeNvmRamMirror = &nvmRamMirror;
    return(nvmRamMirrorSizeBytes);
}

/**
    Set-up read only pointer to the RAM mirror
        
    @param[in]     activeNvmRamMirror pointer for the RAM mirror.
    @return        size of the RAM mirror.
*/
const uint32_t nvmGetRamMirrorPointerRO(const nvmCompleteStructure ** activeNvmRamMirror) {
    *activeNvmRamMirror = &nvmRamMirror;
    return(nvmRamMirrorSizeBytes);
}

/**
    Set-up read only pointer to the NVM configuration
        
    @param[in]     activeNvmRamMirror pointer for the NVM configuration.
    @return        size of the NVM configuration.
*/
const uint32_t nvmGetConfigPointerRO(const nvmStructureConfig ** activeNvmConfig) {
    *activeNvmConfig = &nvmConfig[0];
    return(nvmConfigSizeElements);
}

/**
    Update named RAM mirror structure CRC based on the current structure contents.
  
    @param[in]     name name of the NVM structure the will have the CRC update.
*/
void nvmUpdateRamMirrorCrcByName(nvmSubConfigIndex name) {

    nvmUpdateRamMirrorCrcByIndex(name);
}
