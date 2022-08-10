#ifndef _DEVICE_MANAGER_H
#define _DEVICE_MANAGER_H

#include <functional>
#include "Configuration.h"
#include "wifi/SensorProvider.h"

#ifdef TEMP_SENSOR    
#ifdef TEMP_SENSOR_DS18B20
    #include <OneWire.h>
    #include <DS18B20.h>
#endif
#ifdef TEMP_SENSOR_BME280
    #include <Adafruit_Sensor.h>
    #include <Adafruit_BME280.h>
#endif
#endif

#define STALE_READING_AGE_MS 3000

class CDevice: public ISensorProvider {

public:
	CDevice();
    ~CDevice();
    void loop();

#ifdef TEMP_SENSOR_DS18B20
    virtual float getTemperature(bool *current);
#endif
#ifdef TEMP_SENSOR_BME280
    virtual float getHumidity(bool *current);
    virtual float getAltitude(bool *current);
#endif

private:
    unsigned long tMillisMin;

#ifdef TEMP_SENSOR
    unsigned long tMillisTemp;
    unsigned long tLastReading;
    bool sensorReady;
    
    float _temperature;
#ifdef TEMP_SENSOR_DS18B20
    DS18B20 *_ds18b20;
#endif
#ifdef TEMP_SENSOR_BME280
    float _humidity, _altitude;
    Adafruit_BME280 *_bme;
#endif
#endif

};

#endif