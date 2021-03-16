#ifndef MQTT_H
#define MQTT_H

// mqtt setup
void mqttSetup(void);

// mqtt client loop
void mqttClientLoop(void);

// Send mqtt messages
void mqttMessageLoop(void);

// Get function for messageText
String* const messageTextGet(void);

#endif