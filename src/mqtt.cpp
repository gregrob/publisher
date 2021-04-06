#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "debug.h"
#include "credentials.h"
#include "wifi.h"
#include "mqtt.h"
#include "runtime.h"

// Definitions
#define MQTT_PORT               (1883)
#define JSON_DOC_SIZE           (256)
#define JSON_DOC_VAR_RESET      "reset"
#define JSON_DOC_VAR_ARMDISARM  "armdisarm"

// Static functions
static void mqttMessageCallback(char* topic, byte* payload, unsigned int length);
static void mqttReconnect(void);
static void mqttMessageSubscribe(const char * const shortTopic);
static void mqttMessageSendRaw(const char * const shortTopic, const String * const message);

// MQTT settings
const char* mqtt_server = "swarm.max.lan";
const char* mqtt_user = MQTT_USER;
const char* mqtt_password = MQTT_PASSWORD;
const char* mqtt_prefix = "publisher";
const char* mqtt_alarm_triggers = "alarm triggers";
const char* mqtt_alarm_status = "alarm status";
const char* mqtt_alarm_command = "alarm command";
const char* mqtt_module_software = "module software";
const char* mqtt_module_status = "module status";
const char* mqtt_module_command = "module command";

// MQTT client connection
WiFiClient espClient;
PubSubClient client(espClient);

/**
    Call back for handling received mqtt messages.
    Callback also processes the command message.

    @param[in]     topic pointer to the string containing the topic.
    @param[in]     payload pointer to the message payload.
    @param[in]     length message length.
*/
static void mqttMessageCallback(char* topic, byte* payload, unsigned int length) {
    StaticJsonDocument<JSON_DOC_SIZE> doc;
    char payloadBuffer[MQTT_MAX_PACKET_SIZE];
    String debugMessage;

    // Make sure that message is smaller than the buffer
    if((length + 1) <= MQTT_MAX_PACKET_SIZE) {
        // Extract the message part of the payload
        snprintf(payloadBuffer, (length + 1), "%s", payload);
        
        debugMessage = (String() + "MQTT RX message [" + topic + "]: " + payloadBuffer);
        debugLog(&debugMessage, info);

        DeserializationError jsonError = deserializeJson(doc, payloadBuffer);
    
        // Check if the converted string was valid json
        if (jsonError) {
            debugMessage = (String() + "MQTT deserializeJson() failed: " + jsonError.f_str());
            debugLog(&debugMessage, error);
        }

        else {

            // Module command message (+1 because of /) 
            if (strcmp(topic + strlen(mqtt_prefix) + 1 + strlen(getWiFiModuleDetails()->moduleHostName) + 1, mqtt_module_command) == 0) {
                // Check if the json contains a valid text value
                if (doc.containsKey(JSON_DOC_VAR_RESET)) {               
                    String rxText = doc[JSON_DOC_VAR_RESET];

                    debugMessage = (String() + "MQTT found JSON key: " + JSON_DOC_VAR_RESET);
                    debugLog(&debugMessage, info);

                    resetWifi();
                }
            }

            // Alarm command message (+1 because of /)
            else if (strcmp(topic + strlen(mqtt_prefix) + 1 + strlen(getWiFiModuleDetails()->moduleHostName) + 1, mqtt_alarm_command) == 0) {

                // Check if the json contains a valid text value
                if (doc.containsKey(JSON_DOC_VAR_ARMDISARM)) {               
                    String rxText = doc[JSON_DOC_VAR_ARMDISARM];
                    alarmFireOneShot(5, 5);

                    debugMessage = (String() + "MQTT found JSON key: " + JSON_DOC_VAR_ARMDISARM);
                    debugLog(&debugMessage, info);
                }
            }            

            // Valid json so publish a status message
            mqttMessageSendModuleStatus();
        }
    }

    else {
        debugMessage = (String() + "Received message bigger than allocated buffer of " + MQTT_MAX_PACKET_SIZE);
        debugLog(&debugMessage, error);
    }    
}

/**
    Handle MQTT connection and subscription.
*/
static void mqttReconnect(void) {   
    String debugMessage;

    // Topic prefix and host
    String topicPrefixHost = String() + mqtt_prefix + '/' + getWiFiModuleDetails()->moduleHostName + '/';

    // Create a client ID based on the mac address
    String clientId = (String() + getWiFiModuleDetails()->moduleHostName);

    // Attempt to connect
    debugMessage = (String() + "MQTT attempting connection from " + clientId.c_str() + " to " + mqtt_server  + ":" + MQTT_PORT);
    debugLog(&debugMessage, info);
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
        debugMessage = (String() + "MQTT connected to " + mqtt_server + ":" + MQTT_PORT);
        debugLog(&debugMessage, info);
        
        mqttMessageSubscribe(mqtt_module_command);
        mqttMessageSubscribe(mqtt_alarm_command);     
    } 
    else {
        debugMessage = (String() + "MQTT connection to " + mqtt_server + ":" + MQTT_PORT + " failed (rc=" + client.state() + "), will retry later");
        debugLog(&debugMessage, error);
    }
}


/**
    MQTT setup.
*/
void mqttSetup(void) {
  client.setServer(mqtt_server, MQTT_PORT);
  client.setCallback(mqttMessageCallback);

  // Connect
  mqttReconnect();  
}


/**
    MQTT client loop.
*/
void mqttClientLoop(void) {
    client.loop();
}


/**
    MQTT message loop. 
    Handle disconnections / reconnections.
*/
void mqttMessageLoop(void) {

    // If the client isn't connected, try and reconnect
    if (!client.connected()) {
        mqttReconnect();
    }   
}


/**
    Subscribe to MQTT message. 
    This function will construct the topic [mqtt_prefix]/[hostname]/[shortTopic]. 

    @param[in]     shortTopic pointer to the topic (short form without prefix and hostname)
    @param[in]     message pointer to the message
*/
static void mqttMessageSubscribe(const char * const shortTopic) {
    // Create the full message topic with prefix and hostname
    String fullTopic = String() + mqtt_prefix + '/' + getWiFiModuleDetails()->moduleHostName + '/' + shortTopic;
   
    // Debug message
    String debugMessage;

    // Subscribe to the message
    client.subscribe(fullTopic.c_str());
    debugMessage = (String() + "MQTT subscribed to message [" + fullTopic + "]");
    debugLog(&debugMessage, info);
}


/**
    Send a raw MQTT message. 
    This function will construct the topic [mqtt_prefix]/[hostname]/[shortTopic]. 
    This function will not alter the message to be sent.

    @param[in]     shortTopic pointer to the topic (short form without prefix and hostname)
    @param[in]     message pointer to the message
*/
static void mqttMessageSendRaw(const char * const shortTopic, const String * const message) {
    // Create the full message topic with prefix and hostname
    String fullTopic = String() + mqtt_prefix + '/' + getWiFiModuleDetails()->moduleHostName + '/' + shortTopic;
   
    // Debug message
    String debugMessage;

    // If the client isn't connected, try and reconnect
    if (!client.connected()) {
        mqttReconnect();
    }  

    // Transmit the message
    client.publish(fullTopic.c_str(), message->c_str());
    debugMessage = (String() + "MQTT TX message [" + fullTopic + "]: " + message->c_str());
    debugLog(&debugMessage, info);
}


/**
    Send a Alarm Trigger state message via MQTT. 
    The message contains all alarm trigger status.

    @param[in]     zoneInputData pointer to the zone input structure
    @param[in]     zones number of zones in the zoneInputData
*/
void mqttMessageSendAlarmTriggers(const alarmZoneInput * const zoneInputData, const unsigned int zones) {
    // Crate a JSON object
    DynamicJsonDocument doc(JSON_DOC_SIZE);

    // Message string to be transmitted
    String messageString;

    // Append all the zone status
    for(unsigned int i = 0; i < zones; i++) {
        doc[(zoneInputData + i)->zoneName] = (zoneInputData + i)->triggered;
    }

    // searilise the JSON string
    serializeJson(doc, messageString);
   
    // Transmit the message
    mqttMessageSendRaw(mqtt_alarm_triggers, &messageString);
}


/**
    Send a Alarm Status message via MQTT. 
    The message contains alarm state and debug information.

    @param[in]     alarmState pointer to the alarm state
    @param[in]     alarmRxMxgCtr pointer to the total number of alarm messages processed
*/
void mqttMessageSendAlarmStatus(const char * const alarmState, const unsigned int * const alarmRxMxgCtr) {
    // Crate a JSON object
    DynamicJsonDocument doc(JSON_DOC_SIZE);

    // Message string to be transmitted
    String messageString;

    doc["state"] = alarmState;
    doc["messages"] = *alarmRxMxgCtr;
    
    // searilise the JSON string
    serializeJson(doc, messageString);

    // Transmit the message
    mqttMessageSendRaw(mqtt_alarm_status, &messageString);
}


/**
    Send a Module Software message via MQTT. 
    The message contains build information.
*/
void mqttMessageSendModuleSoftware(void) {
    // Crate a JSON object
    DynamicJsonDocument doc(JSON_DOC_SIZE);

    // Message string to be transmitted
    String messageString;

    doc["date"] = __DATE__;
    doc["time"] = __TIME__;
    
    // searilise the JSON string
    serializeJson(doc, messageString);

    // Transmit the message
    mqttMessageSendRaw(mqtt_module_software, &messageString);
}

/**
    Send a Module Status message via MQTT. 
    The message contains module IP and runtime information.
*/
void mqttMessageSendModuleStatus(void) {
    // Crate a JSON object
    DynamicJsonDocument doc(JSON_DOC_SIZE);

    // Message string to be transmitted
    String messageString;

    doc["millis"] = millis();
    doc["peakrt"] = getPeakRuntimeuS();
    doc["averagert"] = getAverageRuntimeuS();
    doc["ip"] = WiFi.localIP().toString();
    doc["mac"] = WiFi.macAddress();
    doc["rssi"] = WiFi.RSSI();
    doc["led"] = !digitalRead(LED_BUILTIN);

    // searilise the JSON string
    serializeJson(doc, messageString);

    // Transmit the message
    mqttMessageSendRaw(mqtt_module_status, &messageString);
}
