#include <Arduino.h>
#include <EEPROM.h>
#include <CRC32.h>

#include "nvm.h"
#include "nvm_cfg.h"

#include "debug.h"
#include "messages_tx.h"

// Use this to force a corruption of the NVM
//#define NVM_CORRUPT_TEST


#ifdef NVM_CORRUPT_TEST
// Corrupted NVM default configuration
static const nvmCompleteStructure corruptNvmRamMirror = {0};
#endif


#ifdef NVM_CORRUPT_TEST
/**
    Corrupt the NVM for testing purposes.
*/
static void nvmCorrupt(void) {
    
    // Pointer to the RAM mirror
    nvmCompleteStructure * ramMirrorPtr;

    // Set up read write pointers to the RAM mirror
    (void)nvmGetRamMirrorPointerRW(&ramMirrorPtr);

    // Comitt a corrupted default configuration and reload RAM mirror
    EEPROM.put(NVM_BASE_ADDRESS, corruptNvmRamMirror);
    EEPROM.commit();
    EEPROM.get(NVM_BASE_ADDRESS, *ramMirrorPtr);
}
#endif


/**
    Check the CRC's of all NVM structures.
    If there is corruption and correctDefaults is allowed, then recover and rewrite EEPROM.
    Assumes that RAM mirror is pre-populated from EEPROM.
    This call is BLOCKING (can start writing the RAM mirrors to EEPROM).
*/
static void nvmCheckIntegrity(void) {
    
    // Debug message string
    String debugMessage;

    // Pointer to the RAM mirror
    nvmCompleteStructure * ramMirrorPtr;

    // Set-up pointer to RAM mirror
    (void) nvmGetRamMirrorPointerRW(&ramMirrorPtr);
    
    // Pointer to the NVM configuration
    const nvmStructureConfig * nvmConfigPtr;

    // Set-up pointer to NVM configuration and get its size
    const uint32_t nvmConfigSize = nvmGetConfigPointerRO(&nvmConfigPtr);

    // Expected CRC
    crc_t crcExpected = 0;

    // Read CRC
    crc_t crcRead = 0;

    // Char buffer for expected CRC
    char crcStringExpected[NVM_CRC_SIZE_CHARS];

    // Char buffer for read CRC
    char crcStringRead[NVM_CRC_SIZE_CHARS];
    
    // NVM update
    bool nvmUpdate = false;
    
    // Check the CRC's of all the NVM structures
    for (unsigned int i = 0; i < nvmConfigSize; i++) {

        // Calculate the expected CRC
        crcExpected = CRC32::calculate(nvmConfigPtr[i].addressRamMirror, nvmConfigPtr[i].length);

        // Copy in the current CRC
        memcpy(&crcRead, nvmConfigPtr[i].addressCrc, NVM_CRC_SIZE_BYTES);

        // Check if the structure is corrupted (difference between calculated and current read CRC)
        if (crcExpected != crcRead) {
            
            // Debug message
            sprintf(crcStringExpected, "%08X", crcExpected);
            sprintf(crcStringRead, "%08X", crcRead);
            debugMessage = String() + "NVM Structure: " + i + " is corrupt (expected 0x" + crcStringExpected + ", read 0x" + crcStringRead + ").";
            debugLog(&debugMessage, error);

            // Recovery is allowed for the current structure
            if ((nvmConfigPtr + i)->rewriteWhenCorrupt == true) {
                
                // Copy the defaults to the RAM mirror
                memcpy(nvmConfigPtr[i].addressRamMirror, nvmConfigPtr[i].addressRomDefault, nvmConfigPtr[i].length);

                // Update the CRC in the RAM mirror
                nvmUpdateRamMirrorCrcByIndex(i);
                
                nvmUpdate = true;

                // Debug message
                debugMessage = String() + "NVM Structure: " + i + " restored defaults.";
                debugLog(&debugMessage, warning);
            }
        }
    }

    // If there was an NVM update, make sure it is comitted
    if (nvmUpdate == true) {
               
        // Increment error counter
        if(ramMirrorPtr->nvm.core.errorCounter < NVM_MAX_ERROR) {
            ramMirrorPtr->nvm.core.errorCounter++;
        }
       
        // Recaclulate CRC and comitt the NVM
        nvmUpdateRamMirrorCrcByName(nvmNvmStruc);
        nvmComittRamMirror();
    }
}


/**
    Update indexed RAM mirror structure CRC based on the current structure contents.
  
    @param[in]     index index of the NVM structure the will have the CRC update.
*/
void nvmUpdateRamMirrorCrcByIndex(uint32_t index) {

    // Pointer to the RAM mirror
    nvmCompleteStructure * ramMirrorPtr;

    // Set-up pointer to RAM mirror
    (void) nvmGetRamMirrorPointerRW(&ramMirrorPtr);
    
    // Pointer to the NVM configuration
    const nvmStructureConfig * nvmConfigPtr;

    // Set-up pointer to NVM configuration and get its size
    const uint32_t nvmConfigSize = nvmGetConfigPointerRO(&nvmConfigPtr);

    // Calculated CRC
    crc_t crcCalculated = 0;

    // Make sure the index is within range
    if(index < nvmConfigSize) {
        
        // Calculate the CRC and copy it to the RAM mirror
        crcCalculated = CRC32::calculate(nvmConfigPtr[index].addressRamMirror, nvmConfigPtr[index].length);
        memcpy(nvmConfigPtr[index].addressCrc, &crcCalculated, NVM_CRC_SIZE_BYTES);
    }
}


/**
    Comitt the RAM mirror directly to NVM.
    Before calling, make sure the appropiate structure CRC's have been updated.
    This call is BLOCKING (writing the RAM mirrors to EEPROM).
*/
void nvmComittRamMirror(void) {

    // Pointer to the RAM mirror
    nvmCompleteStructure * ramMirrorPtr;

    // Set-up pointer to RAM mirror
    (void) nvmGetRamMirrorPointerRW(&ramMirrorPtr);

    // Write to EEPROM (blocking)
    EEPROM.put(NVM_BASE_ADDRESS, *ramMirrorPtr);
    EEPROM.commit();
}


/**
    Initialise the NVM module.
    Include initialisation of the RAM mirrors and recovery to defaults where allowed.
    This call is BLOCKING (reading / populating the RAM mirrors from EEPROM).
*/
void nvmInit(void) {

    // Debug message string
    String debugMessage;

    // Pointer to the RAM mirror
    nvmCompleteStructure * ramMirrorPtr;

    // Set-up pointer to RAM mirror and get its size
    const uint32_t ramMirrorSize = nvmGetRamMirrorPointerRW(&ramMirrorPtr);

    // Debug message
    debugMessage = String() + "NVM Structure is " + ramMirrorSize + " bytes.";
    debugLog(&debugMessage, info);

    // Read the NVM structure to the RAM mirror
    EEPROM.begin(ramMirrorSize);
    EEPROM.get(NVM_BASE_ADDRESS, *ramMirrorPtr);

    // Check integrity
    #ifdef NVM_CORRUPT_TEST
        nvmCorrupt();
    #else
        nvmCheckIntegrity();
    #endif
}    


/**
    Transmit a NVM status message.
    No processing of the message here.
*/
void nvmTransmitStatusMessage(void) {   
    
    // Structure to store the NVM data to report
    nvmData nvmDataCurrent;

    // Pointer to the RAM mirror
    const nvmCompleteStructure * ramMirrorPtr;

    // Pointer to the NVM configuration
    const nvmStructureConfig * nvmConfigPtr;

    // Set-up pointers and populate data structure
    nvmDataCurrent.bytesConsumed = nvmGetRamMirrorPointerRO(&ramMirrorPtr);
    nvmDataCurrent.structures = nvmGetConfigPointerRO(&nvmConfigPtr);
    nvmDataCurrent.core.errorCounter = ramMirrorPtr->nvm.core.errorCounter;
    nvmDataCurrent.core.version = ramMirrorPtr->nvm.core.version;
    
    messsagesTxNvmStatusMessage(&nvmDataCurrent);
}
