#pragma once

#include <Arduino.h>
#include <functional>
#include <ArduinoLog.h>

#define WIFI    // 2.4Ghz wifi access point
//#define LED     // Individually addressible LED strip
//#define KEYPAD    // Buttons

#define EEPROM_FACTORY_RESET 0       // Byte to be used for factory reset device fails to start or is rebooted within 1 sec 3 consequitive times
#define EEPROM_CONFIGURATION_START 1   // First EEPROM byte to be used for storing the configuration

#define FACTORY_RESET_CLEAR_TIMER_MS 2000   // Clear factory reset counter when elapsed, considered smooth boot

#ifdef ESP32
  #define DEVICE_NAME "ESP32ST"
#elif ESP8266
  #define DEVICE_NAME "ESP8266ST"
#endif

#ifdef WIFI
  #define WIFI_SSID DEVICE_NAME
  #define WIFI_PASS "password123"

  // If unable to connect, it will create a soft accesspoint
  #define WIFI_FALLBACK_SSID DEVICE_NAME // device chip id will be suffixed
  #define WIFI_FALLBACK_PASS "password123"

  #define NTP_SERVER "pool.ntp.org"
  #define NTP_GMT_OFFSET_SEC -25200
  #define NTP_DAYLIGHT_OFFSET_SEC 0

  // Web server
  #define WEB_SERVER_PORT 80
#endif

#ifdef LED
  #define LED_CHANGE_MODE_SEC   60
  #ifdef ESP32
    #define LED_PIN 12
  #elif ESP8266
    #define LED_PIN 2
  #endif
  #define LED_STRIP_SIZE 240  // 267 for RingLight, 240 for PingPong table light
  #define LED_BRIGHTNESS 0.1  // 0-1, 1-max brightness, make sure your LEDs are powered accordingly
  #define LED_TYPE WS2812B
  #define LED_COLOR_ORDER GRB
#endif

#define TEMP_UNIT_CELSIUS     0
#define TEMP_UNIT_FAHRENHEIT  1
//#define TEMP_SENSOR_DS18B20
//#define TEMP_SENSOR_BME280
//#define TEMP_SENSOR_DHT
#ifdef ESP32
  #define TEMP_SENSOR_PIN 0
#elif ESP8266
  #define TEMP_SENSOR_PIN D3
#endif
#ifdef TEMP_SENSOR_DHT
  #define TEMP_SENSOR_DHT_TYPE    DHT22
#endif

#define BATTERY_SENSOR  // ADC A0 using 0-3.3v voltage divider
#ifdef BATTERY_SENSOR
  #define BATTERY_SENSOR_ADC_PIN  A0
#endif

#define INTERNAL_LED_PIN LED_BUILTIN

#define DEEP_SLEEP_INTERVAL_SEC 300 // 5 min
#define BATTERY_VOLTS_DIVIDER 162.3 // 169.0 or 222.3 not sure why it flips
#define DEEP_SLEEP_MIN_AWAKE_MS 5000 // Minimum time to remain awake after smooth boot

#define MQTT_DATA_JSON  0
#define MQTT_DATA_SCALAR  1
#define MQTT_DATA_BOTH  2

struct configuration_t {

  #ifdef WIFI
    char wifiSsid[32];
    char wifiPassword[63];
    
    // ntp
    char ntpServer[128];
    long gmtOffset_sec;
    int daylightOffset_sec;

    // mqtt
    char mqttServer[128];
    uint16_t mqttPort;
    char mqttTopic[128];
    uint16_t mqttDataType; // 0-json(one message); 1-scalar(multiple messages); 2-both; 
  #endif

  #ifdef LED
    float ledBrightness;
    uint8_t ledMode;
    unsigned long ledDelayMs;
    unsigned long ledCycleModeMs;
    uint16_t ledStripSize;
  #endif

  char name[128];
  float battVoltsDivider;
  uint16_t deepSleepDurationSec; // 0 - deep sleep disabled, stay awake
  uint8_t tempUnit;

  char _loaded[7]; // used to check if EEPROM was correctly set
  
};

extern configuration_t configuration;

uint8_t EEPROM_initAndCheckFactoryReset();
void EEPROM_clearFactoryReset();

void EEPROM_saveConfig();
void EEPROM_loadConfig();
void EEPROM_wipe();