#ifndef INPUTS_CFG_H
#define INPUTS_CFG_H

#include "inputs.h"

// Input pin for the reset switch
#define INPUTS_PIN_MAP_RESET            (D6)

// Input pin for alarm sounder
#define INPUTS_PIN_MAP_ALARM_SOUNDER    (RX)


// Enumeration for inputs (index must align into configuration structure)
enum inputIndex {
    resetSw             = 0,
    alarmSounder        = 1,
    
    inputsNumberOfTypes
};

/**
    Set-up read pointer to the input configuration.
        
    @param[in]     activeInputConfig pointer for the inputs configuration.
    @return        size of the inputs configuration.
*/
const uint32_t inputsGetConfigPointerRO(const inputConfiguration ** activeInputConfig);

/**
    Set-up read / write pointer to the input configuration.
        
    @param[in]     activeInputConfig pointer for the inputs configuration.
    @return        size of the inputs configuration.
*/
const uint32_t inputsGetConfigPointerRW(inputConfiguration ** activeInputConfig);

/**
    Read a named input (debounced).
  
    @param[in]     name name of the input to read.
    @return        debounced value at the input.
*/
uint8_t inputsReadInputByName(const inputIndex name);

#endif
