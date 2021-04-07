#ifndef RUNTIME_H
#define RUNTIME_H

// Structure for runtime data
typedef struct {
  const char*           runtimeName;
  const unsigned long*  runtimeContents;
} runtimeData;


/**
    Measure average runtime.
    Measures the runtime over a set number of calls and calculate the average.
    Limitation is that the first average is higher becuse initial loop calls longer.
*/
void runtimeMeasureAverageuS(void);

/**
    Measure peak runtime.
*/
void runtimeMeasurePeakuS(void);

/**
    Transmit a runtime message.
    No processing of the message here.
*/
void runtimeTransmitRuntimeMessage(void);

#endif
