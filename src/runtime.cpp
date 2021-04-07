#include <Arduino.h>

#include "runtime.h"
#include "runtime_cfg.h"

#include "messages_tx.h"


// The number of cycles to measure average runtime over
#define AVERAGE_RUNTIME_CYCLES    (1000)

// Number of calls the peak runtime measurement should be ignored after boot
#define PEAK_RUNTIME_IGNORE_CALLS (4)

// Name for the peak runtime
#define RUNTIME_NAME_PEAK           ("peak")

// Name for the average runtime
#define RUNTIME_NAME_AVERAGE        ("average")

// Name for the uptime
#define RUNTIME_NAME_UPTIME         ("uptime")


// Average runtime
static unsigned long averageRuntimeuS = 0;

// Peak runtime
static unsigned long peakRuntimeuS = 0;

// Uptime
static unsigned long uptimeuS = 0;

// Runtime data for this software
static const runtimeData runtimeDataSoftware[] = {{RUNTIME_NAME_PEAK,      &peakRuntimeuS},
                                                  {RUNTIME_NAME_AVERAGE,   &averageRuntimeuS},
                                                  {RUNTIME_NAME_UPTIME,    &uptimeuS}
};  

// Size of the runtimeDataSoftware structure
static const unsigned int runtimeDataStructureSize = (sizeof(runtimeDataSoftware) / sizeof(runtimeDataSoftware[0]));


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
        averageRuntimeuS = (currentTimeuS - startTimeuS) / AVERAGE_RUNTIME_CYCLES;
        
        // Reset the loop counters and start time snapshot
        loopCounter = AVERAGE_RUNTIME_CYCLES;
        startTimeuS = currentTimeuS;
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
    Transmit a runtime message.
    No processing of the message here.
*/
void runtimeTransmitRuntimeMessage(void) {   
    
    // Updatre the uptime variable before transmitting
    uptimeuS = millis();

    messsagesTxRuntimeMessage(runtimeDataSoftware, &runtimeDataStructureSize);
}
