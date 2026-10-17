#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "Sensor.h"
#include "config.h"

class MgsL75 : public Sensor {
private:
    bool ready = false;
    uint8_t address = ADDR_MGS_L75_PRIMARY;
    float lastIlluminance = NAN;

public:
    bool begin() override {
        ready = false;
        lastIlluminance = NAN;
        const uint8_t addresses[] = { ADDR_MGS_L75_PRIMARY, ADDR_MGS_L75_SECONDARY };
        for (uint8_t candidate : addresses) {
            Wire.beginTransmission(candidate);
            if (Wire.endTransmission() != 0) continue;

            address = candidate;
            const uint8_t commands[] = {
                0x01, // Power on
                0x07, // Reset data register
                0x10  // Continuous high-resolution mode
            };
            bool initialized = true;
            for (uint8_t command : commands) {
                Wire.beginTransmission(address);
                Wire.write(command);
                if (Wire.endTransmission() != 0) {
                    initialized = false;
                    break;
                }
            }
            if (!initialized) continue;

            delay(180);
            ready = true;
            Serial.printf("MGS-L75 (BH1750FVI) найден по I2C-адресу 0x%02X\n", address);
            return true;
        }

        Serial.println("MGS-L75 не отвечает по адресам 0x23 и 0x5C.");
        return false;
    }

    void read() override {
        if (!ready) return;

        const uint8_t bytesRead = Wire.requestFrom(address, static_cast<uint8_t>(2));
        if (bytesRead != 2) {
            Serial.println("MGS-L75: ошибка чтения");
            lastIlluminance = NAN;
            return;
        }

        const int high = Wire.read();
        const int low = Wire.read();
        if (high == -1 || low == -1) {
            lastIlluminance = NAN;
            return;
        }

        const uint16_t raw = (static_cast<uint16_t>(high) << 8) |
                             static_cast<uint16_t>(low);
        lastIlluminance = raw / 1.2f;
    }

    const char* getName() const override { return "MGS-L75 (BH1750FVI)"; }
    bool isReady() const override { return ready; }
    float getIlluminance() const { return lastIlluminance; }
    uint8_t getAddress() const { return address; }
};
