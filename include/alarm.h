#ifndef ALARM_H
#define ALARM_H

// Structure for alarm status data
typedef struct {
    const char*           alarmStateName;
    const char*           alarmStatePtr;
    const char*           alarmMessageCounterName;
    const unsigned long*  alarmMessageCounterPtr;
} alarmStatusData;

// Structure for storing zone details
typedef struct {
  const char*   zoneName;
  const char*   openStatusMsg;
  unsigned int  lastTriggered;
  bool          triggered;
} alarmZoneInput;

// Structure for storing alarm state
typedef struct {
  const char*   alarmState;
  const char*   alarmPanelStateMsg;
} alarmStateMsgDefinitions;

/**
    Alarm module setup.
    Sets up the serial bus.

    @param[in]     serialPort pointer to the serial port to be used for the alarm.
    @return        pointer to the serial port used for the alarm.
*/
HardwareSerial* const alarmSetup(HardwareSerial* const serialPort);

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
    Debounces alarm triggers.
    Called from the scheduler to provide debounce time base.
*/
void alarmTriggerDebounce(void);

/**
    Transmit a alarm status message.
    No processing of the message here.
*/ 
void alarmTransmitAlarmStatusMessage(void);

/**
    Transmit a alarm triggers message.
    No processing of the message here.
*/ 
void alarmTransmitAlarmTriggersMessage(void);

/**
    Transmit a ALL alarm messages.
    No processing of the message here.
*/ 
void alarmTransmitAlarmAllMessage(void);

#endif
