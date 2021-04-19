#include <Arduino.h>

#include "status_ctrl.h"

#include "debug.h"
#include "outputs_cfg.h"
#include "nvm_cfg.h"
#include "reset_ctrl.h"
#include "wifi.h"


// Set the module call interval
#define MODULE_CALL_INTERVAL        STATUS_CTRL_CYCLIC_RATE


// Status state machine state names (must align with the enum)
static const char * statusCtrlStateNames[] = {
    "stmStatusIdle",
    "stmStatusWiFiConnected",
    "stmStatusReset"
};

// Current state
static statusCtrlStm statusCtrlCurrentState = stmStatusIdle;

// Current LED brightness - Config LED
static uint16_t currentLedBrightnessConfigMode = 0;

// Current LED brightness - Run LED
static uint16_t currentLedBrightnessRunMode = 0;


/**
    Status controller init.
*/
void statusCtrlInit(void) {
    // Pointer to the RAM mirror
    const nvmCompleteStructure * ramMirrorPtr;

    // Set-up pointer to RAM mirror
    (void) nvmGetRamMirrorPointerRO(&ramMirrorPtr);


    currentLedBrightnessConfigMode = ramMirrorPtr->io.ledBrightnessConfigMode;
    currentLedBrightnessRunMode = ramMirrorPtr->io.ledBrightnessRunMode;

    statusCtrlCurrentState = stmStatusIdle;
}


/**
    Status controller state machine.
*/
void statusCtrlStateMachine(void) {
    
    // Debug message
    String debugMessage;

    // Last state
    static statusCtrlStm lastState = stmStatusIdle;

    // Next state
    statusCtrlStm nextState = statusCtrlCurrentState;

    // Last reset request
    static resetCtrlTypes lastResetRequest = rstTypeNone;
    
    // Current reset request
    resetCtrlTypes currentResetRequest = restCtrlGetResetRequest();

    // Current WiFi status
    bool wifiConnected = wifiIsConnected();
    
    // Handle the state machine
    switch(statusCtrlCurrentState) {
       
        case(stmStatusIdle):

            // State change so handle LED's
            if(statusCtrlCurrentState != lastState) {
                // Turn off all LED's
                outputsSetOutputByName(configMode, {direct, 0, 0, 0, 0});
                outputsSetOutputByName(runMode,    {direct, 0, 0, 0, 0});
            }

            // PRIO 1 - Reset commanded
            if (currentResetRequest != rstTypeNone) {
                nextState = stmStatusReset;
            }
            // PRIO 2 - WiFi connected
            else if(wifiConnected == true) {
                nextState = stmStatusWiFiConnected;
            }
            
            break;

        case(stmStatusWiFiConnected):
            
            // State change so handle LED's
            if(statusCtrlCurrentState != lastState) {
                // Turn off all LED's, cancel previous flash cycle so LEDs turn off in parallel
                outputsSetOutputByName(configMode, {direct, 0, 0, 0, 0});
                outputsSetOutputByName(runMode,    {direct, 0, 0, 0, 0});

                // Slow flash run LED
                outputsSetOutputByName(runMode, {flash, currentLedBrightnessRunMode, 1, 0, 19});
            }

            // PRIO 1 - Reset commanded
            if (currentResetRequest != rstTypeNone) {
                nextState = stmStatusReset;
            }
            // PRIO 2 - WiFi disconnected
            else if(wifiConnected != true) {
                nextState = stmStatusIdle;
            }

            break;

        case(stmStatusReset):
            
            // State change OR reset request change so handle LED's
            if((statusCtrlCurrentState != lastState)||(currentResetRequest != lastResetRequest)) {
            
                switch(currentResetRequest) {
                
                    case(rstTypeReset):
                        // Turn off all LED's, cancel previous flash cycle
                        outputsSetOutputByName(configMode, {direct, 0, 0, 0, 0});
                        outputsSetOutputByName(runMode,    {direct, 0, 0, 0, 0});

                        // Rapid flash run LED
                        outputsSetOutputByName(runMode, {flash, currentLedBrightnessRunMode, 1, 0, 1});
                        break;
                    
                    case(rstTypeResetWiFi):
                        // Turn off all LED's, cancel previous flash cycle
                        outputsSetOutputByName(configMode, {direct, 0, 0, 0, 0});
                        outputsSetOutputByName(runMode,    {direct, 0, 0, 0, 0});

                        // Rapid flash config LED
                        outputsSetOutputByName(configMode, {flash, currentLedBrightnessConfigMode, 1, 0, 1});
                        break;

                    case(rstTypeResetWiFiNvm):
                        // Turn off all LED's, cancel previous flash cycle
                        outputsSetOutputByName(configMode, {direct, 0, 0, 0, 0});
                        outputsSetOutputByName(runMode,    {direct, 0, 0, 0, 0});

                        // Rapid flash both LED's
                        outputsSetOutputByName(runMode,    {flash, currentLedBrightnessRunMode,    1, 0, 1});
                        outputsSetOutputByName(configMode, {flash, currentLedBrightnessConfigMode, 1, 0, 1});
                        break;

                    case(rstTypeNone):
                    default:
                        break;
                }
            }
            // PRIO 1 - Reset command cleared AND WiFi connected
            if ((currentResetRequest == rstTypeNone)&&(wifiConnected == true)) {
                nextState = stmStatusWiFiConnected;
            }
            // PRIO 2 - Reset command cleared AND WiFi disconnected
            else if ((currentResetRequest == rstTypeNone)&&(wifiConnected != true)) {
                nextState = stmStatusIdle;
            }

            break;

        default:
            nextState = stmStatusIdle;
            break;
    }


    // State change
    if (statusCtrlCurrentState != nextState) {
        debugMessage = String() + "statusCtrl changed to state to " + statusCtrlStateNames[nextState] + ".";
        debugLog(&debugMessage, info);
    }
    
    // Update last states and current states (in this order)
    lastState = statusCtrlCurrentState;
    statusCtrlCurrentState = nextState;
    lastResetRequest = currentResetRequest;
}


/**
    Get the current state of the state machine.

    @return        state machine state.
*/
statusCtrlStm statusCtrlGetResetState(void) {
    return(statusCtrlCurrentState);
}
