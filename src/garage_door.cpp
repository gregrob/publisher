#include <Arduino.h>

#include "garage_door.h"
#include "debug.h"
#include "messages_tx.h"
#include "outputs_cfg.h"
#include "inputs_cfg.h"

// Name for the state
#define GARAGE_DOOR_AJAR_STATE_STRING       ("ajar state")

// Name for the sounding
#define GARAGE_DOOR_AJAR_STATE_ENUM         ("ajar state enum")

// Maximum size for the ajar strings used in garageDoorAjarStateNames
#define GARAGE_DOOR_AJAR_STRING_MAX_SIZE    (10)


// Garage door ajar state names
static const char * garageDoorAjarStateNames[] {
    "ajar",
    "closed",
    "unknown"
};

// Module name for debug messages
static const char* garageDoorModuleName = "garageDoor";

// Garage door ajar state raw
static garageDoorAjarStates garageDoorAjarState = garageDoorStateUnknown;

// Garage door ajar state string
static char garageDoorAjarStateString[GARAGE_DOOR_AJAR_STRING_MAX_SIZE];

// Alarm status data
static  garageDoorStatusData garageDoorStatusDataTable = {GARAGE_DOOR_AJAR_STATE_STRING,  garageDoorAjarStateString,
                                                          GARAGE_DOOR_AJAR_STATE_ENUM,    &garageDoorAjarState
};  


/**
    Update the garage door ajar status string.
*/
static void garageDoorUpdateAjarString(void) {
    strncpy(garageDoorAjarStateString, garageDoorAjarStateNames[garageDoorAjarState], (sizeof(garageDoorAjarStateString) / sizeof(char)));
}

/**
    Garage door module init.
*/
void garageDoorInit(void) {

    // Debug message
    String debugMessage;
    
    debugMessage = String() + "Init";
    debugLog(&debugMessage, garageDoorModuleName, info);
    
    garageDoorUpdateAjarString();
}

/**
    Fire the one shot on the garage door to open / close.
*/
void garageDoorFireOneShot(void) {
    outputsSetOutputByName(garageDoorCtrl, {oneshot, PWMRANGE, 5, 0, 5});
}

/**
    Check and update garage door ajar state.
*/
static void garageDoorAjarCheck(void) {   
    
    // Debug message
    String debugMessage;
    
    garageDoorAjarStates garageDoorAjarNextState;

    // Convert pin state to ajar enumeration
    if(inputsReadInputByName(garageDoorAjar) == LOW) {
        garageDoorAjarNextState = garageDoorStateAjar;
    }
    
    else if (inputsReadInputByName(garageDoorAjar) == HIGH){
        garageDoorAjarNextState = garageDoorStateClosed;
    }

    else {
        garageDoorAjarNextState = garageDoorStateUnknown;
    }

    // State change of sounder so send out mqtt
    if(garageDoorAjarNextState != garageDoorAjarState) {
        garageDoorAjarState = garageDoorAjarNextState;
        garageDoorUpdateAjarString();

        debugMessage = String() + "Garage door ajar state changed to " + garageDoorAjarStateString + " (" + garageDoorAjarState + ")";
        debugLog(&debugMessage, garageDoorModuleName, info);

        garageDoorTransmitDoorStatusMessage();
    }
}

/**
    Cycle task for the garage door.
*/
void garageDoorCyclicTask(void) {

    // Check if the garage door is ajar
    garageDoorAjarCheck();    
}

/**
    Transmit a garage door status message.
    No processing of the message here.
*/ 
void garageDoorTransmitDoorStatusMessage(void) {
    messsagesTxGarageStatusMessage(&garageDoorStatusDataTable);
}

/**
    Transmit a ALL garage door messages.
    No processing of the message here.
*/ 
void garageDoorTransmitAlarmAllMessage(void) {
    garageDoorTransmitDoorStatusMessage();
}
