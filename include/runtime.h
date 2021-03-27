#ifndef RUNTIME_H
#define RUNTIME_H

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
    Get the average runtime in uS.
*/
float getAverageRuntimeuS(void);

/**
    Get the peak runtime in uS.
*/
unsigned long getPeakRuntimeuS(void);

#endif