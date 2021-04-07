#include <Arduino.h>
#include <ArduinoJson.h>

#include "messages_tx.h"
#include "messages_tx_cfg.h"

#include "mqtt.h"


// Size of the JSON document
#define MESSAGES_TX_JSON_DOCUMENT_SIZE  (256)

// Size of the tx message buffer
#define MESSAGES_TX_MESSAGE_BUFFER_SIZE (MESSAGES_TX_JSON_DOCUMENT_SIZE * 2)


// JSON static document
static StaticJsonDocument<MESSAGES_TX_JSON_DOCUMENT_SIZE> doc; 

// TX message buffer
static char messageToSend[MESSAGES_TX_MESSAGE_BUFFER_SIZE];

// MQTT topic for module software
static const char* messageMqttTopicModuleSoftware = MESSAGES_TX_MQTT_TOPIC_MODULE_SOFTWARE;

// MQTT topic for module runtime
static const char* messageMqttTopicModuleRuntime = MESSAGES_TX_MQTT_TOPIC_MODULE_RUNTIME;


/**
    Transmit a version message.
    Convert the message structure into JSON format here.
*/
void messsagesTxVersionMessage(const versionData * const versionDataStructurePtr, const unsigned int * const versionDataStructureSize) {
    
    // Clear the JSON object
    doc.clear();

    // Append all of the version data to the JSON object
    for (unsigned int i = 0; i < *versionDataStructureSize; i++) {
        doc[(versionDataStructurePtr + i)->versionName] = (versionDataStructurePtr + i)->versionContents;
    }
    
    // Searilise the JSON string
    serializeJson(doc, messageToSend, MESSAGES_TX_MESSAGE_BUFFER_SIZE);

    // Transmit the message
    mqttMessageSendRaw(messageMqttTopicModuleSoftware, messageToSend);   
}


/**
    Transmit a runtime message.
    Convert the message structure into JSON format here.
*/
void messsagesTxRuntimeMessage(const runtimeData * const runtimeDataStructurePtr, const unsigned int * const runtimeDataStructureSize) {
  
    // Clear the JSON object
    doc.clear();

    // Append all of the version data to the JSON object
    for (unsigned int i = 0; i < *runtimeDataStructureSize; i++) {
        doc[(runtimeDataStructurePtr + i)->runtimeName] = (unsigned long)*(runtimeDataStructurePtr + i)->runtimeContents;
    }
    
    // Searilise the JSON string
    serializeJson(doc, messageToSend, MESSAGES_TX_MESSAGE_BUFFER_SIZE);
    
    // Transmit the message
    mqttMessageSendRaw(messageMqttTopicModuleRuntime, messageToSend);   
}
