#include <Arduino.h>

#include "reset_ctrl.h"

#include "utils.h"
#include "debug.h"
#include "inputs_cfg.h"
#include "nvm_cfg.h"
#include "wifi.h"
#include "hawkbit_client.h"


// Set the module call interval
#define MODULE_CALL_INTERVAL        RESET_CTRL_CYCLIC_RATE

// Duration to hold reset switch to clear WiFi and reset
#define RESET_CTRL_CLR_WIFI_S       (2)

// Duration to hold reset switch to clear WiFi, NVM and reset
#define RESET_CTRL_CLR_WIFI_NVM_S   (8)

// Fast duration to hold reset switch to cancel clear and reset request
#define RESET_CTRL_CLR_CANCEL_FST_S (2)

// Slow duration to hold reset switch to cancel clear and reset request
#define RESET_CTRL_CLR_CANCEL_SLO_S (5)


// Module name for debug messages
const char* resetControllerModuleName = "resetCtrl";

// Reset state machine state names (must align with the enum)
static const char * resetCtrlStateNames[] = {
    "stmResetIdle",
    "stmReset",
    "stmResetWiFi",
    "stmResetWiFiNvm",
    "stmResetAllowCancel",
    "stmResetAction"
};

// Reset types names (must align with the enum)
static const char *  resetCtrlTypesNames[] = {
    "rstTypeNone",
    "rstTypeReset",
    "rstTypeResetWiFi",
    "rstTypeResetWiFiNvm"
};

// Current requested reset
static resetCtrlTypes resetCtrlRequestedResetType = rstTypeNone;

// Current state
static resetCtrlStm resetCtrlCurrentState = stmResetIdle;

// Switch allowed
static uint8_t resetCtrlResetSwitchEnabled = false;


/**
    Reset handler.
    Does the physical resets immediately / on command.
*/
void restCtrlImmediateHandle(resetCtrlTypes requestedReset) {

    // Debug message
    String debugMessage = (String() + "Rebooting unit");

    switch(requestedReset) {

        case(rstTypeNone):
        case(rstTypeNumberOfTypes):
            break;

        case(rstTypeReset):
            debugMessage = debugMessage + "...";
            debugLog(&debugMessage, resetControllerModuleName, warning);
            break;
        
        case(rstTypeResetWiFi):
            debugMessage = debugMessage + " + clearing WiFi settings...";
            debugLog(&debugMessage, resetControllerModuleName, warning);

            wifiReset();
            break;

        case(rstTypeResetWiFiNvm):
            debugMessage = debugMessage + " + clearing WiFi + NVM settings...";
            debugLog(&debugMessage, resetControllerModuleName, warning);

            nvmClear();
            wifiReset();
            break;
    }
    
    // Perform the physical reboot if a reset type has been specified and is within range
    if ((requestedReset != rstTypeNone) && (requestedReset < rstTypeNumberOfTypes)) {
        ESP.restart();
    }
}


/**
    Reset controller init.
*/
void restCtrlInit(void) {
    
    // Pointer to the RAM mirror
    const nvmCompleteStructure * ramMirrorPtr;

    // Set-up pointer to RAM mirror
    (void) nvmGetRamMirrorPointerRO(&ramMirrorPtr);

    resetCtrlResetSwitchEnabled = ramMirrorPtr->io.resetSwitchEnabled;

    resetCtrlRequestedResetType = rstTypeNone;
    resetCtrlCurrentState = stmResetIdle;
}


/**
    Reset controller state machine.
*/
void restCtrlStateMachine(void) {
    
    // Debug message
    String debugMessage;

    // Last value of the reset switch
    static uint8_t resetSwLastState = LOW;
    
    // Current value of the reset switch
    uint8_t resetSwCurrentState = inputsReadInputByName(resetSw);
    
    // Switch held timer
    static uint32_t switchHeldTimer;
    
    // Last state
    static resetCtrlStm lastState = stmResetIdle;

    // Next state
    resetCtrlStm nextState = resetCtrlCurrentState;

    // If the reset switch is disabled, make sure that the switch status is reset
    if (resetCtrlResetSwitchEnabled == DISABLED) {
        resetSwCurrentState = LOW;
    }

    // Handle the state machine
    switch(resetCtrlCurrentState) {
        
        case(stmResetIdle):
            
            // PRIO 1 - OTA needs a reboot (no option to cancel this)
            if(hawkbitClientGetCurrentState() == stmHawkbitWaitReboot) {
                nextState = stmResetAction;
                resetCtrlRequestedResetType = rstTypeReset;
            }

            // PRIO 2 - Check if a reset request arrives during idle
            else if (resetCtrlRequestedResetType != rstTypeNone) {
                nextState = stmResetAllowCancel;
                switchHeldTimer = SECS_TO_CALLS(RESET_CTRL_CLR_CANCEL_SLO_S);
            }

            // PRIO 3 - Switch transitions to HIGH
            else if ((resetSwLastState == LOW) && (resetSwCurrentState == HIGH)) {
                nextState = stmReset;
                resetCtrlRequestedResetType = rstTypeReset;
                switchHeldTimer = SECS_TO_CALLS(RESET_CTRL_CLR_WIFI_S);
            }

            break;
        
        case(stmReset):

            // Switch held HIGH
            if(resetSwCurrentState == HIGH) {
                
                // Timer running
                if(switchHeldTimer > 0) {
                    switchHeldTimer--;
                }
                
                // Timer elapsed
                else
                {
                    nextState = stmResetWiFi;
                    resetCtrlRequestedResetType = rstTypeResetWiFi;
                    switchHeldTimer = SECS_TO_CALLS(RESET_CTRL_CLR_WIFI_NVM_S);
                }
            }
            
            // Switch released
            else {
                nextState = stmResetAllowCancel;
                switchHeldTimer = SECS_TO_CALLS(RESET_CTRL_CLR_CANCEL_FST_S);
            }

            break;

        case(stmResetWiFi):
            
            // Switch held
            if(resetSwCurrentState == HIGH) {
                
                // Timer running
                if(switchHeldTimer > 0) {
                    switchHeldTimer--;
                }
                
                // Timer elapsed
                else
                {
                    nextState = stmResetWiFiNvm;
                    resetCtrlRequestedResetType = rstTypeResetWiFiNvm;
                }
            }
            
            // Switch released
            else {
                nextState = stmResetAllowCancel;
                switchHeldTimer = SECS_TO_CALLS(RESET_CTRL_CLR_CANCEL_SLO_S);
            }

            break;

        case(stmResetWiFiNvm):
            
            // Switch released
            if(resetSwCurrentState != HIGH) {
                nextState = stmResetAllowCancel;
                switchHeldTimer = SECS_TO_CALLS(RESET_CTRL_CLR_CANCEL_SLO_S);
            }

            break;

         case(stmResetAllowCancel):

            // First transition to this state
            if(resetCtrlCurrentState != lastState) {
                debugMessage = String() + "Reset will occurr in " + CALLS_TO_SECS(switchHeldTimer) + "s";
                debugLog(&debugMessage, resetControllerModuleName, warning);
            }
            
            // Switch released
            if(resetSwCurrentState == LOW) {
                
                // Timer running
                if(switchHeldTimer > 0) {
                    switchHeldTimer--;
                }

                // Timer elapsed
                else
                {
                    nextState = stmResetAction;
                }
            }

            // Switch pressed
            else {
                nextState = stmResetIdle;
                resetCtrlRequestedResetType = rstTypeNone;
            }

            break;

        case(stmResetAction):
            // There should be no coming back from the following call
            restCtrlImmediateHandle(resetCtrlRequestedResetType);
            nextState = stmResetIdle;
            resetCtrlRequestedResetType = rstTypeNone;
            break;

        default:
            nextState = stmResetIdle;
            resetSwLastState = HIGH;
            resetCtrlRequestedResetType = rstTypeNone;
            break;
    }    
 
    // State change
    if (resetCtrlCurrentState != nextState) {
        debugMessage = String() + "State change to " + resetCtrlStateNames[nextState] + " (reqest type " + resetCtrlTypesNames[resetCtrlRequestedResetType] + ")";
        debugLog(&debugMessage, resetControllerModuleName, info);
    }
    
    // Update last states and current states (in this order)
    lastState = resetCtrlCurrentState;
    resetCtrlCurrentState = nextState;
    resetSwLastState = resetSwCurrentState;
}


/**
    Set the reset request.
    Only allow updates with state machine in stmIdle.

    @param[in]     reqeustedRest reset type requested.
*/
void restCtrlSetResetRequest(const resetCtrlTypes reqeustedRest) {
    
    // State machine in stmIdle and request in bounds
    if((resetCtrlCurrentState == stmResetIdle) && (reqeustedRest < rstTypeNumberOfTypes)) {
        resetCtrlRequestedResetType = reqeustedRest;
    }
}


/**
    Get the current requested reset type.

    @return        current requested reset type.
*/
resetCtrlTypes restCtrlGetResetRequest(void) {
    return(resetCtrlRequestedResetType);
}


/**
    Get the current state of the state machine.

    @return        state machine state.
*/
resetCtrlStm restCtrlGetResetState(void) {
    return(resetCtrlCurrentState);
}
