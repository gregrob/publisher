#include <Arduino.h>
#include <TaskScheduler.h>

#include "debug.h"
#include "version.h"
#include "wifi.h"
#include "ota.h"
#include "mqtt.h"
#include "alarm.h"
#include "runtime.h"

// Create the Scheduler that will be in charge of managing the tasks
Scheduler scheduler;

// Debug serial port
HardwareSerial *debugSerialPort;

// Alarm serial port
HardwareSerial *alarmSerialPort;

// Create tasks
Task wifiStatus(30000, TASK_FOREVER, &checkWifi);
Task mqttClientTask(1000, TASK_FOREVER, &mqttClientLoop);
Task mqttMessageTask(10000, TASK_FOREVER, &mqttMessageLoop);

Task alarmTriggerDebounceTask(100, TASK_FOREVER, &alarmTriggerDebounce);
Task alarmOneShotTask(100, TASK_FOREVER, &alarmHandleOneShot);
Task alarmSendAlarmMessageTask(30000, TASK_FOREVER, &alarmSendAlarmMessageAll);

Task systemSendModuleStatusTask(30000, TASK_FOREVER, &mqttMessageSendModuleStatus);

Task versionSendMessageTask(30000, TASK_FOREVER, &versionTransmitVersionMessage);
Task runtimeSendMessageTask(30000, TASK_FOREVER, &runtimeTransmitRuntimeMessage);

void setup(void) {
    // Setup serial
    debugSerialPort = debugSetSerial(&Serial1);
    debugSerialPort->begin(115200);
    debugSerialPort->println();
    
    // Set up the alarm interface
    alarmSerialPort = alarmSetup(&Serial, D6);
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
    scheduler.addTask(alarmOneShotTask);
    scheduler.addTask(alarmSendAlarmMessageTask);

    scheduler.addTask(systemSendModuleStatusTask);
    
    scheduler.addTask(versionSendMessageTask);
    scheduler.addTask(runtimeSendMessageTask);

    wifiStatus.enable();
    mqttClientTask.enable();
    mqttMessageTask.enable();
    
    alarmTriggerDebounceTask.enable();
    alarmOneShotTask.enable();
    alarmSendAlarmMessageTask.enable();

    systemSendModuleStatusTask.enable();
    
    versionSendMessageTask.enable();
    runtimeSendMessageTask.enable();


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
