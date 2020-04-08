#ifndef MOTION_H
#define MOTION_H

class motion
{
public:
    motion(uint8_t motionPin);
    void setup();
    void update();

    bool enabled = false; // Motion sensor is enabled
    bool active = false;  // Motion is being detected

private:
    uint8_t pin = 0;                          // GPIO input pin for motion sensor if connected and enabled
    const unsigned long LatchTimeout = 30000; // Latch time for motion sensor
    const unsigned long BufferTimeout = 1000; // Latch time for motion sensor
};

#endif
