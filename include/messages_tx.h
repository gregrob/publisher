#ifndef MESSAGES_TX_H
#define MESSAGES_TX_H

#include "version.h"
#include "runtime.h"
#include "wifi.h"
#include "alarm.h"
#include "nvm.h"

/**
    Transmit a version message.
    Convert the message structure into JSON format here.
    
    @param[in]     versionDataStructurePtr pointer to the version data structure
    @param[in]     versionDataStructureSize size of the version data structure
*/
void messsagesTxVersionMessage(const versionData * const versionDataStructurePtr, const unsigned int * const versionDataStructureSize);

/**
    Transmit a runtime message.
    Convert the message structure into JSON format here.

    @param[in]     runtimeDataStructurePtr pointer to the runtime data structure
    @param[in]     runtimeDataStructureSize size of the runtime data structure
*/
void messsagesTxRuntimeMessage(const runtimeData * const runtimeDataStructurePtr, const unsigned int * const runtimeDataStructureSize);

/**
    Transmit a wifi message.
    Convert the message structure into JSON format here.

    @param[in]     wifiDataStructurePtr pointer to the wifi data structure
*/
void messsagesTxWifiMessage(const wifiData * const wifiDataStructurePtr);

/**
    Transmit a alarm status message.
    Convert the message structure into JSON format here.

    @param[in]     alarmStatusDataStructurePtr pointer to the alarm status data structure
*/
void messsagesTxAlarmStatusMessage(const alarmStatusData * const alarmStatusDataStructurePtr);

/**
    Transmit a alarm trigger message.
    Convert the message structure into JSON format here.

    @param[in]     alarmDataStructurePtr pointer to the alarm data structure
    @param[in]     alarmStructureSize size of the alarm data structure
    @param[in]     alarmStructureElementSize size of an element in the alarm data structure
    @param[in]     triggerType type of trigger message to be transmitted
*/
void messsagesTxAlarmTriggerMessage(const alarmZoneInput * const alarmDataStructurePtr, const unsigned int * const alarmStructureSize, const unsigned int * const alarmStructureElementSize, const alarmTriggerTypes triggerType);

/**
    Transmit a NVM status message.
    Convert the message structure into JSON format here.

    @param[in]     nvmDataStructurePtr pointer to the NVM data structure
*/
void messsagesTxNvmStatusMessage(const nvmData * const nvmDataStructurePtr);

#endif
