#include <Arduino.h>
#include <functional>
#include <ArduinoLog.h>

#if !( defined(ESP32) ) && !( defined(ESP8266) )
  #error This code is intended to run on ESP8266 platform! Please check your Tools->Board setting.
#endif

#include "wifi/WifiManager.h"
#include "Device.h"

#ifdef ESP32
#elif ESP8266
    ADC_MODE(ADC_TOUT);
#endif

CWifiManager *wifiManager;
CDevice *device;

unsigned long tsSmoothBoot;
bool smoothBoot;

void setup() {
    Serial.begin(115200);  while (!Serial); delay(200);
    randomSeed(analogRead(0));

    //Log.begin(LOG_LEVEL_VERBOSE, &Serial);
    Log.begin(LOG_LEVEL_NOTICE, &Serial);
    Log.noticeln("Initializing...");  

    pinMode(INTERNAL_LED_PIN, OUTPUT);
    #ifdef ESP32
        digitalWrite(INTERNAL_LED_PIN, HIGH);
    #elif ESP8266
        digitalWrite(INTERNAL_LED_PIN, LOW);
    #endif
    

#ifdef LED_PIN_BOARD
    digitalWrite(LED_PIN_BOARD, HIGH);
#endif

    if (EEPROM_initAndCheckFactoryReset() >= 3) {
        Log.warningln("Factory reset conditions met!");
        EEPROM_wipe();    
    }

    tsSmoothBoot = millis();
    smoothBoot = false;

    EEPROM_loadConfig();

    device = new CDevice();
    wifiManager = new CWifiManager(device);

    Log.infoln("Initialized");
}

void loop() {

    static unsigned long tsMillis = millis();
    
    if (!smoothBoot && millis() - tsSmoothBoot > FACTORY_RESET_CLEAR_TIMER_MS) {
        smoothBoot = true;
        EEPROM_clearFactoryReset();
        Log.noticeln("Device booted smoothly!");
    }

    device->loop();
    wifiManager->loop();

    if (wifiManager->isRebootNeeded()) {
        return;
    }
    
    // Conditions for deep sleep:
    // - Smooth boot
    // - Wifi not in AP mode
    // - Succesfully submitted 1 sensor reading over MQTT
    if (smoothBoot && configuration.deepSleepDurationSec > 0 && wifiManager->isJobDone()) {
        delay(100);
        Log.noticeln("Initiating deep sleep for %u usec", configuration.deepSleepDurationSec );
        #ifdef ESP32
            digitalWrite(INTERNAL_LED_PIN, LOW);
            ESP.deepSleep((uint64_t)configuration.deepSleepDurationSec * 1e6);
        #elif ESP8266
            digitalWrite(INTERNAL_LED_PIN, HIGH);
            ESP.deepSleep((uint64_t)configuration.deepSleepDurationSec * 1e6); 
        #endif
    }

    if (configuration.deepSleepDurationSec > 0 && device->getUptime() > configuration.deepSleepDurationSec * 1000) {
        Log.noticeln("Device is not sleeping right, resetting to save battery");
        #ifdef ESP32
            ESP.restart();
        #elif ESP8266
            ESP.reset();
        #endif
    }
    
    delay(200);
    yield();
}