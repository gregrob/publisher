#ifndef MQTT_H
#define MQTT_H

/**
    MQTT setup.
*/
void mqttSetup(void);

/**
    MQTT client loop.
*/
void mqttClientLoop(void);

/**
    MQTT message loop. 
    Handle disconnections / reconnections.
*/
void mqttMessageLoop(void);

/**
    Send a raw MQTT message. 
    This function will construct the topic [mqtt_prefix]/[hostname]/[shortTopic]. 
    This function will not alter the message to be sent.

    @param[in]     shortTopic pointer to the topic (short form without prefix and hostname)
    @param[in]     message pointer to the message
*/
void mqttMessageSendRaw(const char * const shortTopic, const char * const  message);

#endif
