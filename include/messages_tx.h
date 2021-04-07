#ifndef MESSAGES_TX_H
#define MESSAGES_TX_H

#include "version.h"
#include "runtime.h"
#include "wifi.h"

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

#endif
