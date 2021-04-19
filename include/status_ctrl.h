#ifndef STATUS_CTRL_H
#define STATUS_CTRL_H

// Call rate for the cyclic taks (in mS)
#define STATUS_CTRL_CYCLIC_RATE           (100)


// Status state machine
enum statusCtrlStm {
    stmStatusIdle,
    stmStatusWiFiConnected,
    stmStatusReset
};


/**
    Status controller init.
*/
void statusCtrlInit(void);

/**
    Status controller state machine.
*/
void statusCtrlStateMachine(void);

/**
    Get the current state of the state machine.

    @return        state machine state.
*/
statusCtrlStm statusCtrlGetResetState(void);

#endif
