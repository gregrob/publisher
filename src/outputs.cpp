#include <Arduino.h>

#include "outputs.h"
#include "outputs_cfg.h"


/**
    Initialise the outputs module and initial states of the pins.
*/
void outputsInit(void) {

    // Pointer to the outputs configuration
    const outputConfiguration * outputsConfigPtr;

    // Set-up pointer to configuration and get its size
    const uint32_t outputsConfigSize = outputsGetConfigPointerRO(&outputsConfigPtr);
    
    // Initialise all outputs to their initial state
    for(unsigned int i = 0; i < outputsConfigSize; i++) {
        analogWrite(outputsConfigPtr[i].pin, outputsConfigPtr[i].initialLevel);
        pinMode(outputsConfigPtr[i].pin, OUTPUT);
    }

}


/**
    Cycle task to handle output states.
*/
void outputsCyclicTask(void) {
    
    // Pointer to the outputs configuration
    outputConfiguration * outputsConfigPtr;

    // Set-up pointer to configuration and get its size
    const uint32_t outputsConfigSize = outputsGetConfigPointerRW(&outputsConfigPtr);

    // Handle all outputs to their initial state
    for(unsigned int i = 0; i < outputsConfigSize; i++) {

        // Handle the individual output modes
        switch(outputsConfigPtr[i].currentCycle.mode) {
            
            case oneshot:
                // High timer running
                if(outputsConfigPtr[i].currentCycle.durationHigh > 0) {
                    outputsConfigPtr[i].currentCycle.durationHigh--;
                    analogWrite(outputsConfigPtr[i].pin, outputsConfigPtr[i].currentCycle.levelHigh);
                }

                // Low timer running
                else if (outputsConfigPtr[i].currentCycle.durationLow > 0) {
                    outputsConfigPtr[i].currentCycle.durationLow--;
                    analogWrite(outputsConfigPtr[i].pin, outputsConfigPtr[i].currentCycle.levelLow);

                    // Low timer expired so turn everything off and back to direct mode (i.e. idle)
                    if(outputsConfigPtr[i].currentCycle.durationLow == 0) {
                        outputsConfigPtr[i].currentCycle = {direct, 0, 0, 0, 0};
                        outputsConfigPtr[i].nextCycle = {direct, 0, 0, 0, 0};
                        analogWrite(outputsConfigPtr[i].pin, 0);
                    }
                }
                break;

            case flash:
                // High timer running
                if(outputsConfigPtr[i].currentCycle.durationHigh > 0) {
                    outputsConfigPtr[i].currentCycle.durationHigh--;
                    analogWrite(outputsConfigPtr[i].pin, outputsConfigPtr[i].currentCycle.levelHigh);
                }

                // Low timer running
                else if (outputsConfigPtr[i].currentCycle.durationLow > 0) {
                    outputsConfigPtr[i].currentCycle.durationLow--;
                    analogWrite(outputsConfigPtr[i].pin, outputsConfigPtr[i].currentCycle.levelLow);

                    // Low timer expired so set-up next cycle (which could be oneshot or flash)
                    if(outputsConfigPtr[i].currentCycle.durationLow == 0) {
                        outputsConfigPtr[i].currentCycle = outputsConfigPtr[i].nextCycle;
                    }
                }
                break;

            case direct:
            default:
                // Always set-up next cycle in direct (so output can transition to other modes)
                outputsConfigPtr[i].currentCycle = outputsConfigPtr[i].nextCycle;
                break;
        }
    }
}


/**
    Control a indexed output.
  
    @param[in]     output index of the output to control.
    @param[in]     cycleConfig how the output should be controlled.
*/
void outputsSetOutputByIndex(const uint32_t index, const outputCycleConfig cycleConfig) {

    // Pointer to the outputs configuration
    outputConfiguration * outputsConfigPtr;

    // Set-up pointer to configuration
    (void) outputsGetConfigPointerRW(&outputsConfigPtr);

    // Make sure a valid output is selected and levels are within range
    if (((unsigned int) index < outputIndex::outputsNumberOfTypes) && 
        (cycleConfig.levelHigh <= PWMRANGE) && 
        (cycleConfig.levelLow <= PWMRANGE)) {
        
        // Set-up specific control modes
        switch(cycleConfig.mode) {
            
            case oneshot:
                outputsConfigPtr[index].nextCycle = cycleConfig;
                break;

            case flash:
                outputsConfigPtr[index].nextCycle = cycleConfig;
                break;

            case direct:
            default:
                // For direct operations, take control immedately from here
                outputsConfigPtr[index].nextCycle = cycleConfig;
                outputsConfigPtr[index].currentCycle = cycleConfig;
                
                // Set-up the PWM output now
                analogWrite(outputsConfigPtr[index].pin, outputsConfigPtr[index].currentCycle.levelHigh);
                break;
        }
    }
}
