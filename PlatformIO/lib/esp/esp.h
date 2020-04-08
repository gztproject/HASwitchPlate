#ifndef ESP_H
#define ESP_H
#include <WiFiManager.h>

class esp
{
public:
    esp();

private:
};

void debugPrintln(String debugText);
void debugPrint(String debugText);
void espReset();
void espStartOta(String espOtaUrl);
void espWifiConfigCallback(WiFiManager *myWiFiManager);
void espWifiSetup();
void espWifiReconnect();
void espSetupOta();
bool updateCheck();

#endif