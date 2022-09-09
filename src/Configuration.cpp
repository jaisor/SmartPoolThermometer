#include <Arduino.h>
#include <EEPROM.h>
#include "Configuration.h"

configuration_t configuration;

uint8_t EEPROM_initAndCheckFactoryReset() {
  Log.noticeln("Configuration size: %i", sizeof(configuration_t));
  
  EEPROM.begin(sizeof(configuration_t) + EEPROM_FACTORY_RESET + 1);
  uint8_t resetCounter = EEPROM.read(EEPROM_FACTORY_RESET);

  Log.noticeln("Factory reset counter: %i", resetCounter);
  Log.noticeln("EEPROM length: %i", EEPROM.length());

  // Bump reset counter
  EEPROM.write(EEPROM_FACTORY_RESET, resetCounter + 1);
  EEPROM.commit();

  return resetCounter;
}

void EEPROM_clearFactoryReset() {
  EEPROM.write(EEPROM_FACTORY_RESET, 0);
  EEPROM.commit();
}

void EEPROM_saveConfig() {
  Log.infoln("Saving configuration to EEPROM");
  EEPROM.put(EEPROM_CONFIGURATION_START, configuration);
  EEPROM.commit();
}

void EEPROM_loadConfig() {

  configuration = {};
  EEPROM.get(EEPROM_CONFIGURATION_START, configuration);

  Log.noticeln("Configuration loaded: %s", configuration._loaded);

  if (strcmp(configuration._loaded, "jaisor")) {
    // blank
    Log.infoln("Blank configuration, loading defaults");
    strcpy(configuration._loaded, "jaisor");
    strcpy(configuration.name, DEVICE_NAME);
    #ifdef LED
      configuration.ledBrightness = LED_BRIGHTNESS;
      strcpy(configuration.ntpServer, NTP_SERVER);
      configuration.gmtOffset_sec = NTP_GMT_OFFSET_SEC;
      configuration.daylightOffset_sec = NTP_DAYLIGHT_OFFSET_SEC;
      configuration.ledMode = 0;
      configuration.ledCycleModeMs = LED_CHANGE_MODE_SEC * 1000;
      configuration.ledDelayMs = 10;
      configuration.ledBrightness = LED_BRIGHTNESS;
      configuration.ledStripSize = LED_STRIP_SIZE;
    #endif
    #ifdef WIFI
      strcpy(configuration.ntpServer, NTP_SERVER);
      configuration.gmtOffset_sec = NTP_GMT_OFFSET_SEC;
      configuration.daylightOffset_sec = NTP_DAYLIGHT_OFFSET_SEC;
      strcpy(configuration.mqttServer, "");
      configuration.mqttPort = 1883;
      strcpy(configuration.mqttTopic, "");
      configuration.mqttDataType = MQTT_DATA_JSON;
    #endif

    configuration.battVoltsDivider = BATTERY_VOLTS_DIVIDER;
    configuration.deepSleepDurationSec = DEEP_SLEEP_INTERVAL_SEC;
    configuration.tempUnit = TEMP_UNIT_CELSIUS;

    EEPROM_saveConfig();
  }

#ifdef LED
  if (isnan(configuration.ledBrightness)) {
    Log.verboseln("NaN brightness");
    configuration.ledBrightness = LED_BRIGHTNESS;
  }
  if (isnan(configuration.ledMode)) {
    Log.verboseln("NaN ledMode");
    configuration.ledMode = 0;
  }
  if (isnan(configuration.ledCycleModeMs)) {
    Log.verboseln("NaN ledCycleModeMs");
    configuration.ledCycleModeMs = 0;
  }
  if (isnan(configuration.ledDelayMs)) {
    Log.verboseln("NaN ledDelayMs");
    configuration.ledDelayMs = 10;
  }
  if (isnan(configuration.ledStripSize)) {
    Log.verboseln("NaN ledStripSize");
    configuration.ledStripSize = LED_STRIP_SIZE;
  }
#endif

#ifdef WIFI
  String wifiStr = String(configuration.wifiSsid);
  for (auto i : wifiStr) {
    if (!isAscii(i)) {
      Log.verboseln("Bad SSID, loading default: %s", wifiStr.c_str());
      strcpy(configuration.wifiSsid, "");
      break;
    }
  }
#endif

  Log.noticeln("Device name: %s", configuration.name);

  // FIXME: Always default NTP values
  strcpy(configuration.ntpServer, NTP_SERVER);
  configuration.gmtOffset_sec = NTP_GMT_OFFSET_SEC;
  configuration.daylightOffset_sec = NTP_DAYLIGHT_OFFSET_SEC;
}

void EEPROM_wipe() {
  Log.warningln("Wiping configuration with size %i!", EEPROM.length());
  for (uint16_t i = 0; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
}