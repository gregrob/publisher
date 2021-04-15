#include <Arduino.h>

#include "nvm.h"
#include "nvm_cfg.h"


// NVM ROM default for the CRC footer in every NVM structure
static const nvmFooterCrc nvmFooterCrcDefault = {NVM_BUFFER_DEFAULT, NVM_CRC_DEFAULT};

// NVM ROM defaults for nvmSubConfigBase
static const nvmSubConfigBase nvmSubConfigBaseDefault = {"testuser", "password", "publisher", "password", "password", "Home Address", nvmFooterCrcDefault};

// NVM ROM defaults for nvmSubConfigExt1
static const nvmSubConfigExt1 nvmSubConfigExt1Default = {PWMRANGE, nvmFooterCrcDefault};

// NVM ROM defaults for nvmSubConfigExt2
static const nvmSubConfigExt2 nvmSubConfigExt2Default = {'G', 'R', nvmFooterCrcDefault};

// NVM RAM mirror
static nvmCompleteStructure nvmRamMirror;

// NVM RAM mirror size (in bytes)
static const uint32_t nvmRamMirrorSizeBytes = (sizeof(nvmRamMirror) / sizeof(uint8_t));

// NVM configuration
static const nvmStructureConfig nvmConfig[] = { // Memory configuration for nvmSubConfigBase
                                                         {(uint8_t * const) & nvmRamMirror.base, 
                                                          (const uint8_t * const) & nvmSubConfigBaseDefault,
                                                          (uint8_t * const) & nvmRamMirror.base.footer.crc,
                                                          ((sizeof(nvmRamMirror.base) / sizeof(uint8_t)) - NVM_CRC_SIZE_BYTES),
                                                          true},

                                                         // Memory configuration for nvmSubConfigExt1
                                                         {(uint8_t * const) & nvmRamMirror.ext1,
                                                          (const uint8_t * const) & nvmSubConfigExt1Default,
                                                          (uint8_t * const) & nvmRamMirror.ext1.footer.crc,
                                                          ((sizeof(nvmRamMirror.ext1) / sizeof(uint8_t)) - NVM_CRC_SIZE_BYTES),
                                                          true},
                                                         
                                                         // Memory configuration for nvmSubConfigExt2
                                                         {(uint8_t * const) & nvmRamMirror.ext2,
                                                          (const uint8_t * const) & nvmSubConfigExt2Default,
                                                          (uint8_t * const) & nvmRamMirror.ext2.footer.crc,
                                                          ((sizeof(nvmRamMirror.ext2) / sizeof(uint8_t)) - NVM_CRC_SIZE_BYTES),
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
