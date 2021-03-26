#ifndef MQTT_H
#define MQTT_H

#include "alarm.h"

// mqtt setup
void mqttSetup(void);

// mqtt client loop
void mqttClientLoop(void);

// Send mqtt messages
void mqttMessageLoop(void);

// Get function for messageText
String* const messageTextGet(void);

// Send an alarm triggers message via mqtt
void mqttMessageSendAlarmTriggers(alarmZoneInput* const zoneInputData, unsigned int zones);

// Send an alarm status message via mqtt
void mqttMessageSendAlarmStatus(char* alarmState, unsigned int* const alarmRxMxgCtr);

#endif