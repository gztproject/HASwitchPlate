#ifndef MQTT_WRAPPER_H
#define MQTT_WRAPPER_H

#include <Arduino.h>
#include <MQTT.h>
#include <ESP8266WiFi.h>

class mqttWrapper
{
public:
    mqttWrapper(const char *mqttServer, const char *mqttPort, const char *mqttUser, const char *mqttPassword, uint16_t maxPacketSize = 4096);
    void begin();
    void connect();
    void statusUpdate();
    void callback(String &strTopic, String &strPayload);

private:    
    String mqttClientId;                     // Auto-generated MQTT ClientID
    String mqttGetSubtopic;                  // MQTT subtopic for incoming commands requesting .val
    String mqttGetSubtopicJSON;              // MQTT object buffer for JSON status when requesting .val
    String mqttStateTopic;                   // MQTT topic for outgoing panel interactions
    String mqttStateJSONTopic;               // MQTT topic for outgoing panel interactions in JSON format
    String mqttCommandTopic;                 // MQTT topic for incoming panel commands
    String mqttGroupCommandTopic;            // MQTT topic for incoming group panel commands
    String mqttStatusTopic;                  // MQTT topic for publishing device connectivity state
    String mqttSensorTopic;                  // MQTT topic for publishing device information in JSON format
    String mqttLightCommandTopic;            // MQTT topic for incoming panel backlight on/off commands
    String mqttBeepCommandTopic;             // MQTT topic for error beep
    String mqttLightStateTopic;              // MQTT topic for outgoing panel backlight on/off state
    String mqttLightBrightCommandTopic;      // MQTT topic for incoming panel backlight dimmer commands
    String mqttLightBrightStateTopic;        // MQTT topic for outgoing panel backlight dimmer state
    String mqttMotionStateTopic;             // MQTT topic for outgoing motion sensor state
    const char *server;
    const char *port;
    const char *user;
    const char *password;

    WiFiClient wifiMQTTClient;
    MQTTClient mqttClient;
};

#endif