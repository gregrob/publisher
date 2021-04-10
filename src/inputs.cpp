#include <Arduino.h>

#include "inputs.h"
#include "inputs_cfg.h"


// Reset value for inputs debounce timer
#define INPUTS_DEBOUNCE_TIMER_RESET    (4)


// Structure for input configuration data
typedef struct {
    const char *                name;
    const uint8_t               pin;
    const bool                  inverted;
        
    const unsigned int          debounceTimerReset;
    unsigned int                debounceTimer;

    const int                   initialValue;
    int                         lastValue;
    int                         currentValue;
    int                         debouncedValue;
} inputConfiguration;


// Input configuraiton structure
static inputConfiguration moduleInputConfiguration[] = {{"reset switch", INPUTS_PIN_MAP_RESET, true, 0, 0, LOW, LOW, LOW, LOW},
};


/**
    Initialise the inputs module and initial states of the pins.
*/
void inputsInit(void) {

    // Initialise the required pins to inputs with their initial value
    for(unsigned int i = 0; i < (sizeof(moduleInputConfiguration)/sizeof(moduleInputConfiguration[0])); i++) {
        pinMode(moduleInputConfiguration[i].pin, INPUT);
        moduleInputConfiguration[i].lastValue = moduleInputConfiguration[i].initialValue;
        moduleInputConfiguration[i].currentValue = moduleInputConfiguration[i].initialValue;
        moduleInputConfiguration[i].debouncedValue = moduleInputConfiguration[i].initialValue;
    }
}


/**
    Cycle task to handle input debouncing.
*/
void inputsCyclicTask(void) {

    // Handle all pins
    for (unsigned int i = 0; i < (sizeof(moduleInputConfiguration)/sizeof(moduleInputConfiguration[0])); i++) {
        
        // Perform inversion if necessary
        if (moduleInputConfiguration[i].inverted == true) {
            moduleInputConfiguration[i].currentValue = !digitalRead(moduleInputConfiguration[i].pin);
        }
        else {
            moduleInputConfiguration[i].currentValue = digitalRead(moduleInputConfiguration[i].pin);
        }
        
        // If there is a change in pin state, restart debounce timers
        if (moduleInputConfiguration[i].currentValue != moduleInputConfiguration[i].lastValue) {
            moduleInputConfiguration[i].debounceTimer = moduleInputConfiguration[i].debounceTimerReset;
        }

        // If the debounce timer is runing handle it
        if (moduleInputConfiguration[i].debounceTimer != 0) {
            moduleInputConfiguration[i].debounceTimer--;
        }
        // Timer expired without state change so set debounced value
        else {
            moduleInputConfiguration[i].debouncedValue = moduleInputConfiguration[i].currentValue;
        }

        // Update lastValue for next round
        moduleInputConfiguration[i].lastValue = moduleInputConfiguration[i].currentValue;
    }
}


/**
    Read a debounced input.
    
    @param[in]     input index of the input to read
    @return        the current debounced value of the pin being read
*/
int inputsReadInput(const inputIndex input) {
    int returnValue = LOW;

    // Make sure a valid output is selected and levels are within range
    if ((unsigned int) input < inputIndex::IP_LAST_ITEM) {
        returnValue = moduleInputConfiguration[input].debouncedValue;
    } 

    return(returnValue);
}
