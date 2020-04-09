#ifndef MQTT_WRAPPER_H
#define MQTT_WRAPPER_H

#include <Arduino.h>
#include <hasp.h>
#include <web.h>

#include <MQTT.h>
#include <ESP8266WiFi.h>

class mqttWrapper
{
public:
    static String stateTopic;              // MQTT topic for outgoing panel interactions
    static String stateJSONTopic;          // MQTT topic for outgoing panel interactions in JSON format
    static String commandTopic;            // MQTT topic for incoming panel commands
    static String groupCommandTopic;       // MQTT topic for incoming group panel commands
    static String statusTopic;             // MQTT topic for publishing device connectivity state
    static String sensorTopic;             // MQTT topic for publishing device information in JSON format
    static String lightCommandTopic;       // MQTT topic for incoming panel backlight on/off commands
    static String beepCommandTopic;        // MQTT topic for error beep
    static String lightStateTopic;         // MQTT topic for outgoing panel backlight on/off state
    static String lightBrightCommandTopic; // MQTT topic for incoming panel backlight dimmer commands
    static String lightBrightStateTopic;   // MQTT topic for outgoing panel backlight dimmer state
    static String motionStateTopic;        // MQTT topic for outgoing motion sensor state
    static String getSubtopic;             // MQTT subtopic for incoming commands requesting .val
    static String getSubtopicJSON;         // MQTT object buffer for JSON status when requesting .val

    static void begin();
    static void connect();
    static void statusUpdate();
    static MQTTClient getClient();

private:
    static MQTTClient client;
    static String clientId; // Auto-generated MQTT ClientID

    static void callback(String &strTopic, String &strPayload);
    String getSubtringField(String data, char separator, int index);
    mqttWrapper(){};
};

#endif