#ifndef MESSAGES_TX_H
#define MESSAGES_TX_H

#include "version.h"
#include "runtime.h"

/**
    Transmit a version message.
    Convert the message structure into JSON format here.
*/
void messsagesTxVersionMessage(const versionData * const versionDataStructurePtr, const unsigned int * const versionDataStructureSize);

/**
    Transmit a runtime message.
    Convert the message structure into JSON format here.
*/
void messsagesTxRuntimeMessage(const runtimeData * const runtimeDataStructurePtr, const unsigned int * const runtimeDataStructureSize);

#endif
