#include <Arduino.h>

#include "outputs.h"
#include "outputs_cfg.h"


// Output configuraiton structure
static outputConfiguration outputConfig[] = {{"configMode",  OUTPUTS_PIN_MAP_CFG_MODE,   LOW, {direct, 0, 0, 0, 0}, {direct, 0, 0, 0, 0}},
                                             {"runMode",     OUTPUTS_PIN_MAP_RUN_MODE,   LOW, {direct, 0, 0, 0, 0}, {direct, 0, 0, 0, 0}},
                                             {"alarmCtrl",   OUTPUTS_PIN_MAP_ALARM_CTRL, LOW, {direct, 0, 0, 0, 0}, {direct, 0, 0, 0, 0}}
};

// Output configuration size (in elements)
static const uint32_t outputsConfigSizeElements = (sizeof(outputConfig) / sizeof(outputConfig[0]));


/**
    Set-up read pointer to the output configuration.
        
    @param[in]     activeOutputConfig pointer for the outputs configuration.
    @return        size of the outputs configuration.
*/
const uint32_t outputsGetConfigPointerRO(const outputConfiguration ** activeOutputConfig) {
    *activeOutputConfig = &outputConfig[0];
    return(outputsConfigSizeElements);
}

/**
    Set-up read / write pointer to the output configuration.
        
    @param[in]     activeOutputConfig pointer for the outputs configuration.
    @return        size of the outputs configuration.
*/
const uint32_t outputsGetConfigPointerRW(outputConfiguration ** activeOutputConfig) {
    *activeOutputConfig = &outputConfig[0];
    return(outputsConfigSizeElements);
}

/**
    Control a named output.
  
    @param[in]     name name of the output to control.
    @param[in]     cycleConfig how the output should be controlled.
*/
void outputsSetOutputByName(const outputIndex name, const outputCycleConfig cycleConfig) {

    outputsSetOutputByIndex(name, cycleConfig);
}
