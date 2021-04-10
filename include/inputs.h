#ifndef INPUTS_H
#define INPUTS_H


// Call rate for the cyclic taks
#define INPUTS_CYCLIC_RATE (10)


// Enumeration for inputs (index must align into configuration structure)
enum inputIndex {
    resetSw       = 0,
    IP_LAST_ITEM  = 1
};


/**
    Initialise the inputs module and initial states of the pins.
*/
void inputsInit(void);

/**
    Cycle task to handle input debouncing.
*/
void inputsCyclicTask(void);

/**
    Read a debounced input.
    
    @param[in]     input index of the input to read
    @return        the current debounced value of the pin being read
*/
int inputsReadInput(const inputIndex input);

#endif
