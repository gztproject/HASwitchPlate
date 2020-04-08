#include <motion.h>

bool motion::enabled = false;
bool motion::active = false;

void motion::setup()
{
  if (MOTION_PIN == 0)
  {
    enabled = false;
    return;
  }
  enabled = true;
  pinMode(MOTION_PIN, INPUT);
}

void motion::update()
{
  if (!enabled)
  {
    return;
  }
  static unsigned long motionLatchTimer = 0;         // Timer for motion sensor latch
  static unsigned long motionBufferTimer = millis(); // Timer for motion sensor buffer
  static bool motionActiveBuffer = active;
  bool motionRead = digitalRead(hasp::motionPin);

  if (motionRead != motionActiveBuffer)
  { // if we've changed state
    motionBufferTimer = millis();
    motionActiveBuffer = motionRead;
  }
  else if (millis() > (motionBufferTimer + BUFFER_TIMEOUT))
  {
    if ((motionActiveBuffer && !active) && (millis() > (motionLatchTimer + LATCH_TIMEOUT)))
    {
      motionLatchTimer = millis();
      mqttWrapper::getClient().publish(mqttWrapper::motionStateTopic, "ON");
      active = motionActiveBuffer;
      hasp::debugPrintln("MOTION: Active");
    }
    else if ((!motionActiveBuffer && active) && (millis() > (motionLatchTimer + LATCH_TIMEOUT)))
    {
      motionLatchTimer = millis();
      mqttWrapper::getClient().publish(mqttWrapper::motionStateTopic, "OFF");
      active = motionActiveBuffer;
      hasp::debugPrintln("MOTION: Inactive");
    }
  }
}