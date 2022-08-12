#include <Arduino.h>
#include <functional>
#include <ArduinoLog.h>

#if !( defined(ESP32) ) && !( defined(ESP8266) )
  #error This code is intended to run on ESP8266 platform! Please check your Tools->Board setting.
#endif

#include "wifi/WifiManager.h"
#include "Device.h"

CWifiManager *wifiManager;
CDevice *device;

unsigned long tsSmoothBoot;
bool smoothBoot;

void setup() {
    Serial.begin(115200);  while (!Serial); delay(200);
    randomSeed(analogRead(0));

    // Log.begin(LOG_LEVEL_VERBOSE, &Serial);
    Log.begin(LOG_LEVEL_NOTICE, &Serial);
    Log.noticeln("Initializing...");  

    pinMode(INTERNAL_LED_PIN, OUTPUT);
    digitalWrite(INTERNAL_LED_PIN, LOW);

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

    /*
    rtc.adjust(DateTime(2030, 4, 1, 8, 30, 0) );
    rtc.attachInterrupt(dummyfunc); 
    pwrSave.begin(WAKE_RTC_ALARM); 

    rtc.disableAlarm();
    DateTime timeNow = rtc.now();
    const uint16_t uiAlmNextSec = 10;
    DateTime timeAlarm = DateTime(timeNow.year(), timeNow.month(), timeNow.day(), timeNow.hour(), timeNow.minute(), timeNow.second() + uiAlmNextSec);
    rtc.setAlarm(timeAlarm);
    rtc.enableAlarm(rtc.MATCH_SS); 
    */

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
    if (smoothBoot && wifiManager->isJobDone()) {
        Log.noticeln("Ready to sleep!");
        delay(300);
        digitalWrite(INTERNAL_LED_PIN, HIGH);
        ESP.deepSleep(60e6); 
    }
    
    delay(100);
    yield();
}