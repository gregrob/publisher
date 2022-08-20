// THIS IS TEST CODE, NOT VALIDATED


#ifndef ULTRASONICS_CTRL_H
#define ULTRASONICS_CTRL_H

// Call rate for the cyclic taks (in mS)
#define ULTRASONICS_CTRL_CYCLIC_RATE           (50)


// Ultrasonic controller state machine
enum ultrasonicsCtrlStm {
    stmUltrasonicsRestart,
    stmUltrasonicsIdle,
    stmUltrasonicsMeasure,
};


/**
    Ultrasonics controller init.
*/
void ultrasonicsCtrlInit(void);

/**
    Ultrasonics controller state machine.
*/
void ultrasonicCtrlStateMachine(void);

#endif
