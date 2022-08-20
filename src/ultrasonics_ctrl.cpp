// THIS IS TEST CODE, NOT VALIDATED


#include <Arduino.h>

#include "ultrasonics_ctrl.h"

#include "utils.h"
#include "debug.h"
#include "nvm_cfg.h"


// Set the module call interval
#define MODULE_CALL_INTERVAL                    ULTRASONICS_CTRL_CYCLIC_RATE

// Enable extra debug information
#define ULTRASONICS_CTRL_DETAILED_DEBUG

// Time to stay in idle state in S
#define ULTRASONICS_CTRL_IDLE_TIME_RST_S        (1)

// Time to wait between sucessive measurements in S
#define ULTRASONICS_CTRL_INTERMEASURE_DELAY_S   (0.05)

// Min reading possible
#define ULTRASONIC_CTRL_MIN_READING_POSSIBLE    (25)

// Max reading possible
#define ULTRASONIC_CTRL_MAX_READING_POSSIBLE    (450)

// Measurement re-tries (always +1 so 5 will be 6 total)
#define ULTRASONIC_CTRL_MEASUREMENT_RETRY_RST   (5)

// Size of the filter for the measurements
#define ULTRASONIC_CTRL_FILTER_SIZE             (6)

// How many extreme values in the filter to ignore
#define ULTRASONIC_CTRL_EXTREME_IGNORE          (2)

// Make sure that the number of extreme values to ignore leaves at least 1 measurement to average
static_assert((ULTRASONIC_CTRL_FILTER_SIZE - (ULTRASONIC_CTRL_EXTREME_IGNORE * 2)) >= 1, "ULTRASONIC_CTRL_EXTREME_IGNORE is too big in comparison to ULTRASONIC_CTRL_FILTER_SIZE");


// Module name for debug messages
static const char* ultrasonicsCtrlModuleName = "ultrasonicsCtrl";

// Output pin for ultrasonic trigger
static const uint8_t outputMapTrigger = RX;

// Input pin for ultrasonic echo
static const uint8_t inputMapEcho = D7;

// Ultrasonics controller state machine state names
static const char * ultrasonicsCtrlStateNames[] {
    "stmUltrasonicsRestart",
    "stmUltrasonicsIdle",
    "stmUltrasonicsMeasure"
};

// Current state
static ultrasonicsCtrlStm ultrasonicsCtrlCurrentState;

// Total number of measurements
static unsigned long ultrasonicsCtrlMeasurements = 0;

// Total number of errors in measurement - low side
static unsigned long ultrasonicsCtrlErrorsLow = 0;

// Total number of errors in measurement - high side
static unsigned long ultrasonicsCtrlErrorsHigh = 0;


/**
    Ultrasonics controller init.
*/
void ultrasonicsCtrlInit(void) {

    // Set-up direct access to the pins, not filtered
    pinMode(outputMapTrigger, OUTPUT);        
    pinMode(inputMapEcho, INPUT);
    
    // TODO: Check if this is really necessary, seems suss on an input
    digitalWrite(inputMapEcho, HIGH);
}


/**
    Take a ultrasonic measurement.
    Blocking call.
    
    @return        ranging data in CM.
*/
static uint32_t ultrasonicsCtrlMeasureDistance(void) {
    
    // Pulse return time
    uint32_t pulseReturnTimeUS;

    // Distance in cm
    uint32_t distanceCM;

    // Set the trigger pin to low for 2uS
    digitalWrite(outputMapTrigger, LOW);
    delayMicroseconds(2);
    
    // Send a 15uS high to trigger ranging (example was 10uS)
    digitalWrite(outputMapTrigger, HIGH); 
    delayMicroseconds(15);
    
    // Send pin low again
    digitalWrite(outputMapTrigger, LOW);
  
    // Read in times pulse
    pulseReturnTimeUS = pulseIn(inputMapEcho, HIGH, 26000);
    
    // Calculate the distance
    distanceCM = pulseReturnTimeUS / 58;

    // Increment measurement counter
    ultrasonicsCtrlMeasurements++;

    return(distanceCM);
}


/**
    Filter outside of range ultrasonic measurement.

    @param[in]     rawDistance raw distance measured.
    @param[in]     rangeFilteredDistance pointer to where range filtered distance is to be stored.
    @return        true when rawDistance is within range.
*/
static bool ultrasonicsCtrlRangeFilter(const uint32_t rawDistance, uint32_t * const rangeFilteredDistance) {

    // Was the rawDistance within range and rangeFilteredDistance updated?
    bool distanceWithinRange = false;

    // Reading outside range - low
    if (rawDistance <= ULTRASONIC_CTRL_MIN_READING_POSSIBLE) {
        ultrasonicsCtrlErrorsLow++;
    }

    // Reading outside range - high
    else if(rawDistance >= ULTRASONIC_CTRL_MAX_READING_POSSIBLE) {
        ultrasonicsCtrlErrorsHigh++;
    }

    // Distance within range
    else {
        *rangeFilteredDistance = rawDistance;
        distanceWithinRange = true;
    }

    return(distanceWithinRange);
}


/**
    Swap elements in an array of uint32_t's.

    @param[in]     xp pointer to first array element to be swapped.
    @param[in]     yp pointer to second array element to be swapped.
*/
static void ultrasonicsCtrlArrayElementSwap(uint32_t * const xp, uint32_t * const yp)
{
    uint32_t temp = *xp;
    *xp = *yp;
    *yp = temp;
}
 

/**
    Sort the items in an array from smallest to biggest.

    @param[in]     array pointer to the array to sort.
    @param[in]     arraySize size of the array.
*/
static void ultrasonicsCtrlSelectionSort(uint32_t * const array, const uint32_t arraySize)
{
    uint32_t minimumItemIndex;
 
    // One by one move boundary of unsorted subarray
    for (unsigned int i = 0; i < arraySize - 1; i++) {
 
        // Find the minimum element in unsorted array
        minimumItemIndex = i;
        for (unsigned int j = i + 1; j < arraySize; j++) {
            if (array[j] < array[minimumItemIndex]) {
                minimumItemIndex = j;
            }
        }
 
        // Swap the found minimum element with the first element
        ultrasonicsCtrlArrayElementSwap(&array[minimumItemIndex], &array[i]);
    }
}


static void ultrasonicsCtrlValueFilter(const uint32_t currentDistance, uint32_t * const valueFilteredDistance) {
    
    // Debug message
    String debugMessage;

    static uint32_t rawValues[ULTRASONIC_CTRL_FILTER_SIZE];

    static const uint32_t rawValuesSize = (sizeof(rawValues)/sizeof(rawValues[0]));

    static uint32_t rawValuesSorted[rawValuesSize];

    static uint32_t currentValue = 0;

    uint64_t averageValue = 0;

    static bool initCall = true;

    // Initialise the buffers the first time this is called
    if(initCall == true) {
        initCall = false;

        for(unsigned int i = 0; i < rawValuesSize; i++) {
            rawValues[i] = 0;
            rawValuesSorted[i] = 0;
        }
    }

    // Add the next value to the circular buffer
    rawValues[currentValue] = currentDistance;
    currentValue = (currentValue + 1) % rawValuesSize;

    // Copy the circular buffer for sorting
    memcpy(rawValuesSorted, rawValues, sizeof(rawValuesSorted));

    #ifdef ULTRASONICS_CTRL_DETAILED_DEBUG
        // Extended Debug
        debugMessage = String() + "Pre-sorted Results: ";

        for(unsigned int i = 0; i < rawValuesSize; i++) {
            debugMessage = debugMessage + String() + i + " = " + rawValuesSorted[i] + ", ";
        }
    
        debugLog(&debugMessage, ultrasonicsCtrlModuleName, info);
    #endif

    // Sort the values
    ultrasonicsCtrlSelectionSort(rawValuesSorted, sizeof(rawValuesSorted)/sizeof(rawValuesSorted[0]));

    #ifdef ULTRASONICS_CTRL_DETAILED_DEBUG
        // Extended Debug
        debugMessage = String() + "Post-sorted Results: ";

        for(unsigned int i = 0; i < rawValuesSize; i++) {
            debugMessage = debugMessage + String() + i + " = " + rawValuesSorted[i] + ", ";
        }
        
        debugLog(&debugMessage, ultrasonicsCtrlModuleName, info);
    #endif

    // Average
    for(unsigned int i = ULTRASONIC_CTRL_EXTREME_IGNORE; i < (rawValuesSize - ULTRASONIC_CTRL_EXTREME_IGNORE); i++) {
        averageValue += rawValuesSorted[i];
    }

    averageValue = averageValue / (rawValuesSize - (2 * ULTRASONIC_CTRL_EXTREME_IGNORE));

    #ifdef ULTRASONICS_CTRL_DETAILED_DEBUG
        // Extended Debug
        debugMessage = String() + "Averaged result with extreme values removed: " + (uint32_t)averageValue;

        debugLog(&debugMessage, ultrasonicsCtrlModuleName, info);
    #endif

    *valueFilteredDistance = (uint32_t) averageValue;

    //Serial1.println(String() + "BOOM: " + ((rawValuesSorted[1] + rawValuesSorted[2] + rawValuesSorted[3] + rawValuesSorted[4])/4));
}


/**
    Ultrasonics controller state machine.
*/
void ultrasonicCtrlStateMachine(void) {
    
    // Debug message
    String debugMessage;

    // State timer
    static uint32_t stateTimer;

    // Measurement retry counter
    static uint8_t retryCounter;

    // Range filtered distance
    uint32_t rangeFilteredDistance;

    // Value filtered distance
    uint32_t valueFilteredDistance;

    // Next state
    ultrasonicsCtrlStm nextState = ultrasonicsCtrlCurrentState;

    /*
    digitalWrite(outputMapTrigger, LOW); // Set the trigger pin to low for 2uS
    delayMicroseconds(2);
    digitalWrite(outputMapTrigger, HIGH); // Send a 10uS high to trigger ranging
    delayMicroseconds(15);
    digitalWrite(outputMapTrigger, LOW); // Send pin low again
  
    uint32_t pulseReturnTimeUS = pulseIn(inputMapEcho, HIGH, 26000); // Read in times pulse

    debugMessage = String() + "Called..." + pulseReturnTimeUS/58 + "cm";
    debugLog(&debugMessage, ultrasonicsCtrlModuleName, info);
    */

    // Handle the state machine
    switch(ultrasonicsCtrlCurrentState) {
        
        // Start here when the statemachine is reset
        case(stmUltrasonicsRestart):
            stateTimer = SECS_TO_CALLS(ULTRASONICS_CTRL_IDLE_TIME_RST_S);
            retryCounter = ULTRASONIC_CTRL_MEASUREMENT_RETRY_RST;
            nextState = stmUltrasonicsIdle;
            break;
        
        // Wait here for a period of time before polling
        case(stmUltrasonicsIdle):
            // Timer running
            if(stateTimer > 0) {
                stateTimer--;
            }
                
            // Timer elapsed
            else {
                nextState = stmUltrasonicsMeasure;
            }
            break;
        
        // Take a ultrasonic measurement
        case(stmUltrasonicsMeasure):
            
            // Range
            if(ultrasonicsCtrlRangeFilter(ultrasonicsCtrlMeasureDistance(), &rangeFilteredDistance)) {
                ultrasonicsCtrlValueFilter(rangeFilteredDistance, &valueFilteredDistance);
                
                Serial1.println(String() + valueFilteredDistance);
                Serial1.println(String() + rangeFilteredDistance + "cm - range filter");
                Serial1.println(String() + valueFilteredDistance + "cm - value filter");
                Serial1.println(String() + "measurements: " + ultrasonicsCtrlMeasurements + ", errors low: " + ultrasonicsCtrlErrorsLow + ", errors high: " + ultrasonicsCtrlErrorsHigh);
                nextState = stmUltrasonicsRestart;
            }

            // Error in measurement, attempt retry
            else {
                // Retries available
                if(retryCounter > 0) {
                    retryCounter--;
                    stateTimer = SECS_TO_CALLS(ULTRASONICS_CTRL_INTERMEASURE_DELAY_S);
                    nextState = stmUltrasonicsIdle;
                }
                
                // Retries expired
                else {
                    nextState = stmUltrasonicsRestart;
                    ultrasonicsCtrlValueFilter(rangeFilteredDistance, &valueFilteredDistance);
                    Serial1.println(String() + valueFilteredDistance);
                    Serial1.println(String() + "measurements: " + ultrasonicsCtrlMeasurements + ", errors low: " + ultrasonicsCtrlErrorsLow + ", errors high: " + ultrasonicsCtrlErrorsHigh);
                }
            }

            break;
        
        // Reset state machine
        default:
            nextState = stmUltrasonicsRestart;
            break;
    }    
 
    // State change
    if (ultrasonicsCtrlCurrentState != nextState) {
        debugMessage = String() + "State change to " + ultrasonicsCtrlStateNames[nextState];
        debugLog(&debugMessage, ultrasonicsCtrlModuleName, info);
    }
    
    // Update last states and current states (in this order)
    ultrasonicsCtrlCurrentState = nextState;
}
