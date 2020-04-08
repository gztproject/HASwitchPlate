#ifndef PERSISTANCE_H
#define PERSISTANCE_H

#include <EEPROM.h>

class persistance
{
    persistance();
};

void configSaveCallback();
void configSave();
void configRead();
void configClearSaved();

#endif