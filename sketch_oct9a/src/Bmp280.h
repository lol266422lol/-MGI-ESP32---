#pragma once

#include <Wire.h>
#include "Sensor.h"
#include "config.h"

class Bmp280 : public Sensor {
private:
    bool ready = false;
    byte address = ADDR_BMP280_PRIMARY;
    float lastTemperature = 0.0f;
    float lastPressure = 0.0f;

    // Calibration data
    int16_t dig_T1, dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;

    bool readCalibration() {
        Wire.beginTransmission(address);
        Wire.write(0x88);
        Wire.endTransmission();
        Wire.requestFrom(address, 24);

        if (Wire.available() < 24) return false;

        dig_T1 = Wire.read() | (Wire.read() << 8);
        dig_T2 = Wire.read() | (Wire.read() << 8);
        dig_T3 = Wire.read() | (Wire.read() << 8);
        dig_P1 = Wire.read() | (Wire.read() << 8);
        dig_P2 = Wire.read() | (Wire.read() << 8);
        dig_P3 = Wire.read() | (Wire.read() << 8);
        dig_P4 = Wire.read() | (Wire.read() << 8);
        dig_P5 = Wire.read() | (Wire.read() << 8);
        dig_P6 = Wire.read() | (Wire.read() << 8);
        dig_P7 = Wire.read() | (Wire.read() << 8);
        dig_P8 = Wire.read() | (Wire.read() << 8);
        dig_P9 = Wire.read() | (Wire.read() << 8);

        return true;
    }

    bool configure() {
        // Config: normal mode, osrs_t=1, osrs_p=1
        Wire.beginTransmission(address);
        Wire.write(0xF4);
        Wire.write(0x27);
        Wire.endTransmission();

        // Config: t_sb=0.5ms, filter=16
        Wire.beginTransmission(address);
        Wire.write(0xF5);
        Wire.write(0xA0);
        Wire.endTransmission();

        return true;
    }

public:
    Bmp280() = default;

    bool begin() override {
        ready = false;
        // Try primary address first
        address = ADDR_BMP280_PRIMARY;
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            if (readCalibration() && configure()) {
                ready = true;
                return true;
            }
        }

        // Try secondary address
        address = ADDR_BMP280_SECONDARY;
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            if (readCalibration() && configure()) {
                ready = true;
                return true;
            }
        }

        return false;
    }

    void read() override {
        if (!ready) return;

        Wire.beginTransmission(address);
        Wire.write(0xF7);
        Wire.endTransmission();
        Wire.requestFrom(address, 8);

        if (Wire.available() < 8) {
            Serial.println("BMP280: ошибка чтения");
            ready = false;
            return;
        }

        uint32_t p_raw = ((uint32_t)Wire.read() << 12) | ((uint32_t)Wire.read() << 4) | (Wire.read() >> 4);
        uint32_t t_raw = ((uint32_t)Wire.read() << 12) | ((uint32_t)Wire.read() << 4) | (Wire.read() >> 4);

        // Temperature compensation
        int32_t var1, var2, T_fine;
        int32_t adc_T = t_raw;

        var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
        var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
        T_fine = var1 + var2;
        lastTemperature = (T_fine * 5 + 128) >> 8;
        lastTemperature = lastTemperature / 100.0;

        // Pressure compensation
        int32_t var1p, var2p;
        int32_t adc_P = p_raw;

        var1p = (((int32_t)T_fine) >> 1) - (int32_t)64000;
        var2p = (((var1p >> 2) * (var1p >> 2)) >> 11) * ((int32_t)dig_P6);
        var2p = var2p + ((var1p * ((int32_t)dig_P5)) << 1);
        var2p = (var2p >> 2) + (((int32_t)dig_P4) << 16);
        var1p = (((dig_P3 * (((var1p >> 2) * (var1p >> 2)) >> 13)) >> 3) + ((((int32_t)dig_P2) * var1p) >> 1)) >> 18;
        var1p = ((((32768 + var1p)) * ((int32_t)dig_P1)) >> 15);

        if (var1p == 0) return;

        int32_t p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2p >> 12))) * 3125;
        if (p < 0x80000000) {
            p = (p << 1) / ((uint32_t)var1p);
        } else {
            p = (p / (uint32_t)var1p) * 2;
        }

        var1p = (((int32_t)dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
        var2p = (((int32_t)(p >> 2)) * ((int32_t)dig_P8)) >> 13;
        lastPressure = ((uint32_t)((int32_t)p + ((var1p + var2p + dig_P7) >> 4))) / 100.0;

        Serial.print("THP80 - BMP: Температура ");
        Serial.print(lastTemperature, 2);
        Serial.print(" C, Давление ");
        Serial.print(lastPressure, 2);
        Serial.println(" гПа");
    }

    const char* getName() const override { return "BMP280"; }
    bool isReady() const override { return ready; }
    byte getAddress() const { return address; }

    float getTemperature() const { return lastTemperature; }
    float getPressure() const { return lastPressure; }
};