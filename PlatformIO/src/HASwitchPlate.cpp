////////////////////////////////////////////////////////////////////////////////////////////////////
//           _____ _____ _____ _____
//          |  |  |  _  |   __|  _  |
//          |     |     |__   |   __|
//          |__|__|__|__|_____|__|
//        Home Automation Switch Plate
// https://github.com/aderusha/HASwitchPlate
//
// Copyright (c) 2019 Allen Derusha allen@derusha.org
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this hardware,
// software, and associated documentation files (the "Product"), to deal in the Product without
// restriction, including without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Product, and to permit persons to whom the
// Product is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Product.
//
// THE PRODUCT IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE PRODUCT OR THE USE OR OTHER DEALINGS IN THE PRODUCT.
////////////////////////////////////////////////////////////////////////////////////////////////////
#include <Arduino.h>
#include <config.h>
#include <hasp.h>
#include <motion.h>
#include <mqttWrapper.h>
#include <nextion.h>
#include <persistance.h>
#include <telnet.h>
#include <web.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

#include <ESP8266mDNS.h>

#include <DNSServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPUpdateServer.h>

const unsigned long updateCheckInterval = 43200000; // Time in msec between update checks (12 hours)
unsigned long updateCheckTimer = 0;                 // Timer for update check

bool startupCompleteFlag = false;               // Startup process has completed
const long statusUpdateInterval = 300000;       // Time in msec between publishing MQTT status updates (5 minutes)
long statusUpdateTimer = 0;                     // Timer for update check

ESP8266HTTPUpdateServer httpOTAUpdate;
MDNSResponder::hMDNSService hMDNSService;

/**
 *  Function declarations (C/C++ compatibility)
 */

String getSubtringField(String data, char separator, int index);
String printHex8(byte *data, uint8_t length);

void setup()
{
  // System setup
  nextion::begin();

  Serial.begin(115200);  // Serial - LCD RX (after swap), debug TX
  Serial1.begin(115200); // Serial1 - LCD TX, no RX
  Serial.swap();

  hasp::debugPrintln(String(F("SYSTEM: Starting HASwitchPlate v")) + String(hasp::version));
  hasp::debugPrintln(String(F("SYSTEM: Last reset reason: ")) + String(ESP.getResetInfo()));

  persistance::read(); // Check filesystem for a saved config.json

  while (!nextion::lcdConnected && (millis() < 5000))
  { // Wait up to 5 seconds for serial input from LCD
    nextion::handleInput();
  }
  if (nextion::lcdConnected)
  {
    hasp::debugPrintln(F("HMI: LCD responding, continuing program load"));
    nextion::sendCmd("connect");
  }
  else
  {
    hasp::debugPrintln(F("HMI: LCD not responding, continuing program load"));
  }

  hasp::wifiSetup(); // Start up networking

  if (hasp::mdnsEnabled)
  { // Setup mDNS service discovery if enabled
    hMDNSService = MDNS.addService(HASP_NODE, "http", "tcp", 80);
    if (hasp::debugTelnetEnabled)
    {
      MDNS.addService(hasp::node, "telnet", "tcp", 23);
    }
    MDNS.addServiceTxt(hMDNSService, "app_name", "HASwitchPlate");
    MDNS.addServiceTxt(hMDNSService, "app_version", String(hasp::version).c_str());
    MDNS.update();
  }
  if ((WEB_PASS[0] != '\0') && (WEB_USER[0] != '\0'))
  { // Start the webserver with our assigned password if it's been configured...
    httpOTAUpdate.setup(web::getServer(), "/update", WEB_USER, WEB_PASS);
  }
  else
  { // or without a password if not
    httpOTAUpdate.setup(web::getServer(), "/update");
  }
  web::begin();

  hasp::debugPrintln(String(F("HTTP: Server started @ http://")) + WiFi.localIP().toString());

  hasp::setupOta(); // Start OTA firmware update

  mqttWrapper::begin();

  motion::setup(); // Setup motion sensor if configured

  if (hasp::beepEnabled)
  { // Setup beep/tactile if configured
    hasp::beepPin = 4;
    pinMode(hasp::beepPin, OUTPUT);
  }

  if (hasp::debugTelnetEnabled)
  { // Setup telnet server for remote debug output
    telnet::getServer().setNoDelay(true);
    telnet::getServer().begin();
    hasp::debugPrintln(String(F("TELNET: debug server enabled at telnet:")) + WiFi.localIP().toString());
  }

  hasp::debugPrintln(F("SYSTEM: System init complete."));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void loop()
{ // Main execution loop

  if (nextion::handleInput())
  { // Process user input from HMI
    nextion::processInput();
  }

  while ((WiFi.status() != WL_CONNECTED) || (WiFi.localIP().toString() == "0.0.0.0"))
  { // Check WiFi is connected and that we have a valid IP, retry until we do.
    if (WiFi.status() == WL_CONNECTED)
    { // If we're currently connected, disconnect so we can try again
      WiFi.disconnect();
    }
    hasp::wifiReconnect();
  }

  if (!mqttWrapper::getClient().connected())
  { // Check MQTT connection
    hasp::debugPrintln("MQTT: not connected, connecting.");
    mqttWrapper::connect();
  }

  mqttWrapper::getClient().loop();        // MQTT client loop
  ArduinoOTA.handle();      // Arduino OTA loop
  web::handleClient(); // webServer loop
  if (hasp::mdnsEnabled)
  {
    MDNS.update();
  }

  if ((nextion::lcdVersion < 1) && (millis() <= (nextion::retryMax * nextion::checkInterval)))
  { // Attempt to connect to LCD panel to collect model and version info during startup
    nextion::connect();
  }
  else if ((nextion::lcdVersion > 0) && (millis() <= (nextion::retryMax * nextion::checkInterval)) && !startupCompleteFlag)
  { // We have LCD info, so trigger an update check + report
    if (hasp::updateCheck())
    { // Send a status update if the update check worked
      mqttWrapper::statusUpdate();
      startupCompleteFlag = true;
    }
  }
  else if ((millis() > (nextion::retryMax * nextion::checkInterval)) && !startupCompleteFlag)
  { // We still don't have LCD info so go ahead and run the rest of the checks once at startup anyway
    hasp::updateCheck();
    mqttWrapper::statusUpdate();
    startupCompleteFlag = true;
  }

  if ((millis() - statusUpdateTimer) >= statusUpdateInterval)
  { // Run periodic status update
    statusUpdateTimer = millis();
    mqttWrapper::statusUpdate();
  }

  if ((millis() - updateCheckTimer) >= updateCheckInterval)
  { // Run periodic update check
    updateCheckTimer = millis();
    if (hasp::updateCheck())
    { // Send a status update if the update check worked
      mqttWrapper::statusUpdate();
    }
  }

  if (motion::enabled)
  { // Check on our motion sensor
    motion::update();
  }

  if (hasp::debugTelnetEnabled)
  {
    telnet::handleClient(); // telnetClient loop
  }

  if (hasp::beepEnabled)
  { // Process Beeps
    if ((hasp::beepState == true) && (millis() - hasp::beepPrevMillis >= hasp::beepOnTime) && ((hasp::beepCounter > 0)))
    {
      hasp::beepState = false;         // Turn it off
      hasp::beepPrevMillis = millis(); // Remember the time
      analogWrite(hasp::beepPin, 254); // start beep for beepOnTime
      if (hasp::beepCounter > 0)
      { // Update the beep counter.
        hasp::beepCounter--;
      }
    }
    else if ((hasp::beepState == false) && (millis() - hasp::beepPrevMillis >= hasp::beepOffTime) && ((hasp::beepCounter >= 0)))
    {
      hasp::beepState = true;          // turn it on
      hasp::beepPrevMillis = millis(); // Remember the time
      analogWrite(hasp::beepPin, 0);   // stop beep for beepOffTime
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions


////////////////////////////////////////////////////////////////////////////////
String printHex8(byte *data, uint8_t length)
{ // returns input bytes as printable hex values in the format 01 23 FF

  String hex8String;
  for (int i = 0; i < length; i++)
  {
    // hex8String += "0x";
    if (data[i] < 0x10)
    {
      hex8String += "0";
    }
    hex8String += String(data[i], HEX);
    if (i != (length - 1))
    {
      hex8String += " ";
    }
  }
  hex8String.toUpperCase();
  return hex8String;
}
