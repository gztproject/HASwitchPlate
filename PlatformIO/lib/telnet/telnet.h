#ifndef TELNET_H
#define TELNET_H

#include <nextion.h>

#include <ESP8266WiFi.h>

#define TELNET_INPUT_MAX 128 // Size of user input buffer for user telnet session

class telnet
{
public:
    static void handleClient();
    static WiFiServer getServer();

private:
    telnet(){};
    static WiFiServer server;
    static WiFiClient client;
};

#endif