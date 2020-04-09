#include <persistance.h>

void persistance::read()
{ // Read saved config.json from SPIFFS
  hasp::debugPrintln(F("SPIFFS: mounting SPIFFS"));
  if (SPIFFS.begin())
  {
    if (SPIFFS.exists("/config.json"))
    { // File exists, reading and loading
      hasp::debugPrintln(F("SPIFFS: reading /config.json"));
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
          hasp::debugPrintln(String(F("SPIFFS: [ERROR] Failed to parse /config.json: ")) + String(jsonError.c_str()));
        }
        else
        {
          if (!configJson["mqttServer"].isNull())
          {
            strcpy(hasp::mqttServer, configJson["mqttServer"]);
          }
          if (!configJson["mqttPort"].isNull())
          {
            char *port = "";
            itoa(hasp::mqttPort, port, 10);
            strcpy(port, configJson["mqttPort"]);
          }
          if (!configJson["mqttUser"].isNull())
          {
            strcpy(hasp::mqttUser, configJson["mqttUser"]);
          }
          if (!configJson["mqttPassword"].isNull())
          {
            strcpy(hasp::mqttPassword, configJson["mqttPassword"]);
          }
          if (!configJson["haspNode"].isNull())
          {
            strcpy(hasp::node, configJson["haspNode"]);
          }
          if (!configJson["groupName"].isNull())
          {
            strcpy(hasp::groupName, configJson["groupName"]);
          }
          if (!configJson["configUser"].isNull())
          {
            strcpy(hasp::configUser, configJson["configUser"]);
          }
          if (!configJson["configPassword"].isNull())
          {
            strcpy(hasp::configPassword, configJson["configPassword"]);
          }
          if (!configJson["motionPinConfig"].isNull())
          {
            char *pin = "";
            itoa(hasp::motionPin, pin, 10);
            strcpy(pin, configJson["motionPinConfig"]);
          }
          if (!configJson["debugSerialEnabled"].isNull())
          {
            if (configJson["debugSerialEnabled"])
            {
              hasp::debugSerialEnabled = true;
            }
            else
            {
              hasp::debugSerialEnabled = false;
            }
          }
          if (!configJson["debugTelnetEnabled"].isNull())
          {
            if (configJson["debugTelnetEnabled"])
            {
              hasp::debugTelnetEnabled = true;
            }
            else
            {
              hasp::debugTelnetEnabled = false;
            }
          }
          if (!configJson["mdnsEnabled"].isNull())
          {
            if (configJson["mdnsEnabled"])
            {
              hasp::mdnsEnabled = true;
            }
            else
            {
              hasp::mdnsEnabled = false;
            }
          }
          if (!configJson["beepEnabled"].isNull())
          {
            if (configJson["beepEnabled"])
            {
              hasp::beepEnabled = true;
            }
            else
            {
              hasp::beepEnabled = false;
            }
          }
          String configJsonStr;
          serializeJson(configJson, configJsonStr);
          hasp::debugPrintln(String(F("SPIFFS: parsed json:")) + configJsonStr);
        }
      }
      else
      {
        hasp::debugPrintln(F("SPIFFS: [ERROR] Failed to read /config.json"));
      }
    }
    else
    {
      hasp::debugPrintln(F("SPIFFS: [WARNING] /config.json not found, will be created on first config save"));
    }
  }
  else
  {
    hasp::debugPrintln(F("SPIFFS: [ERROR] Failed to mount FS"));
  }
}

void persistance::saveCallback()
{ // Callback notifying us of the need to save config
  hasp::debugPrintln(F("SPIFFS: Configuration changed, flagging for save"));
  hasp::shouldSaveConfig = true;
}

void persistance::save()
{ // Save the custom parameters to config.json
  nextion::setAttr("p[0].b[1].txt", "\"Saving\\rconfig\"");
  hasp::debugPrintln(F("SPIFFS: Saving config"));
  DynamicJsonDocument jsonConfigValues(1024);
  jsonConfigValues["mqttServer"] = hasp::mqttServer;
  jsonConfigValues["mqttPort"] = hasp::mqttPort;
  jsonConfigValues["mqttUser"] = hasp::mqttUser;
  jsonConfigValues["mqttPassword"] = hasp::mqttPassword;
  jsonConfigValues["haspNode"] = hasp::node;
  jsonConfigValues["groupName"] = hasp::groupName;
  jsonConfigValues["configUser"] = hasp::configUser;
  jsonConfigValues["configPassword"] = hasp::configPassword;
  jsonConfigValues["motionPinConfig"] = hasp::motionPin;
  jsonConfigValues["debugSerialEnabled"] = hasp::debugSerialEnabled;
  jsonConfigValues["debugTelnetEnabled"] = hasp::debugTelnetEnabled;
  jsonConfigValues["mdnsEnabled"] = hasp::mdnsEnabled;
  jsonConfigValues["beepEnabled"] = hasp::beepEnabled;

  hasp::debugPrintln(String(F("SPIFFS: mqttServer = ")) + String(hasp::mqttServer));
  hasp::debugPrintln(String(F("SPIFFS: mqttPort = ")) + String(hasp::mqttPort));
  hasp::debugPrintln(String(F("SPIFFS: mqttUser = ")) + String(hasp::mqttUser));
  hasp::debugPrintln(String(F("SPIFFS: mqttPassword = ")) + String(hasp::mqttPassword));
  hasp::debugPrintln(String(F("SPIFFS: haspNode = ")) + String(hasp::node));
  hasp::debugPrintln(String(F("SPIFFS: groupName = ")) + String(hasp::groupName));
  hasp::debugPrintln(String(F("SPIFFS: configUser = ")) + String(hasp::configUser));
  hasp::debugPrintln(String(F("SPIFFS: configPassword = ")) + String(hasp::configPassword));
  hasp::debugPrintln(String(F("SPIFFS: motionPinConfig = ")) + String(hasp::motionPin));
  hasp::debugPrintln(String(F("SPIFFS: debugSerialEnabled = ")) + String(hasp::debugSerialEnabled));
  hasp::debugPrintln(String(F("SPIFFS: debugTelnetEnabled = ")) + String(hasp::debugTelnetEnabled));
  hasp::debugPrintln(String(F("SPIFFS: mdnsEnabled = ")) + String(hasp::mdnsEnabled));
  hasp::debugPrintln(String(F("SPIFFS: beepEnabled = ")) + String(hasp::beepEnabled));

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile)
  {
    hasp::debugPrintln(F("SPIFFS: Failed to open config file for writing"));
  }
  else
  {
    serializeJson(jsonConfigValues, configFile);
    configFile.close();
  }
  hasp::shouldSaveConfig = false;
}

void persistance::clearSaved()
{ // Clear out all local storage
  nextion::setAttr("dims", "100");
  nextion::sendCmd("page 0");
  nextion::setAttr("p[0].b[1].txt", "\"Resetting\\rsystem...\"");
  hasp::debugPrintln(F("RESET: Formatting SPIFFS"));
  SPIFFS.format();
  hasp::debugPrintln(F("RESET: Clearing WiFiManager settings..."));
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  EEPROM.begin(512);
  hasp::debugPrintln(F("Clearing EEPROM..."));
  for (uint16_t i = 0; i < EEPROM.length(); i++)
  {
    EEPROM.write(i, 0);
  }
  nextion::setAttr("p[0].b[1].txt", "\"Rebooting\\rsystem...\"");
  hasp::debugPrintln(F("RESET: Rebooting device"));
  hasp::reset();
}