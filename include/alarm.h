#ifndef ALARM_H
#define ALARM_H

// Call rate for the cyclic taks
#define ALARM_CYCLIC_RATE (100)


// Alarm trigger types
typedef enum {
    alarmTriggerPir      = 0,
    alarmTriggerSource   = 1,

    alarmTriggerNumberOfTypes
} alarmTriggerTypes;

// Structure for alarm status data
typedef struct {
    const char*           alarmStateName;
    const char*           alarmStatePtr;
    const char*           alarmSoundingName;
    const bool*           alarmSoundingPtr;
    const char*           alarmMessageCounterName;
    const unsigned long*  alarmMessageCounterPtr;
} alarmStatusData;

// Structure for storing an individual trigger
// Triggers can be a PIR or an Alarm Source
typedef struct {
  unsigned int  triggeredTimer;
  bool          triggered;
} triggerStatus;

// Structure for storing zone details
typedef struct {
  const char*   zoneName;
  const char*   panelZoneName;
  triggerStatus pirState;
  triggerStatus alarmSource;
} alarmZoneInput;

// Structure for storing alarm state
typedef struct {
  const char*   alarmState;
  const char*   alarmPanelStateMsg;
} alarmStateMsgDefinitions;

/**
    Alarm module init.
    Sets up the serial bus.

    @param[in]     serialPort pointer to the serial port to be used for the alarm.
    @return        pointer to the serial port used for the alarm.
*/
HardwareSerial* const alarmInit(HardwareSerial* const serialPort);

/**
    Fire the one shot on the alarm arm / disarm output.
*/
void alarmFireOneShot(void);

/**
    Alarm background task.
    Reads serial data from the alarm and scans for message start and end markers.
*/
void alarmBackgroundLoop(void);

/**
    Cycle task for the alarm.
*/
void alarmCyclicTask(void);

/**
    Transmit a alarm status message.
    No processing of the message here.
*/ 
void alarmTransmitAlarmStatusMessage(void);

/**
    Transmit a alarm PIR message.
    No processing of the message here.
*/
void alarmTransmitAlarmPirMessage(void);

/**
    Transmit a alarm source message.
    No processing of the message here.
*/ 
void alarmTransmitAlarmSourceMessage(void);

/**
    Transmit a ALL alarm messages.
    No processing of the message here.
*/ 
void alarmTransmitAlarmAllMessage(void);

#endif
