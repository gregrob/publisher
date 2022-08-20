#ifndef GARAGE_DOOR_H
#define GARAGE_DOOR_H

// Call rate for the cyclic taks
#define GARAGE_DOOR_CYCLIC_RATE (100)


// Ajar states
typedef enum {
    garageDoorStateAjar     = 0,
    garageDoorStateClosed   = 1,
    garageDoorStateUnknown  = 2

} garageDoorAjarStates;

// Structure for garage door status data
typedef struct {
    const char*                  garageDoorStateStringName;
    const char*                  garageDoorStateStringPtr;
    const char*                  garageDoorStateName;
    const garageDoorAjarStates*  garageDoorStatePtr;
} garageDoorStatusData;


/**
    Garage door module init.
*/
void garageDoorInit(void);

/**
    Fire the one shot on the garage door to open / close.
*/
void garageDoorFireOneShot(void);

/**
    Cycle task for the garage door.
*/
void garageDoorCyclicTask(void);

/**
    Transmit a garage door status message.
    No processing of the message here.
*/ 
void garageDoorTransmitDoorStatusMessage(void);

/**
    Transmit a ALL garage door messages.
    No processing of the message here.
*/ 
void garageDoorTransmitAlarmAllMessage(void);

#endif
