#include <Arduino.h>

#include "reset_ctrl.h"

#include "utils.h"
#include "debug.h"
#include "inputs_cfg.h"
#include "nvm_cfg.h"
#include "wifi.h"


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


// Reset state machine state names (must align with the enum)
static const char * resetCtrlStateNames[] = {
    "stmIdle",
    "stmReset",
    "stmResetWiFi",
    "stmResetWiFiNvm",
    "stmAllowCancel",
    "stmAction"
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
static resetCtrlStm resetCtrlCurrentState = stmIdle;

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
            debugLog(&debugMessage, warning);
            break;
        
        case(rstTypeResetWiFi):
            debugMessage = debugMessage + " + clearing WiFi settings...";
            debugLog(&debugMessage, warning);

            wifiReset();
            break;

        case(rstTypeResetWiFiNvm):
            debugMessage = debugMessage + " + clearing WiFi + NVM settings...";
            debugLog(&debugMessage, warning);

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
    resetCtrlCurrentState = stmIdle;
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
    
    // Next state
    static resetCtrlStm lastState = stmIdle;

    // Next state
    resetCtrlStm nextState = resetCtrlCurrentState;

    // If the reset switch is disabled, make sure that the switch status is reset
    if (resetCtrlResetSwitchEnabled == DISABLED) {
        resetSwCurrentState = LOW;
    }

    // Handle the state machine
    switch(resetCtrlCurrentState) {
        
        case(stmIdle):
            
            // PRIO 1 - Check if a reset request arrives during idle
            if (resetCtrlRequestedResetType != rstTypeNone) {
                nextState = stmAllowCancel;
                switchHeldTimer = SECS_TO_CALLS(RESET_CTRL_CLR_CANCEL_SLO_S);
            }

            // PRIO 2 - Switch transitions to HIGH
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
                nextState = stmAllowCancel;
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
                nextState = stmAllowCancel;
                switchHeldTimer = SECS_TO_CALLS(RESET_CTRL_CLR_CANCEL_SLO_S);
            }

            break;

        case(stmResetWiFiNvm):
            
            // Switch released
            if(resetSwCurrentState != HIGH) {
                nextState = stmAllowCancel;
                switchHeldTimer = SECS_TO_CALLS(RESET_CTRL_CLR_CANCEL_SLO_S);
            }

            break;

         case(stmAllowCancel):

            // First transition to this state
            if(resetCtrlCurrentState != lastState) {
                debugMessage = String() + "resetCtrl reset will occurr in " + CALLS_TO_SECS(switchHeldTimer) + "s.";
                debugLog(&debugMessage, warning);
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
                    nextState = stmAction;
                }
            }

            // Switch pressed
            else {
                nextState = stmIdle;
                resetCtrlRequestedResetType = rstTypeNone;
            }

            break;

        case(stmAction):
            // There should be no coming back from the following call
            restCtrlImmediateHandle(resetCtrlRequestedResetType);
            nextState = stmIdle;
            resetCtrlRequestedResetType = rstTypeNone;
            break;

        default:
            nextState = stmIdle;
            resetSwLastState = HIGH;
            resetCtrlRequestedResetType = rstTypeNone;
            break;
    }    
 
    // State change
    if (resetCtrlCurrentState != nextState) {
        debugMessage = String() + "resetCtrl changed to state to " + resetCtrlStateNames[nextState] + " (reqest type " + resetCtrlTypesNames[resetCtrlRequestedResetType] + ").";
        debugLog(&debugMessage, info);
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
    if((resetCtrlCurrentState == stmIdle) && (reqeustedRest < rstTypeNumberOfTypes)) {
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
