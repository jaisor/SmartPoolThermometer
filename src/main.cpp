#include <Arduino.h>
#include <functional>
#include <ArduinoLog.h>

#if !( defined(ESP32) ) && !( defined(ESP8266) )
  #error This code is intended to run on ESP8266 platform! Please check your Tools->Board setting.
#endif

#include <OneWire.h>
#include <DS18B20.h>

#include "wifi/WifiManager.h"

#define PIN_LED 13
#define ONE_WIRE_BUS 2

CWifiManager *wifiManager;

unsigned long tsSmoothBoot;
bool smoothBoot;

//EnergySaving pwrSave;
//RTC_SAMD21 rtc;
OneWire oneWire(ONE_WIRE_BUS);
DS18B20 tempSensor(&oneWire);

void dummyfunc() {}

void setup() {
    Serial.begin(115200);  while (!Serial); delay(200);
    randomSeed(analogRead(0));

    Log.begin(LOG_LEVEL_VERBOSE, &Serial);
    Log.noticeln("Initializing...");  

#ifdef LED_PIN_BOARD
    pinMode(LED_PIN_BOARD, OUTPUT);
    digitalWrite(LED_PIN_BOARD, HIGH);
#endif

    if (EEPROM_initAndCheckFactoryReset() >= 3) {
        Log.warningln("Factory reset conditions met!");
        EEPROM_wipe();    
    }

    tsSmoothBoot = millis();
    smoothBoot = false;

    EEPROM_loadConfig();

    wifiManager = new CWifiManager();
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

    tempSensor.setConfig(DS18B20_CRC);
    tempSensor.begin();
    tempSensor.setResolution(12);
    tempSensor.requestTemperatures();

    Log.infoln("Initialized");
}

void loop() {

    static unsigned long tsMillis = millis();
    static bool ledOn = false;

    if (!smoothBoot && millis() - tsSmoothBoot > FACTORY_RESET_CLEAR_TIMER_MS) {
        smoothBoot = true;
        EEPROM_clearFactoryReset();
        Log.noticeln("Device booted smoothly!");
    }

    wifiManager->loop();

    if (wifiManager->isRebootNeeded()) {
        return;
    }
    
    
    if (millis() - tsMillis > 1000) {
        tsMillis = millis();
        ledOn = !ledOn;
        digitalWrite(PIN_LED, ledOn ? HIGH : LOW);
    }

    
    if (tempSensor.isConversionComplete()) {
        float t = tempSensor.getTempC();
        Log.infoln("Temp: %FC %FF", t, t*1.8+32);
        tempSensor.setResolution(12);
        tempSensor.requestTemperatures();
    }
    
    delay(1000);
}