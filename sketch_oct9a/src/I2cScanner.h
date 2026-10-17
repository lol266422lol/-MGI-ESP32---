#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

class I2cScanner {
public:
    struct FoundDevices {
        bool bmp280 = false;
        bool mgsL75 = false;
        bool ads1015 = false;
        byte bmp280Addr = ADDR_BMP280_PRIMARY;
        byte ads1015Addr = ADDR_ADS1015_MIN;
    };

    static FoundDevices scan() {
        FoundDevices devices;
        const byte addresses[] = {
            ADDR_BMP280_PRIMARY, ADDR_BMP280_SECONDARY,
            ADDR_MGS_L75_PRIMARY, ADDR_MGS_L75_SECONDARY,
            ADDR_ADS1015_MIN, static_cast<byte>(ADDR_ADS1015_MIN + 1),
            static_cast<byte>(ADDR_ADS1015_MIN + 2), ADDR_ADS1015_MAX
        };
        Serial.println("Сканирование I2C-адресов...");

        for (byte i = 0; i < sizeof(addresses); i++) {
            const byte addr = addresses[i];
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                Serial.print("  Найден: 0x");
                if (addr < 16) Serial.print("0");
                Serial.println(addr, HEX);

                if ((addr == ADDR_BMP280_PRIMARY || addr == ADDR_BMP280_SECONDARY) && !devices.bmp280) {
                    devices.bmp280Addr = addr;
                    devices.bmp280 = true;
                    Serial.print("  BMP280 обнаружен по адресу 0x");
                    Serial.println(addr, HEX);
                }

                if ((addr == ADDR_MGS_L75_PRIMARY || addr == ADDR_MGS_L75_SECONDARY) &&
                    !devices.mgsL75) {
                    devices.mgsL75 = true;
                    Serial.print("  MGS-L75 обнаружен по адресу 0x");
                    Serial.println(addr, HEX);
                }

                if (addr >= ADDR_ADS1015_MIN && addr <= ADDR_ADS1015_MAX && !devices.ads1015) {
                    devices.ads1015 = true;
                    devices.ads1015Addr = addr;
                    Serial.print("  MGB-D1015 (ADS1015) обнаружен по адресу 0x");
                    Serial.println(addr, HEX);
                }

            }
        }

        if (!devices.bmp280)   Serial.println("  BMP280 НЕ найден!");
        if (!devices.mgsL75)   Serial.println("  MGS-L75 НЕ найден!");
        if (!devices.ads1015)  Serial.println("  MGB-D1015 НЕ найден!");

        return devices;
    }
};