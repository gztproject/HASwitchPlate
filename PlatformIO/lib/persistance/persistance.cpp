#include <persistance.h>

persistance::persistance()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////
void configRead()
{ // Read saved config.json from SPIFFS
  debugPrintln(F("SPIFFS: mounting SPIFFS"));
  if (SPIFFS.begin())
  {
    if (SPIFFS.exists("/config.json"))
    { // File exists, reading and loading
      debugPrintln(F("SPIFFS: reading /config.json"));
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        size_t configFileSize = configFile.size(); // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[configFileSize]);
        configFile.readBytes(buf.get(), configFileSize);

        DynamicJsonDocument configJson(1024);
        DeserializationError jsonError = deserializeJson(configJson, buf.get());
        if (jsonError)
        { // Couldn't parse the saved config
          debugPrintln(String(F("SPIFFS: [ERROR] Failed to parse /config.json: ")) + String(jsonError.c_str()));
        }
        else
        {
          if (!configJson["mqttServer"].isNull())
          {
            strcpy(mqttServer, configJson["mqttServer"]);
          }
          if (!configJson["mqttPort"].isNull())
          {
            strcpy(mqttPort, configJson["mqttPort"]);
          }
          if (!configJson["mqttUser"].isNull())
          {
            strcpy(mqttUser, configJson["mqttUser"]);
          }
          if (!configJson["mqttPassword"].isNull())
          {
            strcpy(mqttPassword, configJson["mqttPassword"]);
          }
          if (!configJson["haspNode"].isNull())
          {
            strcpy(haspNode, configJson["haspNode"]);
          }
          if (!configJson["groupName"].isNull())
          {
            strcpy(groupName, configJson["groupName"]);
          }
          if (!configJson["configUser"].isNull())
          {
            strcpy(configUser, configJson["configUser"]);
          }
          if (!configJson["configPassword"].isNull())
          {
            strcpy(configPassword, configJson["configPassword"]);
          }
          if (!configJson["motionPinConfig"].isNull())
          {
            strcpy(motionPinConfig, configJson["motionPinConfig"]);
          }
          if (!configJson["debugSerialEnabled"].isNull())
          {
            if (configJson["debugSerialEnabled"])
            {
              debugSerialEnabled = true;
            }
            else
            {
              debugSerialEnabled = false;
            }
          }
          if (!configJson["debugTelnetEnabled"].isNull())
          {
            if (configJson["debugTelnetEnabled"])
            {
              debugTelnetEnabled = true;
            }
            else
            {
              debugTelnetEnabled = false;
            }
          }
          if (!configJson["mdnsEnabled"].isNull())
          {
            if (configJson["mdnsEnabled"])
            {
              mdnsEnabled = true;
            }
            else
            {
              mdnsEnabled = false;
            }
          }
          if (!configJson["beepEnabled"].isNull())
          {
            if (configJson["beepEnabled"])
            {
              beepEnabled = true;
            }
            else
            {
              beepEnabled = false;
            }
          }
          String configJsonStr;
          serializeJson(configJson, configJsonStr);
          debugPrintln(String(F("SPIFFS: parsed json:")) + configJsonStr);
        }
      }
      else
      {
        debugPrintln(F("SPIFFS: [ERROR] Failed to read /config.json"));
      }
    }
    else
    {
      debugPrintln(F("SPIFFS: [WARNING] /config.json not found, will be created on first config save"));
    }
  }
  else
  {
    debugPrintln(F("SPIFFS: [ERROR] Failed to mount FS"));
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void configSaveCallback()
{ // Callback notifying us of the need to save config
  debugPrintln(F("SPIFFS: Configuration changed, flagging for save"));
  shouldSaveConfig = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void configSave()
{ // Save the custom parameters to config.json
  nextionSetAttr("p[0].b[1].txt", "\"Saving\\rconfig\"");
  debugPrintln(F("SPIFFS: Saving config"));
  DynamicJsonDocument jsonConfigValues(1024);
  jsonConfigValues["mqttServer"] = mqttServer;
  jsonConfigValues["mqttPort"] = mqttPort;
  jsonConfigValues["mqttUser"] = mqttUser;
  jsonConfigValues["mqttPassword"] = mqttPassword;
  jsonConfigValues["haspNode"] = haspNode;
  jsonConfigValues["groupName"] = groupName;
  jsonConfigValues["configUser"] = configUser;
  jsonConfigValues["configPassword"] = configPassword;
  jsonConfigValues["motionPinConfig"] = motionPinConfig;
  jsonConfigValues["debugSerialEnabled"] = debugSerialEnabled;
  jsonConfigValues["debugTelnetEnabled"] = debugTelnetEnabled;
  jsonConfigValues["mdnsEnabled"] = mdnsEnabled;
  jsonConfigValues["beepEnabled"] = beepEnabled;

  debugPrintln(String(F("SPIFFS: mqttServer = ")) + String(mqttServer));
  debugPrintln(String(F("SPIFFS: mqttPort = ")) + String(mqttPort));
  debugPrintln(String(F("SPIFFS: mqttUser = ")) + String(mqttUser));
  debugPrintln(String(F("SPIFFS: mqttPassword = ")) + String(mqttPassword));
  debugPrintln(String(F("SPIFFS: haspNode = ")) + String(haspNode));
  debugPrintln(String(F("SPIFFS: groupName = ")) + String(groupName));
  debugPrintln(String(F("SPIFFS: configUser = ")) + String(configUser));
  debugPrintln(String(F("SPIFFS: configPassword = ")) + String(configPassword));
  debugPrintln(String(F("SPIFFS: motionPinConfig = ")) + String(motionPinConfig));
  debugPrintln(String(F("SPIFFS: debugSerialEnabled = ")) + String(debugSerialEnabled));
  debugPrintln(String(F("SPIFFS: debugTelnetEnabled = ")) + String(debugTelnetEnabled));
  debugPrintln(String(F("SPIFFS: mdnsEnabled = ")) + String(mdnsEnabled));
  debugPrintln(String(F("SPIFFS: beepEnabled = ")) + String(beepEnabled));

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile)
  {
    debugPrintln(F("SPIFFS: Failed to open config file for writing"));
  }
  else
  {
    serializeJson(jsonConfigValues, configFile);
    configFile.close();
  }
  shouldSaveConfig = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void configClearSaved()
{ // Clear out all local storage
  nextionSetAttr("dims", "100");
  nextionSendCmd("page 0");
  nextionSetAttr("p[0].b[1].txt", "\"Resetting\\rsystem...\"");
  debugPrintln(F("RESET: Formatting SPIFFS"));
  SPIFFS.format();
  debugPrintln(F("RESET: Clearing WiFiManager settings..."));
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  EEPROM.begin(512);
  debugPrintln(F("Clearing EEPROM..."));
  for (uint16_t i = 0; i < EEPROM.length(); i++)
  {
    EEPROM.write(i, 0);
  }
  nextionSetAttr("p[0].b[1].txt", "\"Rebooting\\rsystem...\"");
  debugPrintln(F("RESET: Rebooting device"));
  espReset();
}