#ifndef INPUTS_H
#define INPUTS_H

// Call rate for the cyclic taks
#define INPUTS_CYCLIC_RATE                 (10)

// Input debounce timer - typical input @40ms
#define INPUTS_DEBOUNCE_TIMER_RESET_TYP    (4)


// Structure for input configuration data
typedef struct {
    const char *           name;
    const uint8_t          pin;
    const uint8_t          inverted;
        
    const uint16_t         debounceTimerReset;
    uint16_t               debounceTimer;

    const uint8_t          initialValue;
    uint8_t                lastValue;
    uint8_t                currentValue;
    uint8_t                debouncedValue;
} inputConfiguration;


/**
    Initialise the inputs module and initial states of the pins.
*/
void inputsInit(void);

/**
    Cycle task to handle input debouncing.
*/
void inputsCyclicTask(void);

/**
    Read a indexed input (debounced).
  
    @param[in]     index index of the input to read
    @return        debounced value at the input.
*/
uint8_t inputsReadInputByIndex(const uint32_t index);

#endif
