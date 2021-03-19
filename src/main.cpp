#include <Arduino.h>
#include <TaskScheduler.h>

#include "debug.h"
#include "wifi.h"
#include "ota.h"
#include "mqtt.h"

// heatbeat
//#define HEARTBEAT_INTERVAL  (10000)
#define HEARTBEAT_INTERVAL  (1000)

// Local function definitions
static void testMessage(void);
static void serialRxLoop(void);

// Create the Scheduler that will be in charge of managing the tasks
Scheduler scheduler;

// Debug serial port
HardwareSerial *debugSerialPort;

// Create tasks
Task heartbeatTask(HEARTBEAT_INTERVAL, TASK_FOREVER, &testMessage);
Task wifiStatus(30000, TASK_FOREVER, &checkWifi);
Task mqttClientTask(1000, TASK_FOREVER, &mqttClientLoop);
Task mqttMessageTask(30000, TASK_FOREVER, &mqttMessageLoop);
Task serialRx(1, TASK_FOREVER, &serialRxLoop);

void setup(void) {
    // Setup serial
    debugSerialPort = debugSetSerial(&Serial1);
    debugSerialPort->begin(115200);
    debugSerialPort->println();
    Serial.begin(115200);
    //Put the serial pins on D7 = Rx and D8 = Tx.
    Serial.swap();
    Serial.println("hello there");

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
    scheduler.addTask(serialRx);

    heartbeatTask.enable();
    wifiStatus.enable();
    mqttClientTask.enable();
    mqttMessageTask.enable();
    serialRx.enable();

    randomSeed(micros());
    //digitalWrite(LED_BUILTIN, HIGH);
    //pinMode(LED_BUILTIN, OUTPUT);  

}

static void testMessage(void) {
  static int timesCalled = 0;
  String debugMessage;

  timesCalled++;

}

static void serialRxLoop(void) {
String incomingByte;
      if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.readString();
    Serial.print(incomingByte);
      }  
}

void loop(void) {
  otaLoop();
  scheduler.execute();
}
