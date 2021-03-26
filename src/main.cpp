#include <Arduino.h>
#include <TaskScheduler.h>

#include "debug.h"
#include "wifi.h"
#include "ota.h"
#include "mqtt.h"
#include "alarm.h"

// heatbeat
//#define HEARTBEAT_INTERVAL  (10000)
#define HEARTBEAT_INTERVAL  (1000)

// Local function definitions
static void testMessage(void);
static void measureAverageRuntimeuS(void);
static void measurePeakRuntimeuS(void);

// Create the Scheduler that will be in charge of managing the tasks
Scheduler scheduler;

// Debug serial port
HardwareSerial *debugSerialPort;

// Alarm serial port
HardwareSerial *alarmSerialPort;

// Create tasks
Task heartbeatTask(HEARTBEAT_INTERVAL, TASK_FOREVER, &testMessage);
Task wifiStatus(30000, TASK_FOREVER, &checkWifi);
Task mqttClientTask(1000, TASK_FOREVER, &mqttClientLoop);
Task mqttMessageTask(30000, TASK_FOREVER, &mqttMessageLoop);

Task alarmTriggerDebounceTask(100, TASK_FOREVER, &alarmTriggerDebounce);
Task alarmSendAlarmMessageTask(30000, TASK_FOREVER, &alarmSendAlarmMessageAll);


void setup(void) {
    // Setup serial
    debugSerialPort = debugSetSerial(&Serial1);
    debugSerialPort->begin(115200);
    debugSerialPort->println();
    
    // Set up the alarm interface
    alarmSerialPort = alarmSetup(&Serial);
    //Put the serial pins on D7 = Rx and D8 = Tx.
    alarmSerialPort->swap();
    
    // Set-up wifi
    setupWifi();

    // Set-up ota
    otaSetup();

    // Set-up mqtt
    mqttSetup();

    // Add scheduler tasks and enable
    scheduler.addTask(heartbeatTask);
    scheduler.addTask(wifiStatus);        
    scheduler.addTask(mqttClientTask);
    scheduler.addTask(mqttMessageTask);

    scheduler.addTask(alarmTriggerDebounceTask);
    scheduler.addTask(alarmSendAlarmMessageTask);

    heartbeatTask.enable();
    wifiStatus.enable();
    mqttClientTask.enable();
    mqttMessageTask.enable();

    alarmTriggerDebounceTask.enable();
    alarmSendAlarmMessageTask.enable();

    randomSeed(micros());
    //digitalWrite(LED_BUILTIN, HIGH);
    //pinMode(LED_BUILTIN, OUTPUT);  

}

static void testMessage(void) {
  static int timesCalled = 0;
  String debugMessage;

  timesCalled++;

}

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
static void measureAverageRuntimeuS(void) {

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
static void measurePeakRuntimeuS(void) {

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


void loop(void) { 
    // Handle runtime measurements
    measureAverageRuntimeuS();
    measurePeakRuntimeuS();

    // Handle tasks
    otaLoop();
    alarmBackgroundLoop();
    scheduler.execute();
}

//!!!SW VERSION STUFF!!!
// !!! PUBLISHER NAME IN MQQT PUBLISH!!!