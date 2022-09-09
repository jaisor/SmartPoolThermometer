#pragma once

#include "Configuration.h"

#ifdef LED
  #include <FastLED.h>
#endif
#ifdef OLED
  #include <Adafruit_GFX.h>
#endif

class CBaseManager {

public:

#ifdef LED
  virtual uint16_t LED_Status(CRGB *leds) { return 0; };
#endif
#ifdef OLED
  virtual uint16_t OLED_Status(Adafruit_GFX *oled) { return 0; };
#endif
#ifdef KEYPAD
  virtual void keyEvent(key_status_t key) { };
#endif

  virtual void loop() {};
};
