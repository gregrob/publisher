#include <Arduino.h>

#include "inputs.h"
#include "inputs_cfg.h"


// Input configuraiton structure
static inputConfiguration inputConfig[] = {{"resetSw", INPUTS_PIN_MAP_RESET, true, 0, 0, LOW, LOW, LOW, LOW},
};

// Input configuration size (in elements)
static const uint32_t inputsConfigSizeElements = (sizeof(inputConfig) / sizeof(inputConfig[0]));


/**
    Set-up read pointer to the input configuration.
        
    @param[in]     activeInputConfig pointer for the inputs configuration.
    @return        size of the inputs configuration.
*/
const uint32_t inputsGetConfigPointerRO(const inputConfiguration ** activeInputConfig) {
    *activeInputConfig = &inputConfig[0];
    return(inputsConfigSizeElements);
}

/**
    Set-up read / write pointer to the input configuration.
        
    @param[in]     activeInputConfig pointer for the inputs configuration.
    @return        size of the inputs configuration.
*/
const uint32_t inputsGetConfigPointerRW(inputConfiguration ** activeInputConfig) {
    *activeInputConfig = &inputConfig[0];
    return(inputsConfigSizeElements);
}

/**
    Read a named input (debounced).
  
    @param[in]     name name of the input to read.
    @return        debounced value at the input.
*/
uint8_t inputsReadInputByName(const inputIndex name) {

    return(inputsReadInputByIndex(name));
}
