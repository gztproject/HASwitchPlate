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
#include <esp.h>
#include <motion.h>
#include <mqttWrapper.h>
#include <nextion.h>
#include <persistance.h>
//#include <telnet.h>
#include <web.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
char wifiSSID[32] = WIFI_SSID; 
char wifiPass[64] = WIFI_PASS; 

char haspNode[16] = HASP_NODE;
char groupName[16] = GROUP_NAME;
char configUser[32] = WEB_USER;
char configPassword[32] = WEB_PASS;

////////////////////////////////////////////////////////////////////////////////////////////////////
#include <FS.h>

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPUpdateServer.h>

#include <ArduinoJson.h>

#include <SoftwareSerial.h>

mqttWrapper mqtt(MQTT_SRV, MQTT_PORT, MQTT_USER, MQTT_PASS, MQTT_MAX_PACKET_SIZE);
nextion nextion();

motion motion(MOTION_PIN);

const float haspVersion = 0.40;                     // Current HASP software release version

const char wifiConfigPass[9] = "hasplate";          // First-time config WPA2 password
const char wifiConfigAP[14] = "HASwitchPlate";      // First-time config SSID
bool shouldSaveConfig = false;                      // Flag to save json config to SPIFFS
bool nextionReportPage0 = false;                    // If false, don't report page 0 sendme
const unsigned long updateCheckInterval = 43200000; // Time in msec between update checks (12 hours)
unsigned long updateCheckTimer = 0;                 // Timer for update check
const unsigned long nextionCheckInterval = 5000;    // Time in msec between nextion connection checks
unsigned long nextionCheckTimer = 0;                // Timer for nextion connection checks
unsigned int nextionRetryMax = 5;                   // Attempt to connect to panel this many times
bool updateEspAvailable = false;                    // Flag for update check to report new ESP FW version
float updateEspAvailableVersion;                    // Float to hold the new ESP FW version number
bool updateLcdAvailable = false;                    // Flag for update check to report new LCD FW version
bool debugSerialEnabled = true;                     // Enable USB serial debug output
bool debugTelnetEnabled = false;                    // Enable telnet debug output
bool debugSerialD8Enabled = true;                   // Enable hardware serial debug output on pin D8
const unsigned long telnetInputMax = 128;           // Size of user input buffer for user telnet session

bool mdnsEnabled = true;                            // mDNS enabled
bool beepEnabled = false;                           // Keypress beep enabled
unsigned long beepPrevMillis = 0;                   // will store last time beep was updated
unsigned long beepOnTime = 1000;                    // milliseconds of on-time for beep
unsigned long beepOffTime = 1000;                   // milliseconds of off-time for beep
boolean beepState;                                  // beep currently engaged
unsigned int beepCounter;                           // Count the number of beeps
byte beepPin;                                       // define beep pin output

unsigned long lcdVersion = 0;                       // Int to hold current LCD FW version number
unsigned long updateLcdAvailableVersion;            // Int to hold the new LCD FW version number
bool lcdVersionQueryFlag = false;                   // Flag to set if we've queried lcdVersion
const String lcdVersionQuery = "p[0].b[2].val";     // Object ID for lcdVersion in HMI
bool startupCompleteFlag = false;                   // Startup process has completed
const long statusUpdateInterval = 300000;           // Time in msec between publishing MQTT status updates (5 minutes)
long statusUpdateTimer = 0;                         // Timer for update check
const unsigned long connectTimeout = 300;           // Timeout for WiFi and MQTT connection attempts in seconds
const unsigned long reConnectTimeout = 15;          // Timeout for WiFi reconnection attempts in seconds
byte espMac[6];                                     // Byte array to store our MAC address

String nextionModel;                                // Record reported model number of LCD panel
const byte nextionSuffix[] = {0xFF, 0xFF, 0xFF};    // Standard suffix for Nextion commands
uint32_t tftFileSize = 0;                           // Filesize for TFT firmware upload
uint8_t nextionResetPin = D6;                       // Pin for Nextion power rail switch (GPIO12/D6)

WiFiClient wifiClient;

ESP8266WebServer webServer(80);
ESP8266HTTPUpdateServer httpOTAUpdate;
WiFiServer telnetServer(23);
WiFiClient telnetClient;
MDNSResponder::hMDNSService hMDNSService;

// Additional CSS style to match Hass theme
const char HASP_STYLE[] = "<style>button{background-color:#03A9F4;}body{width:60%;margin:auto;}input:invalid{border:1px solid red;}input[type=checkbox]{width:20px;}</style>";
// URL for auto-update "version.json"
const char UPDATE_URL[] = "http://haswitchplate.com/update/version.json";
// Default link to compiled Arduino firmware image
String espFirmwareUrl = "http://haswitchplate.com/update/HASwitchPlate.ino.d1_mini.bin";
// Default link to compiled Nextion firmware images
String lcdFirmwareUrl = "http://haswitchplate.com/update/HASwitchPlate.tft";





/**
 *  Function declarations (C/C++ compatibility)
 */

String getSubtringField(String data, char separator, int index);
String printHex8(byte *data, uint8_t length);


void setup()
{ // System setup
  pinMode(nextionResetPin, OUTPUT);
  digitalWrite(nextionResetPin, HIGH);
  Serial.begin(115200);  // Serial - LCD RX (after swap), debug TX
  Serial1.begin(115200); // Serial1 - LCD TX, no RX
  Serial.swap();

  debugPrintln(String(F("SYSTEM: Starting HASwitchPlate v")) + String(haspVersion));
  debugPrintln(String(F("SYSTEM: Last reset reason: ")) + String(ESP.getResetInfo()));

  configRead(); // Check filesystem for a saved config.json

  while (!lcdConnected && (millis() < 5000))
  { // Wait up to 5 seconds for serial input from LCD
    nextionHandleInput();
  }
  if (lcdConnected)
  {
    debugPrintln(F("HMI: LCD responding, continuing program load"));
    nextionSendCmd("connect");
  }
  else
  {
    debugPrintln(F("HMI: LCD not responding, continuing program load"));
  }

  espWifiSetup(); // Start up networking

  if (mdnsEnabled)
  { // Setup mDNS service discovery if enabled
    hMDNSService = MDNS.addService(haspNode, "http", "tcp", 80);
    if (debugTelnetEnabled)
    {
      MDNS.addService(haspNode, "telnet", "tcp", 23);
    }
    MDNS.addServiceTxt(hMDNSService, "app_name", "HASwitchPlate");
    MDNS.addServiceTxt(hMDNSService, "app_version", String(haspVersion).c_str());
    MDNS.update();
  }

  if ((configPassword[0] != '\0') && (configUser[0] != '\0'))
  { // Start the webserver with our assigned password if it's been configured...
    httpOTAUpdate.setup(&webServer, "/update", configUser, configPassword);
  }
  else
  { // or without a password if not
    httpOTAUpdate.setup(&webServer, "/update");
  }
  webServer.on("/", webHandleRoot);
  webServer.on("/saveConfig", webHandleSaveConfig);
  webServer.on("/resetConfig", webHandleResetConfig);
  webServer.on("/resetBacklight", webHandleResetBacklight);
  webServer.on("/firmware", webHandleFirmware);
  webServer.on("/espfirmware", webHandleEspFirmware);
  webServer.on("/lcdupload", HTTP_POST, []() { webServer.send(200); }, webHandleLcdUpload);
  webServer.on("/tftFileSize", webHandleTftFileSize);
  webServer.on("/lcddownload", webHandleLcdDownload);
  webServer.on("/lcdOtaSuccess", webHandleLcdUpdateSuccess);
  webServer.on("/lcdOtaFailure", webHandleLcdUpdateFailure);
  webServer.on("/reboot", webHandleReboot);
  webServer.onNotFound(webHandleNotFound);
  webServer.begin();
  debugPrintln(String(F("HTTP: Server started @ http://")) + WiFi.localIP().toString());

  espSetupOta(); // Start OTA firmware update  
  
  mqtt.begin();

  motionSetup(); // Setup motion sensor if configured

  if (beepEnabled)
  { // Setup beep/tactile if configured
    beepPin = 4;
    pinMode(beepPin, OUTPUT);
  }

  if (debugTelnetEnabled)
  { // Setup telnet server for remote debug output
    telnetServer.setNoDelay(true);
    telnetServer.begin();
    debugPrintln(String(F("TELNET: debug server enabled at telnet:")) + WiFi.localIP().toString());
  }

  debugPrintln(F("SYSTEM: System init complete."));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void loop()
{ // Main execution loop

  if (nextionHandleInput())
  { // Process user input from HMI
    nextionProcessInput();
  }

  while ((WiFi.status() != WL_CONNECTED) || (WiFi.localIP().toString() == "0.0.0.0"))
  { // Check WiFi is connected and that we have a valid IP, retry until we do.
    if (WiFi.status() == WL_CONNECTED)
    { // If we're currently connected, disconnect so we can try again
      WiFi.disconnect();
    }
    espWifiReconnect();
  }

  if (!mqttClient.connected())
  { // Check MQTT connection
    debugPrintln("MQTT: not connected, connecting.");
    mqttConnect();
  }

  mqttClient.loop();        // MQTT client loop
  ArduinoOTA.handle();      // Arduino OTA loop
  webServer.handleClient(); // webServer loop
  if (mdnsEnabled)
  {
    MDNS.update();
  }

  if ((lcdVersion < 1) && (millis() <= (nextionRetryMax * nextionCheckInterval)))
  { // Attempt to connect to LCD panel to collect model and version info during startup
    nextionConnect();
  }
  else if ((lcdVersion > 0) && (millis() <= (nextionRetryMax * nextionCheckInterval)) && !startupCompleteFlag)
  { // We have LCD info, so trigger an update check + report
    if (updateCheck())
    { // Send a status update if the update check worked
      mqttStatusUpdate();
      startupCompleteFlag = true;
    }
  }
  else if ((millis() > (nextionRetryMax * nextionCheckInterval)) && !startupCompleteFlag)
  { // We still don't have LCD info so go ahead and run the rest of the checks once at startup anyway
    updateCheck();
    mqttStatusUpdate();
    startupCompleteFlag = true;
  }

  if ((millis() - statusUpdateTimer) >= statusUpdateInterval)
  { // Run periodic status update
    statusUpdateTimer = millis();
    mqttStatusUpdate();
  }

  if ((millis() - updateCheckTimer) >= updateCheckInterval)
  { // Run periodic update check
    updateCheckTimer = millis();
    if (updateCheck())
    { // Send a status update if the update check worked
      mqttStatusUpdate();
    }
  }

  if (motionEnabled)
  { // Check on our motion sensor
    motionUpdate();
  }

  if (debugTelnetEnabled)
  {
    handleTelnetClient(); // telnetClient loop
  }

  if (beepEnabled)
  { // Process Beeps
    if ((beepState == true) && (millis() - beepPrevMillis >= beepOnTime) && ((beepCounter > 0)))
    {
      beepState = false;         // Turn it off
      beepPrevMillis = millis(); // Remember the time
      analogWrite(beepPin, 254); // start beep for beepOnTime
      if (beepCounter > 0)
      { // Update the beep counter.
        beepCounter--;
      }
    }
    else if ((beepState == false) && (millis() - beepPrevMillis >= beepOffTime) && ((beepCounter >= 0)))
    {
      beepState = true;          // turn it on
      beepPrevMillis = millis(); // Remember the time
      analogWrite(beepPin, 0);   // stop beep for beepOffTime
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions

////////////////////////////////////////////////////////////////////////////////////////////////////
// Submitted by benmprojects to handle "beep" commands. Split
// incoming String by separator, return selected field as String
// Original source: https://arduino.stackexchange.com/a/1237
String getSubtringField(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length();

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

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



