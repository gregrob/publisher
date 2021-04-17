#include <Arduino.h>

#include "inputs.h"
#include "inputs_cfg.h"


/**
    Initialise the inputs module and initial states of the pins.
*/
void inputsInit(void) {

    // Pointer to the outputs configuration
    inputConfiguration * inputsConfigPtr;

    // Set-up pointer to configuration and get its size
    const uint32_t inputsConfigSize = inputsGetConfigPointerRW(&inputsConfigPtr);
    
    // Initialise the required pins to inputs with their initial value
    for(unsigned int i = 0; i < inputsConfigSize; i++) {
        pinMode(inputsConfigPtr[i].pin, INPUT);
        inputsConfigPtr[i].lastValue = inputsConfigPtr[i].initialValue;
        inputsConfigPtr[i].currentValue = inputsConfigPtr[i].initialValue;
        inputsConfigPtr[i].debouncedValue = inputsConfigPtr[i].initialValue;
    }
}


/**
    Cycle task to handle input debouncing.
*/
void inputsCyclicTask(void) {

    // Pointer to the outputs configuration
    inputConfiguration * inputsConfigPtr;

    // Set-up pointer to configuration and get its size
    const uint32_t inputsConfigSize = inputsGetConfigPointerRW(&inputsConfigPtr);

    // Handle all pins
    for (unsigned int i = 0; i < inputsConfigSize; i++) {
        
        // Perform inversion if necessary
        if (inputsConfigPtr[i].inverted == true) {
            inputsConfigPtr[i].currentValue = !digitalRead(inputsConfigPtr[i].pin);
        }
        else {
            inputsConfigPtr[i].currentValue = digitalRead(inputsConfigPtr[i].pin);
        }
        
        // If there is a change in pin state, restart debounce timers
        if (inputsConfigPtr[i].currentValue != inputsConfigPtr[i].lastValue) {
            inputsConfigPtr[i].debounceTimer = inputsConfigPtr[i].debounceTimerReset;
        }

        // If the debounce timer is runing handle it
        if (inputsConfigPtr[i].debounceTimer != 0) {
            inputsConfigPtr[i].debounceTimer--;
        }
        // Timer expired without state change so set debounced value
        else {
            inputsConfigPtr[i].debouncedValue = inputsConfigPtr[i].currentValue;
        }

        // Update lastValue for next round
        inputsConfigPtr[i].lastValue = inputsConfigPtr[i].currentValue;
    }
}


/**
    Read a indexed input (debounced).
  
    @param[in]     index index of the input to read
    @return        debounced value at the input.
*/
uint8_t inputsReadInputByIndex(const uint32_t index) {
    // Pointer to the outputs configuration
    const inputConfiguration * inputsConfigPtr;

    // Set-up pointer to configuration and get its size
    (void) inputsGetConfigPointerRO(&inputsConfigPtr);

    uint32_t returnValue = LOW;

    // Make sure a valid output is selected and levels are within range
    if ((uint32_t) index < inputIndex::inputsNumberOfTypes) {
        returnValue = inputsConfigPtr[index].debouncedValue;
    } 

    return(returnValue);
}
