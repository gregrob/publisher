#include <Arduino.h>

#include "alarm.h"
#include "credentials.h"
#include "debug.h"
#include "nvm_cfg.h"
#include "messages_tx.h"
#include "outputs_cfg.h"
#include "inputs_cfg.h"

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

// Alarm trigger timer reset value
#define ALARM_TRIGGER_TIMER_RESET   (0)

// Definitions for armed and disarmed
#define ALARM_ARMED                 ("armed")
#define ALARM_DISARMED              ("disarmed")

// Size of alarm state string
#define ALARM_STATE_STR_SIZE        (10)

// Preamble for a PIR message definition
#define ALARM_PIR_MSG_PREAMBLE      ("Open ")

// Preamble for a alarm source message definition
#define ALARM_SOURCE_MSG_PREAMBLE   ("ALARM ")

// Panel common text for armed
#define ALARM_PANEL_TEXT_CMN_ARM    (" ON")

// Panel common text for disarmed
#define ALARM_PANEL_TEXT_CMN_DISARM (" OFF")

// Name for the state
#define ALARM_NAME_STATE            ("state")

// Name for the sounding
#define ALARM_NAME_SOUNDING         ("sounding")

// Name for the message counter
#define ALARM_NAME_MESSAGES         ("messages")

// Enable extra debug information on alarm messages
//#define ALARM_MESSAGE_DETAILED_DEBUG

// Current state of the alarm (assume disarm on reset)
static char alarmCurrentState[ALARM_STATE_STR_SIZE] = ALARM_DISARMED;

// Alarm sounding status
static bool alarmSounding = false;

// Total number of alarm messages read from the panel
static unsigned long alarmRxMsgTotal = 0;

// Alarm status data
static const alarmStatusData alarmStatusDataTable = {ALARM_NAME_STATE,      alarmCurrentState,
                                                     ALARM_NAME_SOUNDING,   &alarmSounding,
                                                     ALARM_NAME_MESSAGES,   &alarmRxMsgTotal
};  

// Preamble for a PIR message
static const char* pirMessagePreamble = ALARM_PIR_MSG_PREAMBLE;

// Preamble for a alarm source message
static const char* sourceMessagePreamble = ALARM_SOURCE_MSG_PREAMBLE;

// Initial value for a trigger
static const triggerStatus triggerInitialState = {ALARM_MAX_LAST_SEEN, false};

// Structure for all PIR and alarm trigger sources 
static alarmZoneInput alarmHomeStatus[] = {{"garage",         "Garage",         triggerInitialState, triggerInitialState},
                                           {"foyer",          "Foyer",          triggerInitialState, triggerInitialState},
                                           {"office",         "Study",          triggerInitialState, triggerInitialState},
                                           {"laundry",        "Laundry",        triggerInitialState, triggerInitialState},
                                           {"family",         "Family",         triggerInitialState, triggerInitialState},
                                           {"store",          "Store",          triggerInitialState, triggerInitialState},
                                           {"landing",        "Landing",        triggerInitialState, triggerInitialState},
                                           {"theatre",        "Theatre",        triggerInitialState, triggerInitialState},
                                           {"guest bedroom",  "Guest Bedroom",  triggerInitialState, triggerInitialState},
                                           {"finns room",     "Kids Room",      triggerInitialState, triggerInitialState},
                                           {"master bedroom", "Master Bedroom", triggerInitialState, triggerInitialState},
                                           {"walk in robe",   "Walk In Robe",   triggerInitialState, triggerInitialState}
};

// Size of the alarmHomeStatus structure
static const unsigned int alarmHomeStatusSize = (sizeof(alarmHomeStatus) / sizeof(alarmHomeStatus[0]));

// Size of an element in the alarmHomeStatus structure
static const unsigned int alarmHomeStatusElementSize = (sizeof(alarmHomeStatus[0]) / sizeof(uint8_t));

// Panel armed message buffered from NVM
static char alarmPanelStateMsgArmed[NVM_MAX_LENGTH_ADDRESS + sizeof(ALARM_PANEL_TEXT_CMN_ARM)];

// Panel disarmed message buffered from NVM
static char alarmPanelStateMsgDisarmed[NVM_MAX_LENGTH_ADDRESS + sizeof(ALARM_PANEL_TEXT_CMN_DISARM)];

// Structure for all alarm state definitions
static alarmStateMsgDefinitions alarmHomeStates[] = {{ALARM_DISARMED, "\x0c" "DISARMED " "\x1b\x1b\x13\x01\x1b\x1b"},
                                                     {ALARM_DISARMED, alarmPanelStateMsgDisarmed},
                                                     {ALARM_ARMED,    alarmPanelStateMsgArmed},
                                                     {ALARM_ARMED,    "\x04\x17\x6E\x1E\x01\x1B\x1B\x13\x02\x1B\x1B\x12\x01\x1B\x1B\x09\x02\x1B\x1B\x0C\x01\x1B\x1B\x0B\x01\x1B\x1B\x0F\x01\x1B\x1B\x10\x01\x1B\x1C\x11"}
};

// Alarm message buffer
static char alarmMsgBuffer[ALARM_MSG_BUFFER];

// Pointer to the alarm serial port 
static HardwareSerial *alarmSerial;

/**
    Alarm module init.
    Sets up the serial bus.

    @param[in]     serialPort pointer to the serial port to be used for the alarm.
    @return        pointer to the serial port used for the alarm.
*/
HardwareSerial* const alarmInit(HardwareSerial* const serialPort) {
    // Pointer to the RAM mirror
    const nvmCompleteStructure * ramMirrorPtr;

    // Set-up pointer to RAM mirror
    (void) nvmGetRamMirrorPointerRO(&ramMirrorPtr);

    // Buffer panel armed message from NVM (and append constant part of the string)
    strncpy(alarmPanelStateMsgArmed, ramMirrorPtr->alarm.homeAddress, sizeof(ramMirrorPtr->alarm.homeAddress));
    strncat(alarmPanelStateMsgArmed, ALARM_PANEL_TEXT_CMN_ARM, sizeof(ALARM_PANEL_TEXT_CMN_ARM));

    // Buffer panel disarmed message from NVM (and append constant part of the string)
    strncpy(alarmPanelStateMsgDisarmed, ramMirrorPtr->alarm.homeAddress, sizeof(ramMirrorPtr->alarm.homeAddress));
    strncat(alarmPanelStateMsgDisarmed, ALARM_PANEL_TEXT_CMN_DISARM, sizeof(ALARM_PANEL_TEXT_CMN_DISARM));

    // Set-up serial interface to the alarm
    alarmSerial = serialPort;
    alarmSerial->begin(ALARM_SERIAL_BAUD);

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
    Fire the one shot on the alarm arm / disarm output.
*/
void alarmFireOneShot(void) {
    outputsSetOutputByName(alarmCtrl, {oneshot, PWMRANGE, 5, 0, 5});
}

/**
    Update triggers based on raw message from the alarm.
    The same function is used for PIR and alarm source triggers.

    @param[in]     rawMessage pointer to the raw alarm message.
    @param[in]     preamble pointer to the preamble to ignore in the message.
    @param[in]     trigger pointer to the first trigger in the structure array.
    @return        bool if any trigger was updated or not.
*/
static bool alarmUpdateTrigger(const char * const rawMessage, const char * const preamble, triggerStatus * trigger) {
    
    // Return value (true when there was an update)
    bool returnValue = false;
    
    // Check what elements in the home structure need updating
    for (unsigned int i = 0; i < alarmHomeStatusSize; i++) {

        // Zone match found (only compare text after preamble to speed compare up)
        if (strcmp(rawMessage + strlen(preamble), alarmHomeStatus[i].panelZoneName) == 0) {
            
            // First transition to true                
            if (trigger->triggered == false) {
                returnValue = true;
            }

            // Reset the timer and set set triggered
            trigger->triggeredTimer = ALARM_TRIGGER_TIMER_RESET;
            trigger->triggered = true;
        }

        // Increment pointer to next trigger
        trigger = (triggerStatus*) ((uint8_t *) trigger + alarmHomeStatusElementSize);
    }

    return (returnValue);
}

/**
    Home structure updater.
    Chacks an alarm message to see if the trigger source is in the structure.
    If the trigger source is in the structure, update trigger activation. 

    @param[in]     rawMessage pointer to the raw alarm message.
*/
static void alarmUpdateHome(char* const rawMessage) {
    // PIR transition to active detected
    bool pirTransitionActive = false;

    // Source transition to active detected
    bool sourceTransitionActive = false;

    // Alarm state update detected
    bool alarmStateUpdate = false;

    #ifdef ALARM_MESSAGE_DETAILED_DEBUG
        alarmDetailedMessageDebug(rawMessage);
    #endif

    // Check if message contains a PIR preamble
    if (strncmp(rawMessage, pirMessagePreamble, strlen(pirMessagePreamble)) == 0) {
        pirTransitionActive = alarmUpdateTrigger(rawMessage, pirMessagePreamble, &alarmHomeStatus[0].pirState);
    }

    // Check if message contains an alarm source preamble
    else if (strncmp(rawMessage, sourceMessagePreamble, strlen(sourceMessagePreamble)) == 0) {
        sourceTransitionActive = alarmUpdateTrigger(rawMessage, sourceMessagePreamble, &alarmHomeStatus[0].alarmSource);
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
        alarmTransmitAlarmStatusMessage();
    }

    // Alarm PIR update so send mqqt message
    else if (pirTransitionActive == true) {
        alarmTransmitAlarmPirMessage();
    }

    // Alarm source update so send mqqt message
    else if (sourceTransitionActive == true) {
        alarmTransmitAlarmSourceMessage();
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
    Debounce triggers to the false state.
    The same function is used for PIR and alarm source triggers.

    @param[in]     trigger pointer to the first trigger in the structure array.
    @return        bool if any trigger was updated or not.
*/
static bool alarmDebounceTrigger(triggerStatus * trigger) {
    
    // Return value (true when there was an update)
    bool returnValue = false;
    
    // Debounce each source
    for (unsigned int i = 0; i < alarmHomeStatusSize; i++) {

        // Only update last seen value if it is less than the max
        if (trigger->triggeredTimer < ALARM_MAX_LAST_SEEN) {
            trigger->triggeredTimer++;
        }

        else {
            // First transition to false                
            if (trigger->triggered == true) {
                returnValue = true;
            }
            
            trigger->triggered = false;                        
        }
        
        // Increment pointer to next trigger
        trigger = (triggerStatus*) ((uint8_t *) trigger + alarmHomeStatusElementSize);
    }

    return (returnValue);
}

/**
    Check and update sounding state.
*/
static void alarmSoundingCheck(void) {   
    bool alarmSoundingNextState;

    // Convert pin state to bool
    if(inputsReadInputByName(alarmSounder) == LOW) {
        alarmSoundingNextState = false;
    }
    
    else {
        alarmSoundingNextState = true;
    }

    // State change of sounder so send out mqtt
    if(alarmSoundingNextState != alarmSounding) {
        alarmSounding = alarmSoundingNextState;
        alarmTransmitAlarmStatusMessage();
    }
}

/**
    Cycle task for the alarm.
*/
void alarmCyclicTask(void) {
    
    // Debounce alarm PIRs
    if(alarmDebounceTrigger(&alarmHomeStatus[0].pirState) == true) {
        alarmTransmitAlarmPirMessage();
    }

    // Debounce alarm source
    if(alarmDebounceTrigger(&alarmHomeStatus[0].alarmSource) == true) {
        alarmTransmitAlarmSourceMessage();
    }

    // Check if the alarm is sounding
    alarmSoundingCheck();    
}

/**
    Transmit a alarm status message.
    No processing of the message here.
*/ 
void alarmTransmitAlarmStatusMessage(void) {
    messsagesTxAlarmStatusMessage(&alarmStatusDataTable);
}

/**
    Transmit a alarm PIR message.
    No processing of the message here.
*/ 
void alarmTransmitAlarmPirMessage(void) {
    messsagesTxAlarmTriggerMessage(alarmHomeStatus, &alarmHomeStatusSize, &alarmHomeStatusElementSize, alarmTriggerPir);
}

/**
    Transmit a alarm source message.
    No processing of the message here.
*/ 
void alarmTransmitAlarmSourceMessage(void) {
    messsagesTxAlarmTriggerMessage(alarmHomeStatus, &alarmHomeStatusSize, &alarmHomeStatusElementSize, alarmTriggerSource);
}

/**
    Transmit a ALL alarm messages.
    No processing of the message here.
*/ 
void alarmTransmitAlarmAllMessage(void) {
    alarmTransmitAlarmStatusMessage();
    alarmTransmitAlarmPirMessage();
    alarmTransmitAlarmSourceMessage();
}
