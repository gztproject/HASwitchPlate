#include <nextion.h>

byte nextion::returnBuffer[128];  // Byte array to pass around data coming from the panel
uint8_t nextion::returnIndex = 0; // Index for nextionReturnBuffer
uint8_t nextion::activePage = 0;  // Track active LCD page
bool nextion::lcdConnected = false;
const byte suffix[] = {0xFF, 0xFF, 0xFF};
uint32_t tftFileSize = 0;

bool nextion::reportPage0 = false;

unsigned long nextion::lcdVersion = 0;
unsigned long nextion::updateLcdAvailableVersion;
bool nextion::lcdVersionQueryFlag = false;
const String nextion::lcdVersionQuery = "p[0].b[2].val"; 

void nextion::begin()
{
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
}

void nextion::processInput()
{ // Process incoming serial commands from the Nextion panel
  // Command reference: https://www.itead.cc/wiki/Nextion_Instruction_Set#Format_of_Device_Return_Data
  // tl;dr: command byte, command data, 0xFF 0xFF 0xFF

  if (returnBuffer[0] == 0x65)
  { // Handle incoming touch command
    // 0x65+Page ID+Component ID+TouchEvent+End
    // Return this data when the touch event created by the user is pressed.
    // Definition of TouchEvent: Press Event 0x01, Release Event 0X00
    // Example: 0x65 0x00 0x02 0x01 0xFF 0xFF 0xFF
    // Meaning: Touch Event, Page 0, Object 2, Press
    String nextionPage = String(returnBuffer[1]);
    String nextionButtonID = String(returnBuffer[2]);
    byte nextionButtonAction = returnBuffer[3];

    if (nextionButtonAction == 0x01)
    {
      hasp::debugPrintln(String(F("HMI IN: [Button ON] 'p[")) + nextionPage + "].b[" + nextionButtonID + "]'");
      String mqttButtonTopic = mqttWrapper::stateTopic + "/p[" + nextionPage + "].b[" + nextionButtonID + "]";
      hasp::debugPrintln(String(F("MQTT OUT: '")) + mqttButtonTopic + "' : 'ON'");
      mqttWrapper::getClient().publish(mqttButtonTopic, "ON");
      String mqttButtonJSONEvent = String(F("{\"event\":\"p[")) + String(nextionPage) + String(F("].b[")) + String(nextionButtonID) + String(F("]\", \"value\":\"ON\"}"));
      mqttWrapper::getClient().publish(mqttWrapper::stateJSONTopic, mqttButtonJSONEvent);
      if (hasp::beepEnabled)
      {
        hasp::beepOnTime = 500;
        hasp::beepOffTime = 100;
        hasp::beepCounter = 1;
      }
    }
    if (nextionButtonAction == 0x00)
    {
      hasp::debugPrintln(String(F("HMI IN: [Button OFF] 'p[")) + nextionPage + "].b[" + nextionButtonID + "]'");
      String mqttButtonTopic = mqttWrapper::stateTopic + "/p[" + nextionPage + "].b[" + nextionButtonID + "]";
      hasp::debugPrintln(String(F("MQTT OUT: '")) + mqttButtonTopic + "' : 'OFF'");
      mqttWrapper::getClient().publish(mqttButtonTopic, "OFF");
      // Now see if this object has a .val that might have been updated.  Works for sliders,
      // two-state buttons, etc, throws a 0x1A error for normal buttons which we'll catch and ignore
      getSubtopic = "/p[" + nextionPage + "].b[" + nextionButtonID + "].val";
      getSubtopicJSON = "p[" + nextionPage + "].b[" + nextionButtonID + "].val";
      nextionGetAttr("p[" + nextionPage + "].b[" + nextionButtonID + "].val");
    }
  }
  else if (returnBuffer[0] == 0x66)
  { // Handle incoming "sendme" page number
    // 0x66+PageNum+End
    // Example: 0x66 0x02 0xFF 0xFF 0xFF
    // Meaning: page 2
    String nextionPage = String(returnBuffer[1]);
    hasp::debugPrintln(String(F("HMI IN: [sendme Page] '")) + nextionPage + "'");
    // if ((activePage != nextionPage.toInt()) && ((nextionPage != "0") || reportPage0))
    if ((nextionPage != "0") || reportPage0)
    { // If we have a new page AND ( (it's not "0") OR (we've set the flag to report 0 anyway) )
      activePage = nextionPage.toInt();
      String mqttPageTopic = mqttWrapper::stateTopic + "/page";
      hasp::debugPrintln(String(F("MQTT OUT: '")) + mqttPageTopic + "' : '" + nextionPage + "'");
      mqttWrapper::getClient().publish(mqttPageTopic, nextionPage);
    }
  }
  else if (nextionReturnBuffer[0] == 0x67)
  { // Handle touch coordinate data
    // 0X67+Coordinate X High+Coordinate X Low+Coordinate Y High+Coordinate Y Low+TouchEvent+End
    // Example: 0X67 0X00 0X7A 0X00 0X1E 0X01 0XFF 0XFF 0XFF
    // Meaning: Coordinate (122,30), Touch Event: Press
    // issue Nextion command "sendxy=1" to enable this output
    uint16_t xCoord = nextionReturnBuffer[1];
    xCoord = xCoord * 256 + nextionReturnBuffer[2];
    uint16_t yCoord = nextionReturnBuffer[3];
    yCoord = yCoord * 256 + nextionReturnBuffer[4];
    String xyCoord = String(xCoord) + ',' + String(yCoord);
    byte nextionTouchAction = nextionReturnBuffer[5];
    if (nextionTouchAction == 0x01)
    {
      hasp::debugPrintln(String(F("HMI IN: [Touch ON] '")) + xyCoord + "'");
      String mqttTouchTopic = mqttWrapper::stateTopic + "/touchOn";
      hasp::debugPrintln(String(F("MQTT OUT: '")) + mqttTouchTopic + "' : '" + xyCoord + "'");
      mqttWrapper::getClient().publish(mqttTouchTopic, xyCoord);
    }
    else if (nextionTouchAction == 0x00)
    {
      hasp::debugPrintln(String(F("HMI IN: [Touch OFF] '")) + xyCoord + "'");
      String mqttTouchTopic = mqttWrapper::stateTopic + "/touchOff";
      hasp::debugPrintln(String(F("MQTT OUT: '")) + mqttTouchTopic + "' : '" + xyCoord + "'");
      mqttWrapper::getClient().publish(mqttTouchTopic, xyCoord);
    }
  }
  else if (returnBuffer[0] == 0x70)
  { // Handle get string return
    // 0x70+ASCII string+End
    // Example: 0x70 0x41 0x42 0x43 0x44 0x31 0x32 0x33 0x34 0xFF 0xFF 0xFF
    // Meaning: String data, ABCD1234
    String getString;
    for (int i = 1; i < returnIndex - 3; i++)
    { // convert the payload into a string
      getString += (char)returnBuffer[i];
    }
    hasp::debugPrintln(String(F("HMI IN: [String Return] '")) + getString + "'");
    if (mqttGetSubtopic == "")
    { // If there's no outstanding request for a value, publish to mqttStateTopic
      hasp::debugPrintln(String(F("MQTT OUT: '")) + mqttWrapper::stateTopic + "' : '" + getString + "]");
      mqttWrapper::getClient().publish(mqttWrapper::stateTopic, getString);
    }
    else
    { // Otherwise, publish the to saved mqttGetSubtopic and then reset mqttGetSubtopic
      String mqttReturnTopic = mqttWrapper::stateTopic + mqttGetSubtopic;
      hasp::debugPrintln(String(F("MQTT OUT: '")) + mqttReturnTopic + "' : '" + getString + "]");
      mqttWrapper::getClient().publish(mqttReturnTopic, getString);
      mqttGetSubtopic = "";
    }
  }
  else if (nextionReturnBuffer[0] == 0x71)
  { // Handle get int return
    // 0x71+byte1+byte2+byte3+byte4+End (4 byte little endian)
    // Example: 0x71 0x7B 0x00 0x00 0x00 0xFF 0xFF 0xFF
    // Meaning: Integer data, 123
    unsigned long getInt = nextionReturnBuffer[4];
    getInt = getInt * 256 + nextionReturnBuffer[3];
    getInt = getInt * 256 + nextionReturnBuffer[2];
    getInt = getInt * 256 + nextionReturnBuffer[1];
    String getString = String(getInt);
    hasp::debugPrintln(String(F("HMI IN: [Int Return] '")) + getString + "'");

    if (lcdVersionQueryFlag)
    {
      lcdVersion = getInt;
      lcdVersionQueryFlag = false;
      hasp::debugPrintln(String(F("HMI IN: lcdVersion '")) + String(lcdVersion) + "'");
    }
    else if (mqttGetSubtopic == "")
    {
      mqttClient.publish(mqttStateTopic, getString);
    }
    // Otherwise, publish the to saved mqttGetSubtopic and then reset mqttGetSubtopic
    else
    {
      String mqttReturnTopic = mqttStateTopic + mqttGetSubtopic;
      mqttClient.publish(mqttReturnTopic, getString);
      String mqttButtonJSONEvent = String(F("{\"event\":\"")) + mqttGetSubtopicJSON + String(F("\", \"value\":")) + getString + String(F("}"));
      mqttClient.publish(mqttStateJSONTopic, mqttButtonJSONEvent);
      mqttGetSubtopic = "";
    }
  }
  else if (nextionReturnBuffer[0] == 0x63 && nextionReturnBuffer[1] == 0x6f && nextionReturnBuffer[2] == 0x6d && nextionReturnBuffer[3] == 0x6f && nextionReturnBuffer[4] == 0x6b)
  { // Catch 'comok' response to 'connect' command: https://www.itead.cc/blog/nextion-hmi-upload-protocol
    String comokField;
    uint8_t comokFieldCount = 0;
    byte comokFieldSeperator = 0x2c; // ","

    for (uint8_t i = 0; i <= nextionReturnIndex; i++)
    { // cycle through each byte looking for our field seperator
      if (nextionReturnBuffer[i] == comokFieldSeperator)
      { // Found the end of a field, so do something with it.  Maybe.
        if (comokFieldCount == 2)
        {
          nextionModel = comokField;
          debugPrintln(String(F("HMI IN: nextionModel: ")) + nextionModel);
        }
        comokFieldCount++;
        comokField = "";
      }
      else
      {
        comokField += String(char(nextionReturnBuffer[i]));
      }
    }
  }

  else if (nextionReturnBuffer[0] == 0x1A)
  { // Catch 0x1A error, possibly from .val query against things that might not support that request
    // 0x1A+End
    // ERROR: Variable name invalid
    // We'll be triggering this a lot due to requesting .val on every component that sends us a Touch Off
    // Just reset mqttGetSubtopic and move on with life.
    mqttGetSubtopic = "";
  }
  nextionReturnIndex = 0; // Done handling the buffer, reset index back to 0
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool nextion::handleInput()
{ // Handle incoming serial data from the Nextion panel
  // This will collect serial data from the panel and place it into the global buffer
  // nextionReturnBuffer[nextionReturnIndex]
  // Return: true if we've received a string of 3 consecutive 0xFF values
  // Return: false otherwise
  bool nextionCommandComplete = false;
  static int nextionTermByteCnt = 0;   // counter for our 3 consecutive 0xFFs
  static String hmiDebug = "HMI IN: "; // assemble a string for debug output

  if (Serial.available())
  {
    lcdConnected = true;
    byte nextionCommandByte = Serial.read();
    hmiDebug += (" 0x" + String(nextionCommandByte, HEX));
    // check to see if we have one of 3 consecutive 0xFF which indicates the end of a command
    if (nextionCommandByte == 0xFF)
    {
      nextionTermByteCnt++;
      if (nextionTermByteCnt >= 3)
      { // We have received a complete command
        nextionCommandComplete = true;
        nextionTermByteCnt = 0; // reset counter
      }
    }
    else
    {
      nextionTermByteCnt = 0; // reset counter if a non-term byte was encountered
    }
    nextionReturnBuffer[nextionReturnIndex] = nextionCommandByte;
    nextionReturnIndex++;
  }
  if (nextionCommandComplete)
  {
    debugPrintln(hmiDebug);
    hmiDebug = "HMI IN: ";
  }
  return nextionCommandComplete;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void nextion::setAttr(String hmiAttribute, String hmiValue)
{ // Set the value of a Nextion component attribute
  Serial1.print(hmiAttribute);
  Serial1.print("=");
  Serial1.print(hmiValue);
  Serial1.write(nextionSuffix, sizeof(nextionSuffix));
  debugPrintln(String(F("HMI OUT: '")) + hmiAttribute + "=" + hmiValue + "'");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void nextion::getAttr(String hmiAttribute)
{ // Get the value of a Nextion component attribute
  // This will only send the command to the panel requesting the attribute, the actual
  // return of that value will be handled by nextionProcessInput and placed into mqttGetSubtopic
  Serial1.print("get " + hmiAttribute);
  Serial1.write(nextionSuffix, sizeof(nextionSuffix));
  debugPrintln(String(F("HMI OUT: 'get ")) + hmiAttribute + "'");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void nextion::sendCmd(String nextionCmd)
{ // Send a raw command to the Nextion panel
  Serial1.print(nextionCmd);
  Serial1.write(nextionSuffix, sizeof(nextionSuffix));
  debugPrintln(String(F("HMI OUT: ")) + nextionCmd);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void nextion::parseJson(String &strPayload)
{ // Parse an incoming JSON array into individual Nextion commands
  if (strPayload.endsWith(",]"))
  { // Trailing null array elements are an artifact of older Home Assistant automations and need to
    // be removed before parsing by ArduinoJSON 6+
    strPayload.remove(strPayload.length() - 2, 2);
    strPayload.concat("]");
  }
  DynamicJsonDocument nextionCommands(mqttMaxPacketSize + 1024);
  DeserializationError jsonError = deserializeJson(nextionCommands, strPayload);
  if (jsonError)
  { // Couldn't parse incoming JSON command
    debugPrintln(String(F("MQTT: [ERROR] Failed to parse incoming JSON command with error: ")) + String(jsonError.c_str()));
  }
  else
  {
    deserializeJson(nextionCommands, strPayload);
    for (uint8_t i = 0; i < nextionCommands.size(); i++)
    {
      nextionSendCmd(nextionCommands[i]);
      delayMicroseconds(500); // Larger JSON objects can take a while to run through over serial,
    }                         // give the ESP and Nextion a moment to deal with life
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void nextion::startOtaDownload(String otaUrl)
{ // Upload firmware to the Nextion LCD via HTTP download
  // based in large part on code posted by indev2 here:
  // http://support.iteadstudio.com/support/discussions/topics/11000007686/page/2

  uint32_t lcdOtaFileSize = 0;
  String lcdOtaNextionCmd;
  uint32_t lcdOtaChunkCounter = 0;
  uint16_t lcdOtaPartNum = 0;
  uint32_t lcdOtaTransferred = 0;
  uint8_t lcdOtaPercentComplete = 0;
  const uint32_t lcdOtaTimeout = 30000; // timeout for receiving new data in milliseconds
  static uint32_t lcdOtaTimer = 0;      // timer for upload timeout

  debugPrintln(String(F("LCD OTA: Attempting firmware download from: ")) + otaUrl);
  WiFiClient lcdOtaWifi;
  HTTPClient lcdOtaHttp;
  lcdOtaHttp.begin(lcdOtaWifi, otaUrl);
  int lcdOtaHttpReturn = lcdOtaHttp.GET();
  if (lcdOtaHttpReturn > 0)
  { // HTTP header has been sent and Server response header has been handled
    debugPrintln(String(F("LCD OTA: HTTP GET return code:")) + String(lcdOtaHttpReturn));
    if (lcdOtaHttpReturn == HTTP_CODE_OK)
    {                                                 // file found at server
      int32_t lcdOtaRemaining = lcdOtaHttp.getSize(); // get length of document (is -1 when Server sends no Content-Length header)
      lcdOtaFileSize = lcdOtaRemaining;
      static uint16_t lcdOtaParts = (lcdOtaRemaining / 4096) + 1;
      static const uint16_t lcdOtaBufferSize = 1024; // upload data buffer before sending to UART
      static uint8_t lcdOtaBuffer[lcdOtaBufferSize] = {};

      debugPrintln(String(F("LCD OTA: File found at Server. Size ")) + String(lcdOtaRemaining) + String(F(" bytes in ")) + String(lcdOtaParts) + String(F(" 4k chunks.")));

      WiFiUDP::stopAll(); // Keep mDNS responder and MQTT traffic from breaking things
      if (mqttClient.connected())
      {
        debugPrintln(F("LCD OTA: LCD firmware upload starting, closing MQTT connection."));
        mqttClient.publish(mqttStatusTopic, "OFF", true, 1);
        mqttClient.publish(mqttSensorTopic, "{\"status\": \"unavailable\"}", true, 1);
        mqttClient.disconnect();
      }

      WiFiClient *stream = lcdOtaHttp.getStreamPtr();      // get tcp stream
      Serial1.write(nextionSuffix, sizeof(nextionSuffix)); // Send empty command
      Serial1.flush();
      nextionHandleInput();
      String lcdOtaNextionCmd = "whmi-wri " + String(lcdOtaFileSize) + ",115200,0";
      debugPrintln(String(F("LCD OTA: Sending LCD upload command: ")) + lcdOtaNextionCmd);
      Serial1.print(lcdOtaNextionCmd);
      Serial1.write(nextionSuffix, sizeof(nextionSuffix));
      Serial1.flush();

      if (nextionOtaResponse())
      {
        debugPrintln(F("LCD OTA: LCD upload command accepted."));
      }
      else
      {
        debugPrintln(F("LCD OTA: LCD upload command FAILED.  Restarting device."));
        espReset();
      }
      debugPrintln(F("LCD OTA: Starting update"));
      lcdOtaTimer = millis();
      while (lcdOtaHttp.connected() && (lcdOtaRemaining > 0 || lcdOtaRemaining == -1))
      {                                                // Write incoming data to panel as it arrives
        uint16_t lcdOtaHttpSize = stream->available(); // get available data size

        if (lcdOtaHttpSize)
        {
          uint16_t lcdOtaChunkSize = 0;
          if ((lcdOtaHttpSize <= lcdOtaBufferSize) && (lcdOtaHttpSize <= (4096 - lcdOtaChunkCounter)))
          {
            lcdOtaChunkSize = lcdOtaHttpSize;
          }
          else if ((lcdOtaBufferSize <= lcdOtaHttpSize) && (lcdOtaBufferSize <= (4096 - lcdOtaChunkCounter)))
          {
            lcdOtaChunkSize = lcdOtaBufferSize;
          }
          else
          {
            lcdOtaChunkSize = 4096 - lcdOtaChunkCounter;
          }
          stream->readBytes(lcdOtaBuffer, lcdOtaChunkSize);
          Serial1.flush();                              // make sure any previous writes the UART have completed
          Serial1.write(lcdOtaBuffer, lcdOtaChunkSize); // now send buffer to the UART
          lcdOtaChunkCounter += lcdOtaChunkSize;
          if (lcdOtaChunkCounter >= 4096)
          {
            Serial1.flush();
            lcdOtaPartNum++;
            lcdOtaTransferred += lcdOtaChunkCounter;
            lcdOtaPercentComplete = (lcdOtaTransferred * 100) / lcdOtaFileSize;
            lcdOtaChunkCounter = 0;
            if (nextionOtaResponse())
            { // We've completed a chunk
              debugPrintln(String(F("LCD OTA: Part ")) + String(lcdOtaPartNum) + String(F(" OK, ")) + String(lcdOtaPercentComplete) + String(F("% complete")));
              lcdOtaTimer = millis();
            }
            else
            {
              debugPrintln(String(F("LCD OTA: Part ")) + String(lcdOtaPartNum) + String(F(" FAILED, ")) + String(lcdOtaPercentComplete) + String(F("% complete")));
              debugPrintln(F("LCD OTA: failure"));
              delay(2000); // extra delay while the LCD does its thing
              espReset();
            }
          }
          else
          {
            delay(20);
          }
          if (lcdOtaRemaining > 0)
          {
            lcdOtaRemaining -= lcdOtaChunkSize;
          }
        }
        delay(10);
        if ((lcdOtaTimer > 0) && ((millis() - lcdOtaTimer) > lcdOtaTimeout))
        { // Our timer expired so reset
          debugPrintln(F("LCD OTA: ERROR: LCD upload timeout.  Restarting."));
          espReset();
        }
      }
      lcdOtaPartNum++;
      lcdOtaTransferred += lcdOtaChunkCounter;
      if ((lcdOtaTransferred == lcdOtaFileSize) && nextionOtaResponse())
      {
        debugPrintln(String(F("LCD OTA: Success, wrote ")) + String(lcdOtaTransferred) + " of " + String(tftFileSize) + " bytes.");
        uint32_t lcdOtaDelay = millis();
        while ((millis() - lcdOtaDelay) < 5000)
        { // extra 5sec delay while the LCD handles any local firmware updates from new versions of code sent to it
          webServer.handleClient();
          delay(1);
        }
        espReset();
      }
      else
      {
        debugPrintln(F("LCD OTA: Failure"));
        espReset();
      }
    }
  }
  else
  {
    debugPrintln(String(F("LCD OTA: HTTP GET failed, error code ")) + lcdOtaHttp.errorToString(lcdOtaHttpReturn));
    espReset();
  }
  lcdOtaHttp.end();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool nextion::otaResponse()
{ // Monitor the serial port for a 0x05 response within our timeout

  unsigned long nextionCommandTimeout = 2000;   // timeout for receiving termination string in milliseconds
  unsigned long nextionCommandTimer = millis(); // record current time for our timeout
  bool otaSuccessVal = false;
  while ((millis() - nextionCommandTimer) < nextionCommandTimeout)
  {
    if (Serial.available())
    {
      byte inByte = Serial.read();
      if (inByte == 0x5)
      {
        otaSuccessVal = true;
        break;
      }
    }
    else
    {
      delay(1);
    }
  }
  return otaSuccessVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void nextion::connect()
{
  if ((millis() - nextionCheckTimer) >= nextionCheckInterval)
  {
    static unsigned int nextionRetryCount = 0;
    if ((nextionModel.length() == 0) && (nextionRetryCount < (nextionRetryMax - 2)))
    { // Try issuing the "connect" command a few times
      debugPrintln(F("HMI: sending Nextion connect request"));
      nextionSendCmd("connect");
      nextionRetryCount++;
      nextionCheckTimer = millis();
    }
    else if ((nextionModel.length() == 0) && (nextionRetryCount < nextionRetryMax))
    { // If we still don't have model info, try to change nextion serial speed from 9600 to 115200
      nextionSetSpeed();
      nextionRetryCount++;
      debugPrintln(F("HMI: sending Nextion serial speed 115200 request"));
      nextionCheckTimer = millis();
    }
    else if ((lcdVersion < 1) && (nextionRetryCount <= nextionRetryMax))
    {
      if (nextionModel.length() == 0)
      { // one last hail mary, maybe the serial speed is set correctly now
        nextionSendCmd("connect");
      }
      nextionSendCmd("get " + lcdVersionQuery);
      lcdVersionQueryFlag = true;
      nextionRetryCount++;
      debugPrintln(F("HMI: sending Nextion version query"));
      nextionCheckTimer = millis();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void nextion::setSpeed()
{
  debugPrintln(F("HMI: No Nextion response, attempting 9600bps connection"));
  Serial1.begin(9600);
  Serial1.write(nextionSuffix, sizeof(nextionSuffix));
  Serial1.print("bauds=115200");
  Serial1.write(nextionSuffix, sizeof(nextionSuffix));
  Serial1.flush();
  Serial1.begin(115200);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void nextion::reset()
{
  debugPrintln(F("HMI: Rebooting LCD"));
  digitalWrite(nextionResetPin, LOW);
  Serial1.print("rest");
  Serial1.write(nextionSuffix, sizeof(nextionSuffix));
  Serial1.flush();
  delay(100);
  digitalWrite(nextionResetPin, HIGH);

  unsigned long lcdResetTimer = millis();
  const unsigned long lcdResetTimeout = 5000;

  lcdConnected = false;
  while (!lcdConnected && (millis() < (lcdResetTimer + lcdResetTimeout)))
  {
    nextionHandleInput();
  }
  if (lcdConnected)
  {
    debugPrintln(F("HMI: Rebooting LCD completed"));
    if (nextionActivePage)
    {
      nextionSendCmd("page " + String(nextionActivePage));
    }
  }
  else
  {
    debugPrintln(F("ERROR: Rebooting LCD completed, but LCD is not responding."));
  }
  mqttClient.publish(mqttStatusTopic, "OFF");
}