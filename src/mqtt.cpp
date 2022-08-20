#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "debug.h"
#include "nvm_cfg.h"

#include "wifi.h"
#include "mqtt.h"
#include "alarm.h"
#include "garage_door.h"
#include "outputs_cfg.h"
#include "reset_ctrl.h"

// Definitions
#define MQTT_PORT               (1883)
#define JSON_DOC_SIZE           (256)
#define JSON_DOC_VAR_RESET      "reset"
#define JSON_DOC_VAR_ARMDISARM  "armdisarm"
#define JSON_DOC_VAR_OPENCLOSE  "openclose"

// Static functions
static void mqttMessageCallback(char* topic, byte* payload, unsigned int length);
static void mqttReconnect(void);
static void mqttMessageSubscribe(const char * const shortTopic);
static void mqttMessageFullTopic(const char * const shortTopic, String * const longTopicPtr);

// MQTT settings
const char* mqtt_garage_door_command = "garage door command";
const char* mqtt_alarm_command = "alarm command";
const char* mqtt_module_command = "module command";
const char* mqttLwtMessage = "module status";

// LWT values
const char* mqttLwtValueOnline = "online";
const char* mqttLwtValueOffline = "offline";

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

    // Message topic base
    String mqttMessageTopicBase;

    StaticJsonDocument<JSON_DOC_SIZE> doc;
    char payloadBuffer[MQTT_MAX_PACKET_SIZE];
    String debugMessage;

    // Construct the message topic base
    mqttMessageFullTopic("", &mqttMessageTopicBase);

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
            if (strcmp(topic + mqttMessageTopicBase.length(), mqtt_module_command) == 0) {
                // Check if the json contains a valid text value
                if (doc.containsKey(JSON_DOC_VAR_RESET)) {               
                    String rxText = doc[JSON_DOC_VAR_RESET];

                    debugMessage = (String() + "MQTT found JSON key: " + JSON_DOC_VAR_RESET);
                    debugLog(&debugMessage, info);

                    resetCtrlTypes resetCommanded = doc[JSON_DOC_VAR_RESET];
                    restCtrlSetResetRequest(resetCommanded);
                }
                //uint16_t a = doc["command"];
                //uint16_t b = doc["hi"];
                //uint16_t c = doc["hi-tim"];
                //uint16_t d = doc["lo"];
                //uint16_t e = doc["lo-tim"];
               // outputsSetOutputByName(wifiRun, {(outputControlType)a, b, c, d, e});
                //outputsSetOutputByName(configMode, {(outputControlType)a, b, c, d, e});
                //Serial1.println(String() + a + " " + b + " " + c + " " + d + " " + e + " ");
            }

            // Alarm command message (+1 because of /)
            else if (strcmp(topic + mqttMessageTopicBase.length(), mqtt_alarm_command) == 0) {

                // Check if the json contains a valid text value
                if (doc.containsKey(JSON_DOC_VAR_ARMDISARM)) {               
                    String rxText = doc[JSON_DOC_VAR_ARMDISARM];
                    alarmFireOneShot();

                    debugMessage = (String() + "MQTT found JSON key: " + JSON_DOC_VAR_ARMDISARM);
                    debugLog(&debugMessage, info);
                }
            }

            // Garage door command message (+1 because of /)
            else if (strcmp(topic + mqttMessageTopicBase.length(), mqtt_garage_door_command) == 0) {

                // Check if the json contains a valid text value
                if (doc.containsKey(JSON_DOC_VAR_OPENCLOSE)) {               
                    String rxText = doc[JSON_DOC_VAR_OPENCLOSE];
                    garageDoorFireOneShot();

                    debugMessage = (String() + "MQTT found JSON key: " + JSON_DOC_VAR_OPENCLOSE);
                    debugLog(&debugMessage, info);
                }
            }
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
    
    // Debug message
    String debugMessage;

    // Full topic string
    String fullTopicLwt;

    // Pointer to the RAM mirror
    const nvmCompleteStructure * ramMirrorPtr;

    // Set-up pointer to RAM mirror
    (void) nvmGetRamMirrorPointerRO(&ramMirrorPtr);
    
    // Create a client ID based on the mac address
    String clientId = (String() + getWiFiModuleDetails()->moduleHostName);

    // Create the full message topic with prefix and hostname for LWT
    mqttMessageFullTopic(mqttLwtMessage, &fullTopicLwt);
    
    // Attempt to connect
    debugMessage = (String() + "MQTT attempting connection from " + clientId.c_str() + " to " + ramMirrorPtr->mqtt.mqttServer  + ":" + MQTT_PORT);
    debugLog(&debugMessage, info);

    if (client.connect(clientId.c_str(), ramMirrorPtr->mqtt.mqttUser, ramMirrorPtr->mqtt.mqttPassword, fullTopicLwt.c_str(), 0, true, mqttLwtValueOffline)) {
        debugMessage = (String() + "MQTT connected to " + ramMirrorPtr->mqtt.mqttServer + ":" + MQTT_PORT);
        debugLog(&debugMessage, info);
        
        mqttMessageSubscribe(mqtt_module_command);

        // Subscribing for module specific messages
        if (getWiFiModuleDetails()->moduleHostType == alarmModule) {
            mqttMessageSubscribe(mqtt_alarm_command);
        }
        else if (getWiFiModuleDetails()->moduleHostType == ultrasonicsModule) {
            
        }
        else if (getWiFiModuleDetails()->moduleHostType == garageDoorModule) {
            mqttMessageSubscribe(mqtt_garage_door_command);           
        }      

        // Transmit the LWT message
        client.publish(fullTopicLwt.c_str(), mqttLwtValueOnline, true);
        debugMessage = (String() + "MQTT TX LWT message [" + fullTopicLwt + "]: " + mqttLwtValueOnline);
        debugLog(&debugMessage, info);    
    } 
    else {
        debugMessage = (String() + "MQTT connection to " + ramMirrorPtr->mqtt.mqttServer + ":" + MQTT_PORT + " failed (rc=" + client.state() + "), will retry later");
        debugLog(&debugMessage, error);
    }
}


/**
    MQTT setup.
*/
void mqttSetup(void) {
    
    // Pointer to the RAM mirror
    const nvmCompleteStructure * ramMirrorPtr;

    // Set-up pointer to RAM mirror
    (void) nvmGetRamMirrorPointerRO(&ramMirrorPtr);
  
    client.setServer(ramMirrorPtr->mqtt.mqttServer, MQTT_PORT);
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
    Generate a MQTT topic. 
    This function will construct the full topic [mqtt_prefix]/[hostname]/[shortTopic]. 

    @param[in]     shortTopic pointer to the topic (short form without prefix and hostname)
    @param[in]     longTopicPtr pointer to the final location for the full topic
*/
static void mqttMessageFullTopic(const char * const shortTopic, String * const longTopicPtr) {
    
    // Full topic string
    String fullTopic;
    
    // Pointer to the RAM mirror
    const nvmCompleteStructure * ramMirrorPtr;

    // Set-up pointer to RAM mirror
    (void) nvmGetRamMirrorPointerRO(&ramMirrorPtr);

    // Create the full message topic with prefix and hostname
    *longTopicPtr = String() + ramMirrorPtr->mqtt.mqttTopicRoot + '/' + getWiFiModuleDetails()->moduleHostName + '/' + shortTopic;
}


/**
    Subscribe to MQTT message. 
    This function will construct the topic [mqtt_prefix]/[hostname]/[shortTopic]. 

    @param[in]     shortTopic pointer to the topic (short form without prefix and hostname)
    @param[in]     message pointer to the message
*/
static void mqttMessageSubscribe(const char * const shortTopic) {
    
    // Full topic string
    String fullTopic;
    
    // Create the full message topic with prefix and hostname
    mqttMessageFullTopic(shortTopic, &fullTopic);

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
void mqttMessageSendRaw(const char * const shortTopic, const char * const  message) {

    // Full topic string
    String fullTopic;

    // Create the full message topic with prefix and hostname
    mqttMessageFullTopic(shortTopic, &fullTopic);
    
    // Debug message
    String debugMessage;

    // If the client isn't connected, try and reconnect
    if (!client.connected()) {
        mqttReconnect();
    }  

    // Transmit the message
    client.publish(fullTopic.c_str(), message);
    debugMessage = (String() + "MQTT TX message [" + fullTopic + "]: " + message);
    debugLog(&debugMessage, info);
}
