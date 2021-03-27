#include <Arduino.h>

#include "runtime.h"

// The number of cycles to measure average runtime over
#define AVERAGE_RUNTIME_CYCLES    (1000)

// Number of calls the peak runtime measurement should be ignored after boot
#define PEAK_RUNTIME_IGNORE_CALLS (4)

// Average runtime
static float averageRuntimeuS = 0;

// Peak runtime
static unsigned long peakRuntimeuS = 0;

/**
    Measure average runtime.
    Measures the runtime over a set number of calls and calculate the average.
    Limitation is that the first average is higher becuse initial loop calls longer.
*/
void runtimeMeasureAverageuS(void) {

    // Number of calls to average over
    static unsigned int loopCounter = AVERAGE_RUNTIME_CYCLES;   

    // Snapshot time when loop timing measurement started (in uS)
    static unsigned long startTimeuS = micros();

    // Snapshot the current time (in uS)
    unsigned long currentTimeuS = micros();

    // Average cycle has completed so take measurement
    if (loopCounter == 0) {
                
        // Calculate the average runtime
        averageRuntimeuS = ((float)currentTimeuS - (float)startTimeuS) / (float)AVERAGE_RUNTIME_CYCLES;
        
        // Reset the loop counters and start time snapshot
        loopCounter = AVERAGE_RUNTIME_CYCLES;
        startTimeuS = currentTimeuS;

        // DEBUG
        //Serial1.println(String() + "AVERAGE " + averageRuntimeuS + "uS, PEAK " + (peakRuntimeuS/1000) + "mS");
    }

    else {
        loopCounter--;
    }
}

/**
    Measure peak runtime.
*/
void runtimeMeasurePeakuS(void) {

    // Snapshot - current time (uS)
    unsigned long curentTimeSnapshotuS = micros();
    
    // Snapshot - time when function was last called (uS)
    static unsigned long lastCallTimeSnapshotuS = 0;

    // Time elapsed since function was last called (uS)
    unsigned long timeElapsedSinceLastCalluS = curentTimeSnapshotuS - lastCallTimeSnapshotuS;

    // Number of calls the peak runtime measurement should be ignored after boot
    static unsigned char ignorePeakRuntimeMeasurement = PEAK_RUNTIME_IGNORE_CALLS;

    // Start processing peak runtime measurements
    if (ignorePeakRuntimeMeasurement == 0) {
        
        // Update peak runtime if the current runtime is bigger
        if (timeElapsedSinceLastCalluS > peakRuntimeuS) {
            peakRuntimeuS = timeElapsedSinceLastCalluS;
        }
    }

    else {
        ignorePeakRuntimeMeasurement--;
    }

    // Update the last call time snapshot
    lastCallTimeSnapshotuS = curentTimeSnapshotuS;
}

/**
    Get the average runtime in uS.
*/
float getAverageRuntimeuS(void) {
    return(averageRuntimeuS);
}

/**
    Get the peak runtime in uS.
*/
unsigned long getPeakRuntimeuS(void) {
    return(peakRuntimeuS);
}
