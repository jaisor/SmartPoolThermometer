#ifndef _SENSOR_PROVIDER_H
#define _SENSOR_PROVIDER_H

#include "Configuration.h"

class ISensorProvider {
public:
    virtual float getTemperature(bool *current) { if (current != NULL) { *current = false; } return 0; };
    virtual float getHumidity(bool *current) { if (current != NULL) { *current = false; } return 0; };
    virtual float getAltitude(bool *current) { if (current != NULL) { *current = false; } return 0; };
    virtual float getBatteryVoltage(bool *current) { if (current != NULL) { *current = false; } return 0; };
    virtual uint32_t getDeviceId() { return 0; };
    virtual unsigned long getUptime() { return 0; };
};

#endif