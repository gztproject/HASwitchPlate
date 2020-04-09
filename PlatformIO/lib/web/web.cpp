#include <web.h>

ESP8266WebServer web::server(80);

void web::begin()
{   
  server.on("/", handleRoot);
  server.on("/saveConfig", handleSaveConfig);
  server.on("/resetConfig", handleResetConfig);
  server.on("/resetBacklight", handleResetBacklight);
  server.on("/firmware", handleFirmware);
  server.on("/espfirmware", handleEspFirmware);
  server.on(
      "/lcdupload", HTTP_POST, []() { server.send(200); }, handleLcdUpload);
  server.on("/tftFileSize", handleTftFileSize);
  server.on("/lcddownload", handleLcdDownload);
  server.on("/lcdOtaSuccess", handleLcdUpdateSuccess);
  server.on("/lcdOtaFailure", handleLcdUpdateFailure);
  server.on("/reboot", handleReboot);
  server.onNotFound(handleNotFound);
  server.begin();
}

void web::wandleNotFound()
{
  // server 404
  haps::debugPrintln(String(F("HTTP: Sending 404 to client connected from: ")) + server.client().remoteIP().toString());
  String httpMessage = "File Not Found\n\n";
  httpMessage += "URI: ";
  httpMessage += server.uri();
  httpMessage += "\nMethod: ";
  httpMessage += (server.method() == HTTP_GET) ? "GET" : "POST";
  httpMessage += "\nArguments: ";
  httpMessage += server.args();
  httpMessage += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    httpMessage += " " + server
.argName(i) + ": " + server
.arg(i) + "\n";
  }
  server.send(404, "text/plain", httpMessage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void web::handleRoot()
{ // http://plate01/
  if (configPassword[0] != '\0')
  { //Request HTTP auth if configPassword is set
    if (!server
.authenticate(configUser, configPassword))
    {
      return server
  .requestAuthentication();
    }
  }
  hasp::debugPrintln(String(F("HTTP: Sending root page to client connected from: ")) + server.client().remoteIP().toString());
  String httpMessage = FPSTR(HTTP_HEADER);
  httpMessage.replace("{v}", String(hasp::node));
  httpMessage += FPSTR(HTTP_SCRIPT);
  httpMessage += FPSTR(HTTP_STYLE);
  httpMessage += String(HASP_STYLE_STRING);
  httpMessage += FPSTR(HTTP_HEADER_END);
  httpMessage += String(F("<h1>"));
  httpMessage += String(haspNode);
  httpMessage += String(F("</h1>"));

  httpMessage += String(F("<form method='POST' action='saveConfig'>"));
  httpMessage += String(F("<b>WiFi SSID</b> <i><small>(required)</small></i><input id='wifiSSID' required name='wifiSSID' maxlength=32 placeholder='WiFi SSID' value='")) + String(WiFi.SSID()) + "'>";
  httpMessage += String(F("<br/><b>WiFi Password</b> <i><small>(required)</small></i><input id='wifiPass' required name='wifiPass' type='password' maxlength=64 placeholder='WiFi Password' value='")) + String("********") + "'>";
  httpMessage += String(F("<br/><br/><b>HASP Node Name</b> <i><small>(required. lowercase letters, numbers, and _ only)</small></i><input id='haspNode' required name='haspNode' maxlength=15 placeholder='HASP Node Name' pattern='[a-z0-9_]*' value='")) + String(haspNode) + "'>";
  httpMessage += String(F("<br/><br/><b>Group Name</b> <i><small>(required)</small></i><input id='groupName' required name='groupName' maxlength=15 placeholder='Group Name' value='")) + String(groupName) + "'>";
  httpMessage += String(F("<br/><br/><b>MQTT Broker</b> <i><small>(required)</small></i><input id='mqttServer' required name='mqttServer' maxlength=63 placeholder='mqttServer' value='")) + String(mqttServer) + "'>";
  httpMessage += String(F("<br/><b>MQTT Port</b> <i><small>(required)</small></i><input id='mqttPort' required name='mqttPort' type='number' maxlength=5 placeholder='mqttPort' value='")) + String(mqttPort) + "'>";
  httpMessage += String(F("<br/><b>MQTT User</b> <i><small>(optional)</small></i><input id='mqttUser' name='mqttUser' maxlength=31 placeholder='mqttUser' value='")) + String(mqttUser) + "'>";
  httpMessage += String(F("<br/><b>MQTT Password</b> <i><small>(optional)</small></i><input id='mqttPassword' name='mqttPassword' type='password' maxlength=31 placeholder='mqttPassword' value='"));
  if (strlen(hasp::mqttPassword) != 0)
  {
    httpMessage += String("********");
  }
  httpMessage += String(F("'><br/><br/><b>HASP Admin Username</b> <i><small>(optional)</small></i><input id='configUser' name='configUser' maxlength=31 placeholder='Admin User' value='")) + String(configUser) + "'>";
  httpMessage += String(F("<br/><b>HASP Admin Password</b> <i><small>(optional)</small></i><input id='configPassword' name='configPassword' type='password' maxlength=31 placeholder='Admin User Password' value='"));
  if (strlen(hasp::configPassword) != 0)
  {
    httpMessage += String("********");
  }
  httpMessage += String(F("'><br/><hr><b>Motion Sensor Pin:&nbsp;</b><select id='motionPinConfig' name='motionPinConfig'>"));
  httpMessage += String(F("<option value='0'"));
  if (!hasp::motionPin)
  {
    httpMessage += String(F(" selected"));
  }
  httpMessage += String(F(">disabled/not installed</option><option value='D0'"));
  if (hasp::motionPin == D0)
  {
    httpMessage += String(F(" selected"));
  }
  httpMessage += String(F(">D0</option><option value='D1'"));
  if (hasp::motionPin == D1)
  {
    httpMessage += String(F(" selected"));
  }
  httpMessage += String(F(">D1</option></select>"));

  httpMessage += String(F("<br/><b>Serial debug output enabled:</b><input id='debugSerialEnabled' name='debugSerialEnabled' type='checkbox'"));
  if (hasp::debugSerialEnabled)
  {
    httpMessage += String(F(" checked='checked'"));
  }
  httpMessage += String(F("><br/><b>Telnet debug output enabled:</b><input id='debugTelnetEnabled' name='debugTelnetEnabled' type='checkbox'"));
  if (hasp::debugTelnetEnabled)
  {
    httpMessage += String(F(" checked='checked'"));
  }
  httpMessage += String(F("><br/><b>mDNS enabled:</b><input id='mdnsEnabled' name='mdnsEnabled' type='checkbox'"));
  if (hasp::mdnsEnabled)
  {
    httpMessage += String(F(" checked='checked'"));
  }

  httpMessage += String(F("><br/><b>Keypress beep enabled:</b><input id='beepEnabled' name='beepEnabled' type='checkbox'"));
  if (hasp::beepEnabled)
  {
    httpMessage += String(F(" checked='checked'"));
  }

  httpMessage += String(F("><br/><hr><button type='submit'>save settings</button></form>"));

  if (hasp::updateEspAvailable)
  {
    httpMessage += String(F("<br/><hr><font color='green'><center><h3>HASP Update available!</h3></center></font>"));
    httpMessage += String(F("<form method='get' action='espfirmware'>"));
    httpMessage += String(F("<input id='espFirmwareURL' type='hidden' name='espFirmware' value='")) + hasp::espFirmwareUrl + "'>";
    httpMessage += String(F("<button type='submit'>update HASP to v")) + String(hasp::updateEspAvailableVersion) + String(F("</button></form>"));
  }

  httpMessage += String(F("<hr><form method='get' action='firmware'>"));
  httpMessage += String(F("<button type='submit'>update firmware</button></form>"));

  httpMessage += String(F("<hr><form method='get' action='reboot'>"));
  httpMessage += String(F("<button type='submit'>reboot device</button></form>"));

  httpMessage += String(F("<hr><form method='get' action='resetBacklight'>"));
  httpMessage += String(F("<button type='submit'>reset lcd backlight</button></form>"));

  httpMessage += String(F("<hr><form method='get' action='resetConfig'>"));
  httpMessage += String(F("<button type='submit'>factory reset settings</button></form>"));

  httpMessage += String(F("<hr><b>MQTT Status: </b>"));
  if (mqttWrapper::getClient().connected())
  { // Check MQTT connection
    httpMessage += String(F("Connected"));
  }
  else
  {
    httpMessage += String(F("<font color='red'><b>Disconnected</b></font>, return code: ")) + String(mqttClient.returnCode());
  }
  httpMessage += String(F("<br/><b>MQTT ClientID: </b>")) + String(hasp::mqttClientId);
  httpMessage += String(F("<br/><b>HASP Version: </b>")) + String(hasp::haspVersion);
  httpMessage += String(F("<br/><b>LCD Model: </b>")) + String(hasp::nextionModel);
  httpMessage += String(F("<br/><b>LCD Version: </b>")) + String(hasp::lcdVersion);
  httpMessage += String(F("<br/><b>LCD Active Page: </b>")) + String(nextion::activePage);
  httpMessage += String(F("<br/><b>CPU Frequency: </b>")) + String(ESP.getCpuFreqMHz()) + String(F("MHz"));
  httpMessage += String(F("<br/><b>Sketch Size: </b>")) + String(ESP.getSketchSize()) + String(F(" bytes"));
  httpMessage += String(F("<br/><b>Free Sketch Space: </b>")) + String(ESP.getFreeSketchSpace()) + String(F(" bytes"));
  httpMessage += String(F("<br/><b>Heap Free: </b>")) + String(ESP.getFreeHeap());
  httpMessage += String(F("<br/><b>Heap Fragmentation: </b>")) + String(ESP.getHeapFragmentation());
  httpMessage += String(F("<br/><b>ESP core version: </b>")) + String(ESP.getCoreVersion());
  httpMessage += String(F("<br/><b>IP Address: </b>")) + String(WiFi.localIP().toString());
  httpMessage += String(F("<br/><b>Signal Strength: </b>")) + String(WiFi.RSSI());
  httpMessage += String(F("<br/><b>Uptime: </b>")) + String(long(millis() / 1000));
  httpMessage += String(F("<br/><b>Last reset: </b>")) + String(ESP.getResetInfo());

  httpMessage += FPSTR(HTTP_END);
  server.send(200, "text/html", httpMessage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void webHandleSaveConfig()
{ // http://plate01/saveConfig
  if (configPassword[0] != '\0')
  { //Request HTTP auth if configPassword is set
    if (!server
.authenticate(configUser, configPassword))
    {
      return server
  .requestAuthentication();
    }
  }
  debugPrintln(String(F("HTTP: Sending /saveConfig page to client connected from: ")) + server.client().remoteIP().toString());
  String httpMessage = FPSTR(HTTP_HEADER);
  httpMessage.replace("{v}", String(hasp::node));
  httpMessage += FPSTR(HTTP_SCRIPT);
  httpMessage += FPSTR(HTTP_STYLE);
  httpMessage += String(HASP_STYLE_STRING);

  bool shouldSaveWifi = false;
  // Check required values
  if (server.arg("wifiSSID") != "" && server.arg("wifiSSID") != String(WiFi.SSID()))
  { // Handle WiFi update
    hasp::shouldSaveConfig = true;
    shouldSaveWifi = true;
    server
.arg("wifiSSID").toCharArray(hasp::wifiSSID, 32);
    if (server
.arg("wifiPass") != String("********"))
    {
      server
  .arg("wifiPass").toCharArray(hasp::wifiPass, 64);
    }
  }
  if (server.arg("mqttServer") != "" && server.arg("mqttServer") != String(hasp::mqttServer))
  { // Handle mqttServer
    hasp::shouldSaveConfig = true;
    server
.arg("mqttServer").toCharArray(hasp::mqttServer, 64);
  }
  if (server.arg("mqttPort") != "" && server.arg("mqttPort") != String(hasp::mqttPort))
  { // Handle mqttPort
    hasp::shouldSaveConfig = true;
    server
.arg("mqttPort").toCharArray(hasp::mqttPort, 6);
  }
  if (server.arg("haspNode") != "" && server.arg("haspNode") != String(hasp::node))
  { // Handle haspNode
    hasp::shouldSaveConfig = true;
    String lowerHaspNode = server
.arg("haspNode");
    lowerHaspNode.toLowerCase();
    lowerHaspNode.toCharArray(hasp::node, 16);
  }
  if (server.arg("groupName") != "" && server.arg("groupName") != String(hasp::groupName))
  { // Handle groupName
    hasp::shouldSaveConfig = true;
    server
.arg("groupName").toCharArray(hasp::groupName, 16);
  }
  // Check optional values
  if (server.arg("mqttUser") != String(hasp::mqttUser))
  { // Handle mqttUser
    hasp::shouldSaveConfig = true;
    server
.arg("mqttUser").toCharArray(hasp::mqttUser, 32);
  }
  if (server.arg("mqttPassword") != String("********"))
  { // Handle mqttPassword
    hasp::shouldSaveConfig = true;
    server
.arg("mqttPassword").toCharArray(hasp::mqttPassword, 32);
  }
  if (server.arg("configUser") != String(hasp::configUser))
  { // Handle configUser
    hasp::shouldSaveConfig = true;
    server
.arg("configUser").toCharArray(hasp::configUser, 32);
  }
  if (server.arg("configPassword") != String("********"))
  { // Handle configPassword
    hasp::shouldSaveConfig = true;
    server
.arg("configPassword").toCharArray(hasp::configPassword, 32);
  }
  if (server.arg("motionPinConfig") != String(hasp::motionPin))
  { // Handle motionPinConfig
    hasp::shouldSaveConfig = true;
    server
.arg("motionPinConfig").toCharArray(hasp::motionPin, 3);
  }
  if ((server.arg("debugSerialEnabled") == String("on")) && !hasp::debugSerialEnabled)
  { // debugSerialEnabled was disabled but should now be enabled
    hasp::shouldSaveConfig = true;
    hasp::debugSerialEnabled = true;
  }
  else if ((server.arg("debugSerialEnabled") == String("")) && hasp::debugSerialEnabled)
  { // debugSerialEnabled was enabled but should now be disabled
    hasp::shouldSaveConfig = true;
    hasp::debugSerialEnabled = false;
  }
  if ((server.arg("debugTelnetEnabled") == String("on")) && !hasp::debugTelnetEnabled)
  { // debugTelnetEnabled was disabled but should now be enabled
    hasp::shouldSaveConfig = true;
    hasp::debugTelnetEnabled = true;
  }
  else if ((server.arg("debugTelnetEnabled") == String("")) && hasp::debugTelnetEnabled)
  { // debugTelnetEnabled was enabled but should now be disabled
    hasp::shouldSaveConfig = true;
    hasp::debugTelnetEnabled = false;
  }
  if ((server.arg("mdnsEnabled") == String("on")) && !hasp::mdnsEnabled)
  { // mdnsEnabled was disabled but should now be enabled
    hasp::shouldSaveConfig = true;
    hasp::mdnsEnabled = true;
  }
  else if ((server.arg("mdnsEnabled") == String("")) && hasp::mdnsEnabled)
  { // mdnsEnabled was enabled but should now be disabled
    hasp::shouldSaveConfig = true;
    hasp::mdnsEnabled = false;
  }
  if ((server.arg("beepEnabled") == String("on")) && !hasp::beepEnabled)
  { // beepEnabled was disabled but should now be enabled
    hasp::shouldSaveConfig = true;
    hasp::beepEnabled = true;
  }
  else if ((server.arg("beepEnabled") == String("")) && hasp::beepEnabled)
  { // beepEnabled was enabled but should now be disabled
    hasp::shouldSaveConfig = true;
    hasp::beepEnabled = false;
  }

  if (hasp::shouldSaveConfig)
  { // Config updated, notify user and trigger write to SPIFFS
    httpMessage += String(F("<meta http-equiv='refresh' content='15;url=/' />"));
    httpMessage += FPSTR(HTTP_HEADER_END);
    httpMessage += String(F("<h1>")) + String(haspNode) + String(F("</h1>"));
    httpMessage += String(F("<br/>Saving updated configuration values and restarting device"));
    httpMessage += FPSTR(HTTP_END);
    server
.send(200, "text/html", httpMessage);

    persistance::configSave();
    if (shouldSaveWifi)
    {
      hasp::debugPrintln(String(F("CONFIG: Attempting connection to SSID: ")) + server
  .arg("wifiSSID"));
      hasp::wifiSetup();
    }
    hasp::reset();
  }
  else
  { // No change found, notify user and link back to config page
    httpMessage += String(F("<meta http-equiv='refresh' content='3;url=/' />"));
    httpMessage += FPSTR(HTTP_HEADER_END);
    httpMessage += String(F("<h1>")) + String(haspNode) + String(F("</h1>"));
    httpMessage += String(F("<br/>No changes found, returning to <a href='/'>home page</a>"));
    httpMessage += FPSTR(HTTP_END);
    server
.send(200, "text/html", httpMessage);
  }
}

void handleResetConfig()
{ // http://plate01/resetConfig
  if (hasp::configPassword[0] != '\0')
  { //Request HTTP auth if configPassword is set
    if (!server
.authenticate(hasp::configUser, configPassword))
    {
      return server
  .requestAuthentication();
    }
  }
  debugPrintln(String(F("HTTP: Sending /resetConfig page to client connected from: ")) + server.client().remoteIP().toString());
  String httpMessage = FPSTR(HTTP_HEADER);
  httpMessage.replace("{v}", String(hasp::node));
  httpMessage += FPSTR(HTTP_SCRIPT);
  httpMessage += FPSTR(HTTP_STYLE);
  httpMessage += String(HASP_STYLE_STRING);
  httpMessage += FPSTR(HTTP_HEADER_END);

  if (server.arg("confirm") == "yes")
  { // User has confirmed, so reset everything
    httpMessage += String(F("<h1>"));
    httpMessage += String(haspNode);
    httpMessage += String(F("</h1><b>Resetting all saved settings and restarting device into WiFi AP mode</b>"));
    httpMessage += FPSTR(HTTP_END);
    server
.send(200, "text/html", httpMessage);
    delay(1000);
    configClearSaved();
  }
  else
  {
    httpMessage += String(F("<h1>Warning</h1><b>This process will reset all settings to the default values and restart the device.  You may need to connect to the WiFi AP displayed on the panel to re-configure the device before accessing it again."));
    httpMessage += String(F("<br/><hr><br/><form method='get' action='resetConfig'>"));
    httpMessage += String(F("<br/><br/><button type='submit' name='confirm' value='yes'>reset all settings</button></form>"));
    httpMessage += String(F("<br/><hr><br/><form method='get' action='/'>"));
    httpMessage += String(F("<button type='submit'>return home</button></form>"));
    httpMessage += FPSTR(HTTP_END);
    server
.send(200, "text/html", httpMessage);
  }
}

void handleResetBacklight()
{ // http://plate01/resetBacklight
  if (configPassword[0] != '\0')
  { //Request HTTP auth if configPassword is set
    if (!server
.authenticate(configUser, configPassword))
    {
      return server
  .requestAuthentication();
    }
  }

  debugPrintln(String(F("HTTP: Sending /resetBacklight page to client connected from: ")) + server.client().remoteIP().toString());
  String httpMessage = FPSTR(HTTP_HEADER);
  httpMessage.replace("{v}", (String(haspNode) + " HASP backlight reset"));
  httpMessage += FPSTR(HTTP_SCRIPT);
  httpMessage += FPSTR(HTTP_STYLE);
  httpMessage += String(HASP_STYLE_STRING);
  httpMessage += String(F("<meta http-equiv='refresh' content='3;url=/' />"));
  httpMessage += FPSTR(HTTP_HEADER_END);
  httpMessage += String(F("<h1>")) + String(haspNode) + String(F("</h1>"));
  httpMessage += String(F("<br/>Resetting backlight to 100%"));
  httpMessage += FPSTR(HTTP_END);
  server.send(200, "text/html", httpMessage);
  debugPrintln(F("HTTP: Resetting backlight to 100%"));
  nextionSetAttr("dims", "100");
}

void handleFirmware()
{ // http://plate01/firmware
  if (configPassword[0] != '\0')
  { //Request HTTP auth if configPassword is set
    if (!server
.authenticate(configUser, configPassword))
    {
      return server
  .requestAuthentication();
    }
  }
  debugPrintln(String(F("HTTP: Sending /firmware page to client connected from: ")) + server.client().remoteIP().toString());
  String httpMessage = FPSTR(HTTP_HEADER);
  httpMessage.replace("{v}", (String(hasp::node) + " update"));
  httpMessage += FPSTR(HTTP_SCRIPT);
  httpMessage += FPSTR(HTTP_STYLE);
  httpMessage += String(HASP_STYLE_STRING);
  httpMessage += FPSTR(HTTP_HEADER_END);
  httpMessage += String(F("<h1>")) + String(haspNode) + String(F(" firmware</h1>"));

  // Display main firmware page
  // HTTPS Disabled pending resolution of issue: https://github.com/esp8266/Arduino/issues/4696
  // Until then, using a proxy host at http://haswitchplate.com to deliver unsecured firmware images from GitHub
  httpMessage += String(F("<form method='get' action='/espfirmware'>"));
  if (hasp::updateEspAvailable)
  {
    httpMessage += String(F("<font color='green'><b>HASP ESP8266 update available!</b></font>"));
  }
  httpMessage += String(F("<br/><b>Update ESP8266 from URL</b><small><i> http only</i></small>"));
  httpMessage += String(F("<br/><input id='espFirmwareURL' name='espFirmware' value='")) + hasp::espFirmwareUrl + "'>";
  httpMessage += String(F("<br/><br/><button type='submit'>Update ESP from URL</button></form>"));

  httpMessage += String(F("<br/><form method='POST' action='/update' enctype='multipart/form-data'>"));
  httpMessage += String(F("<b>Update ESP8266 from file</b><input type='file' id='espSelect' name='espSelect' accept='.bin'>"));
  httpMessage += String(F("<br/><br/><button type='submit' id='espUploadSubmit' onclick='ackEspUploadSubmit()'>Update ESP from file</button></form>"));

  httpMessage += String(F("<br/><br/><hr><h1>WARNING!</h1>"));
  httpMessage += String(F("<b>Nextion LCD firmware updates can be risky.</b> If interrupted, the HASP will need to be manually power cycled which might mean a trip to the breaker box. "));
  httpMessage += String(F("After a power cycle, the LCD will display an error message until a successful firmware update has completed.<br/>"));

  httpMessage += String(F("<br/><hr><form method='get' action='lcddownload'>"));
  if (hasp::updateLcdAvailable)
  {
    httpMessage += String(F("<font color='green'><b>HASP LCD update available!</b></font>"));
  }
  httpMessage += String(F("<br/><b>Update Nextion LCD from URL</b><small><i> http only</i></small>"));
  httpMessage += String(F("<br/><input id='lcdFirmware' name='lcdFirmware' value='")) + hasp::lcdFirmwareUrl + "'>";
  httpMessage += String(F("<br/><br/><button type='submit'>Update LCD from URL</button></form>"));

  httpMessage += String(F("<br/><form method='POST' action='/lcdupload' enctype='multipart/form-data'>"));
  httpMessage += String(F("<br/><b>Update Nextion LCD from file</b><input type='file' id='lcdSelect' name='files[]' accept='.tft'/>"));
  httpMessage += String(F("<br/><br/><button type='submit' id='lcdUploadSubmit' onclick='ackLcdUploadSubmit()'>Update LCD from file</button></form>"));

  // Javascript to collect the filesize of the LCD upload and send it to /tftFileSize
  httpMessage += String(F("<script>function handleLcdFileSelect(evt) {"));
  httpMessage += String(F("var uploadFile = evt.target.files[0];"));
  httpMessage += String(F("document.getElementById('lcdUploadSubmit').innerHTML = 'Upload LCD firmware ' + uploadFile.name;"));
  httpMessage += String(F("var tftFileSize = '/tftFileSize?tftFileSize=' + uploadFile.size;"));
  httpMessage += String(F("var xhttp = new XMLHttpRequest();xhttp.open('GET', tftFileSize, true);xhttp.send();}"));
  httpMessage += String(F("function ackLcdUploadSubmit() {document.getElementById('lcdUploadSubmit').innerHTML = 'Uploading LCD firmware...';}"));
  httpMessage += String(F("function handleEspFileSelect(evt) {var uploadFile = evt.target.files[0];document.getElementById('espUploadSubmit').innerHTML = 'Upload ESP firmware ' + uploadFile.name;}"));
  httpMessage += String(F("function ackEspUploadSubmit() {document.getElementById('espUploadSubmit').innerHTML = 'Uploading ESP firmware...';}"));
  httpMessage += String(F("document.getElementById('lcdSelect').addEventListener('change', handleLcdFileSelect, false);"));
  httpMessage += String(F("document.getElementById('espSelect').addEventListener('change', handleEspFileSelect, false);</script>"));

  httpMessage += FPSTR(HTTP_END);
  server.send(200, "text/html", httpMessage);
}

void handleEspFirmware()
{ 
  // http://plate01/espfirmware
  if (configPassword[0] != '\0')
  { //Request HTTP auth if configPassword is set
    if (!server
.authenticate(configUser, configPassword))
    {
      return server
  .requestAuthentication();
    }
  }

  debugPrintln(String(F("HTTP: Sending /espfirmware page to client connected from: ")) + server.client().remoteIP().toString());
  String httpMessage = FPSTR(HTTP_HEADER);
  httpMessage.replace("{v}", (String(haspNode) + " ESP update"));
  httpMessage += FPSTR(HTTP_SCRIPT);
  httpMessage += FPSTR(HTTP_STYLE);
  httpMessage += String(HASP_STYLE);
  httpMessage += String(F("<meta http-equiv='refresh' content='60;url=/' />"));
  httpMessage += FPSTR(HTTP_HEADER_END);
  httpMessage += String(F("<h1>"));
  httpMessage += String(haspNode) + " ESP update";
  httpMessage += String(F("</h1>"));
  httpMessage += "<br/>Updating ESP firmware from: " + String(server.arg("espFirmware"));
  httpMessage += FPSTR(HTTP_END);
  server.send(200, "text/html", httpMessage);

  debugPrintln("ESPFW: Attempting ESP firmware update from: " + String(server.arg("espFirmware")));
  espStartOta(server.arg("espFirmware"));
}

void handleLcdUpload()
{ 
  // http://plate01/lcdupload
  // Upload firmware to the Nextion LCD via HTTP upload

  if (configPassword[0] != '\0')
  { //Request HTTP auth if configPassword is set
    if (!server
.authenticate(configUser, configPassword))
    {
      return server
  .requestAuthentication();
    }
  }

  static uint32_t lcdOtaTransferred = 0;
  static uint32_t lcdOtaRemaining;
  static uint16_t lcdOtaParts;
  const uint32_t lcdOtaTimeout = 30000; // timeout for receiving new data in milliseconds
  static uint32_t lcdOtaTimer = 0;      // timer for upload timeout

  HTTPUpload &upload = server.upload();

  if (tftFileSize == 0)
  {
    debugPrintln(String(F("LCD OTA: FAILED, no filesize sent.")));
    String httpMessage = FPSTR(HTTP_HEADER);
    httpMessage.replace("{v}", (String(haspNode) + " LCD update"));
    httpMessage += FPSTR(HTTP_SCRIPT);
    httpMessage += FPSTR(HTTP_STYLE);
    httpMessage += String(HASP_STYLE);
    httpMessage += String(F("<meta http-equiv='refresh' content='5;url=/' />"));
    httpMessage += FPSTR(HTTP_HEADER_END);
    httpMessage += String(F("<h1>")) + String(haspNode) + " LCD update FAILED</h1>";
    httpMessage += String(F("No update file size reported.  You must use a modern browser with Javascript enabled."));
    httpMessage += FPSTR(HTTP_END);
    server
.send(200, "text/html", httpMessage);
  }
  else if ((lcdOtaTimer > 0) && ((millis() - lcdOtaTimer) > lcdOtaTimeout))
  { // Our timer expired so reset
    debugPrintln(F("LCD OTA: ERROR: LCD upload timeout.  Restarting."));
    espReset();
  }
  else if (upload.status == UPLOAD_FILE_START)
  {
    WiFiUDP::stopAll(); // Keep mDNS responder from breaking things

    debugPrintln(String(F("LCD OTA: Attempting firmware upload")));
    debugPrintln(String(F("LCD OTA: upload.filename: ")) + String(upload.filename));
    debugPrintln(String(F("LCD OTA: TFTfileSize: ")) + String(tftFileSize));

    lcdOtaRemaining = tftFileSize;
    lcdOtaParts = (lcdOtaRemaining / 4096) + 1;
    debugPrintln(String(F("LCD OTA: File upload beginning. Size ")) + String(lcdOtaRemaining) + String(F(" bytes in ")) + String(lcdOtaParts) + String(F(" 4k chunks.")));

    Serial1.write(nextionSuffix, sizeof(nextionSuffix)); // Send empty command to LCD
    Serial1.flush();
    nextionHandleInput();

    String lcdOtaNextionCmd = "whmi-wri " + String(tftFileSize) + ",115200,0";
    debugPrintln(String(F("LCD OTA: Sending LCD upload command: ")) + lcdOtaNextionCmd);
    Serial1.print(lcdOtaNextionCmd);
    Serial1.write(nextionSuffix, sizeof(nextionSuffix));
    Serial1.flush();

    if (nextionOtaResponse())
    {
      debugPrintln(F("LCD OTA: LCD upload command accepted"));
    }
    else
    {
      debugPrintln(F("LCD OTA: LCD upload command FAILED."));
      espReset();
    }
    lcdOtaTimer = millis();
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  { // Handle upload data
    static int lcdOtaChunkCounter = 0;
    static uint16_t lcdOtaPartNum = 0;
    static int lcdOtaPercentComplete = 0;
    static const uint16_t lcdOtaBufferSize = 1024; // upload data buffer before sending to UART
    static uint8_t lcdOtaBuffer[lcdOtaBufferSize] = {};
    uint16_t lcdOtaUploadIndex = 0;
    int32_t lcdOtaPacketRemaining = upload.currentSize;

    while (lcdOtaPacketRemaining > 0)
    { // Write incoming data to panel as it arrives
      // determine chunk size as lowest value of lcdOtaPacketRemaining, lcdOtaBufferSize, or 4096 - lcdOtaChunkCounter
      uint16_t lcdOtaChunkSize = 0;
      if ((lcdOtaPacketRemaining <= lcdOtaBufferSize) && (lcdOtaPacketRemaining <= (4096 - lcdOtaChunkCounter)))
      {
        lcdOtaChunkSize = lcdOtaPacketRemaining;
      }
      else if ((lcdOtaBufferSize <= lcdOtaPacketRemaining) && (lcdOtaBufferSize <= (4096 - lcdOtaChunkCounter)))
      {
        lcdOtaChunkSize = lcdOtaBufferSize;
      }
      else
      {
        lcdOtaChunkSize = 4096 - lcdOtaChunkCounter;
      }

      for (uint16_t i = 0; i < lcdOtaChunkSize; i++)
      { // Load up the UART buffer
        lcdOtaBuffer[i] = upload.buf[lcdOtaUploadIndex];
        lcdOtaUploadIndex++;
      }
      Serial1.flush();                              // Clear out current UART buffer
      Serial1.write(lcdOtaBuffer, lcdOtaChunkSize); // And send the most recent data
      lcdOtaChunkCounter += lcdOtaChunkSize;
      lcdOtaTransferred += lcdOtaChunkSize;
      if (lcdOtaChunkCounter >= 4096)
      {
        Serial1.flush();
        lcdOtaPartNum++;
        lcdOtaPercentComplete = (lcdOtaTransferred * 100) / tftFileSize;
        lcdOtaChunkCounter = 0;
        if (nextionOtaResponse())
        {
          debugPrintln(String(F("LCD OTA: Part ")) + String(lcdOtaPartNum) + String(F(" OK, ")) + String(lcdOtaPercentComplete) + String(F("% complete")));
        }
        else
        {
          debugPrintln(String(F("LCD OTA: Part ")) + String(lcdOtaPartNum) + String(F(" FAILED, ")) + String(lcdOtaPercentComplete) + String(F("% complete")));
        }
      }
      else
      {
        delay(10);
      }
      if (lcdOtaRemaining > 0)
      {
        lcdOtaRemaining -= lcdOtaChunkSize;
      }
      if (lcdOtaPacketRemaining > 0)
      {
        lcdOtaPacketRemaining -= lcdOtaChunkSize;
      }
    }

    if (lcdOtaTransferred >= tftFileSize)
    {
      if (nextionOtaResponse())
      {
        debugPrintln(String(F("LCD OTA: Success, wrote ")) + String(lcdOtaTransferred) + " of " + String(tftFileSize) + " bytes.");
        server
    .sendHeader("Location", "/lcdOtaSuccess");
        server
    .send(303);
        uint32_t lcdOtaDelay = millis();
        while ((millis() - lcdOtaDelay) < 5000)
        { // extra 5sec delay while the LCD handles any local firmware updates from new versions of code sent to it
          server
      .handleClient();
          delay(1);
        }
        espReset();
      }
      else
      {
        debugPrintln(F("LCD OTA: Failure"));
        server
    .sendHeader("Location", "/lcdOtaFailure");
        server
    .send(303);
        uint32_t lcdOtaDelay = millis();
        while ((millis() - lcdOtaDelay) < 1000)
        { // extra 1sec delay for client to grab failure page
          server
      .handleClient();
          delay(1);
        }
        espReset();
      }
    }
    lcdOtaTimer = millis();
  }
  else if (upload.status == UPLOAD_FILE_END)
  { // Upload completed
    if (lcdOtaTransferred >= tftFileSize)
    {
      if (nextionOtaResponse())
      { // YAY WE DID IT
        debugPrintln(String(F("LCD OTA: Success, wrote ")) + String(lcdOtaTransferred) + " of " + String(tftFileSize) + " bytes.");
        server
    .sendHeader("Location", "/lcdOtaSuccess");
        server
    .send(303);
        uint32_t lcdOtaDelay = millis();
        while ((millis() - lcdOtaDelay) < 5000)
        { // extra 5sec delay while the LCD handles any local firmware updates from new versions of code sent to it
          server
      .handleClient();
          delay(1);
        }
        espReset();
      }
      else
      {
        debugPrintln(F("LCD OTA: Failure"));
        server
    .sendHeader("Location", "/lcdOtaFailure");
        server
    .send(303);
        uint32_t lcdOtaDelay = millis();
        while ((millis() - lcdOtaDelay) < 1000)
        { // extra 1sec delay for client to grab failure page
          server
      .handleClient();
          delay(1);
        }
        espReset();
      }
    }
  }
  else if (upload.status == UPLOAD_FILE_ABORTED)
  { // Something went kablooey
    debugPrintln(F("LCD OTA: ERROR: upload.status returned: UPLOAD_FILE_ABORTED"));
    debugPrintln(F("LCD OTA: Failure"));
    server
.sendHeader("Location", "/lcdOtaFailure");
    server
.send(303);
    uint32_t lcdOtaDelay = millis();
    while ((millis() - lcdOtaDelay) < 1000)
    { // extra 1sec delay for client to grab failure page
      server
  .handleClient();
      delay(1);
    }
    espReset();
  }
  else
  { // Something went weird, we should never get here...
    debugPrintln(String(F("LCD OTA: upload.status returned: ")) + String(upload.status));
    debugPrintln(F("LCD OTA: Failure"));
    server
.sendHeader("Location", "/lcdOtaFailure");
    server
.send(303);
    uint32_t lcdOtaDelay = millis();
    while ((millis() - lcdOtaDelay) < 1000)
    { // extra 1sec delay for client to grab failure page
      server
  .handleClient();
      delay(1);
    }
    espReset();
  }
}

void handleLcdUpdateSuccess()
{ 
  // http://plate01/lcdOtaSuccess
  if (configPassword[0] != '\0')
  { //Request HTTP auth if configPassword is set
    if (!server
.authenticate(configUser, configPassword))
    {
      return server
  .requestAuthentication();
    }
  }
  debugPrintln(String(F("HTTP: Sending /lcdOtaSuccess page to client connected from: ")) + server.client().remoteIP().toString());
  String httpMessage = FPSTR(HTTP_HEADER);
  httpMessage.replace("{v}", (String(haspNode) + " LCD update success"));
  httpMessage += FPSTR(HTTP_SCRIPT);
  httpMessage += FPSTR(HTTP_STYLE);
  httpMessage += String(HASP_STYLE);
  httpMessage += String(F("<meta http-equiv='refresh' content='15;url=/' />"));
  httpMessage += FPSTR(HTTP_HEADER_END);
  httpMessage += String(F("<h1>")) + String(haspNode) + String(F(" LCD update success</h1>"));
  httpMessage += String(F("Restarting HASwitchPlate to apply changes..."));
  httpMessage += FPSTR(HTTP_END);
  server.send(200, "text/html", httpMessage);
}

void handleLcdUpdateFailure()
{ 
  // http://plate01/lcdOtaFailure
  if (configPassword[0] != '\0')
  { //Request HTTP auth if configPassword is set
    if (!server
.authenticate(configUser, configPassword))
    {
      return server
  .requestAuthentication();
    }
  }
  debugPrintln(String(F("HTTP: Sending /lcdOtaFailure page to client connected from: ")) + server.client().remoteIP().toString());
  String httpMessage = FPSTR(HTTP_HEADER);
  httpMessage.replace("{v}", (String(haspNode) + " LCD update failed"));
  httpMessage += FPSTR(HTTP_SCRIPT);
  httpMessage += FPSTR(HTTP_STYLE);
  httpMessage += String(HASP_STYLE);
  httpMessage += String(F("<meta http-equiv='refresh' content='15;url=/' />"));
  httpMessage += FPSTR(HTTP_HEADER_END);
  httpMessage += String(F("<h1>")) + String(haspNode) + String(F(" LCD update failed :(</h1>"));
  httpMessage += String(F("Restarting HASwitchPlate to reset device..."));
  httpMessage += FPSTR(HTTP_END);
  server.send(200, "text/html", httpMessage);
}

void handleLcdDownload()
{ 
  // http://plate01/lcddownload
  if (configPassword[0] != '\0')
  { //Request HTTP auth if configPassword is set
    if (!server
.authenticate(configUser, configPassword))
    {
      return server
  .requestAuthentication();
    }
  }
  debugPrintln(String(F("HTTP: Sending /lcddownload page to client connected from: ")) + server.client().remoteIP().toString());
  String httpMessage = FPSTR(HTTP_HEADER);
  httpMessage.replace("{v}", (String(haspNode) + " LCD update"));
  httpMessage += FPSTR(HTTP_SCRIPT);
  httpMessage += FPSTR(HTTP_STYLE);
  httpMessage += String(HASP_STYLE);
  httpMessage += FPSTR(HTTP_HEADER_END);
  httpMessage += String(F("<h1>"));
  httpMessage += String(haspNode) + " LCD update";
  httpMessage += String(F("</h1>"));
  httpMessage += "<br/>Updating LCD firmware from: " + String(server.arg("lcdFirmware"));
  httpMessage += FPSTR(HTTP_END);
  server.send(200, "text/html", httpMessage);

  nextionStartOtaDownload(server.arg("lcdFirmware"));
}

void handleTftFileSize()
{
  // http://plate01/tftFileSize
  if (configPassword[0] != '\0')
  { //Request HTTP auth if configPassword is set
    if (!server
.authenticate(configUser, configPassword))
    {
      return server
  .requestAuthentication();
    }
  }
  debugPrintln(String(F("HTTP: Sending /tftFileSize page to client connected from: ")) + server.client().remoteIP().toString());
  String httpMessage = FPSTR(HTTP_HEADER);
  httpMessage.replace("{v}", (String(haspNode) + " TFT Filesize"));
  httpMessage += FPSTR(HTTP_HEADER_END);
  httpMessage += FPSTR(HTTP_END);
  server.send(200, "text/html", httpMessage);
  tftFileSize = server.arg("tftFileSize").toInt();
  debugPrintln(String(F("WEB: tftFileSize: ")) + String(tftFileSize));
}

void handleReboot()
{ 
  // http://plate01/reboot
  if (configPassword[0] != '\0')
  { //Request HTTP auth if configPassword is set
    if (!server
.authenticate(configUser, configPassword))
    {
      return server
  .requestAuthentication();
    }
  }
  debugPrintln(String(F("HTTP: Sending /reboot page to client connected from: ")) + server.client().remoteIP().toString());
  String httpMessage = FPSTR(HTTP_HEADER);
  httpMessage.replace("{v}", (String(haspNode) + " HASP reboot"));
  httpMessage += FPSTR(HTTP_SCRIPT);
  httpMessage += FPSTR(HTTP_STYLE);
  httpMessage += String(HASP_STYLE);
  httpMessage += String(F("<meta http-equiv='refresh' content='10;url=/' />"));
  httpMessage += FPSTR(HTTP_HEADER_END);
  httpMessage += String(F("<h1>")) + String(haspNode) + String(F("</h1>"));
  httpMessage += String(F("<br/>Rebooting device"));
  httpMessage += FPSTR(HTTP_END);
  server.send(200, "text/html", httpMessage);
  debugPrintln(F("RESET: Rebooting device"));
  nextionSendCmd("page 0");
  nextionSetAttr("p[0].b[1].txt", "\"Rebooting...\"");
  hasp::reset();
}

void web::handleClient()
{
  server.handleClient();
}

ESP8266WebServer *web::getServer()
{
  return &server;
};