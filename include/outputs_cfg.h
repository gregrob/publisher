#ifndef OUTPUTS_CFG_H
#define OUTPUTS_CFG_H

#include "outputs.h"

// Output pin for configuration mode LED (no high transition on this pin on boot)
#define OUTPUTS_PIN_MAP_CFG_MODE    (D2)

// Output pin for the run mode LED (there is a small 100ms HIGH pulse during boot)
#define OUTPUTS_PIN_MAP_RUN_MODE    (D5)

// Output pin for the alarm control (no high transition on this pin on boot)
#define OUTPUTS_PIN_MAP_ALARM_CTRL  (D1)


// Enumeration for outputs (index must align into configuration structure)
enum outputIndex {
    configMode              = 0,
    runMode                 = 1,
    alarmCtrl               = 2,
    garageDoorCtrl          = 2,
    
    outputsNumberOfTypes
};


/**
    Set-up read pointer to the output configuration.
        
    @param[in]     activeOutputConfig pointer for the outputs configuration.
    @return        size of the outputs configuration.
*/
const uint32_t outputsGetConfigPointerRO(const outputConfiguration ** activeOutputConfig);

/**
    Set-up read / write pointer to the output configuration.
        
    @param[in]     activeOutputConfig pointer for the outputs configuration.
    @return        size of the outputs configuration.
*/
const uint32_t outputsGetConfigPointerRW(outputConfiguration ** activeOutputConfig);

/**
    Control a named output.
  
    @param[in]     name name of the output to control.
    @param[in]     cycleConfig how the output should be controlled.
*/
void outputsSetOutputByName(const outputIndex name, const outputCycleConfig cycleConfig);

#endif
