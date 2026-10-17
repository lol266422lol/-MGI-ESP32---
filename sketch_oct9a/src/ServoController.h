#pragma once

#include <ESP32Servo.h>
#include "config.h"

class ServoController {
private:
    Servo servo;
    bool ready = false;
    bool isOpen = false;
    bool positionKnown = false;

public:
    ServoController() = default;

    bool begin() {
        servo.setPeriodHertz(50);
        int channel = servo.attach(runtimeSettings.pinServo, 1000, 2000);
        
        if (channel >= 0) {
            ready = true;
            positionKnown = false;
            servo.write(SERVO_STOP);
            delay(500);
            return true;
        }
        return false;
    }

    bool isReady() const { return ready; }
    bool getState() const { return isOpen; }

    void open() {
        if (!ready || (positionKnown && isOpen)) return;
        
        servo.write(SERVO_SPEED_FW);
        delay(runtimeSettings.servoOpenTimeMs);
        servo.write(SERVO_STOP);
        isOpen = true;
        positionKnown = true;
    }

    void close() {
        if (!ready || (positionKnown && !isOpen)) return;
        
        servo.write(SERVO_SPEED_BW);
        delay(runtimeSettings.servoCloseTimeMs);
        servo.write(SERVO_STOP);
        isOpen = false;
        positionKnown = true;
    }

    void stop() {
        if (ready) {
            servo.write(SERVO_STOP);
        }
    }

    void update(float temperature) {
        if (!ready) return;

        if (temperature >= runtimeSettings.tempThreshold) {
            open();
        } else {
            close();
        }
    }

    void printStatus(float temperature) {
        if (temperature >= runtimeSettings.tempThreshold) {
            if (!positionKnown || !isOpen) {
                Serial.print(">>> ТЕМПЕРАТУРА ");
                Serial.print(temperature, 1);
                Serial.print("°C >= ");
                Serial.print(runtimeSettings.tempThreshold, 1);
                Serial.println("°C. ОТКРЫВАЕМ заслонку...");
            }
        } else {
            if (!positionKnown || isOpen) {
                Serial.print(">>> ТЕМПЕРАТУРА ");
                Serial.print(temperature, 1);
                Serial.print("°C < ");
                Serial.print(runtimeSettings.tempThreshold, 1);
                Serial.println("°C. ЗАКРЫВАЕМ заслонку...");
            }
        }
    }
};