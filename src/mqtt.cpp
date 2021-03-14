#include "mqtt.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "credentials.h"

// json settings
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

    @param[in]     topic Pointer to the string containing the topic.
    @param[in]     payload Pointer to the message payload.
    @param[in]     length Message length.
*/
static void mqttMessageCallback(char* topic, byte* payload, unsigned int length) {
    StaticJsonDocument<JSON_DOC_SIZE> doc;

    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("]: ");
  
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    
    Serial.println();

    DeserializationError error = deserializeJson(doc, payload);
    
    // Check if the converted string was valid json
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
    }

    else {
        // Check if the json contains a valid led value
        if(doc.containsKey(JSON_DOC_VAR_LED)) {
            Serial.println("LED var found");

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

/**
    mqtt connection and subscription.
*/
static void mqttReconnect(void) {   
    // Get the mac of the wifi
    byte rawMAC[6];
    char processedMAC[13];
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
    String clientId = String() + "ESP8266Client-" + processedMAC;
    //String clientId = "ESP8266Client-" + String(WiFi.macAddress()); 

    // Attempt to connect
    Serial.print("Attempting MQTT connection...");
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
        Serial.println(" Connected...");
        client.subscribe((String() + mqtt_prefix + '/' + mqtt_command).c_str());
    } 
    else {
        Serial.print(" Failed, rc=");
        Serial.print(client.state());
        Serial.println(". Try again later.");
    }
}

/**
    mqtt setup.
*/
void mqttSetup(void) {
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttMessageCallback);

  // Connect and publish a first message
  mqttReconnect();  
  client.publish((String() + mqtt_prefix + '/' + mqtt_status).c_str(), messageStatus().c_str());
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
    // If the client isn't connected, try and reconnect
    if (!client.connected()) {
        mqttReconnect();
    }

    client.publish((String() + mqtt_prefix + '/' + mqtt_status).c_str(), messageStatus().c_str());
}
