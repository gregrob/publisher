#include <Arduino.h>
#include <EEPROM.h>
#include <CRC32.h>

#include "nvm.h"
#include "nvm_cfg.h"

#include "debug.h"


// NVM base address
#define NVM_BASE_ADDRESS            (0)

// Default parameter value when running off defaults
#define NVM_BUFFER_DEFAULT          (0xDEADBEEF)

// CRC value to use for 
#define NVM_CRC_DEFAULT             (0)

// Use this to force a corruption of the NVM
//#define NVM_CORRUPT_TEST

// NVM structure footer
typedef struct {
     uint32_t                buffer;             // Buffer for detecting defaults and to make sure no 32bit structure with 0xFFFFFFFF
     uint32_t                crc;                // 32bit CRC
} nvmFooterCrc32;

// Base NVM structure
typedef struct __attribute__ ((packed)) {
     char                    mqttUser[NVM_MAX_LENGTH_USER];             // MQTT user name
     char                    mqttPassword[NVM_MAX_LENGTH_PASSWORD];     // MQTT password
     char                    mqttTopicRoot[NVM_MAX_LENGTH_TOPIC];       // MQTT root topic
     
     char                    otaPassword[NVM_MAX_LENGTH_PASSWORD];      // OTA password
     char                    wifiAPPassword[NVM_MAX_LENGTH_PASSWORD];   // Configuraitone mode WiFi AP password

     char                    homeAddress[NVM_MAX_LENGTH_ADDRESS];       // Home address for the alarm
    
     nvmFooterCrc32          footer;
} nvmSubConfigBase;

// Extensions 1 NVM structure
typedef struct __attribute__ ((packed)) {
     unsigned int           ledDimmingLevel;     // LED Dimming Level
     
     nvmFooterCrc32         footer;
} nvmSubConfigExt1;

// Extensions 2 NVM structure
typedef struct __attribute__ ((packed)) {
     unsigned char          char1;               // Character 1
     unsigned char          char2;               // Character 2

     nvmFooterCrc32         footer;
} nvmSubConfigExt2;

// Complete NVM structure
typedef struct __attribute__ ((packed)) {
    nvmSubConfigBase        base;                // Base NVM structure
    nvmSubConfigExt1        ext1;                // Extensions 1
    nvmSubConfigExt2        ext2;                // Extensions 2
} nvmCompleteConfig;

// Structure to store NVM structure addresses
typedef struct {
    uint8_t * const         addressRamMirror;    // Pointer to base address of the RAM mirror (pointer constant, data updatable)
    const uint8_t * const   addressRomDefault;   // Pointer to base address of the ROM defaults (pointer constant, data constant)
    uint8_t * const         addressCrc;          // Pointer to crc address of the RAM mirror (pointer constant, data updatable)

    const unsigned int      length;              // Length of the structure excluding the CRC32

    const bool              rewriteWhenCorrupt;  // Allow the software to repair corrupted NVM structures
} nvmStructure;


// NVM RAM Mirror
static nvmCompleteConfig activeNvmConfig;


// Corrupted NVM Configuration
static const nvmCompleteConfig corruptNvmConfig = {0};

// NVM ROM defaults for nvmSubConfigBase
static const nvmSubConfigBase nvmSubConfigBaseDefault = {"testuser", "password", "publisher", "password", "password", "Home Address", NVM_BUFFER_DEFAULT, NVM_CRC_DEFAULT};

// NVM ROM defaults for nvmSubConfigExt1
static const nvmSubConfigExt1 nvmSubConfigExt1Default = {PWMRANGE, NVM_BUFFER_DEFAULT, NVM_CRC_DEFAULT};

// NVM ROM defaults for nvmSubConfigExt2
static const nvmSubConfigExt2 nvmSubConfigExt2Default = {'G', 'R', NVM_BUFFER_DEFAULT, NVM_CRC_DEFAULT};

// NVM memory map
static const nvmStructure activeNvmConfigMemoryMap[] = { // Memory configuration for nvmSubConfigBase
                                                         {(uint8_t * const) & activeNvmConfig.base, 
                                                          (const uint8_t * const) & nvmSubConfigBaseDefault,
                                                          (uint8_t * const) & activeNvmConfig.base.footer.crc,
                                                          ((sizeof(nvmSubConfigBase) / sizeof(char)) - (sizeof(nvmFooterCrc32::crc) / sizeof(char))),
                                                          true},

                                                         // Memory configuration for nvmSubConfigExt1
                                                         {(uint8_t * const) & activeNvmConfig.ext1,
                                                          (const uint8_t * const) & nvmSubConfigExt1Default,
                                                          (uint8_t * const) & activeNvmConfig.ext1.footer.crc,
                                                          ((sizeof(nvmSubConfigExt1) / sizeof(char)) - (sizeof(nvmFooterCrc32::crc) / sizeof(char))),
                                                          true},
                                                         
                                                         // Memory configuration for nvmSubConfigExt2
                                                         {(uint8_t * const) & activeNvmConfig.ext2,
                                                          (const uint8_t * const) & nvmSubConfigExt2Default,
                                                          (uint8_t * const) & activeNvmConfig.ext2.footer.crc,
                                                          ((sizeof(nvmSubConfigExt2) / sizeof(char)) - (sizeof(nvmFooterCrc32::crc) / sizeof(char))),
                                                          false}
};


/**
    Calculate the CRC32 for a NVM structure in RAM.  
    Update the RAM mirror CRC if update is true.
        
    @param[in]     index index of NVM structure to calculate CRC for.
    @param[in]     update update the NVM structure CRC with the calculated CRC.
    @return        calculated CRC32.
*/
static uint32_t nvmStructureCalculateCrc32(unsigned int index, bool update) {
    
    uint32_t calculatedCrc;

    // Make sure the index is within range and update the CRC of 
    if(index < (sizeof(activeNvmConfigMemoryMap) / sizeof(activeNvmConfigMemoryMap[0]))) {

        calculatedCrc = CRC32::calculate(activeNvmConfigMemoryMap[index].addressRamMirror, activeNvmConfigMemoryMap[index].length);

        // Update the RAM mirror CRC if necessary
        if(update == true) {
            memcpy(activeNvmConfigMemoryMap[index].addressCrc, &calculatedCrc, (sizeof(uint32_t) / sizeof(char)));
        }
    }

    return(calculatedCrc);
}


#ifdef NVM_CORRUPT_TEST
/**
    Corrupt the NVM for testing purposes.
*/
static void nvmCorrupt(void) {
    EEPROM.put(NVM_BASE_ADDRESS, corruptNvmConfig);
    EEPROM.commit();
    EEPROM.get(NVM_BASE_ADDRESS, activeNvmConfig);
}
#endif


/**
    Check the CRC's of all NVM structures.
    If there is corruption and correctDefaults is allowed, then recover and rewrite EEPROM.
    Assumes that activeNvmConfigMemoryMap is pre-populated from EEPROM.
*/
static void nvmCheckIntegrity(void) {
    
    // Expected CRC
    uint32_t crcExpected;

    // Read CRC
    uint32_t crcRead;

    // Char buffer for expected CRC
    char crcStringExpected[((sizeof(uint32_t) / sizeof(char)) * 2) + 1];

    // Char buffer for read CRC
    char crcStringRead[((sizeof(uint32_t) / sizeof(char)) * 2) + 1];
    
    // NVM update
    bool nvmUpdate = false;

    // Debug message string
    String debugMessage;

    // Check the CRC's of all the NVM structures
    for (unsigned int i = 0; i < sizeof(activeNvmConfigMemoryMap) / sizeof(activeNvmConfigMemoryMap[0]); i++) {

        crcExpected = nvmStructureCalculateCrc32(i, false);
        memcpy(&crcRead, activeNvmConfigMemoryMap[i].addressCrc, (sizeof(uint32_t) / sizeof(char)));

        // Check if the structure is corrupted 
        if (crcExpected != crcRead) {
            sprintf(crcStringExpected, "%08X", crcExpected);
            sprintf(crcStringRead, "%08X", crcRead);

            debugMessage = String() + "NVM Structure: " + i + " is corrupt (expected 0x" + crcStringExpected + ", read 0x" + crcStringRead + ").";
            debugLog(&debugMessage, error);

            if (activeNvmConfigMemoryMap[i].rewriteWhenCorrupt == true) {
                memcpy(activeNvmConfigMemoryMap[i].addressRamMirror, activeNvmConfigMemoryMap[i].addressRomDefault, activeNvmConfigMemoryMap[i].length);
                nvmStructureCalculateCrc32(i, true);
                nvmUpdate = true;

                debugMessage = String() + "NVM Structure: " + i + " restored defaults.";
                debugLog(&debugMessage, warning);
            }
        }
    }

    // If there was an NVM update, make sure it is comitted
    if (nvmUpdate == true) {
        EEPROM.put(NVM_BASE_ADDRESS, activeNvmConfig);
        EEPROM.commit();
    }
}


/**
    Initialise the NVM and read the module configuration.
*/
void nvmInit(void) {

    // Debug message string
    String debugMessage;

    debugMessage = String() + "NVM Structure is " + sizeof(activeNvmConfig) / sizeof(char) + " bytes.";
    debugLog(&debugMessage, info);

    EEPROM.begin(sizeof(activeNvmConfig) / sizeof(char));

    EEPROM.get(NVM_BASE_ADDRESS, activeNvmConfig);
    
    #ifdef NVM_CORRUPT_TEST
        nvmCorrupt();
    #else
        nvmCheckIntegrity();
    #endif

    //Serial1.println(activeNvmConfig.base.footer.buffer,HEX);
    //Serial1.println(activeNvmConfig.base.mqttTopicRoot);
    //Serial1.println(activeNvmConfig.ext1.ledDimmingLevel);
}    
