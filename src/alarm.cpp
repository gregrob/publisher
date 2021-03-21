#include <Arduino.h>

#include "alarm.h"
#include "mqtt.h"

// Alarm serial baud rate
#define ALARM_SERIAL_BAUD   (19200)

// Alarm message buffer size
#define ALARM_MSG_BUFFER    (50)

// Message start and end identifiers
#define ALARM_MSG_START     (0x19)
#define ALARM_MSG_END       (0x0A)

// Maximum value for last seen (units)
#define ALARM_MAX_LAST_SEEN (50)

// Structure for all alarm sources in the home 
static alarmZoneInput alarmHomeStatus[] = {{"garage",         "Open Garage",         ALARM_MAX_LAST_SEEN, false},
                                           {"foyer",          "Open Foyer",          ALARM_MAX_LAST_SEEN, false},
                                           {"office",         "Open Study",          ALARM_MAX_LAST_SEEN, false},
                                           {"laundry",        "Open Laundry",        ALARM_MAX_LAST_SEEN, false},
                                           {"family",         "Open Family",         ALARM_MAX_LAST_SEEN, false},
                                           {"store",          "Open Store",          ALARM_MAX_LAST_SEEN, false},
                                           {"landing",        "Open Landing",        ALARM_MAX_LAST_SEEN, false},
                                           {"theatre",        "Open Theatre",        ALARM_MAX_LAST_SEEN, false},
                                           {"guest bedroom",  "Open Guest Bedroom",  ALARM_MAX_LAST_SEEN, false},
                                           {"finns room",     "Open Kids Room",      ALARM_MAX_LAST_SEEN, false},
                                           {"master bedroom", "Open Master Bedroom", ALARM_MAX_LAST_SEEN, false},
                                           {"walk in robe",   "Open Walk In Robe",   ALARM_MAX_LAST_SEEN, false}
};
                                           
// Alarm message buffer
static char alarmMsgBuffer[ALARM_MSG_BUFFER];

// Total number of alarm messages read from the panel
static unsigned int alarmRxMsgTotal = 0;

// Pointer to the alarm serial port 
static HardwareSerial *alarmSerial;

/**
    Alarm module setup.
    Sets up the serial bus.

    @param[in]     serialPort pointer to the serial port to be used for the alarm.
    @return        pointer to the serial port used for the alarm.
*/
HardwareSerial* const alarmSetup(HardwareSerial* const serialPort) {
    alarmSerial = serialPort;

    alarmSerial->begin(ALARM_SERIAL_BAUD);

    return (alarmSerial);
}

/**
    Home structure updater.
    Chacks an alarm message to see if the trigger source is in the structure.
    If the trigger source is in the structure, update trigger activation. 

    @param[in]     rawMessage pointer to the raw alarm message.
*/
static void alarmUpdateHome(char* const rawMessage) {
    // Zone transition to active detected
    bool triggerTransitionActive = false;

    // Check what elements in the home structure need updating
    for (unsigned int i = 0; i < (sizeof(alarmHomeStatus)/sizeof(alarmHomeStatus[0])); i++) {

        // Zone match found
        if (strcmp(rawMessage, alarmHomeStatus[i].openStatusMsg) == 0) {
            
            // First transition to true                
            if (alarmHomeStatus[i].triggered == false) {
                triggerTransitionActive = true;
            }

            alarmHomeStatus[i].lastTriggered = 0;
            alarmHomeStatus[i].triggered = true;
        }
    }

    // A trigger was reset so send out a mqqt message
    if (triggerTransitionActive == true) {
        alarmSendAlarmMessage();
    }
}

/**
    Alarm background task.
    Reads serial data from the alarm and scans for message start and end markers.
*/
void alarmBackgroundLoop(void) {
    // Currently read character from the serial bus
    char charRead;

    // Current position in the message buffer
    static unsigned char alarmMsgBufferPosition = 0;

    // Has the alarm message buffer population started
    static bool alarmMsgBufferPopulationStarted = false;
  
    // Is there serial data available?
    if (alarmSerial->available() > 0) {

        // Read a byte
        charRead = Serial.read();

        // Start of an alarm message so set things up (start character and not already started)
        if ((charRead == ALARM_MSG_START) && (alarmMsgBufferPopulationStarted == false)) {
            memset(alarmMsgBuffer, 0x00, (sizeof(alarmMsgBuffer)/sizeof(char)));

            alarmMsgBufferPosition = 0;
            alarmMsgBufferPopulationStarted = true;
        }

        // End of an alarm message so close off (end character and already started)
        else if ((charRead == ALARM_MSG_END) && (alarmMsgBufferPopulationStarted == true)) {
            alarmMsgBufferPopulationStarted = false;
            alarmRxMsgTotal++;
            alarmUpdateHome(alarmMsgBuffer);
        }

        // Continue populating alarm message buffer
        else if (alarmMsgBufferPopulationStarted == true) {
            
            // Make sure the buffer is not full
            if (alarmMsgBufferPosition < ALARM_MSG_BUFFER) {
                alarmMsgBuffer[alarmMsgBufferPosition] = charRead;
                alarmMsgBufferPosition++;
            }

            // Buffer over run so reset
            else {
                alarmMsgBufferPopulationStarted = false;
            }
        }
    }
}

/**
    Debounces alarm triggers.
    Called from the scheduler to provide debounce time base.
*/
void alarmTriggerDebounce(void) {
    // Zone transition to inactive detected
    bool triggerTransitionInactive = false;

    // Update alarm schedule loop
    for (unsigned int i = 0; i < (sizeof(alarmHomeStatus)/sizeof(alarmHomeStatus[0])); i++) {

        // Only update last seen value if it is less than the max
        if (alarmHomeStatus[i].lastTriggered < ALARM_MAX_LAST_SEEN) {
            alarmHomeStatus[i].lastTriggered++;
        }

        else {
            // First transition to false                
            if (alarmHomeStatus[i].triggered == true) {
                triggerTransitionInactive = true;
            }
            
            alarmHomeStatus[i].triggered = false;                        
        }
    }

    // A trigger was reset so send out a mqqt message
    if (triggerTransitionInactive == true) {
        alarmSendAlarmMessage();
    }
}

/**
    Send an alarm message via MQTT.
    Called from:
      1. Within this module for event transmission (on transitions of triggers).
      2. The scheduler for periodic transmission (slow rate).
*/
void alarmSendAlarmMessage(void) {
    mqttMessageSendAlarm(alarmHomeStatus, (sizeof(alarmHomeStatus)/sizeof(alarmHomeStatus[0])), &alarmRxMsgTotal);
}
