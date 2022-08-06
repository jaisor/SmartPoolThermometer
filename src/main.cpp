#include <Arduino.h>
#include "RTC_SAMD21.h" //install seeed_arduino_RTC : https://github.com/Seeed-Studio/Seeed_Arduino_RTC
#include "DateTime.h"
#include <EnergySaving.h>
#include <OneWire.h>
#include <DS18B20.h>


#define PIN_LED 13
#define ONE_WIRE_BUS 2

EnergySaving pwrSave;
RTC_SAMD21 rtc;
OneWire oneWire(ONE_WIRE_BUS);
DS18B20 tempSensor(&oneWire);

void dummyfunc() {}

void setup() {
    rtc.begin();
    Serial.begin(115200);  while (!Serial); delay(200);

    pinMode(PIN_LED,OUTPUT);
    digitalWrite(PIN_LED,HIGH);

    rtc.adjust(DateTime(2030, 4, 1, 8, 30, 0) );
    rtc.attachInterrupt(dummyfunc); 
    pwrSave.begin(WAKE_RTC_ALARM); 

    rtc.disableAlarm();
    DateTime timeNow = rtc.now();
    const uint16_t uiAlmNextSec = 10;
    DateTime timeAlarm = DateTime(timeNow.year(), timeNow.month(), timeNow.day(), timeNow.hour(), timeNow.minute(), timeNow.second() + uiAlmNextSec);
    rtc.setAlarm(timeAlarm);
    rtc.enableAlarm(rtc.MATCH_SS); 

    tempSensor.setConfig(DS18B20_CRC);
    tempSensor.begin();

    Serial.println(F("Initialized"));
}

void loop() {
    
    Serial.println(F("Woke up"));

    for(uint16_t uilp=0; uilp<3; uilp++) {
        digitalWrite(PIN_LED,HIGH);
        delay(1000);
        digitalWrite(PIN_LED,LOW);
        delay(1000);
    }
    digitalWrite(PIN_LED,HIGH);

    tempSensor.setResolution(12);
    tempSensor.requestTemperatures();
    while (!tempSensor.isConversionComplete()) {}
    Serial.print(tempSensor.getTempC());
    Serial.println(F("C"));
    
    Serial.println(F("Sleeping"));
    pwrSave.standby();
}