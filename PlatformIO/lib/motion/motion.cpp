#include <motion.h>

motion::motion(uint8_t motionPin)
{
    pin = motionPin;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void motion::setup()
{
  if (strcmp(motionPinConfig, "D0") == 0)
  {
    motionEnabled = true;
    motionPin = D0;
    pinMode(motionPin, INPUT);
  }
  else if (strcmp(motionPinConfig, "D1") == 0)
  {
    motionEnabled = true;
    motionPin = D1;
    pinMode(motionPin, INPUT);
  }
  else if (strcmp(motionPinConfig, "D2") == 0)
  {
    motionEnabled = true;
    motionPin = D2;
    pinMode(motionPin, INPUT);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void motion::update()
{
  static unsigned long motionLatchTimer = 0;         // Timer for motion sensor latch
  static unsigned long motionBufferTimer = millis(); // Timer for motion sensor buffer
  static bool motionActiveBuffer = motionActive;
  bool motionRead = digitalRead(motionPin);

  if (motionRead != motionActiveBuffer)
  { // if we've changed state
    motionBufferTimer = millis();
    motionActiveBuffer = motionRead;
  }
  else if (millis() > (motionBufferTimer + motionBufferTimeout))
  {
    if ((motionActiveBuffer && !motionActive) && (millis() > (motionLatchTimer + motionLatchTimeout)))
    {
      motionLatchTimer = millis();
      mqttClient.publish(mqttMotionStateTopic, "ON");
      motionActive = motionActiveBuffer;
      debugPrintln("MOTION: Active");
    }
    else if ((!motionActiveBuffer && motionActive) && (millis() > (motionLatchTimer + motionLatchTimeout)))
    {
      motionLatchTimer = millis();
      mqttClient.publish(mqttMotionStateTopic, "OFF");
      motionActive = motionActiveBuffer;
      debugPrintln("MOTION: Inactive");
    }
  }
}