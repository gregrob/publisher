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

// MQTT topic for module NVM
static const char* messageMqttTopicModuleNvm = MESSAGES_TX_MQTT_TOPIC_MODULE_NVM;

// MQTT topic for module runtime
static const char* messageMqttTopicModuleRuntime = MESSAGES_TX_MQTT_TOPIC_MODULE_RUNTIME;

// MQTT topic for module wifi
static const char* messageMqttTopicModuleWifi = MESSAGES_TX_MQTT_TOPIC_MODULE_WIFI;

// MQTT topic for alarm status
static const char* messageMqttTopicAlarmStatus = MESSAGES_TX_MQTT_TOPIC_ALARM_STATUS;

// MQTT topic for alarm triggers
static const char* messageMqttTopicAlarmTriggers = MESSAGES_TX_MQTT_TOPIC_ALARM_TRIGGERS;

/**
    Transmit a version message.
    Convert the message structure into JSON format here.
    
    @param[in]     versionDataStructurePtr pointer to the version data structure
    @param[in]     versionDataStructureSize size of the version data structure
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

    @param[in]     runtimeDataStructurePtr pointer to the runtime data structure
    @param[in]     runtimeDataStructureSize size of the runtime data structure
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


/**
    Transmit a wifi message.
    Convert the message structure into JSON format here.

    @param[in]     wifiDataStructurePtr pointer to the wifi data structure
*/
void messsagesTxWifiMessage(const wifiData * const wifiDataStructurePtr) {
  
    // Clear the JSON object
    doc.clear();
   
    // Populate the date (manually because of mixed types)
    doc[wifiDataStructurePtr->ssidName] = wifiDataStructurePtr->ssidDataPtr;
    doc[wifiDataStructurePtr->ipAddressName] = wifiDataStructurePtr->ipAddressDataPtr;
    doc[wifiDataStructurePtr->gatewayAddressName] = wifiDataStructurePtr->gatewayAddressDataPtr;
    doc[wifiDataStructurePtr->subnetMaskName] = wifiDataStructurePtr->subnetMaskDataPtr;
    doc[wifiDataStructurePtr->macAddressName] = wifiDataStructurePtr->macAddressDataPtr;
    doc[wifiDataStructurePtr->rssiName] = *wifiDataStructurePtr->rssiDataPtr;

    // Searilise the JSON string
    serializeJson(doc, messageToSend, MESSAGES_TX_MESSAGE_BUFFER_SIZE);
    
    // Transmit the message
    mqttMessageSendRaw(messageMqttTopicModuleWifi, messageToSend);   
}

/**
    Transmit a alarm status message.
    Convert the message structure into JSON format here.

    @param[in]     alarmDataStructurePtr pointer to the alarm status data structure
*/
void messsagesTxAlarmStatusMessage(const alarmStatusData * const alarmStatusDataStructurePtr) {
    
    // Clear the JSON object
    doc.clear();
   
    // Populate the date (manually because of mixed types)
    doc[alarmStatusDataStructurePtr->alarmStateName] = alarmStatusDataStructurePtr->alarmStatePtr;
    doc[alarmStatusDataStructurePtr->alarmMessageCounterName] = *alarmStatusDataStructurePtr->alarmMessageCounterPtr;

    // Searilise the JSON string
    serializeJson(doc, messageToSend, MESSAGES_TX_MESSAGE_BUFFER_SIZE);
    
    // Transmit the message
    mqttMessageSendRaw(messageMqttTopicAlarmStatus, messageToSend);   
}

/**
    Transmit a alarm triggers message. 
    Convert the message structure into JSON format here.

    @param[in]     alarmTriggersDataStructurePtr pointer to the alarm triggers data structure
    @param[in]     alarmTriggersStructureSize size of the alarm triggers data structure
*/
void messsagesTxAlarmTriggersMessage(const alarmZoneInput * const alarmTriggersDataStructurePtr, const unsigned int * const alarmTriggersStructureSize) {
    
    // Clear the JSON object
    doc.clear();

    // Append all of the version data to the JSON object
    for (unsigned int i = 0; i < *alarmTriggersStructureSize; i++) {
        doc[(alarmTriggersDataStructurePtr + i)->zoneName] = (alarmTriggersDataStructurePtr + i)->triggered;
    }
    
    // Searilise the JSON string
    serializeJson(doc, messageToSend, MESSAGES_TX_MESSAGE_BUFFER_SIZE);
    
    // Transmit the message
    mqttMessageSendRaw(messageMqttTopicAlarmTriggers, messageToSend);
}


/**
    Transmit a NVM status message.
    Convert the message structure into JSON format here.

    @param[in]     nvmDataStructurePtr pointer to the NVM data structure
*/
void messsagesTxNvmStatusMessage(const nvmData * const nvmDataStructurePtr) {
    
    // Clear the JSON object
    doc.clear();
   
    // Populate the date (manually because of mixed types)
    doc["version"] = nvmDataStructurePtr->core.version;
    doc["bytesConsumed"] = nvmDataStructurePtr->bytesConsumed;
    doc["structures"] = nvmDataStructurePtr->structures;
    doc["errorCounter"] = nvmDataStructurePtr->core.errorCounter;
    
    // Searilise the JSON string
    serializeJson(doc, messageToSend, MESSAGES_TX_MESSAGE_BUFFER_SIZE);
    
    // Transmit the message
    mqttMessageSendRaw(messageMqttTopicModuleNvm, messageToSend);   
}
