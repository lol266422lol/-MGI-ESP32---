#pragma once

#include <Wire.h>
#include "Sensor.h"
#include "config.h"
#include <Adafruit_ADS1X15.h>

class MgsGuva : public Sensor {
private:
    Adafruit_ADS1015 ads;
    bool ready = false;
    float lastUvIndex = 0.0f;

public:
    MgsGuva() = default;

    bool begin() override {
        return begin(ADDR_ADS1015_MIN);
    }

    bool begin(uint8_t address) {
        ready = false;
        Wire.beginTransmission(address);
        if (Wire.endTransmission() != 0) {
            return false;
        }

        if (!ads.begin(address)) {
            return false;
        }
        ads.setGain(GAIN_TWOTHIRDS);
        ready = true;
        return true;
    }

    void read() override {
        if (!ready) return;

        int32_t sum = 0;
        for (uint8_t i = 0; i < GUVA_SAMPLES; i++) {
            sum += ads.readADC_SingleEnded(0);
            delayMicroseconds(GUVA_SAMPLE_DELAY_US);
        }
        int16_t rawUV = sum / GUVA_SAMPLES;

        float voltageOut = rawUV * ADS1015_V_PER_CODE;
        float voltageSensor = voltageOut / GUVA_GAIN;
        lastUvIndex = voltageSensor / GUVA_V_PER_UVI;
        lastUvIndex = constrain(lastUvIndex, 0.0f, 15.0f);

        Serial.print("GUVA  - УФ-индекс: ");
        Serial.print(lastUvIndex, 2);
        Serial.print("\t(Сырое: ");
        Serial.print(rawUV);
        Serial.print(", U_вых=");
        Serial.print(voltageOut, 3);
        Serial.print(" В, U_сенс=");
        Serial.print(voltageSensor, 3);
        Serial.println(" В)");
    }

    const char* getName() const override { return "MGS-GUVA"; }
    bool isReady() const override { return ready; }

    float getUvIndex() const { return lastUvIndex; }
};