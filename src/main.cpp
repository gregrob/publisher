#include <Arduino.h>
#include <TaskScheduler.h>

#include "debug.h"
#include "version.h"
#include "wifi.h"
#include "ota.h"
#include "mqtt.h"
#include "alarm.h"
#include "runtime.h"

#include "nvm_cfg.h"
#include "inputs.h"
#include "outputs.h"


// Local function definitions
void periodicMessageTx(void);


// Create the Scheduler that will be in charge of managing the tasks
static Scheduler scheduler;

// Debug serial port
static HardwareSerial *debugSerialPort;

// Alarm serial port
static HardwareSerial *alarmSerialPort;

// Create tasks
Task wifiStatus(30000, TASK_FOREVER, &checkWifi);
Task mqttClientTask(100, TASK_FOREVER, &mqttClientLoop);
Task mqttMessageTask(10000, TASK_FOREVER, &mqttMessageLoop);

Task alarmTriggerDebounceTask(100, TASK_FOREVER, &alarmTriggerDebounce);

Task taskInputsCyclic(INPUTS_CYCLIC_RATE, TASK_FOREVER, &inputsCyclicTask);
Task taskOutputsCyclic(OUTPUTS_CYCLIC_RATE, TASK_FOREVER, &outputsCyclicTask);
Task taskPeriodicMessageTx(30000, TASK_FOREVER, &periodicMessageTx);

void testo() {
    //Serial1.println(inputsReadInput(resetSw));

    //if(inputsReadInput(resetSw)) {ESP.restart();}
}

Task switcher(1000, TASK_FOREVER, &testo);

void setup(void) {
    // STEP 1 - Setup debug serial
    debugSerialPort = debugSetSerial(&Serial1);
    debugSerialPort->begin(115200);
    debugSerialPort->println();    
    
    // STEP 2 - Set up basic software
    nvmInit();
    inputsInit();
    outputsInit();

    //outputsSetOutput(wifiRun, flash, 0, 1000, 10, 1);
    //outputsSetOutput(wifiCfg, flash, 1000, 0, 1, 10);


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
    scheduler.addTask(wifiStatus);        
    scheduler.addTask(mqttClientTask);
    scheduler.addTask(mqttMessageTask);

    scheduler.addTask(alarmTriggerDebounceTask);
    
    scheduler.addTask(taskInputsCyclic);
    scheduler.addTask(taskOutputsCyclic);
    scheduler.addTask(taskPeriodicMessageTx);

    wifiStatus.enable();
    mqttClientTask.enable();
    mqttMessageTask.enable();
    
    alarmTriggerDebounceTask.enable();

    taskInputsCyclic.enable();
    taskOutputsCyclic.enable();
    taskPeriodicMessageTx.enable();

    scheduler.addTask(switcher);
    switcher.enable();



    randomSeed(micros());
}


void loop(void) { 
    // Handle runtime measurements
    runtimeMeasureAverageuS();
    runtimeMeasurePeakuS();

    // Handle tasks
    otaLoop();
    alarmBackgroundLoop();
    scheduler.execute();
}


/**
    Transmit periodic messages.
*/
void periodicMessageTx(void) {

    // System messages
    versionTransmitVersionMessage();
    runtimeTransmitRuntimeMessage();
    wifiTransmitWifiMessage();
    
    // Only handle alarm messages if this is an alarm unit
    if (getWiFiModuleDetails()->moduleHostType == alarmModule) {
        alarmTransmitAlarmAllMessage();
    }
}
