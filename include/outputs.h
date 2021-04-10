#ifndef OUTPUTS_H
#define OUTPUTS_H


// Call rate for the cyclic taks
#define OUTPUTS_CYCLIC_RATE (100)


// Enumeration for outputs (index must align into configuration structure)
enum outputIndex {
    wifiCfg       = 0,
    wifiRun       = 1,
    alarmCtrl     = 2,
    OP_LAST_ITEM  = 3
};

// Enumerator for output control types
enum outputControlType {
    direct    = 0,
    oneshot   = 1,
    flash     = 2
};

// Structure for output cycle configuration
typedef struct {
    outputControlType   mode;
    unsigned int        levelHigh;
    unsigned int        durationHigh;
    unsigned int        levelLow;
    unsigned int        durationLow;
} outputCycleConfiguration;


/**
    Initialise the outputs module and initial states of the pins.
*/
void outputsInit(void);

/**
    Cycle task to handle output states.
*/
void outputsCyclicTask(void);

/**
    Control an output.
    
    @param[in]     output index of the output to control
    @param[in]     cycleConfiguration how the output should be controlled
*/
void outputsSetOutput(const outputIndex output, const outputCycleConfiguration cycleConfiguration);

#endif
