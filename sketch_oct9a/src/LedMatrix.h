#pragma once

#include <Adafruit_NeoPixel.h>
#include "config.h"

class LedMatrix {
private:
    Adafruit_NeoPixel matrix;
    
    int xyToIndex(int x, int y) const {
        if (y % 2 == 0) {
            return y * MATRIX_WIDTH + x;
        } else {
            return y * MATRIX_WIDTH + (MATRIX_WIDTH - 1 - x);
        }
    }

public:
    LedMatrix() : matrix(NUM_LEDS, PIN_LED_MATRIX, NEO_GRB + NEO_KHZ800) {}

    void begin() {
        matrix.setPin(runtimeSettings.pinLedMatrix);
        matrix.begin();
        matrix.setBrightness(MATRIX_BRIGHTNESS);
        clear();
    }

    void setPixel(int x, int y, uint32_t color) {
        if (x >= 0 && x < MATRIX_WIDTH && y >= 0 && y < MATRIX_HEIGHT) {
            matrix.setPixelColor(xyToIndex(x, y), color);
        }
    }

    void clear() {
        matrix.clear();
        matrix.show();
    }

    void fill(uint32_t color) {
        for (int i = 0; i < NUM_LEDS; i++) {
            matrix.setPixelColor(i, color);
        }
        matrix.show();
    }

    void show() {
        matrix.show();
    }

    uint32_t Color(uint8_t r, uint8_t g, uint8_t b) {
        return matrix.Color(r, g, b);
    }

    enum TempColor {
        TEMP_COLD,    // < 25°C - Green
        TEMP_WARM,    // 25-30°C - Yellow
        TEMP_HOT      // >= 30°C - Red
    };

    TempColor displayTemperature(float temp) {
        uint8_t r, g, b;
        TempColor color;

        if (temp >= runtimeSettings.tempThreshold) {
            r = 255; g = 0; b = 0;
            color = TEMP_HOT;
        } else if (temp >= runtimeSettings.tempThreshold - 5) {
            r = 255; g = 255; b = 0;
            color = TEMP_WARM;
        } else {
            r = 0; g = 255; b = 0;
            color = TEMP_COLD;
        }

        fill(Color(r, g, b));
        return color;
    }

    const char* tempColorName(TempColor color) {
        switch (color) {
            case TEMP_HOT:   return "КРАСНЫЙ (>= 30°C)";
            case TEMP_WARM:  return "ЖЕЛТЫЙ (25-30°C)";
            case TEMP_COLD:  return "ЗЕЛЕНЫЙ (< 25°C)";
        }
        return "UNKNOWN";
    }

    void blink(uint32_t color, int times, int delayMs) {
        for (int i = 0; i < times; i++) {
            fill(color);
            delay(delayMs);
            clear();
            delay(delayMs);
        }
    }

    void showServoState(bool isOpen) {
        uint32_t color = isOpen ? Color(0, 0, 255) : Color(128, 0, 128);
        
        setPixel(0, 0, color);
        setPixel(7, 0, color);
        setPixel(0, 7, color);
        setPixel(7, 7, color);
        show();
    }

    void startupAnimation() {
        fill(Color(0, 255, 0)); delay(300);
        fill(Color(0, 0, 255)); delay(300);
        fill(Color(255, 255, 0)); delay(300);
        clear();
    }

    void calibrationMode() {
        fill(Color(255, 165, 0)); // Orange
    }
};