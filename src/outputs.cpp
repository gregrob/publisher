#include <Arduino.h>

#include "outputs.h"
#include "outputs_cfg.h"


// Structure for output configuration data
typedef struct {
    const uint8_t               outputPin;
    const int                   outputInitialLevel;    
    outputCycleConfiguration    outputCurrentCycle;
    outputCycleConfiguration    outputNextCycle;
} outputConfiguration;


// Output configuraiton structure
static outputConfiguration moduleOutputConfiguration[] = {{OUTPUTS_PIN_MAP_WIFI_CFG,   LOW, {direct, 0, 0, 0, 0}, {direct, 0, 0, 0, 0}},
                                                          {OUTPUTS_PIN_MAP_WIFI_RUN,   LOW, {direct, 0, 0, 0, 0}, {direct, 0, 0, 0, 0}},
                                                          {OUTPUTS_PIN_MAP_ALARM_CTRL, LOW, {direct, 0, 0, 0, 0}, {direct, 0, 0, 0, 0}}
};


/**
    Initialise the outputs module and initial states of the pins.
*/
void outputsInit(void) {

    // Initialise all outputs to their initial state
    for(unsigned int i = 0; i < (sizeof(moduleOutputConfiguration)/sizeof(moduleOutputConfiguration[0])); i++) {
        analogWrite(moduleOutputConfiguration[i].outputPin, moduleOutputConfiguration[i].outputInitialLevel);
        pinMode(moduleOutputConfiguration[i].outputPin, OUTPUT);
    }

}


/**
    Cycle task to handle output states.
*/
void outputsCyclicTask(void) {
    
    // Handle all outputs to their initial state
    for(unsigned int i = 0; i < (sizeof(moduleOutputConfiguration)/sizeof(moduleOutputConfiguration[0])); i++) {

        // Handle the individual output modes
        switch(moduleOutputConfiguration[i].outputCurrentCycle.mode) {
            
            case oneshot:
                // High timer running
                if(moduleOutputConfiguration[i].outputCurrentCycle.durationHigh > 0) {
                    moduleOutputConfiguration[i].outputCurrentCycle.durationHigh--;
                    analogWrite(moduleOutputConfiguration[i].outputPin, moduleOutputConfiguration[i].outputCurrentCycle.levelHigh);
                }

                // Low timer running
                else if (moduleOutputConfiguration[i].outputCurrentCycle.durationLow > 0) {
                    moduleOutputConfiguration[i].outputCurrentCycle.durationLow--;
                    analogWrite(moduleOutputConfiguration[i].outputPin, moduleOutputConfiguration[i].outputCurrentCycle.levelLow);

                    // Low timer expired so turn everything off and back to direct mode (i.e. idle)
                    if(moduleOutputConfiguration[i].outputCurrentCycle.durationLow == 0) {
                        moduleOutputConfiguration[i].outputCurrentCycle = {direct, 0, 0, 0, 0};
                        moduleOutputConfiguration[i].outputNextCycle = {direct, 0, 0, 0, 0};
                        analogWrite(moduleOutputConfiguration[i].outputPin, 0);
                    }
                }
                break;

            case flash:
                // High timer running
                if(moduleOutputConfiguration[i].outputCurrentCycle.durationHigh > 0) {
                    moduleOutputConfiguration[i].outputCurrentCycle.durationHigh--;
                    analogWrite(moduleOutputConfiguration[i].outputPin, moduleOutputConfiguration[i].outputCurrentCycle.levelHigh);
                }

                // Low timer running
                else if (moduleOutputConfiguration[i].outputCurrentCycle.durationLow > 0) {
                    moduleOutputConfiguration[i].outputCurrentCycle.durationLow--;
                    analogWrite(moduleOutputConfiguration[i].outputPin, moduleOutputConfiguration[i].outputCurrentCycle.levelLow);

                    // Low timer expired so set-up next cycle (which could be oneshot or flash)
                    if(moduleOutputConfiguration[i].outputCurrentCycle.durationLow == 0) {
                        moduleOutputConfiguration[i].outputCurrentCycle = moduleOutputConfiguration[i].outputNextCycle;
                    }
                }
                break;

            case direct:
            default:
                // Always set-up next cycle in direct (so output can transition to other modes)
                moduleOutputConfiguration[i].outputCurrentCycle = moduleOutputConfiguration[i].outputNextCycle;
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
    if (((unsigned int) output < LAST_ITEM) && 
        (cycleConfiguration.levelHigh <= PWMRANGE) && 
        (cycleConfiguration.levelLow <= PWMRANGE)) {
        
        // Set-up specific control modes
        switch(cycleConfiguration.mode) {
            
            case oneshot:
                moduleOutputConfiguration[output].outputNextCycle = cycleConfiguration;
                break;

            case flash:
                moduleOutputConfiguration[output].outputNextCycle = cycleConfiguration;
                break;

            case direct:
            default:
                // For direct operations, take control immedately from here
                moduleOutputConfiguration[output].outputNextCycle = cycleConfiguration;
                moduleOutputConfiguration[output].outputCurrentCycle = cycleConfiguration;
                
                // Set-up the PWM output now
                analogWrite(moduleOutputConfiguration[output].outputPin, moduleOutputConfiguration[output].outputCurrentCycle.levelHigh);
                break;
        }
    }
}
