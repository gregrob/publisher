#ifndef OUTPUTS_H
#define OUTPUTS_H

// Call rate for the cyclic taks
#define OUTPUTS_CYCLIC_RATE (100)


// Output control types
enum outputControlType {
    direct    = 0,
    oneshot   = 1,
    flash     = 2
};

// Structure for output cycle configuration
typedef struct {
    outputControlType   mode;

    uint16_t            levelHigh;
    uint16_t            durationHigh;
    uint16_t            levelLow;
    uint16_t            durationLow;
} outputCycleConfig;

// Structure for output configuration data
typedef struct {
    const char *        name;
    const uint8_t       pin;

    const uint16_t      initialLevel;    
    outputCycleConfig   currentCycle;
    outputCycleConfig   nextCycle;
} outputConfiguration;


/**
    Initialise the outputs module and initial states of the pins.
*/
void outputsInit(void);

/**
    Cycle task to handle output states.
*/
void outputsCyclicTask(void);

/**
    Control a indexed output.
  
    @param[in]     output index of the output to control.
    @param[in]     cycleConfig how the output should be controlled.
*/
void outputsSetOutputByIndex(const uint32_t index, const outputCycleConfig cycleConfig);

#endif
