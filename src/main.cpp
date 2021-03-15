#include <Arduino.h>
#include <TaskScheduler.h>

#include "debug.h"
#include "wifi.h"
#include "mqtt.h"

// heatbeat
#define HEARTBEAT_INTERVAL  (10000)

// Local function definitions
static void testMessage(void);

// Create the Scheduler that will be in charge of managing the tasks
Scheduler scheduler;

// Debug serial port
HardwareSerial *debugSerialPort;

// Create tasks
Task heartbeatTask(HEARTBEAT_INTERVAL, TASK_FOREVER, &testMessage);
Task wifiStatus(10000, TASK_FOREVER, &checkWifi);
Task mqttClientTask(1000, TASK_FOREVER, &mqttClientLoop);
Task mqttMessageTask(30000, TASK_FOREVER, &mqttMessageLoop);

void setup(void) {
    // Setup serial
    debugSerialPort = debugSetSerial(&Serial1);
    debugSerialPort->begin(115200);
    debugSerialPort->println();
    Serial.begin(115200);

    // Set-up wifi
    setupWifi();

    // Set-up mqtt
    mqttSetup();

    // Add scheduler tasks and enable
    scheduler.addTask(heartbeatTask);
    scheduler.addTask(wifiStatus);        
    scheduler.addTask(mqttClientTask);
    scheduler.addTask(mqttMessageTask);

    //heartbeatTask.enable();
    wifiStatus.enable();
    mqttClientTask.enable();
    mqttMessageTask.enable();

    randomSeed(micros());
    //digitalWrite(LED_BUILTIN, HIGH);
    //pinMode(LED_BUILTIN, OUTPUT);    
}

static void testMessage(void) {
  static int timesCalled = 0;
  String debugMessage;

  timesCalled++;

  debugMessage = (String() + "heartbeat " + (timesCalled * (HEARTBEAT_INTERVAL/1000)) + "s");
  debugLog(&debugMessage, info);
}


void loop(void) {
  scheduler.execute();
}
