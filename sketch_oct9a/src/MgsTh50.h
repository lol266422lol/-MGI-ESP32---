#pragma once

#include "Sensor.h"
#include "config.h"

class MgsTh50 : public Sensor {
private:
    bool ready = false;
    float lastTemperature = 0.0f;
    float lastMoisture = 0.0f;

    int readAvg(uint8_t pin) {
        uint32_t sum = 0;
        for (uint8_t i = 0; i < ADC_SAMPLES; i++) {
            sum += analogRead(pin);
            delayMicroseconds(ADC_SAMPLE_DELAY_US);
        }
        return sum / ADC_SAMPLES;
    }

public:
    MgsTh50() = default;

    bool begin() override {
        analogReadResolution(12);
        analogSetPinAttenuation(runtimeSettings.pinMoisture, ADC_11db);
        analogSetPinAttenuation(runtimeSettings.pinTemp50, ADC_11db);
        ready = true;
        return true;
    }

    void read() override {
        if (!ready) return;

        int moistureRaw = readAvg(runtimeSettings.pinMoisture);
        int tempRaw = readAvg(runtimeSettings.pinTemp50);

        // Moisture calculation
        if (AIR_VALUE != WATER_VALUE) {
            lastMoisture = (float)(moistureRaw - AIR_VALUE) * 100.0f / (float)(WATER_VALUE - AIR_VALUE);
        } else {
            lastMoisture = 0;
        }
        lastMoisture = constrain(lastMoisture, 0.0f, 100.0f);

        // Temperature calculation
        float voltage = (tempRaw / ADC_MAX) * ADC_VREF;
        lastTemperature = (voltage - TEMP_V0) / TEMP_SLOPE;

        // Output
        Serial.print("TH50  - Влажность: ");
        Serial.print(lastMoisture, 1);
        Serial.print(" %\t");
        Serial.print("Температура: ");
        Serial.print(lastTemperature, 1);
        Serial.print(" C\t");
        Serial.print("(АЦП В=");
        Serial.print(moistureRaw);
        Serial.print(" Т=");
        Serial.print(tempRaw);
        Serial.print(", U=");
        Serial.print(voltage, 3);
        Serial.println(" В)");
    }

    const char* getName() const override { return "MGS-TH50"; }
    bool isReady() const override { return ready; }

    float getTemperature() const { return lastTemperature; }
    float getMoisture() const { return lastMoisture; }
};