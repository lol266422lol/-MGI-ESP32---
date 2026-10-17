#pragma once

#include <Arduino.h>
#include "WebConfig.h"
#include "config.h"
#include "MgsTh50.h"
#include "MgsL75.h"
#include "Bmp280.h"
#include "MgsGuva.h"
#include "ServoController.h"
#include "LedMatrix.h"

struct SensorReading {
    unsigned long timestamp;
    
    // MGS-TH50
    float th50_temp = NAN;
    float th50_moisture = NAN;
    
    // MGS-L75
    float l75_illuminance = NAN;
    
    // BMP280
    float bmp_temp = NAN;
    float bmp_pressure = NAN;
    
    // MGS-GUVA
    float guva_uvi = NAN;
    
    // Actuators
    bool servo_open = false;
    uint8_t matrix_r = 0;
    uint8_t matrix_g = 0;
    uint8_t matrix_b = 0;
};

class SensorLogger {
private:
    SensorReading latest;
    bool hasLatest = false;
    unsigned long lastLogTime = 0;
    
    float readSensorSafe(float (*getter)(), bool ready) {
        return ready ? getter() : NAN;
    }
    
    template<typename T>
    float readSensorSafe(float (T::*getter)() const, const T* sensor, bool ready) {
        return (ready && sensor) ? (sensor->*getter)() : NAN;
    }

public:
    SensorLogger() = default;
    
    void log(const MgsTh50& th50, const MgsL75& l75, const Bmp280& bmp,
             const MgsGuva& guva, const ServoController& servo,
             const LedMatrix& matrix) {
        
        unsigned long now = millis();
        if (now - lastLogTime < LOG_INTERVAL_MS) return;
        lastLogTime = now;
        
        SensorReading reading;
        reading.timestamp = now;
        
        reading.th50_temp = readSensorSafe(&MgsTh50::getTemperature, &th50, th50.isReady());
        reading.th50_moisture = readSensorSafe(&MgsTh50::getMoisture, &th50, th50.isReady());
        
        reading.l75_illuminance = readSensorSafe(&MgsL75::getIlluminance, &l75, l75.isReady());
        
        reading.bmp_temp = readSensorSafe(&Bmp280::getTemperature, &bmp, bmp.isReady());
        reading.bmp_pressure = readSensorSafe(&Bmp280::getPressure, &bmp, bmp.isReady());
        
        reading.guva_uvi = readSensorSafe(&MgsGuva::getUvIndex, &guva, guva.isReady());
        
        reading.servo_open = servo.isReady() ? servo.getState() : false;
        
        // Get current matrix color (simplified - we track last set color)
        // For now, infer from temperature
        float temp = reading.th50_temp;
        if (!isnan(temp)) {
            if (temp >= runtimeSettings.tempThreshold) {
                reading.matrix_r = 255; reading.matrix_g = 0; reading.matrix_b = 0;
            } else if (temp >= runtimeSettings.tempThreshold - 5) {
                reading.matrix_r = 255; reading.matrix_g = 255; reading.matrix_b = 0;
            } else {
                reading.matrix_r = 0; reading.matrix_g = 255; reading.matrix_b = 0;
            }
        }
        
        latest = reading;
        hasLatest = true;
    }
    
    int getCount() const { return hasLatest ? 1 : 0; }
    
    // Get latest reading as JSON
    void getLatestJson(String& json) {
        if (!hasLatest) {
            json = "{}";
            return;
        }
        json = readingToJson(latest);
    }
    
    String readingToJson(const SensorReading& r) {
        String json = "{";
        json += "\"ts\":" + String(r.timestamp) + ",";
        json += "\"th50_temp\":" + String(isnan(r.th50_temp) ? "null" : String(r.th50_temp, 2)) + ",";
        json += "\"th50_moisture\":" + String(isnan(r.th50_moisture) ? "null" : String(r.th50_moisture, 1)) + ",";
        json += "\"l75_illuminance\":" + String(isnan(r.l75_illuminance) ? "null" : String(r.l75_illuminance, 1)) + ",";
        json += "\"bmp_temp\":" + String(isnan(r.bmp_temp) ? "null" : String(r.bmp_temp, 2)) + ",";
        json += "\"bmp_pressure\":" + String(isnan(r.bmp_pressure) ? "null" : String(r.bmp_pressure, 1)) + ",";
        json += "\"guva_uvi\":" + String(isnan(r.guva_uvi) ? "null" : String(r.guva_uvi, 2)) + ",";
        json += "\"servo_open\":" + String(r.servo_open ? "true" : "false") + ",";
        json += "\"matrix_r\":" + String(r.matrix_r) + ",";
        json += "\"matrix_g\":" + String(r.matrix_g) + ",";
        json += "\"matrix_b\":" + String(r.matrix_b);
        json += "}";
        return json;
    }
    
    void clear() {
        hasLatest = false;
    }
};

extern SensorLogger sensorLogger;
