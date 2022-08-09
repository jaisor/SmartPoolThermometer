#include <Arduino.h>
#include <functional>
#include <ArduinoLog.h>

#if !( defined(ESP32) ) && !( defined(ESP8266) )
  #error This code is intended to run on ESP8266 platform! Please check your Tools->Board setting.
#endif

#include <OneWire.h>
#include <DS18B20.h>


#define PIN_LED 13
#define ONE_WIRE_BUS 2

//EnergySaving pwrSave;
//RTC_SAMD21 rtc;
OneWire oneWire(ONE_WIRE_BUS);
DS18B20 tempSensor(&oneWire);

void dummyfunc() {}

void setup() {
    //rtc.begin();
    Serial.begin(115200);  while (!Serial); delay(200);
    randomSeed(analogRead(0));

    Log.begin(LOG_LEVEL_VERBOSE, &Serial);

    pinMode(PIN_LED,OUTPUT);
    digitalWrite(PIN_LED,HIGH);

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

    Log.infoln("Initialized");
    Serial.println(F("Initialized"));
}

void loop() {
    
    /*
    for(uint16_t uilp=0; uilp<3; uilp++) {
        digitalWrite(PIN_LED,HIGH);
        delay(1000);
        digitalWrite(PIN_LED,LOW);
        delay(1000);
    }
    digitalWrite(PIN_LED,HIGH);
    */

    tempSensor.setResolution(12);
    tempSensor.requestTemperatures();
    while (!tempSensor.isConversionComplete()) {}

    Log.infoln("Temp %0.2f", tempSensor.getTempC());
    
    //pwrSave.standby();
}