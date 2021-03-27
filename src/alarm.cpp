#include <Arduino.h>

#include "alarm.h"
#include "credentials.h"
#include "debug.h"
#include "mqtt.h"

// Alarm Panel Settings
// 1. Area > Properties > Arm/Disarm Speaker Beeps Via RF Keyfob
// 2. Inputs > PGM Input > Momentary - On/Off (RF Relay)

// Alarm serial baud rate
#define ALARM_SERIAL_BAUD           (19200)

// Alarm message buffer size
#define ALARM_MSG_BUFFER            (50)

// Message start and end identifiers
#define ALARM_MSG_START             (0x19)
#define ALARM_MSG_END               (0x0A)

// Maximum value for last seen (units)
#define ALARM_MAX_LAST_SEEN         (50)

// Definitions for armed and disarmed
#define ALARM_ARMED                 "armed"
#define ALARM_DISARMED              "disarmed"

// Size of alarm state string
#define ALARM_STATE_STR_SIZE        (10)

// Preamble for a trigger message definition
#define ALARM_TRIGGER_MSG_PREAMBLE  "Open "

// Enable extra debug information on alarm messages
//#define ALARM_MESSAGE_DETAILED_DEBUG

// Preamble for a trigger message
static const char* triggerMessagePreamble = ALARM_TRIGGER_MSG_PREAMBLE;

// Structure for all alarm sources in the home 
static alarmZoneInput alarmHomeStatus[] = {{"garage",         "Garage",         ALARM_MAX_LAST_SEEN, false},
                                           {"foyer",          "Foyer",          ALARM_MAX_LAST_SEEN, false},
                                           {"office",         "Study",          ALARM_MAX_LAST_SEEN, false},
                                           {"laundry",        "Laundry",        ALARM_MAX_LAST_SEEN, false},
                                           {"family",         "Family",         ALARM_MAX_LAST_SEEN, false},
                                           {"store",          "Store",          ALARM_MAX_LAST_SEEN, false},
                                           {"landing",        "Landing",        ALARM_MAX_LAST_SEEN, false},
                                           {"theatre",        "Theatre",        ALARM_MAX_LAST_SEEN, false},
                                           {"guest bedroom",  "Guest Bedroom",  ALARM_MAX_LAST_SEEN, false},
                                           {"finns room",     "Kids Room",      ALARM_MAX_LAST_SEEN, false},
                                           {"master bedroom", "Master Bedroom", ALARM_MAX_LAST_SEEN, false},
                                           {"walk in robe",   "Walk In Robe",   ALARM_MAX_LAST_SEEN, false}
};

// Structure for all alarm state definitions
static alarmStateMsgDefinitions alarmHomeStates[] = {{ALARM_DISARMED, "\x0c" "DISARMED " "\x1b\x1b\x13\x01\x1b\x1b"},
                                                     {ALARM_DISARMED, ALARM_HOME_ADDRESS " OFF"},
                                                     {ALARM_ARMED,    ALARM_HOME_ADDRESS " ON"},
                                                     {ALARM_ARMED,    "\x04\x17\x6E\x1E\x01\x1B\x1B\x13\x02\x1B\x1B\x12\x01\x1B\x1B\x09\x02\x1B\x1B\x0C\x01\x1B\x1B\x0B\x01\x1B\x1B\x0F\x01\x1B\x1B\x10\x01\x1B\x1C\x11"}
};

// Current state of the alarm (assume disarm on reset)
static char alarmCurrentState[ALARM_STATE_STR_SIZE] = ALARM_DISARMED;

// Alarm message buffer
static char alarmMsgBuffer[ALARM_MSG_BUFFER];

// Total number of alarm messages read from the panel
static unsigned int alarmRxMsgTotal = 0;

// Pointer to the alarm serial port 
static HardwareSerial *alarmSerial;

// Alarm arm / disarm pin
static uint8_t alarmArmDisarmPin = 0;

// Structure for oneShot output
static alarmOneShot oneShot = {0, 0};

/**
    Alarm module setup.
    Sets up the serial bus.

    @param[in]     serialPort pointer to the serial port to be used for the alarm.
    @param[in]     setArmDisarmPin pin to use for arm / diarm of the alarm.
    @return        pointer to the serial port used for the alarm.
*/
HardwareSerial* const alarmSetup(HardwareSerial* const serialPort, uint8_t setArmDisarmPin) {
    
    // Set-up serial interface to the alarm
    alarmSerial = serialPort;
    alarmSerial->begin(ALARM_SERIAL_BAUD);

    // Set-up arm / disarm output
    alarmArmDisarmPin = setArmDisarmPin;
    digitalWrite(alarmArmDisarmPin, LOW);
    pinMode(alarmArmDisarmPin, OUTPUT);

    return (alarmSerial);
}

#ifdef ALARM_MESSAGE_DETAILED_DEBUG
/**
    Detailed alarm message debugger.

    @param[in]     rawMessage pointer to the raw alarm message.
*/
static void alarmDetailedMessageDebug(char* const rawMessage) {
        // Debug message
        String debugMessage;

        // Temp storage for a hex char
        char hexChar[2];

        // Print detailed message debug
        debugMessage = String() + "Alarm Msg " + alarmRxMsgTotal + " Time:     ";
        debugPrint(&debugMessage, brblack);    
        debugMessage = String() + millis() + "ms";
        debugPrintln(&debugMessage, white);

        debugMessage = String() + "Alarm Msg " + alarmRxMsgTotal + " Length:   ";
        debugPrint(&debugMessage, brblack);    
        debugMessage = String() + strlen(rawMessage);
        debugPrintln(&debugMessage, white);

        debugMessage = String() + "Alarm Msg " + alarmRxMsgTotal + " Contents: ";
        debugPrint(&debugMessage, brblack);    
        debugMessage = String() + rawMessage;
        debugPrint(&debugMessage, white);
        debugMessage = String() + ".";
        debugPrintln(&debugMessage, brblack);

        debugMessage = String() + "Alarm Msg " + alarmRxMsgTotal + " Contents: ";
        debugPrint(&debugMessage, brblack);    

        for (unsigned int i = 0; i < strlen(rawMessage); i++) {
            
            sprintf(hexChar, "%.2X", rawMessage[i]);
            
            debugMessage = String() + hexChar + " ";
            debugPrint(&debugMessage, white);   
        }
        
        debugMessage = "";
        debugPrintln(&debugMessage, white);
}
#endif

/**
    Handle the one shot on the alarm arm / disarm output.
    Call at a cyclic rate.
*/
void alarmHandleOneShot(void) {

    // Handle on part of the pulse
    if (oneShot.onTime != 0) {
        oneShot.onTime--;
        digitalWrite(alarmArmDisarmPin, HIGH);
    }

    // Handle off part of the pulse
    else if (oneShot.offTime != 0) {
        oneShot.offTime--;
        digitalWrite(alarmArmDisarmPin, LOW);
    }

    // Idle
    else {
    }
}

/**
    Fire the one shot on the alarm arm / disarm output.
    
    @param[in]     onTime on duration in time unit of the handler call rate.
    @param[in]     offTime off duration in time unit of the handler call rate.
*/
void alarmFireOneShot(unsigned char onTime, unsigned char offTime) {
    oneShot.onTime = onTime;
    oneShot.offTime = offTime;
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

    // Alarm state update detected
    bool alarmStateUpdate = false;

    #ifdef ALARM_MESSAGE_DETAILED_DEBUG
        alarmDetailedMessageDebug(rawMessage);
    #endif

    // Check if message contains a trigger preamble
    if (strncmp(rawMessage, triggerMessagePreamble, (sizeof(triggerMessagePreamble)/sizeof(char))) == 0) {

        // Check what elements in the home structure need updating
        for (unsigned int i = 0; i < (sizeof(alarmHomeStatus)/sizeof(alarmHomeStatus[0])); i++) {

            // Zone match found (only compare text after preamble to speed compare up)
            if (strcmp(rawMessage + (sizeof(triggerMessagePreamble)/sizeof(char)) + 1, alarmHomeStatus[i].openStatusMsg) == 0) {
                
                // First transition to true                
                if (alarmHomeStatus[i].triggered == false) {
                    triggerTransitionActive = true;
                }

                alarmHomeStatus[i].lastTriggered = 0;
                alarmHomeStatus[i].triggered = true;
            }
        }
    }

    // General messages
    else {
        
        // Search for alarm state information
        for (unsigned int i = 0; i < (sizeof(alarmHomeStates)/sizeof(alarmHomeStates[0])); i++) {

            // Alarm state found
            if (strcmp(rawMessage, alarmHomeStates[i].alarmPanelStateMsg) == 0) {
                
                // Alarm state update (strings not equal)
                if (strcmp(alarmHomeStates[i].alarmState, alarmCurrentState) != 0) {
                    
                    alarmStateUpdate = true;
                    strcpy(alarmCurrentState, alarmHomeStates[i].alarmState);
                }
            }
        }
    }

    // Alarm state update so send mqqt message
    if (alarmStateUpdate == true) {
        alarmSendAlarmMessageStatus();
    }

    // Alarm triggers update so send mqqt message
    else if (triggerTransitionActive == true) {
        alarmSendAlarmMessageTriggers();
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
        alarmSendAlarmMessageTriggers();
    }
}

/**
    Send an alarm status message via MQTT.
    Called from:
      1. Within this module for event transmission (on transitions of status).
      2. The scheduler for periodic transmission (slow rate).
*/
void alarmSendAlarmMessageStatus(void) {
    mqttMessageSendAlarmStatus(alarmCurrentState, &alarmRxMsgTotal);
}

/**
    Send an alarm triggers message via MQTT.
    Called from:
      1. Within this module for event transmission (on transitions of triggers).
      2. The scheduler for periodic transmission (slow rate).
*/
void alarmSendAlarmMessageTriggers(void) {
    mqttMessageSendAlarmTriggers(alarmHomeStatus, (sizeof(alarmHomeStatus)/sizeof(alarmHomeStatus[0])));
}

/**
    Send all alarm messages via MQTT.
    Called from:
      1. Within this module for event transmission (on transitions of triggers).
      2. The scheduler for periodic transmission (slow rate).
*/
void alarmSendAlarmMessageAll(void) {
    alarmSendAlarmMessageTriggers();
    alarmSendAlarmMessageStatus();
}