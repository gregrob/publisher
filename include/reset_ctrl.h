#ifndef RESET_CTRL_H
#define RESET_CTRL_H

// Call rate for the cyclic taks (in mS)
#define RESET_CTRL_CYCLIC_RATE           (100)


// Reset state machine
enum resetCtrlStm {
    stmResetIdle,
    stmReset,
    stmResetWiFi,
    stmResetWiFiNvm,
    stmResetAllowCancel,
    stmResetAction
};

// Reset types
enum resetCtrlTypes {
    rstTypeNone          = 0,
    rstTypeReset         = 1,
    rstTypeResetWiFi     = 2,
    rstTypeResetWiFiNvm  = 3,

    rstTypeNumberOfTypes
};

/**
    Reset handler.
    Does the physical resets immediately / on command.
*/
void restCtrlImmediateHandle(resetCtrlTypes requestedReset);

/**
    Reset controller init.
*/
void restCtrlInit(void);

/**
    Reset controller state machine.
*/
void restCtrlStateMachine(void);

/**
    Set the reset request.
    Only allow updates with state machine in stmIdle.

    @param[in]     reqeustedRest reset type requested.
*/
void restCtrlSetResetRequest(const resetCtrlTypes reqeustedRest);

/**
    Get the current requested reset type.

    @return        current requested reset type.
*/
resetCtrlTypes restCtrlGetResetRequest(void);

/**
    Get the current state of the state machine.

    @return        state machine state.
*/
resetCtrlStm restCtrlGetResetState(void);

#endif
