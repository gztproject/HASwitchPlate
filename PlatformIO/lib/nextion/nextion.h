#ifndef NEXTION_H
#define NEXTION_H

#include <Arduino.h>
#include <mqttWrapper.h>

class nextion
{
public:
    nextion();

private:
};

byte nextionReturnBuffer[128];                      // Byte array to pass around data coming from the panel
uint8_t nextionReturnIndex = 0;                     // Index for nextionReturnBuffer
uint8_t nextionActivePage = 0;                      // Track active LCD page
bool lcdConnected = false;                          // Set to true when we've heard something from the LCD

void nextionSetAttr(String hmiAttribute, String hmiValue);
void nextionGetAttr(String hmiAttribute);
void nextionSendCmd(String nextionCmd);
void nextionParseJson(String &strPayload);
bool nextionHandleInput();
void nextionReset();
void nextionProcessInput();
bool nextionOtaResponse();
void nextionStartOtaDownload(String otaUrl);
void nextionSetSpeed();
void nextionConnect();

#endif