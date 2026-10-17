#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "LedMatrix.h"
#include "ServoController.h"
#include "MgsTh50.h"
#include "MgsGuva.h"
#include "Bmp280.h"
#include "MgsL75.h"
#include "I2cScanner.h"

class GreenhouseSystem {
private:
    LedMatrix ledMatrix;
    ServoController servo;
    MgsTh50 th50;
    MgsGuva guva;
    Bmp280 bmp280;
    MgsL75 l75;

    bool bmpReady = false;
    bool l75Ready = false;
    bool guvaReady = false;

    unsigned long lastLoopTime = 0;

    void initSensors() {
        I2cScanner::FoundDevices devices = I2cScanner::scan();

        if (devices.bmp280) {
            bmpReady = bmp280.begin();
            if (bmpReady) {
                Serial.print("  BMP280 инициализирован по адресу 0x");
                Serial.println(devices.bmp280Addr, HEX);
            }
        }

        if (devices.mgsL75) l75Ready = l75.begin();

        if (devices.ads1015) guvaReady = guva.begin(devices.ads1015Addr);
        if (guvaReady) {
            Serial.println("  MGB-D1015 инициализирован");
            Serial.println("  Диапазон ADS1015: ±6.144 В (1 код = 3 мВ)");
        }

        th50.begin();
    }

    void runCalibration() {
        Serial.println("------------------------------------");
        Serial.println("РЕЖИМ КАЛИБРОВКИ (10 сек)");
        Serial.println("Датчик в ВОЗДУХЕ  -> запишите MOIST_RAW");
        Serial.println("Датчик в ВОДЕ     -> запишите MOIST_RAW");
        Serial.println("------------------------------------");

        ledMatrix.calibrationMode();

        unsigned long start = millis();
        while (millis() - start < CALIBRATION_TIME_MS) {
            int m = analogRead(runtimeSettings.pinMoisture);
            int t = analogRead(runtimeSettings.pinTemp50);
            Serial.print("СЫРОЕ_ВЛАЖН=");
            Serial.print(m);
            Serial.print("  СЫРОЕ_ТЕМП=");
            Serial.print(t);
            Serial.print("  Напряжение=");
            Serial.print((t / ADC_MAX) * ADC_VREF, 3);
            Serial.println(" В");
            delay(300);
        }

        ledMatrix.clear();
        Serial.println("------------------------------------");
        Serial.println("Калибровка завершена. Запуск основного цикла...");
        Serial.println("====================================");
        delay(500);
    }

    void readAllSensors() {
        if (bmp280.isReady()) bmp280.read();
        if (l75.isReady()) l75.read();
        
        th50.read();
        
        if (guva.isReady()) guva.read();
    }

    void updateActuators() {
        float temp = th50.getTemperature();
        
        servo.printStatus(temp);
        servo.update(temp);
        
        LedMatrix::TempColor color = ledMatrix.displayTemperature(temp);
        Serial.print("LED   - Цвет матрицы: ");
        Serial.println(ledMatrix.tempColorName(color));
        
        if (servo.isReady()) {
            ledMatrix.showServoState(servo.getState());
        }
    }

public:
    void begin() {
        Serial.begin(115200);
        delay(500);
        Serial.println("=== Система: датчики + серво 360° + LED матрица ===");
        Serial.println("MGS-THP80 (I2C) + MGS-TH50 (аналог) + MGS-GUVA");

        // LED Matrix
        Serial.println("------------------------------------");
        Serial.println("Инициализация LED матрицы 8x8...");
        ledMatrix.begin();
        ledMatrix.startupAnimation();
        Serial.println("  LED матрица готова (пин 18)");

        // I2C
        Wire.begin(runtimeSettings.i2cSda, runtimeSettings.i2cScl);
        delay(100);

        // Servo
        Serial.println("------------------------------------");
        Serial.println("Инициализация сервопривода 360°...");
        if (servo.begin()) {
            Serial.print("  Сервопривод подключён на пине ");
            Serial.print(runtimeSettings.pinServo);
            Serial.println(")");
            Serial.print("  Текущее значение СТОП: ");
            Serial.println(SERVO_STOP);
            Serial.println("  (Если серва медленно ползёт, измените SERVO_STOP в config.h на 88, 89, 91 или 92)");
        } else {
            Serial.println("  Сервопривод НЕ подключён! (ошибка инициализации ШИМ)");
        }

        // Sensors
        Serial.println("------------------------------------");
        Serial.println("Инициализация I2C датчиков...");
        initSensors();

        // Calibration mode
        runCalibration();
    }

    void loop() {
        readAllSensors();
        updateActuators();
        
        Serial.println("------------------------------------");
        delay(LOOP_DELAY_MS);
    }
};