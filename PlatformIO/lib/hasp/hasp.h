#ifndef HASP_H
#define HASP_H

#include <Arduino.h>
#include <config.h>

#include <nextion.h>
#include <persistance.h>

#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPUpdateServer.h>

#define CONNECT_TIMEOUT 300  // Timeout for WiFi and MQTT connection attempts in seconds
#define RECONNECT_TIMEOUT 15 // Timeout for WiFi reconnection attempts in seconds

class hasp
{
public:
    // Variables
    static const float version; // Current HASP software release version
    static byte mac[6]; // Byte array to store our MAC address
    static char *node;
    static char *wifiSSID;
    static char *wifiPass;
    static char *groupName;
    static char *configUser;
    static char *configPassword;
    static char *mqttServer;
    static uint8_t mqttPort;
    static char *mqttUser;
    static char *mqttPassword;
    static uint8_t motionPin;

    static bool shouldSaveConfig; // Flag to save json config to SPIFFS

    static bool mdnsEnabled;             // mDNS enabled
    static bool beepEnabled;             // Keypress beep enabled
    static unsigned long beepPrevMillis; // will store last time beep was updated
    static unsigned long beepOnTime;     // milliseconds of on-time for beep
    static unsigned long beepOffTime;    // milliseconds of off-time for beep
    static boolean beepState;            // beep currently engaged
    static unsigned int beepCounter;     // Count the number of beeps
    static byte beepPin;                 // define beep pin output

    static bool debugTelnetEnabled;         // Enable telnet debug output

    // Functions
    static void wifiSetup();
    static void debugPrintln(String debugText);
    static void debugPrint(String debugText);
    static void reset();
    static void startOta(String espOtaUrl);
    static void wifiConfigCallback(WiFiManager *myWiFiManager);
    static void wifiReconnect();
    static void setupOta();
    static bool updateCheck();
    static WiFiClient getClient();

private:
    hasp(){};
    static WiFiClient wifiClient;

    static bool updateEspAvailable;         // Flag for update check to report new ESP FW version
    static float updateEspAvailableVersion; // Float to hold the new ESP FW version number
    static bool updateLcdAvailable;         // Flag for update check to report new LCD FW version
    static bool debugSerialEnabled;         // Enable USB serial debug output    
    static bool debugSerialD8Enabled;       // Enable hardware serial debug output on pin D8
};

#endif