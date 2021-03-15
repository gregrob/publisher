#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "debug.h"
#include "credentials.h"
#include "mqtt.h"

// definitions
#define MQTT_PORT         (1883)
#define JSON_DOC_SIZE     (256)
#define JSON_DOC_VAR_LED  "led"

// Static functions
static void mqttMessageCallback(char* topic, byte* payload, unsigned int length);
static void mqttReconnect(void);
static String messageStatus(void);

// mqtt settings
const char* mqtt_server = "swarm.max.lan";
const char* mqtt_user = MQTT_USER;
const char* mqtt_password = MQTT_PASSWORD;
const char* mqtt_prefix = "publisher";
const char* mqtt_status = "status";
const char* mqtt_command = "command";

// mqtt client connection
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
        
        debugMessage = (String() + "message arrived [" + topic + "]: " + payloadBuffer);
        debugLog(&debugMessage, info);

        DeserializationError jsonError = deserializeJson(doc, payloadBuffer);
    
        // Check if the converted string was valid json
        if (jsonError) {
            debugMessage = (String() + "deserializeJson() failed: " + jsonError.f_str());
            debugLog(&debugMessage, error);
        }

        else {
            // Check if the json contains a valid led value
            if(doc.containsKey(JSON_DOC_VAR_LED)) {
                debugMessage = (String() + "found json key: " + JSON_DOC_VAR_LED);
                debugLog(&debugMessage, info);

                // Adjust the LED state based on the led value
                if(doc[JSON_DOC_VAR_LED] == false) {
                    digitalWrite(LED_BUILTIN, HIGH);
                }
                else {
                    digitalWrite(LED_BUILTIN, LOW);
                }
            }

            // Valid json so publish a status message
            client.publish((String() + mqtt_prefix + '/' + mqtt_status).c_str(), messageStatus().c_str());
        }
    }

    else {
        debugMessage = (String() + "received message bigger than allocated buffer of " + MQTT_MAX_PACKET_SIZE);
        debugLog(&debugMessage, error);
    }    
}

/**
    mqtt connection and subscription.
*/
static void mqttReconnect(void) {   
    // Get the mac of the wifi
    byte rawMAC[6];
    char processedMAC[13];
    String debugMessage;
    WiFi.macAddress(rawMAC);

    // Preprocess a mac string without the : character
    sprintf(processedMAC, "%X%X%X%X%X%X",
      (unsigned)rawMAC[0], 
      (unsigned)rawMAC[1],
      (unsigned)rawMAC[2],
      (unsigned)rawMAC[3], 
      (unsigned)rawMAC[4],
      (unsigned)rawMAC[5]);

    // Create a client ID based on the mac address
    String clientId = (String() + "ESP8266Client-" + processedMAC);
    //String clientId = "ESP8266Client-" + String(WiFi.macAddress()); 

    // Attempt to connect
    debugMessage = (String() + "mqtt attempting connection from " + clientId.c_str() + " to " + mqtt_server  + ":" + MQTT_PORT);
    debugLog(&debugMessage, info);
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
        debugMessage = (String() + "mqtt connected to " + mqtt_server + ":" + MQTT_PORT);
        debugLog(&debugMessage, info);
        
        client.subscribe((String() + mqtt_prefix + '/' + mqtt_command).c_str());
    } 
    else {
        debugMessage = (String() + "mqtt connection to " + mqtt_server + ":" + MQTT_PORT + " failed (rc=" + client.state() + "), will retry later");
        debugLog(&debugMessage, error);
    }
}

/**
    mqtt setup.
*/
void mqttSetup(void) {
  String debugMessage;

  client.setServer(mqtt_server, MQTT_PORT);
  client.setCallback(mqttMessageCallback);

  // Connect and publish a first message
  mqttReconnect();  
  client.publish((String() + mqtt_prefix + '/' + mqtt_status).c_str(), messageStatus().c_str());
  debugMessage = (String() + "first message transmitted [" + mqtt_prefix + '/' + mqtt_status + "]: " + messageStatus().c_str());
  debugLog(&debugMessage, info);
}

/**
    Create a status message.

    @return        status message in json string format.
*/
static String messageStatus(void) {
    DynamicJsonDocument doc(JSON_DOC_SIZE);
    String returnString;

    doc["millis"] = millis();
    doc["ip"] = WiFi.localIP().toString();
    doc["mac"] = WiFi.macAddress();
    doc["rssi"] = WiFi.RSSI();
    doc["led"] = !digitalRead(LED_BUILTIN);

    serializeJson(doc, returnString);

    return(returnString);
}

/**
    mqtt client loop.
*/
void mqttClientLoop(void) {
    // If the client isn't connected, try and reconnect
    if (!client.connected()) {
        mqttReconnect();
    }

    client.loop();
}

/**
    Send mqtt messages.
*/
void mqttMessageLoop(void) {
    String debugMessage;

    // If the client isn't connected, try and reconnect
    if (!client.connected()) {
        mqttReconnect();
    }

    client.publish((String() + mqtt_prefix + '/' + mqtt_status).c_str(), messageStatus().c_str());
    debugMessage = (String() + "subsequent message transmitted [" + mqtt_prefix + '/' + mqtt_status + "]: " + messageStatus().c_str());
    debugLog(&debugMessage, info);
}
