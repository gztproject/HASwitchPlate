#ifndef NEXTION_H
#define NEXTION_H

#include <Arduino.h>
#include <mqttWrapper.h>

#define RESET_PIN D6

class nextion
{
public:

    static byte returnBuffer[128]; // Byte array to pass around data coming from the panel
    static uint8_t returnIndex;    // Index for nextionReturnBuffer
    static uint8_t activePage;     // Track active LCD page
    static bool lcdConnected;      // Set to true when we've heard something from the LCD
    static String model;                                // Record reported model number of LCD panel
    static const byte suffix[];    // Standard suffix for Nextion commands
    static uint32_t tftFileSize;                      // Pin for Nextion power rail switch (GPIO12/D6)

    static bool reportPage0;                    // If false, don't report page 0 sendme

    static unsigned long lcdVersion;                   // Int to hold current LCD FW version number

    static void begin();
    static void setAttr(String hmiAttribute, String hmiValue);
    static void getAttr(String hmiAttribute);
    static void sendCmd(String nextionCmd);
    static void parseJson(String &strPayload);
    static bool handleInput();
    static void reset();
    static void processInput();
    static bool otaResponse();
    static void startOtaDownload(String otaUrl);
    static void setSpeed();
    static void connect();

private:
    nextion(){};
    
    static unsigned long updateLcdAvailableVersion;        // Int to hold the new LCD FW version number
    static bool lcdVersionQueryFlag;               // Flag to set if we've queried lcdVersion
    static const String lcdVersionQuery; // Object ID for lcdVersion in HMI
};

#endif