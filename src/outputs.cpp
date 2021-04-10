#include <Arduino.h>

#include "outputs.h"
#include "outputs_cfg.h"


// Structure for output configuration data
typedef struct {
    const char *                name;
    const uint8_t               pin;

    const int                   initialLevel;    
    outputCycleConfiguration    currentCycle;
    outputCycleConfiguration    nextCycle;
} outputConfiguration;


// Output configuraiton structure
static outputConfiguration moduleOutputConfiguration[] = {{"wifi config mode led", OUTPUTS_PIN_MAP_WIFI_CFG,   LOW, {direct, 0, 0, 0, 0}, {direct, 0, 0, 0, 0}},
                                                          {"wifi connected led",   OUTPUTS_PIN_MAP_WIFI_RUN,   LOW, {direct, 0, 0, 0, 0}, {direct, 0, 0, 0, 0}},
                                                          {"alarm control",        OUTPUTS_PIN_MAP_ALARM_CTRL, LOW, {direct, 0, 0, 0, 0}, {direct, 0, 0, 0, 0}}
};


/**
    Initialise the outputs module and initial states of the pins.
*/
void outputsInit(void) {

    // Initialise all outputs to their initial state
    for(unsigned int i = 0; i < (sizeof(moduleOutputConfiguration)/sizeof(moduleOutputConfiguration[0])); i++) {
        analogWrite(moduleOutputConfiguration[i].pin, moduleOutputConfiguration[i].initialLevel);
        pinMode(moduleOutputConfiguration[i].pin, OUTPUT);
    }

}


/**
    Cycle task to handle output states.
*/
void outputsCyclicTask(void) {
    
    // Handle all outputs to their initial state
    for(unsigned int i = 0; i < (sizeof(moduleOutputConfiguration)/sizeof(moduleOutputConfiguration[0])); i++) {

        // Handle the individual output modes
        switch(moduleOutputConfiguration[i].currentCycle.mode) {
            
            case oneshot:
                // High timer running
                if(moduleOutputConfiguration[i].currentCycle.durationHigh > 0) {
                    moduleOutputConfiguration[i].currentCycle.durationHigh--;
                    analogWrite(moduleOutputConfiguration[i].pin, moduleOutputConfiguration[i].currentCycle.levelHigh);
                }

                // Low timer running
                else if (moduleOutputConfiguration[i].currentCycle.durationLow > 0) {
                    moduleOutputConfiguration[i].currentCycle.durationLow--;
                    analogWrite(moduleOutputConfiguration[i].pin, moduleOutputConfiguration[i].currentCycle.levelLow);

                    // Low timer expired so turn everything off and back to direct mode (i.e. idle)
                    if(moduleOutputConfiguration[i].currentCycle.durationLow == 0) {
                        moduleOutputConfiguration[i].currentCycle = {direct, 0, 0, 0, 0};
                        moduleOutputConfiguration[i].nextCycle = {direct, 0, 0, 0, 0};
                        analogWrite(moduleOutputConfiguration[i].pin, 0);
                    }
                }
                break;

            case flash:
                // High timer running
                if(moduleOutputConfiguration[i].currentCycle.durationHigh > 0) {
                    moduleOutputConfiguration[i].currentCycle.durationHigh--;
                    analogWrite(moduleOutputConfiguration[i].pin, moduleOutputConfiguration[i].currentCycle.levelHigh);
                }

                // Low timer running
                else if (moduleOutputConfiguration[i].currentCycle.durationLow > 0) {
                    moduleOutputConfiguration[i].currentCycle.durationLow--;
                    analogWrite(moduleOutputConfiguration[i].pin, moduleOutputConfiguration[i].currentCycle.levelLow);

                    // Low timer expired so set-up next cycle (which could be oneshot or flash)
                    if(moduleOutputConfiguration[i].currentCycle.durationLow == 0) {
                        moduleOutputConfiguration[i].currentCycle = moduleOutputConfiguration[i].nextCycle;
                    }
                }
                break;

            case direct:
            default:
                // Always set-up next cycle in direct (so output can transition to other modes)
                moduleOutputConfiguration[i].currentCycle = moduleOutputConfiguration[i].nextCycle;
                break;
        }
    }
}


/**
    Control an output.
    
    @param[in]     output index of the output to control
    @param[in]     cycleConfiguration how the output should be controlled
*/
void outputsSetOutput(const outputIndex output, const outputCycleConfiguration cycleConfiguration) {

    // Make sure a valid output is selected and levels are within range
    if (((unsigned int) output < outputIndex::OP_LAST_ITEM) && 
        (cycleConfiguration.levelHigh <= PWMRANGE) && 
        (cycleConfiguration.levelLow <= PWMRANGE)) {
        
        // Set-up specific control modes
        switch(cycleConfiguration.mode) {
            
            case oneshot:
                moduleOutputConfiguration[output].nextCycle = cycleConfiguration;
                break;

            case flash:
                moduleOutputConfiguration[output].nextCycle = cycleConfiguration;
                break;

            case direct:
            default:
                // For direct operations, take control immedately from here
                moduleOutputConfiguration[output].nextCycle = cycleConfiguration;
                moduleOutputConfiguration[output].currentCycle = cycleConfiguration;
                
                // Set-up the PWM output now
                analogWrite(moduleOutputConfiguration[output].pin, moduleOutputConfiguration[output].currentCycle.levelHigh);
                break;
        }
    }
}
