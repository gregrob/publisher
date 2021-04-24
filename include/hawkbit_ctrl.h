#ifndef HAWKBIT_CTRL_H
#define HAWKBIT_CTRL_H

// Call rate for the cyclic taks (in mS)
#define HAWKBIT_CTRL_CYCLIC_RATE           (100)


// Hawkbit state machine
enum hawkbitCtrlStm {
    stmHawkbitRestart,
    stmHawkbitIdle,
    stmHawkbitPoll,
    stmHawkbitCancel,
    stmHawkbitCancelAck,
    stmHawkbitDeploy,
    stmHawkbitDeployAckFail,
    stmHawkbitConfig
};


/**
    Hawkbit controller init.
*/
void hawkbitCtrlInit(void);

/**
    Hawkbit controller state machine.
*/
void hawkbitCtrlStateMachine(void);

/**
    Get the current state of the hawkbit state machine.

    @return        state machine state.
*/
hawkbitCtrlStm hawkbitCtrlGetCurrentState(void);

#endif
