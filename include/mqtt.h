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

/**
    Send a Module Software message via MQTT. 
    The message contains build information.
*/
void mqttMessageSendModuleSoftware(void);

/**
    Send a Module Status message via MQTT. 
    The message contains module IP and runtime information.
*/
void mqttMessageSendModuleStatus(void);

#endif
