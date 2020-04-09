#include <mqttWrapper.h>

MQTTClient mqttWrapper::client;

void mqttWrapper::begin()
{
  client.begin(hasp::mqttServer, hasp::mqttPort, hasp::client); // Create MQTT service object
  client.onMessage(callback);                                 // Setup MQTT callback function
  connect();                                                  // Connect to MQTT
}

void mqttWrapper::connect()
{
  // MQTT connection and subscriptions
  static bool mqttFirstConnect = true; // For the first connection, we want to send an OFF/ON state to
                                       // trigger any automations, but skip that if we reconnect while
                                       // still running the sketch

  // Check to see if we have a broker configured and notify the user if not
  if (hasp::mqttServer == 0)
  {
    nextion::sendCmd("page 0");
    nextion::setAttr("p[0].b[1].font", "6");
    nextion::setAttr("p[0].b[1].txt", "\"WiFi Connected!\\r " + String(WiFi.SSID()) + "\\rIP: " + WiFi.localIP().toString() + "\\r\\rConfigure MQTT:\\rhttp://" + WiFi.localIP().toString() + "\"");
    while (hasp::mqttServer == 0)
    { // Handle HTTP and OTA while we're waiting for MQTT to be configured
      yield();
      if (nextion::handleInput())
      { // Process user input from HMI
        nextion::processInput();
      }
      web::server.handleClient();
      ArduinoOTA.handle();
    }
  }
  // MQTT topic string definitions
  stateTopic = "hasp/" + String(hasp::node) + "/state";
  stateJSONTopic = "hasp/" + String(hasp::node) + "/state/json";
  commandTopic = "hasp/" + String(hasp::node) + "/command";
  groupCommandTopic = "hasp/" + String(hasp::groupName) + "/command";
  statusTopic = "hasp/" + String(hasp::node) + "/status";
  sensorTopic = "hasp/" + String(hasp::node) + "/sensor";
  lightCommandTopic = "hasp/" + String(hasp::node) + "/light/switch";
  lightStateTopic = "hasp/" + String(hasp::node) + "/light/state";
  lightBrightCommandTopic = "hasp/" + String(hasp::node) + "/brightness/set";
  lightBrightStateTopic = "hasp/" + String(hasp::node) + "/brightness/state";
  motionStateTopic = "hasp/" + String(hasp::node) + "/motion/state";

  const String commandSubscription = commandTopic + "/#";
  const String groupCommandSubscription = groupCommandTopic + "/#";
  const String lightSubscription = "hasp/" + String(hasp::node) + "/light/#";
  const String lightBrightSubscription = "hasp/" + String(hasp::node) + "/brightness/#";

  // Loop until we're reconnected to MQTT
  while (!client.connected())
  {
    // Create a reconnect counter
    static uint8_t mqttReconnectCount = 0;

    // Generate an MQTT client ID as haspNode + our MAC address
    clientId = String(hasp::node) + "-" + String(hasp::mac[0], HEX) + String(hasp::mac[1], HEX) + String(hasp::mac[2], HEX) + String(hasp::mac[3], HEX) + String(hasp::mac[4], HEX) + String(hasp::mac[5], HEX);
    nextion::sendCmd("page 0");
    nextion::setAttr("p[0].b[1].font", "6");
    nextion::setAttr("p[0].b[1].txt", "\"WiFi Connected!\\r " + String(WiFi.SSID()) + "\\rIP: " + WiFi.localIP().toString() + "\\r\\rMQTT Connecting:\\r " + String(hasp::mqttServer) + "\"");
    hasp::debugPrintln(String(F("MQTT: Attempting connection to broker ")) + String(hasp::mqttServer) + " as clientID " + clientId);

    // Set keepAlive, cleanSession, timeout
    client.setOptions(30, true, 5000);

    // declare LWT
    client.setWill(statusTopic.c_str(), "OFF");

    if (client.connect(clientId.c_str(), hasp::mqttUser, hasp::mqttPassword))
    { // Attempt to connect to broker, setting last will and testament
      // Subscribe to our incoming topics
      if (client.subscribe(commandSubscription))
      {
        hasp::debugPrintln(String(F("MQTT: subscribed to ")) + commandSubscription);
      }
      if (client.subscribe(groupCommandSubscription))
      {
        hasp::debugPrintln(String(F("MQTT: subscribed to ")) + groupCommandSubscription);
      }
      if (client.subscribe(lightSubscription))
      {
        hasp::debugPrintln(String(F("MQTT: subscribed to ")) + lightSubscription);
      }
      if (client.subscribe(lightBrightSubscription))
      {
        hasp::debugPrintln(String(F("MQTT: subscribed to ")) + lightSubscription);
      }
      if (client.subscribe(statusTopic))
      {
        hasp::debugPrintln(String(F("MQTT: subscribed to ")) + statusTopic);
      }

      if (mqttFirstConnect)
      { // Force any subscribed clients to toggle OFF/ON when we first connect to
        // make sure we get a full panel refresh at power on.  Sending OFF,
        // "ON" will be sent by the mqttStatusTopic subscription action.
        hasp::debugPrintln(String(F("MQTT: binary_sensor state: [")) + statusTopic + "] : [OFF]");
        client.publish(statusTopic, "OFF", true, 1);
        mqttFirstConnect = false;
      }
      else
      {
        hasp::debugPrintln(String(F("MQTT: binary_sensor state: [")) + statusTopic + "] : [ON]");
        client.publish(statusTopic, "ON", true, 1);
      }

      mqttReconnectCount = 0;

      // Update panel with MQTT status
      nextion::setAttr("p[0].b[1].txt", "\"WiFi Connected!\\r " + String(WiFi.SSID()) + "\\rIP: " + WiFi.localIP().toString() + "\\r\\rMQTT Connected:\\r " + String(hasp::mqttServer) + "\"");
      hasp::debugPrintln(F("MQTT: connected"));
      if (nextion::activePage)
      {
        nextion::sendCmd("page " + String(nextion::activePage));
      }
    }
    else
    { // Retry until we give up and restart after connectTimeout seconds
      mqttReconnectCount++;
      if (mqttReconnectCount > ((CONNECT_TIMEOUT / 10) - 1))
      {
        hasp::debugPrintln(String(F("MQTT connection attempt ")) + String(mqttReconnectCount) + String(F(" failed with rc ")) + String(client.returnCode()) + String(F(".  Restarting device.")));
        hasp::reset();
      }
      hasp::debugPrintln(String(F("MQTT connection attempt ")) + String(mqttReconnectCount) + String(F(" failed with rc ")) + String(client.returnCode()) + String(F(".  Trying again in 30 seconds.")));
      nextion::setAttr("p[0].b[1].txt", "\"WiFi Connected:\\r " + String(WiFi.SSID()) + "\\rIP: " + WiFi.localIP().toString() + "\\r\\rMQTT Connect to:\\r " + String(hasp::mqttServer) + "\\rFAILED rc=" + String(client.returnCode()) + "\\r\\rRetry in 30 sec\"");
      unsigned long mqttReconnectTimer = millis(); // record current time for our timeout
      while ((millis() - mqttReconnectTimer) < 30000)
      { // Handle HTTP and OTA while we're waiting 30sec for MQTT to reconnect
        if (nextion::handleInput())
        { // Process user input from HMI
          nextion::processInput();
        }
        web::handleClient();
        ArduinoOTA.handle();
        delay(10);
      }
    }
  }
}

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

  hasp::debugPrintln(String(F("MQTT IN: '")) + strTopic + "' : '" + strPayload + "'");

  if (((strTopic == commandTopic) || (strTopic == groupCommandTopic)) && (strPayload == ""))
  {                     // '[...]/device/command' -m '' = No command requested, respond with mqttStatusUpdate()
    statusUpdate(); // return status JSON via MQTT
  }
  else if (strTopic == commandTopic || strTopic == groupCommandTopic)
  { // '[...]/device/command' -m 'dim=50' == nextionSendCmd("dim=50")
    nextion::sendCmd(strPayload);
  }
  else if (strTopic == (commandTopic + "/page") || strTopic == (groupCommandTopic + "/page"))
  { // '[...]/device/command/page' -m '1' == nextionSendCmd("page 1")
    if (nextion::activePage != strPayload.toInt())
    { // Hass likes to send duplicate responses to things like page requests and there are no plans to fix that behavior, so try and track it locally
      nextion::activePage = strPayload.toInt();
      nextion::sendCmd("page " + strPayload);
    }
  }
  else if (strTopic == (commandTopic + "/json") || strTopic == (groupCommandTopic + "/json"))
  {                               // '[...]/device/command/json' -m '["dim=5", "page 1"]' = nextionSendCmd("dim=50"), nextionSendCmd("page 1")
    nextion::parseJson(strPayload); // Send to nextionParseJson()
  }
  else if (strTopic == (commandTopic + "/statusupdate") || strTopic == (groupCommandTopic + "/statusupdate"))
  {                     // '[...]/device/command/statusupdate' == mqttStatusUpdate()
    statusUpdate(); // return status JSON via MQTT
  }
  else if (strTopic == (commandTopic + "/lcdupdate") || strTopic == (groupCommandTopic + "/lcdupdate"))
  { // '[...]/device/command/lcdupdate' -m 'http://192.168.0.10/local/HASwitchPlate.tft' == nextionStartOtaDownload("http://192.168.0.10/local/HASwitchPlate.tft")
    if (strPayload == "")
    {
      nextion::startOtaDownload(LCD_FIRMWARE_URL);
    }
    else
    {
      nextion::startOtaDownload(strPayload);
    }
  }
  else if (strTopic == (commandTopic + "/espupdate") || strTopic == (groupCommandTopic + "/espupdate"))
  { // '[...]/device/command/espupdate' -m 'http://192.168.0.10/local/HASwitchPlate.ino.d1_mini.bin' == espStartOta("http://192.168.0.10/local/HASwitchPlate.ino.d1_mini.bin")
    if (strPayload == "")
    {
      hasp::startOta(ESP_FIRMWARE_URL);
    }
    else
    {
      hasp::startOta(strPayload);
    }
  }
  else if (strTopic == (commandTopic + "/reboot") || strTopic == (groupCommandTopic + "/reboot"))
  { // '[...]/device/command/reboot' == reboot microcontroller)
    hasp::debugPrintln(F("MQTT: Rebooting device"));
    hasp::reset();
  }
  else if (strTopic == (commandTopic + "/lcdreboot") || strTopic == (groupCommandTopic + "/lcdreboot"))
  { // '[...]/device/command/lcdreboot' == reboot LCD panel)
    hasp::debugPrintln(F("MQTT: Rebooting LCD"));
    nextion::reset();
  }
  else if (strTopic == (commandTopic + "/factoryreset") || strTopic == (groupCommandTopic + "/factoryreset"))
  { // '[...]/device/command/factoryreset' == clear all saved settings)
    persistance::clearSaved();
  }
  else if (strTopic == (commandTopic + "/beep") || strTopic == (groupCommandTopic + "/beep"))
  { // '[...]/device/command/beep')
    String mqqtvar1 = getSubtringField(strPayload, ',', 0);
    String mqqtvar2 = getSubtringField(strPayload, ',', 1);
    String mqqtvar3 = getSubtringField(strPayload, ',', 2);

    hasp::beepOnTime = mqqtvar1.toInt();
    hasp::beepOffTime = mqqtvar2.toInt();
    hasp::beepCounter = mqqtvar3.toInt();
  }
  else if (strTopic.startsWith(commandTopic) && (strPayload == ""))
  { // '[...]/device/command/p[1].b[4].txt' -m '' == nextionGetAttr("p[1].b[4].txt")
    String subTopic = strTopic.substring(commandTopic.length() + 1);
    getSubtopic = "/" + subTopic;
    nextion::getAttr(subTopic);
  }
  else if (strTopic.startsWith(groupCommandTopic) && (strPayload == ""))
  { // '[...]/group/command/p[1].b[4].txt' -m '' == nextionGetAttr("p[1].b[4].txt")
    String subTopic = strTopic.substring(groupCommandTopic.length() + 1);
    getSubtopic = "/" + subTopic;
    nextion::getAttr(subTopic);
  }
  else if (strTopic.startsWith(commandTopic))
  { // '[...]/device/command/p[1].b[4].txt' -m '"Lights On"' == nextionSetAttr("p[1].b[4].txt", "\"Lights On\"")
    String subTopic = strTopic.substring(commandTopic.length() + 1);
    nextion::setAttr(subTopic, strPayload);
  }
  else if (strTopic.startsWith(groupCommandTopic))
  { // '[...]/group/command/p[1].b[4].txt' -m '"Lights On"' == nextionSetAttr("p[1].b[4].txt", "\"Lights On\"")
    String subTopic = strTopic.substring(groupCommandTopic.length() + 1);
    nextion::setAttr(subTopic, strPayload);
  }
  else if (strTopic == lightBrightCommandTopic)
  { // change the brightness from the light topic
    int panelDim = map(strPayload.toInt(), 0, 255, 0, 100);
    nextion::setAttr("dim", String(panelDim));
    nextion::sendCmd("dims=dim");
    client.publish(lightBrightStateTopic, strPayload);
  }
  else if (strTopic == lightCommandTopic && strPayload == "OFF")
  { // set the panel dim OFF from the light topic, saving current dim level first
    nextion::sendCmd("dims=dim");
    nextion::setAttr("dim", "0");
    client.publish(lightStateTopic, "OFF");
  }
  else if (strTopic == lightCommandTopic && strPayload == "ON")
  { // set the panel dim ON from the light topic, restoring saved dim level
    nextion::sendCmd("dim=dims");
    client.publish(lightStateTopic, "ON");
  }
  else if (strTopic == statusTopic && strPayload == "OFF")
  { // catch a dangling LWT from a previous connection if it appears
    client.publish(statusTopic, "ON");
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void mqttWrapper::statusUpdate()
{ // Periodically publish a JSON string indicating system status
  String statusPayload = "{";
  statusPayload += String(F("\"status\":\"available\","));
  statusPayload += String(F("\"espVersion\":")) + String(hasp::version) + String(F(","));
  if (hasp::updateEspAvailable)
  {
    statusPayload += String(F("\"updateEspAvailable\":true,"));
  }
  else
  {
    statusPayload += String(F("\"updateEspAvailable\":false,"));
  }
  if (nextion::lcdConnected)
  {
    statusPayload += String(F("\"lcdConnected\":true,"));
  }
  else
  {
    statusPayload += String(F("\"lcdConnected\":false,"));
  }
  statusPayload += String(F("\"lcdVersion\":\"")) + String(nextion::lcdVersion) + String(F("\","));
  if (hasp::updateLcdAvailable)
  {
    statusPayload += String(F("\"updateLcdAvailable\":true,"));
  }
  else
  {
    statusPayload += String(F("\"updateLcdAvailable\":false,"));
  }
  statusPayload += String(F("\"espUptime\":")) + String(long(millis() / 1000)) + String(F(","));
  statusPayload += String(F("\"signalStrength\":")) + String(WiFi.RSSI()) + String(F(","));
  statusPayload += String(F("\"haspIP\":\"")) + WiFi.localIP().toString() + String(F("\","));
  statusPayload += String(F("\"heapFree\":")) + String(ESP.getFreeHeap()) + String(F(","));
  statusPayload += String(F("\"heapFragmentation\":")) + String(ESP.getHeapFragmentation()) + String(F(","));
  statusPayload += String(F("\"espCore\":\"")) + String(ESP.getCoreVersion()) + String(F("\""));
  statusPayload += "}";

  client.publish(sensorTopic, statusPayload, true, 1);
  client.publish(statusTopic, "ON", true, 1);
  hasp::debugPrintln(String(F("MQTT: status update: ")) + String(statusPayload));
  hasp::debugPrintln(String(F("MQTT: binary_sensor state: [")) + statusTopic + "] : [ON]");
}

MQTTClient mqttWrapper::getClient()
{
  return client;
}

// Submitted by benmprojects to handle "beep" commands. Split
// incoming String by separator, return selected field as String
// Original source: https://arduino.stackexchange.com/a/1237
String mqttWrapper::getSubtringField(String data, char separator, int index)
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