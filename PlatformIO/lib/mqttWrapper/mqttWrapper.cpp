#include <mqttWrapper.h>

mqttWrapper::mqttWrapper(const char *mqttServer, const char *mqttPort, const char *mqttUser, const char *mqttPassword, uint16_t maxPacketSize):mqttClient(maxPacketSize)
{
  server = mqttServer;
  port = mqttPort;
  user = mqttUser;
  password = mqttPassword;
}

void mqttWrapper::begin()
{
  mqttClient.begin(server, atoi(port), wifiMQTTClient); // Create MQTT service object
  mqttClient.onMessage(callback);                           // Setup MQTT callback function
  mqttConnect();                                                // Connect to MQTT
}

void mqttWrapper::connect()
{ // MQTT connection and subscriptions

  static bool mqttFirstConnect = true; // For the first connection, we want to send an OFF/ON state to
                                       // trigger any automations, but skip that if we reconnect while
                                       // still running the sketch

  // Check to see if we have a broker configured and notify the user if not
  if (server[0] == 0)
  {
    nextionSendCmd("page 0");
    nextionSetAttr("p[0].b[1].font", "6");
    nextionSetAttr("p[0].b[1].txt", "\"WiFi Connected!\\r " + String(WiFi.SSID()) + "\\rIP: " + WiFi.localIP().toString() + "\\r\\rConfigure MQTT:\\rhttp://" + WiFi.localIP().toString() + "\"");
    while (server[0] == 0)
    { // Handle HTTP and OTA while we're waiting for MQTT to be configured
      yield();
      if (nextionHandleInput())
      { // Process user input from HMI
        nextionProcessInput();
      }
      webServer.handleClient();
      ArduinoOTA.handle();
    }
  }
  // MQTT topic string definitions
  mqttStateTopic = "hasp/" + String(haspNode) + "/state";
  mqttStateJSONTopic = "hasp/" + String(haspNode) + "/state/json";
  mqttCommandTopic = "hasp/" + String(haspNode) + "/command";
  mqttGroupCommandTopic = "hasp/" + String(groupName) + "/command";
  mqttStatusTopic = "hasp/" + String(haspNode) + "/status";
  mqttSensorTopic = "hasp/" + String(haspNode) + "/sensor";
  mqttLightCommandTopic = "hasp/" + String(haspNode) + "/light/switch";
  mqttLightStateTopic = "hasp/" + String(haspNode) + "/light/state";
  mqttLightBrightCommandTopic = "hasp/" + String(haspNode) + "/brightness/set";
  mqttLightBrightStateTopic = "hasp/" + String(haspNode) + "/brightness/state";
  mqttMotionStateTopic = "hasp/" + String(haspNode) + "/motion/state";

  const String mqttCommandSubscription = mqttCommandTopic + "/#";
  const String mqttGroupCommandSubscription = mqttGroupCommandTopic + "/#";
  const String mqttLightSubscription = "hasp/" + String(haspNode) + "/light/#";
  const String mqttLightBrightSubscription = "hasp/" + String(haspNode) + "/brightness/#";

  // Loop until we're reconnected to MQTT
  while (!mqttClient.connected())
  {
    // Create a reconnect counter
    static uint8_t mqttReconnectCount = 0;

    // Generate an MQTT client ID as haspNode + our MAC address
    mqttClientId = String(haspNode) + "-" + String(espMac[0], HEX) + String(espMac[1], HEX) + String(espMac[2], HEX) + String(espMac[3], HEX) + String(espMac[4], HEX) + String(espMac[5], HEX);
    nextionSendCmd("page 0");
    nextionSetAttr("p[0].b[1].font", "6");
    nextionSetAttr("p[0].b[1].txt", "\"WiFi Connected!\\r " + String(WiFi.SSID()) + "\\rIP: " + WiFi.localIP().toString() + "\\r\\rMQTT Connecting:\\r " + String(mqttServer) + "\"");
    debugPrintln(String(F("MQTT: Attempting connection to broker ")) + String(mqttServer) + " as clientID " + mqttClientId);

    // Set keepAlive, cleanSession, timeout
    mqttClient.setOptions(30, true, 5000);

    // declare LWT
    mqttClient.setWill(mqttStatusTopic.c_str(), "OFF");

    if (mqttClient.connect(mqttClientId.c_str(), mqttUser, mqttPassword))
    { // Attempt to connect to broker, setting last will and testament
      // Subscribe to our incoming topics
      if (mqttClient.subscribe(mqttCommandSubscription))
      {
        debugPrintln(String(F("MQTT: subscribed to ")) + mqttCommandSubscription);
      }
      if (mqttClient.subscribe(mqttGroupCommandSubscription))
      {
        debugPrintln(String(F("MQTT: subscribed to ")) + mqttGroupCommandSubscription);
      }
      if (mqttClient.subscribe(mqttLightSubscription))
      {
        debugPrintln(String(F("MQTT: subscribed to ")) + mqttLightSubscription);
      }
      if (mqttClient.subscribe(mqttLightBrightSubscription))
      {
        debugPrintln(String(F("MQTT: subscribed to ")) + mqttLightSubscription);
      }
      if (mqttClient.subscribe(mqttStatusTopic))
      {
        debugPrintln(String(F("MQTT: subscribed to ")) + mqttStatusTopic);
      }

      if (mqttFirstConnect)
      { // Force any subscribed clients to toggle OFF/ON when we first connect to
        // make sure we get a full panel refresh at power on.  Sending OFF,
        // "ON" will be sent by the mqttStatusTopic subscription action.
        debugPrintln(String(F("MQTT: binary_sensor state: [")) + mqttStatusTopic + "] : [OFF]");
        mqttClient.publish(mqttStatusTopic, "OFF", true, 1);
        mqttFirstConnect = false;
      }
      else
      {
        debugPrintln(String(F("MQTT: binary_sensor state: [")) + mqttStatusTopic + "] : [ON]");
        mqttClient.publish(mqttStatusTopic, "ON", true, 1);
      }

      mqttReconnectCount = 0;

      // Update panel with MQTT status
      nextionSetAttr("p[0].b[1].txt", "\"WiFi Connected!\\r " + String(WiFi.SSID()) + "\\rIP: " + WiFi.localIP().toString() + "\\r\\rMQTT Connected:\\r " + String(mqttServer) + "\"");
      debugPrintln(F("MQTT: connected"));
      if (nextionActivePage)
      {
        nextionSendCmd("page " + String(nextionActivePage));
      }
    }
    else
    { // Retry until we give up and restart after connectTimeout seconds
      mqttReconnectCount++;
      if (mqttReconnectCount > ((connectTimeout / 10) - 1))
      {
        debugPrintln(String(F("MQTT connection attempt ")) + String(mqttReconnectCount) + String(F(" failed with rc ")) + String(mqttClient.returnCode()) + String(F(".  Restarting device.")));
        espReset();
      }
      debugPrintln(String(F("MQTT connection attempt ")) + String(mqttReconnectCount) + String(F(" failed with rc ")) + String(mqttClient.returnCode()) + String(F(".  Trying again in 30 seconds.")));
      nextionSetAttr("p[0].b[1].txt", "\"WiFi Connected:\\r " + String(WiFi.SSID()) + "\\rIP: " + WiFi.localIP().toString() + "\\r\\rMQTT Connect to:\\r " + String(mqttServer) + "\\rFAILED rc=" + String(mqttClient.returnCode()) + "\\r\\rRetry in 30 sec\"");
      unsigned long mqttReconnectTimer = millis(); // record current time for our timeout
      while ((millis() - mqttReconnectTimer) < 30000)
      { // Handle HTTP and OTA while we're waiting 30sec for MQTT to reconnect
        if (nextionHandleInput())
        { // Process user input from HMI
          nextionProcessInput();
        }
        webServer.handleClient();
        ArduinoOTA.handle();
        delay(10);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void mqttWrapper::callback(String &strTopic, String &strPayload)
{ // Handle incoming commands from MQTT

  // strTopic: homeassistant/haswitchplate/devicename/command/p[1].b[4].txt
  // strPayload: "Lights On"
  // subTopic: p[1].b[4].txt

  // Incoming Namespace (replace /device/ with /group/ for group commands)
  // '[...]/device/command' -m '' = No command requested, respond with mqttStatusUpdate()
  // '[...]/device/command' -m 'dim=50' = nextionSendCmd("dim=50")
  // '[...]/device/command/json' -m '["dim=5", "page 1"]' = nextionSendCmd("dim=50"), nextionSendCmd("page 1")
  // '[...]/device/command/page' -m '1' = nextionSendCmd("page 1")
  // '[...]/device/command/statusupdate' -m '' = mqttStatusUpdate()
  // '[...]/device/command/lcdupdate' -m 'http://192.168.0.10/local/HASwitchPlate.tft' = nextionStartOtaDownload("http://192.168.0.10/local/HASwitchPlate.tft")
  // '[...]/device/command/lcdupdate' -m '' = nextionStartOtaDownload("lcdFirmwareUrl")
  // '[...]/device/command/espupdate' -m 'http://192.168.0.10/local/HASwitchPlate.ino.d1_mini.bin' = espStartOta("http://192.168.0.10/local/HASwitchPlate.ino.d1_mini.bin")
  // '[...]/device/command/espupdate' -m '' = espStartOta("espFirmwareUrl")
  // '[...]/device/command/p[1].b[4].txt' -m '' = nextionGetAttr("p[1].b[4].txt")
  // '[...]/device/command/p[1].b[4].txt' -m '"Lights On"' = nextionSetAttr("p[1].b[4].txt", "\"Lights On\"")

  debugPrintln(String(F("MQTT IN: '")) + strTopic + "' : '" + strPayload + "'");

  if (((strTopic == mqttCommandTopic) || (strTopic == mqttGroupCommandTopic)) && (strPayload == ""))
  {                     // '[...]/device/command' -m '' = No command requested, respond with mqttStatusUpdate()
    mqttStatusUpdate(); // return status JSON via MQTT
  }
  else if (strTopic == mqttCommandTopic || strTopic == mqttGroupCommandTopic)
  { // '[...]/device/command' -m 'dim=50' == nextionSendCmd("dim=50")
    nextionSendCmd(strPayload);
  }
  else if (strTopic == (mqttCommandTopic + "/page") || strTopic == (mqttGroupCommandTopic + "/page"))
  { // '[...]/device/command/page' -m '1' == nextionSendCmd("page 1")
    if (nextionActivePage != strPayload.toInt())
    { // Hass likes to send duplicate responses to things like page requests and there are no plans to fix that behavior, so try and track it locally
      nextionActivePage = strPayload.toInt();
      nextionSendCmd("page " + strPayload);
    }
  }
  else if (strTopic == (mqttCommandTopic + "/json") || strTopic == (mqttGroupCommandTopic + "/json"))
  {                               // '[...]/device/command/json' -m '["dim=5", "page 1"]' = nextionSendCmd("dim=50"), nextionSendCmd("page 1")
    nextionParseJson(strPayload); // Send to nextionParseJson()
  }
  else if (strTopic == (mqttCommandTopic + "/statusupdate") || strTopic == (mqttGroupCommandTopic + "/statusupdate"))
  {                     // '[...]/device/command/statusupdate' == mqttStatusUpdate()
    mqttStatusUpdate(); // return status JSON via MQTT
  }
  else if (strTopic == (mqttCommandTopic + "/lcdupdate") || strTopic == (mqttGroupCommandTopic + "/lcdupdate"))
  { // '[...]/device/command/lcdupdate' -m 'http://192.168.0.10/local/HASwitchPlate.tft' == nextionStartOtaDownload("http://192.168.0.10/local/HASwitchPlate.tft")
    if (strPayload == "")
    {
      nextionStartOtaDownload(lcdFirmwareUrl);
    }
    else
    {
      nextionStartOtaDownload(strPayload);
    }
  }
  else if (strTopic == (mqttCommandTopic + "/espupdate") || strTopic == (mqttGroupCommandTopic + "/espupdate"))
  { // '[...]/device/command/espupdate' -m 'http://192.168.0.10/local/HASwitchPlate.ino.d1_mini.bin' == espStartOta("http://192.168.0.10/local/HASwitchPlate.ino.d1_mini.bin")
    if (strPayload == "")
    {
      espStartOta(espFirmwareUrl);
    }
    else
    {
      espStartOta(strPayload);
    }
  }
  else if (strTopic == (mqttCommandTopic + "/reboot") || strTopic == (mqttGroupCommandTopic + "/reboot"))
  { // '[...]/device/command/reboot' == reboot microcontroller)
    debugPrintln(F("MQTT: Rebooting device"));
    espReset();
  }
  else if (strTopic == (mqttCommandTopic + "/lcdreboot") || strTopic == (mqttGroupCommandTopic + "/lcdreboot"))
  { // '[...]/device/command/lcdreboot' == reboot LCD panel)
    debugPrintln(F("MQTT: Rebooting LCD"));
    nextionReset();
  }
  else if (strTopic == (mqttCommandTopic + "/factoryreset") || strTopic == (mqttGroupCommandTopic + "/factoryreset"))
  { // '[...]/device/command/factoryreset' == clear all saved settings)
    configClearSaved();
  }
  else if (strTopic == (mqttCommandTopic + "/beep") || strTopic == (mqttGroupCommandTopic + "/beep"))
  { // '[...]/device/command/beep')
    String mqqtvar1 = getSubtringField(strPayload, ',', 0);
    String mqqtvar2 = getSubtringField(strPayload, ',', 1);
    String mqqtvar3 = getSubtringField(strPayload, ',', 2);

    beepOnTime = mqqtvar1.toInt();
    beepOffTime = mqqtvar2.toInt();
    beepCounter = mqqtvar3.toInt();
  }
  else if (strTopic.startsWith(mqttCommandTopic) && (strPayload == ""))
  { // '[...]/device/command/p[1].b[4].txt' -m '' == nextionGetAttr("p[1].b[4].txt")
    String subTopic = strTopic.substring(mqttCommandTopic.length() + 1);
    mqttGetSubtopic = "/" + subTopic;
    nextionGetAttr(subTopic);
  }
  else if (strTopic.startsWith(mqttGroupCommandTopic) && (strPayload == ""))
  { // '[...]/group/command/p[1].b[4].txt' -m '' == nextionGetAttr("p[1].b[4].txt")
    String subTopic = strTopic.substring(mqttGroupCommandTopic.length() + 1);
    mqttGetSubtopic = "/" + subTopic;
    nextionGetAttr(subTopic);
  }
  else if (strTopic.startsWith(mqttCommandTopic))
  { // '[...]/device/command/p[1].b[4].txt' -m '"Lights On"' == nextionSetAttr("p[1].b[4].txt", "\"Lights On\"")
    String subTopic = strTopic.substring(mqttCommandTopic.length() + 1);
    nextionSetAttr(subTopic, strPayload);
  }
  else if (strTopic.startsWith(mqttGroupCommandTopic))
  { // '[...]/group/command/p[1].b[4].txt' -m '"Lights On"' == nextionSetAttr("p[1].b[4].txt", "\"Lights On\"")
    String subTopic = strTopic.substring(mqttGroupCommandTopic.length() + 1);
    nextionSetAttr(subTopic, strPayload);
  }
  else if (strTopic == mqttLightBrightCommandTopic)
  { // change the brightness from the light topic
    int panelDim = map(strPayload.toInt(), 0, 255, 0, 100);
    nextionSetAttr("dim", String(panelDim));
    nextionSendCmd("dims=dim");
    mqttClient.publish(mqttLightBrightStateTopic, strPayload);
  }
  else if (strTopic == mqttLightCommandTopic && strPayload == "OFF")
  { // set the panel dim OFF from the light topic, saving current dim level first
    nextionSendCmd("dims=dim");
    nextionSetAttr("dim", "0");
    mqttClient.publish(mqttLightStateTopic, "OFF");
  }
  else if (strTopic == mqttLightCommandTopic && strPayload == "ON")
  { // set the panel dim ON from the light topic, restoring saved dim level
    nextionSendCmd("dim=dims");
    mqttClient.publish(mqttLightStateTopic, "ON");
  }
  else if (strTopic == mqttStatusTopic && strPayload == "OFF")
  { // catch a dangling LWT from a previous connection if it appears
    mqttClient.publish(mqttStatusTopic, "ON");
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void mqttWrapper::statusUpdate()
{ // Periodically publish a JSON string indicating system status
  String mqttStatusPayload = "{";
  mqttStatusPayload += String(F("\"status\":\"available\","));
  mqttStatusPayload += String(F("\"espVersion\":")) + String(haspVersion) + String(F(","));
  if (updateEspAvailable)
  {
    mqttStatusPayload += String(F("\"updateEspAvailable\":true,"));
  }
  else
  {
    mqttStatusPayload += String(F("\"updateEspAvailable\":false,"));
  }
  if (lcdConnected)
  {
    mqttStatusPayload += String(F("\"lcdConnected\":true,"));
  }
  else
  {
    mqttStatusPayload += String(F("\"lcdConnected\":false,"));
  }
  mqttStatusPayload += String(F("\"lcdVersion\":\"")) + String(lcdVersion) + String(F("\","));
  if (updateLcdAvailable)
  {
    mqttStatusPayload += String(F("\"updateLcdAvailable\":true,"));
  }
  else
  {
    mqttStatusPayload += String(F("\"updateLcdAvailable\":false,"));
  }
  mqttStatusPayload += String(F("\"espUptime\":")) + String(long(millis() / 1000)) + String(F(","));
  mqttStatusPayload += String(F("\"signalStrength\":")) + String(WiFi.RSSI()) + String(F(","));
  mqttStatusPayload += String(F("\"haspIP\":\"")) + WiFi.localIP().toString() + String(F("\","));
  mqttStatusPayload += String(F("\"heapFree\":")) + String(ESP.getFreeHeap()) + String(F(","));
  mqttStatusPayload += String(F("\"heapFragmentation\":")) + String(ESP.getHeapFragmentation()) + String(F(","));
  mqttStatusPayload += String(F("\"espCore\":\"")) + String(ESP.getCoreVersion()) + String(F("\""));
  mqttStatusPayload += "}";

  mqttClient.publish(mqttSensorTopic, mqttStatusPayload, true, 1);
  mqttClient.publish(mqttStatusTopic, "ON", true, 1);
  debugPrintln(String(F("MQTT: status update: ")) + String(mqttStatusPayload));
  debugPrintln(String(F("MQTT: binary_sensor state: [")) + mqttStatusTopic + "] : [ON]");
}