#pragma once

#include <Arduino.h>

// ============================================================
// PIN DEFINITIONS
// ============================================================
// I2C
#define I2C_SDA       21
#define I2C_SCL       22

// Analog pins for MGS-TH50
#define PIN_MOISTURE  32
#define PIN_TEMP_50   33

// LED Matrix (WS2812 8x8)
#define PIN_LED_MATRIX  18
#define NUM_LEDS        64
#define MATRIX_WIDTH    8
#define MATRIX_HEIGHT   8

// Servo 360°
#define PIN_SERVO           19
#define SERVO_STOP          90
#define SERVO_SPEED_FW      120
#define SERVO_SPEED_BW      50
#define SERVO_OPEN_TIME_MS  800
#define SERVO_CLOSE_TIME_MS 420

// MGR-2 relay control pins and active level (per manufacturer sample)
#define PIN_RELAY_1         13
#define PIN_RELAY_2         14
#define RELAY_ACTIVE_LEVEL  HIGH
#define RELAY_INACTIVE_LEVEL LOW

// Temperature threshold for servo control
#define TEMP_THRESHOLD      30.0f

// ============================================================
// MGS-TH50 CALIBRATION
// ============================================================
#define AIR_VALUE     2235
#define WATER_VALUE   1200
#define TEMP_V0       0.341f
#define TEMP_SLOPE    0.01f

// ============================================================
// ADC SETTINGS
// ============================================================
#define ADC_MAX       4095.0f
#define ADC_VREF      3.3f

// ============================================================
// MGB-D1015 (ADS1015) + MGS-GUVA
// ============================================================
#define ADS1015_V_PER_CODE   0.003f
#define GUVA_GAIN            4.3f
#define GUVA_V_PER_UVI       0.1f

// ============================================================
// I2C ADDRESSES
// ============================================================
#define ADDR_BMP280_PRIMARY   0x76
#define ADDR_BMP280_SECONDARY 0x77
#define ADDR_MGS_L75_PRIMARY  0x23
#define ADDR_MGS_L75_SECONDARY 0x5C
#define ADDR_ADS1015_MIN      0x48
#define ADDR_ADS1015_MAX      0x4B

// ============================================================
// TIMING
// ============================================================
#define LOOP_DELAY_MS         2000
#define CALIBRATION_TIME_MS   10000
#define AIR_VALUE             2235
#define WATER_VALUE           1200
#define TEMP_V0               0.341f
#define TEMP_SLOPE            0.01f
#define ADC_MAX               4095.0f
#define ADC_VREF              3.3f
#define ADC_SAMPLES           8
#define ADC_SAMPLE_DELAY_US   200
#define GUVA_SAMPLES          8
#define GUVA_SAMPLE_DELAY_US  200

// ============================================================
// LED MATRIX COLORS
// ============================================================
#define COLOR_RED       0xFF0000
#define COLOR_GREEN     0x00FF00
#define COLOR_BLUE      0x0000FF
#define COLOR_YELLOW    0xFFFF00
#define COLOR_ORANGE    0xFFA500
#define COLOR_PURPLE    0x800080
#define COLOR_WHITE     0xFFFFFF
#define COLOR_OFF       0x000000

#define MATRIX_BRIGHTNESS 50

struct RuntimeSettings {
    uint8_t pinServo;
    uint8_t pinRelay1;
    uint8_t pinRelay2;
    uint8_t pinMoisture;
    uint8_t pinTemp50;
    uint8_t pinLedMatrix;
    uint8_t i2cSda;
    uint8_t i2cScl;
    uint16_t servoOpenTimeMs;
    uint16_t servoCloseTimeMs;
    float tempThreshold;
};

extern RuntimeSettings runtimeSettings;