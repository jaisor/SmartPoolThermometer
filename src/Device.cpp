#include <Arduino.h>
#include <functional>
#include <ArduinoLog.h>

#include "Device.h"

#include <Wire.h>
#include <EEPROM.h>

OneWire oneWire(TEMP_SENSOR_PIN);

CDevice::CDevice() {

#ifdef TEMP_SENSOR
sensorReady = true;
tLastReading = 0;
#ifdef TEMP_SENSOR_DS18B20
    DeviceAddress da;
    _ds18b20 = new DS18B20(&oneWire);
    _ds18b20->setConfig(DS18B20_CRC);
    _ds18b20->begin();

    _ds18b20->getAddress(da);
    Log.notice("DS18B20 sensor at address: ");
    for (uint8_t i = 0; i < 8; i++) {
        if (da[i] < 16) Log.notice("o");
        Log.notice("%x", da[i]);
    }
    Log.noticeln("");
    
    _ds18b20->setResolution(12);
    _ds18b20->requestTemperatures();
#endif
#ifdef TEMP_SENSOR_BME280
    _bme = new Adafruit_BME280();
    if (!_bme->begin(BME_I2C_ID)) {
        Log.errorln("BME280 sensor initialiation failed with ID %x", BME_I2C_ID);
        sensorReady = false;
    }
#endif
#endif

    Log.infoln("Device initialized");
}

CDevice::~CDevice() { 
#ifdef TEMP_SENSOR
#ifdef TEMP_SENSOR_DS18B20
    delete _ds18b20;
#endif
#ifdef TEMP_SENSOR_BME280
    delete _bme;
#endif
#endif
    Log.noticeln("Device destroyed");
}

void CDevice::loop() {

    if (sensorReady && millis() - tMillisTemp > 1000) {
        if (millis() - tLastReading < STALE_READING_AGE_MS) {
            tMillisTemp = millis();
        }
        #ifdef TEMP_SENSOR
        #ifdef TEMP_SENSOR_DS18B20
            if (_ds18b20->isConversionComplete()) {
                _temperature = _ds18b20->getTempC();
                _ds18b20->setResolution(12);
                _ds18b20->requestTemperatures();
                tLastReading = millis();
                Log.verboseln("DS18B20 temp: %FC %FF", _temperature, _temperature*1.8+32);
            } else {
                Log.verboseln("DS18B20 conversion not complete");
            }
        #endif
        #ifdef TEMP_SENSOR_BME280
            _temperature = _bme->readTemperature();
            _humidity = _bme->readHumidity();
            _altitude = _bme->readAltitude();
            tLastReading = millis();
        #endif
        #endif
    }

}

#ifdef TEMP_SENSOR_DS18B20
float CDevice::getTemperature(bool *current) {
    if (current != NULL) { 
        *current = millis() - tLastReading < STALE_READING_AGE_MS; 
    }
    return _temperature;
}
#endif

float CDevice::getBatteryVoltage(bool *current) {  
    if (current != NULL) { *current = true; } 
    int v = analogRead(BATTERY_SENSOR_ADC_PIN);
    Log.verboseln("Battery voltage: %i", v);
    return (float)v/configuration.battVoltsDivider; 
}

#ifdef TEMP_SENSOR_BME280
float CDevice::getHumidity(bool *current) const;
float CDevice::getAltitude(bool *current) const;
#endif