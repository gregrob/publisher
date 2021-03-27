#include <Arduino.h>
#include <TaskScheduler.h>

#include "debug.h"
#include "wifi.h"
#include "ota.h"
#include "mqtt.h"
#include "alarm.h"
#include "runtime.h"

// heatbeat
//#define HEARTBEAT_INTERVAL  (10000)
#define HEARTBEAT_INTERVAL    (1000)

// Local function definitions
static void testMessage(void);

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
Task alarmOneShotTask(100, TASK_FOREVER, &alarmHandleOneShot);
Task alarmSendAlarmMessageTask(30000, TASK_FOREVER, &alarmSendAlarmMessageAll);


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
    scheduler.addTask(heartbeatTask);
    scheduler.addTask(wifiStatus);        
    scheduler.addTask(mqttClientTask);
    scheduler.addTask(mqttMessageTask);

    scheduler.addTask(alarmTriggerDebounceTask);
    scheduler.addTask(alarmOneShotTask);
    scheduler.addTask(alarmSendAlarmMessageTask);

    heartbeatTask.enable();
    wifiStatus.enable();
    mqttClientTask.enable();
    mqttMessageTask.enable();

    alarmTriggerDebounceTask.enable();
    alarmOneShotTask.enable();
    alarmSendAlarmMessageTask.enable();

    randomSeed(micros());

}

static void testMessage(void) {
  static int timesCalled = 0;
  String debugMessage;

  timesCalled++;

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

//!!!SW VERSION STUFF!!!
// !!! PUBLISHER NAME IN MQQT PUBLISH!!!
// PUBLISH PERFORMANCE DATA IN CYCLIC MQTT