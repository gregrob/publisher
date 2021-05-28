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

#include "reset_ctrl.h"
#include "status_ctrl.h"
#include "hawkbit_client.h"

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
Task taskResetCtrl(RESET_CTRL_CYCLIC_RATE, TASK_FOREVER, &restCtrlStateMachine);
Task taskStatusCtrl(STATUS_CTRL_CYCLIC_RATE, TASK_FOREVER, &statusCtrlStateMachine);
Task taskHawkbitCtrl(HAWKBIT_CLIENT_CYCLIC_RATE, TASK_FOREVER, &hawkbitClientStateMachine);
Task taskPeriodicMessageTx(30000, TASK_FOREVER, &periodicMessageTx);

void testo() {
    static bool pino = true;
    //Serial1.println(inputsReadInputByName(resetSw));
    //if(inputsReadInput(resetSw)) {ESP.restart();}
    //hawkbitClientCyclicTask();
    analogWrite(RX, 1024);
    digitalWrite(RX, pino);
    if(pino) pino = false;
    else pino = true;
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
    versionInit();

    // STEP 3 - Set up the applications
    restCtrlInit();
    statusCtrlInit();
    

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

    hawkbitClientInit();
    
    // Add scheduler tasks and enable
    scheduler.addTask(wifiStatus);        
    scheduler.addTask(mqttClientTask);
    scheduler.addTask(mqttMessageTask);

    scheduler.addTask(alarmTriggerDebounceTask);
    
    scheduler.addTask(taskInputsCyclic);
    scheduler.addTask(taskOutputsCyclic);
    scheduler.addTask(taskResetCtrl);
    scheduler.addTask(taskStatusCtrl);
    scheduler.addTask(taskHawkbitCtrl);
    scheduler.addTask(taskPeriodicMessageTx);

    wifiStatus.enable();
    mqttClientTask.enable();
    mqttMessageTask.enable();
    
    alarmTriggerDebounceTask.enable();

    taskInputsCyclic.enable();
    taskOutputsCyclic.enable();
    taskResetCtrl.enable();
    taskStatusCtrl.enable();
    taskHawkbitCtrl.enable();
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
    nvmTransmitStatusMessage();
    
    // Only handle alarm messages if this is an alarm unit
    if (getWiFiModuleDetails()->moduleHostType == alarmModule) {
        alarmTransmitAlarmAllMessage();
    }
}
