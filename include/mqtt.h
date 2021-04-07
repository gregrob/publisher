#ifndef MQTT_H
#define MQTT_H

#include "alarm.h"

/**
    MQTT setup.
*/
void mqttSetup(void);

/**
    MQTT client loop.
*/
void mqttClientLoop(void);

/**
    MQTT message loop. 
    Handle disconnections / reconnections.
*/
void mqttMessageLoop(void);

/**
    Send a raw MQTT message. 
    This function will construct the topic [mqtt_prefix]/[hostname]/[shortTopic]. 
    This function will not alter the message to be sent.

    @param[in]     shortTopic pointer to the topic (short form without prefix and hostname)
    @param[in]     message pointer to the message
*/
void mqttMessageSendRaw(const char * const shortTopic, const char * const  message);

/**
    Send a Alarm Trigger state message via MQTT. 
    The message contains all alarm trigger status.

    @param[in]     zoneInputData pointer to the zone input structure
    @param[in]     zones number of zones in the zoneInputData
*/
void mqttMessageSendAlarmTriggers(const alarmZoneInput * const zoneInputData, const unsigned int zones);

/**
    Send a Alarm Status message via MQTT. 
    The message contains alarm state and debug information.

    @param[in]     alarmState pointer to the alarm state
    @param[in]     alarmRxMxgCtr pointer to the total number of alarm messages processed
*/
void mqttMessageSendAlarmStatus(const char * const alarmState, const unsigned int * const alarmRxMxgCtr);

#endif
