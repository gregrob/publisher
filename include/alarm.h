#ifndef ALARM_H
#define ALARM_H

// Structure for storing zone details
typedef struct {
  const char*   zoneName;
  const char*   openStatusMsg;
  unsigned int  lastTriggered;
  bool          triggered;
} alarmZoneInput;

/**
    Alarm module setup.
    Sets up the serial bus.

    @param[in]     serialPort pointer to the serial port to be used for the alarm.
    @return        pointer to the serial port used for the alarm.
*/
HardwareSerial* const alarmSetup(HardwareSerial* const serialPort);

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
    Send an alarm message via MQTT.
    Called from:
      1. Within this module for event transmission (on transitions of triggers).
      2. The scheduler for periodic transmission (slow rate).
*/
void alarmSendAlarmMessage(void);

#endif