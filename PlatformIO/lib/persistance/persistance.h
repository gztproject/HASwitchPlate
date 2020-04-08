#ifndef PERSISTANCE_H
#define PERSISTANCE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <FS.h>
#include <hasp.h>

class persistance
{   public:
        static void saveCallback();
        static void save();
        static void read();
        static void clearSaved();
    private:
        persistance(){};
};



#endif