#ifndef WEB_H
#define WEB_H

#include <Arduino.h>
#include <hasp.h>
#include <config.h>
#include <ESP8266WebServer.h>


class web
{
public:
    static ESP8266WebServer server;
    
    static void begin();
    static void handleNotFound();
    static void handleRoot();
    static void handleSaveConfig();
    static void handleResetConfig();
    static void handleResetBacklight();
    static void handleFirmware();
    static void handleEspFirmware();
    static void handleLcdUpload();
    static void handleLcdUpdateSuccess();
    static void handleLcdUpdateFailure();
    static void handleLcdDownload();
    static void handleTftFileSize();
    static void handleReboot();
    static void handleClient();

    static ESP8266WebServer *getServer();

private:    
    web(){};
};

#endif