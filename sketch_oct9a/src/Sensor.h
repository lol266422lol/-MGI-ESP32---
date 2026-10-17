#pragma once

#include <Arduino.h>

class Sensor {
public:
    virtual ~Sensor() = default;
    virtual bool begin() = 0;
    virtual void read() = 0;
    virtual const char* getName() const = 0;
    virtual bool isReady() const = 0;
};