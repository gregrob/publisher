#include <Arduino.h>
#include <TaskScheduler.h>
#include "wifi.h"
#include "mqtt.h"

void testMessage(void);

// Create the Scheduler that will be in charge of managing the tasks
Scheduler scheduler;

// Create tasks
Task tastTest(500, TASK_FOREVER, &testMessage);
Task wifiStatus(10000, TASK_FOREVER, &checkWifi);
Task mqttClientTask(1000, TASK_FOREVER, &mqttClientLoop);
Task mqttMessageTask(30000, TASK_FOREVER, &mqttMessageLoop);

void setup(void) {
    // Setup serial
    Serial.begin(115200);

    // Set-up wifi
    setupWifi();

    // Set-up mqtt
    mqttSetup();

    // Add scheduler tasks and enable
    scheduler.addTask(tastTest);
    scheduler.addTask(wifiStatus);        
    scheduler.addTask(mqttClientTask);
    scheduler.addTask(mqttMessageTask);

    //tastTest.enable();
    wifiStatus.enable();
    mqttClientTask.enable();
    mqttMessageTask.enable();

    randomSeed(micros());
    digitalWrite(LED_BUILTIN, HIGH);
    pinMode(LED_BUILTIN, OUTPUT);    
}

void testMessage(void) {
  static int timesCalled = 0;

  timesCalled++;

  Serial.println("Times called " + (String)timesCalled + " " + (String)millis());
}


void loop(void) {
  scheduler.execute();
}