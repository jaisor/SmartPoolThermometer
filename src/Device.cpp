#include <Arduino.h>
#include <functional>
#include <ArduinoLog.h>

#include "Device.h"

#include <Wire.h>
#include <EEPROM.h>

CDevice::CDevice() {

  tMillisUp = millis();
  tMillisTemp = millis();
  sensorReady = false;

  tLastReading = 0;
#ifdef TEMP_SENSOR_DS18B20
  oneWire = new OneWire(TEMP_SENSOR_PIN);
  DeviceAddress da;
  _ds18b20 = new DS18B20(oneWire);
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

  sensorReady = true;
  tMillisTemp = 0;
#endif
#ifdef TEMP_SENSOR_BME280
  _bme = new Adafruit_BME280();
  if (!_bme->begin(BME_I2C_ID)) {
    Log.errorln("BME280 sensor initialiation failed with ID %x", BME_I2C_ID);
    sensorReady = false;
  } else {
    sensorReady = true;
    tMillisTemp = 0;
  }
#endif
#ifdef TEMP_SENSOR_DHT
  _dht = new DHT_Unified(TEMP_SENSOR_PIN, TEMP_SENSOR_DHT_TYPE);
  _dht->begin();
  sensor_t sensor;
  _dht->temperature().getSensor(&sensor);
  Log.noticeln("DHT temperature sensor name(%s) v(%u) id(%u) range(%F - %F) res(%F)",
    sensor.name, sensor.version, sensor.sensor_id, 
    sensor.min_value, sensor.max_value, sensor.resolution);
  _dht->humidity().getSensor(&sensor);
  Log.noticeln("DHT humidity sensor name(%s) v(%u) id(%u) range(%F - %F) res(%F)",
    sensor.name, sensor.version, sensor.sensor_id, 
    sensor.min_value, sensor.max_value, sensor.resolution);
  minDelayMs = sensor.min_delay / 1000;
  Log.noticeln("DHT sensor min delay %i", minDelayMs);
#endif

  Log.infoln("Device initialized");
}

CDevice::~CDevice() { 
#ifdef TEMP_SENSOR_DS18B20
  delete _ds18b20;
#endif
#ifdef TEMP_SENSOR_BME280
  delete _bme;
#endif
#ifdef TEMP_SENSOR_DHT
  delete _dht;
#endif
  Log.noticeln("Device destroyed");
}

uint32_t CDevice::getDeviceId() {
    // Create AP using fallback and chip ID
  uint32_t chipId = 0;
  #ifdef ESP32
    for(int i=0; i<17; i=i+8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
  #elif ESP8266
    chipId = ESP.getChipId();
  #endif

  return chipId;
}

void CDevice::loop() {

  uint32_t delay = 1000;
  #ifdef TEMP_SENSOR_DHT
    delay = minDelayMs;
  #endif

  if (!sensorReady && millis() - tMillisTemp > delay) {
    sensorReady = true;
  }

  if (sensorReady && millis() - tMillisTemp > delay) {
    if (millis() - tLastReading < STALE_READING_AGE_MS) {
      tMillisTemp = millis();
    }
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
    #ifdef TEMP_SENSOR_DHT
      if (millis() - tLastReading > minDelayMs) {
        sensors_event_t event;
        bool goodRead = true;
        // temperature
        _dht->temperature().getEvent(&event);
        if (isnan(event.temperature)) {
          Log.warningln(F("Error reading DHT temperature!"));
          goodRead = false;
        } else {
          _temperature = event.temperature;
          Log.noticeln("DHT temp: %FC %FF", _temperature, _temperature*1.8+32);
        }
        // humidity
        _dht->humidity().getEvent(&event);
        if (isnan(event.relative_humidity)) {
          Log.warningln(F("Error reading DHT humidity!"));
          goodRead = false;
        }
        else {
          _humidity = event.relative_humidity;
          Log.noticeln("DHT humidity: %F%%", _humidity);
        }
        
        tLastReading = millis();
      }
    #endif
  }

}

#if defined(TEMP_SENSOR_DS18B20) || defined(TEMP_SENSOR_DHT)
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
float CDevice::getHumidity(bool *current);
float CDevice::getAltitude(bool *current);
#endif

#ifdef TEMP_SENSOR_DHT
float CDevice::getHumidity(bool *current) {
  if (current != NULL) { 
    *current = millis() - tLastReading < STALE_READING_AGE_MS; 
  }
  return _humidity;
}
#endif
