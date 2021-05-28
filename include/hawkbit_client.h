#ifndef HAWKBIT_CLIENT_H
#define HAWKBIT_CLIENT_H

// Call rate for the cyclic taks (in mS)
#define HAWKBIT_CLIENT_CYCLIC_RATE           (100)


// Hawkbit state machine
enum hawkbitClientStm {
    stmHawkbitRestart,
    stmHawkbitIdle,
    stmHawkbitPoll,
    stmHawkbitCancel,
    stmHawkbitCancelAck,
    stmHawkbitDeploy,
    stmHawkbitDeployAck,
    stmHawkbitWaitReboot,
    stmHawkbitConfig
};


/**
    Hawkbit controller init.
*/
void hawkbitClientInit(void);

/**
    Hawkbit controller state machine.
*/
void hawkbitClientStateMachine(void);

/**
    Get the current state of the hawkbit state machine.

    @return        state machine state.
*/
hawkbitClientStm hawkbitClientGetCurrentState(void);

#endif
