#ifndef MOTION_H
#define MOTION_H

#include <Arduino.h>
#include <config.h>
#include <hasp.h>

#define LATCH_TIMEOUT 30000 // Latch time for motion sensor
#define BUFFER_TIMEOUT 1000 // Latch time for motion sensor

class motion
{
public:
    static bool enabled; // Motion sensor is enabled
    static bool active;  // Motion is being detected

    static void setup();
    static void update();

private:
    motion(){};
};

#endif
