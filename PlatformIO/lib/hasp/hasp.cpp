#include <hasp.h>

const float hasp::version = 0.40; // Current HASP software release version

byte hasp::mac[6];
char *hasp::node = HASP_NODE;
char *hasp::wifiSSID = WIFI_SSID;
char *hasp::wifiPass = WIFI_PASS;
char *hasp::groupName = GROUP_NAME;
char *hasp::mqttServer = MQTT_SRV;
uint8_t hasp::mqttPort = MQTT_PORT;
char *hasp::mqttUser = MQTT_USER;
char *hasp::mqttPassword = MQTT_USER;
char *hasp::configUser = WEB_USER;
char *hasp::configPassword = WEB_PASS;
uint8_t hasp::motionPin = MOTION_PIN;

bool hasp::shouldSaveConfig = false;

bool hasp::mdnsEnabled = true;          // mDNS enabled
bool hasp::beepEnabled = false;         // Keypress beep enabled
unsigned long hasp::beepPrevMillis = 0; // will store last time beep was updated
unsigned long hasp::beepOnTime = 1000;  // milliseconds of on-time for beep
unsigned long hasp::beepOffTime = 1000; // milliseconds of off-time for beep
boolean hasp::beepState;                // beep currently engaged
unsigned int hasp::beepCounter;         // Count the number of beeps
byte hasp::beepPin;    

bool hasp::updateEspAvailable = false;                    // Flag for update check to report new ESP FW version
float hasp::updateEspAvailableVersion;                    // Float to hold the new ESP FW version number
bool hasp::updateLcdAvailable = false;                    // Flag for update check to report new LCD FW version
bool hasp::debugSerialEnabled = true;                     // Enable USB serial debug output
bool hasp::debugTelnetEnabled = false;                    // Enable telnet debug output
bool hasp::debugSerialD8Enabled = true;                   // Enable hardware serial debug output on pin D8

WiFiClient hasp::wifiClient;

void hasp::wifiSetup()
{ // Connect to WiFi
  nextion::sendCmd("page 0");
  nextion::setAttr("p[0].b[1].font", "6");
  nextion::setAttr("p[0].b[1].txt", "\"WiFi Connecting...\\r " + String(WiFi.SSID()) + "\"");

  WiFi.macAddress(mac);               // Read our MAC address and save it to espMac
  WiFi.hostname(node);            // Assign our hostname before connecting to WiFi
  WiFi.setAutoReconnect(true);        // Tell WiFi to autoreconnect if connection has dropped
  WiFi.setSleepMode(WIFI_NONE_SLEEP); // Disable WiFi sleep modes to prevent occasional disconnects

  if (String(wifiSSID) == "")
  { // If the sketch has not defined a static wifiSSID use WiFiManager to collect required information from the user.

    // id/name, placeholder/prompt, default value, length, extra tags
    WiFiManagerParameter custom_haspNodeHeader("<br/><br/><b>HASP Node Name</b>");
    WiFiManagerParameter custom_haspNode("haspNode", "HASP Node (required. lowercase letters, numbers, and _ only)", node, 15, " maxlength=15 required pattern='[a-z0-9_]*'");
    WiFiManagerParameter custom_groupName("groupName", "Group Name (required)", groupName, 15, " maxlength=15 required");
    WiFiManagerParameter custom_mqttHeader("<br/><br/><b>MQTT Broker</b>");
    WiFiManagerParameter custom_mqttServer("mqttServer", "MQTT Server", mqttServer, 63, " maxlength=39");
    char *port = "";
    itoa(mqttPort, port, 10);
    WiFiManagerParameter custom_mqttPort("mqttPort", "MQTT Port", port, 5, " maxlength=5 type='number'");
    WiFiManagerParameter custom_mqttUser("mqttUser", "MQTT User", mqttUser, 31, " maxlength=31");
    WiFiManagerParameter custom_mqttPassword("mqttPassword", "MQTT Password", mqttPassword, 31, " maxlength=31 type='password'");
    WiFiManagerParameter custom_configHeader("<br/><br/><b>Admin access</b>");
    WiFiManagerParameter custom_configUser("configUser", "Config User", configUser, 15, " maxlength=31'");
    WiFiManagerParameter custom_configPassword("configPassword", "Config Password", configPassword, 31, " maxlength=31 type='password'");

    WiFiManager wifiManager;
    wifiManager.setSaveConfigCallback(persistance::saveCallback); // set config save notify callback
    wifiManager.setCustomHeadElement(HASP_STYLE_STRING);                 // add custom style
    wifiManager.addParameter(&custom_haspNodeHeader);
    wifiManager.addParameter(&custom_haspNode);
    wifiManager.addParameter(&custom_groupName);
    wifiManager.addParameter(&custom_mqttHeader);
    wifiManager.addParameter(&custom_mqttServer);
    wifiManager.addParameter(&custom_mqttPort);
    wifiManager.addParameter(&custom_mqttUser);
    wifiManager.addParameter(&custom_mqttPassword);
    wifiManager.addParameter(&custom_configHeader);
    wifiManager.addParameter(&custom_configUser);
    wifiManager.addParameter(&custom_configPassword);

    // Timeout config portal after connectTimeout seconds, useful if configured wifi network was temporarily unavailable
    wifiManager.setTimeout(CONNECT_TIMEOUT);

    wifiManager.setAPCallback(hasp::wifiConfigCallback);

    // Fetches SSID and pass from EEPROM and tries to connect
    // If it does not connect it starts an access point with the specified name
    // and goes into a blocking loop awaiting configuration.
    if (!wifiManager.autoConnect(WIFI_CONFIG_SSID, WIFI_CONFIG_PASS))
    { // Reset and try again
      debugPrintln(F("WIFI: Failed to connect and hit timeout"));
      reset();
    }

    // Read updated parameters
    strcpy(mqttServer, custom_mqttServer.getValue());
    mqttPort = atoi(custom_mqttPort.getValue());
    strcpy(mqttUser, custom_mqttUser.getValue());
    strcpy(mqttPassword, custom_mqttPassword.getValue());
    strcpy(node, custom_haspNode.getValue());
    strcpy(groupName, custom_groupName.getValue());
    strcpy(configUser, custom_configUser.getValue());
    strcpy(configPassword, custom_configPassword.getValue());

    if (shouldSaveConfig)
    { // Save the custom parameters to FS
      persistance::save();
    }
  }
  else
  { 
    // wifiSSID has been defined, so attempt to connect to it forever
    debugPrintln(String(F("Connecting to WiFi network: ")) + String(wifiSSID));
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID, wifiPass);

    unsigned long wifiReconnectTimer = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      if (millis() >= (wifiReconnectTimer + (CONNECT_TIMEOUT * 1000)))
      {
        // If we've been trying to reconnect for connectTimeout seconds, reboot and try again
        debugPrintln(F("WIFI: Failed to connect and hit timeout"));
        reset();
      }
    }
  }
  // If you get here you have connected to WiFi
  nextion::setAttr("p[0].b[1].font", "6");
  nextion::setAttr("p[0].b[1].txt", "\"WiFi Connected!\\r " + String(WiFi.SSID()) + "\\rIP: " + WiFi.localIP().toString() + "\"");
  debugPrintln(String(F("WIFI: Connected successfully and assigned IP: ")) + WiFi.localIP().toString());
  if (nextion::activePage)
  {
    nextion::sendCmd("page " + String(nextion::activePage));
  }
}

void hasp::wifiReconnect()
{ // Existing WiFi connection dropped, try to reconnect
  debugPrintln(F("Reconnecting to WiFi network..."));
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID, wifiPass);

  unsigned long wifiReconnectTimer = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    if (millis() >= (wifiReconnectTimer + (RECONNECT_TIMEOUT * 1000)))
    { // If we've been trying to reconnect for reConnectTimeout seconds, reboot and try again
      debugPrintln(F("WIFI: Failed to reconnect and hit timeout"));
      reset();
    }
  }
}


void hasp::wifiConfigCallback(WiFiManager *myWiFiManager)
{ // Notify the user that we're entering config mode
  debugPrintln(F("WIFI: Failed to connect to assigned AP, entering config mode"));
  while (millis() < 800)
  { // for factory-reset system this will be called before display is responsive. give it a second.
    delay(10);
  }
  nextion::sendCmd("page 0");
  nextion::setAttr("p[0].b[1].font", "6");
  nextion::setAttr("p[0].b[1].txt", "\" HASP WiFi Setup\\r AP: " + String(WIFI_CONFIG_SSID) + "\\rPassword: " + String(WIFI_CONFIG_PASS) + "\\r\\r\\r\\r\\r\\r\\r  http://192.168.4.1\"");
  nextion::sendCmd("vis 3,1");
}

void hasp::setupOta()
{ // (mostly) boilerplate OTA setup from library examples

  ArduinoOTA.setHostname(node);
  ArduinoOTA.setPassword(configPassword);

  ArduinoOTA.onStart([]() {
    debugPrintln(F("ESP OTA: update start"));
    nextion::sendCmd("page 0");
    nextion::setAttr("p[0].b[1].txt", "\"ESP OTA Update\"");
  });
  ArduinoOTA.onEnd([]() {
    nextion::sendCmd("page 0");
    debugPrintln(F("ESP OTA: update complete"));
    nextion::setAttr("p[0].b[1].txt", "\"ESP OTA Update\\rComplete!\"");
    reset();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    nextion::setAttr("p[0].b[1].txt", "\"ESP OTA Update\\rProgress: " + String(progress / (total / 100)) + "%\"");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    debugPrintln(String(F("ESP OTA: ERROR code ")) + String(error));
    if (error == OTA_AUTH_ERROR)
      debugPrintln(F("ESP OTA: ERROR - Auth Failed"));
    else if (error == OTA_BEGIN_ERROR)
      debugPrintln(F("ESP OTA: ERROR - Begin Failed"));
    else if (error == OTA_CONNECT_ERROR)
      debugPrintln(F("ESP OTA: ERROR - Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR)
      debugPrintln(F("ESP OTA: ERROR - Receive Failed"));
    else if (error == OTA_END_ERROR)
      debugPrintln(F("ESP OTA: ERROR - End Failed"));
    nextion::setAttr("p[0].b[1].txt", "\"ESP OTA FAILED\"");
    delay(5000);
    nextion::sendCmd("page " + String(nextion::activePage));
  });
  ArduinoOTA.begin();
  debugPrintln(F("ESP OTA: Over the Air firmware update ready"));
}

void hasp::startOta(String espOtaUrl)
{ // Update ESP firmware from HTTP
  nextion::sendCmd("page 0");
  nextion::setAttr("p[0].b[1].txt", "\"HTTP update\\rstarting...\"");
  WiFiUDP::stopAll(); // Keep mDNS responder from breaking things

  t_httpUpdate_return returnCode = ESPhttpUpdate.update(wifiClient, espOtaUrl);
  switch (returnCode)
  {
  case HTTP_UPDATE_FAILED:
    debugPrintln("ESPFW: HTTP_UPDATE_FAILED error " + String(ESPhttpUpdate.getLastError()) + " " + ESPhttpUpdate.getLastErrorString());
    nextion::setAttr("p[0].b[1].txt", "\"HTTP Update\\rFAILED\"");
    break;

  case HTTP_UPDATE_NO_UPDATES:
    debugPrintln(F("ESPFW: HTTP_UPDATE_NO_UPDATES"));
    nextion::setAttr("p[0].b[1].txt", "\"HTTP Update\\rNo update\"");
    break;

  case HTTP_UPDATE_OK:
    debugPrintln(F("ESPFW: HTTP_UPDATE_OK"));
    nextion::setAttr("p[0].b[1].txt", "\"HTTP Update\\rcomplete!\\r\\rRestarting.\"");
    reset();
  }
  delay(5000);
  nextion::sendCmd("page " + String(nextion::activePage));
}

void hasp::reset()
{
  debugPrintln(F("RESET: HASP reset"));
  if (mqttWrapper::getClient().connected())
  {
    mqttWrapper::getClient().publish(mqttWrapper::statusTopic, "OFF", true, 1);
    mqttWrapper::getClient().publish(mqttWrapper::sensorTopic, "{\"status\": \"unavailable\"}", true, 1);
    mqttWrapper::getClient().disconnect();
  }
  nextion::reset();
  ESP.reset();
  delay(5000);
}

void hasp::debugPrintln(String debugText)
{ // Debug output line of text to our debug targets
  String debugTimeText = "[+" + String(float(millis()) / 1000, 3) + "s] " + debugText;
  Serial.println(debugTimeText);
  if (debugSerialEnabled)
  {
    SoftwareSerial debugSerial(-1, 1); // -1==nc for RX, 1==TX pin
    debugSerial.begin(115200);
    debugSerial.println(debugTimeText);
    debugSerial.flush();
  }
  if (debugTelnetEnabled)
  {
    if (telnet::getClient().connected())
    {
      telnet::getClient().println(debugTimeText);
    }
  }
}

void hasp::debugPrint(String debugText)
{ // Debug output single character to our debug targets (DON'T USE THIS!)
  // Try to avoid using this function if at all possible.  When connected to telnet, printing each
  // character requires a full TCP round-trip + acknowledgement back and execution halts while this
  // happens.  Far better to put everything into a line and send it all out in one packet using
  // debugPrintln.
  if (debugSerialEnabled)
    Serial.print(debugText);
  {
    SoftwareSerial debugSerial(-1, 1); // -1==nc for RX, 1==TX pin
    debugSerial.begin(115200);
    debugSerial.print(debugText);
    debugSerial.flush();
  }
  if (debugTelnetEnabled)
  {
    if (telnet::getClient().connected())
    {
      telnet::getClient().print(debugText);
    }
  }
}

bool hasp::updateCheck()
{ // firmware update check
  HTTPClient updateClient;
  debugPrintln(String(F("UPDATE: Checking update URL: ")) + String(UPDATE_URL));
  String updatePayload;
  updateClient.begin(wifiClient, UPDATE_URL);
  int httpCode = updateClient.GET(); // start connection and send HTTP header

  if (httpCode > 0)
  { // httpCode will be negative on error
    if (httpCode == HTTP_CODE_OK)
    { // file found at server
      updatePayload = updateClient.getString();
    }
  }
  else
  {
    debugPrintln(String(F("UPDATE: Update check failed: ")) + updateClient.errorToString(httpCode));
    return false;
  }
  updateClient.end();

  DynamicJsonDocument updateJson(2048);
  DeserializationError jsonError = deserializeJson(updateJson, updatePayload);

  if (jsonError)
  { // Couldn't parse the returned JSON, so bail
    debugPrintln(String(F("UPDATE: JSON parsing failed: ")) + String(jsonError.c_str()));
    return false;
  }
  else
  {
    if (!updateJson["d1_mini"]["version"].isNull())
    {
      updateEspAvailableVersion = updateJson["d1_mini"]["version"].as<float>();
      debugPrintln(String(F("UPDATE: updateEspAvailableVersion: ")) + String(updateEspAvailableVersion));
      espFirmwareUrl = updateJson["d1_mini"]["firmware"].as<String>();
      if (updateEspAvailableVersion > version)
      {
        updateEspAvailable = true;
        debugPrintln(String(F("UPDATE: New ESP version available: ")) + String(updateEspAvailableVersion));
      }
    }
    if (nextion::model && !updateJson[nextion::model]["version"].isNull())
    {
      nextion::updateLcdAvailableVersion = updateJson[nextionModel]["version"].as<int>();
      debugPrintln(String(F("UPDATE: updateLcdAvailableVersion: ")) + String(nextion::updateLcdAvailableVersion));
      lcdFirmwareUrl = updateJson[nextionModel]["firmware"].as<String>();
      if (updateLcdAvailableVersion > nextion::lcdVersion)
      {
        updateLcdAvailable = true;
        debugPrintln(String(F("UPDATE: New LCD version available: ")) + String(updateLcdAvailableVersion));
      }
    }
    debugPrintln(F("UPDATE: Update check completed"));
  }
  return true;
}

WiFiClient hasp::getClient()
{
  return wifiClient;
}