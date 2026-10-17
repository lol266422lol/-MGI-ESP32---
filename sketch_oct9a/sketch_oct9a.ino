#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>
#include <Adafruit_ADS1X15.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>

#include "src/config.h"
#include "src/LedMatrix.h"
#include "src/ServoController.h"
#include "src/RelayController.h"
#include "src/MgsTh50.h"
#include "src/MgsGuva.h"
#include "src/Bmp280.h"
#include "src/MgsL75.h"
#include "src/I2cScanner.h"
#include "src/WebConfig.h"
#include "src/SensorLogger.h"
#include "src/WebServer.h"

// ============================================================
// ГЛОБАЛЬНЫЕ ОБЪЕКТЫ
// ============================================================
RuntimeSettings runtimeSettings = {
    PIN_SERVO, PIN_RELAY_1, PIN_RELAY_2,
    PIN_MOISTURE, PIN_TEMP_50, PIN_LED_MATRIX,
    I2C_SDA, I2C_SCL,
    SERVO_OPEN_TIME_MS, SERVO_CLOSE_TIME_MS,
    TEMP_THRESHOLD
};

LedMatrix       ledMatrix;
ServoController servo;
RelayController relays;
MgsTh50         th50;        // Аналоговый датчик влажности/температуры почвы
MgsGuva         guva;        // УФ-индекс через ADS1015
Bmp280          bmp280;      // Давление/температура (I2C)
MgsL75          l75;         // Освещённость на общей I2C-шине GPIO21/22

GreenhouseWebServer* webServer = nullptr;

// ============================================================
// SETUP — ИНИЦИАЛИЗАЦИЯ ВСЕХ ПОДСИСТЕМ
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== ТЕПЛИЦА: мониторинг + автовентиляция + веб-сервер ===");

    int configuredI2cSda = I2C_SDA;
    int configuredI2cScl = I2C_SCL;
    Preferences settingsPrefs;
    if (settingsPrefs.begin("gh", true)) {
        runtimeSettings.pinServo = settingsPrefs.getInt("pinServo", PIN_SERVO);
        runtimeSettings.pinRelay1 = settingsPrefs.getInt("pinRelay1", PIN_RELAY_1);
        runtimeSettings.pinRelay2 = settingsPrefs.getInt("pinRelay2", PIN_RELAY_2);
        runtimeSettings.pinMoisture = settingsPrefs.getInt("pinMoisture", PIN_MOISTURE);
        runtimeSettings.pinTemp50 = settingsPrefs.getInt("pinTemp50", PIN_TEMP_50);
        runtimeSettings.pinLedMatrix = settingsPrefs.getInt("pinLedMatrix", PIN_LED_MATRIX);
        configuredI2cSda = settingsPrefs.getInt("i2cSda", I2C_SDA);
        configuredI2cScl = settingsPrefs.getInt("i2cScl", I2C_SCL);
        runtimeSettings.servoOpenTimeMs = settingsPrefs.getInt("servoOpenTimeMs", SERVO_OPEN_TIME_MS);
        runtimeSettings.servoCloseTimeMs = settingsPrefs.getInt("servoCloseMs", SERVO_CLOSE_TIME_MS);
        runtimeSettings.tempThreshold = settingsPrefs.getFloat("tempThreshold", TEMP_THRESHOLD);
        settingsPrefs.end();
    } else {
        Serial.println("Не удалось загрузить настройки; будут использованы значения по умолчанию.");
    }

    if (configuredI2cSda < 0 || configuredI2cSda > 33 ||
        configuredI2cScl < 0 || configuredI2cScl > 33 ||
        configuredI2cSda == configuredI2cScl) {
        Serial.println("Некорректные I2C-пины в настройках; используются GPIO 21 (SDA) и GPIO 22 (SCL).");
        runtimeSettings.i2cSda = I2C_SDA;
        runtimeSettings.i2cScl = I2C_SCL;
    } else {
        runtimeSettings.i2cSda = configuredI2cSda;
        runtimeSettings.i2cScl = configuredI2cScl;
    }

    // 1. LED матрица 8x8 — индикация состояния
    ledMatrix.begin();
    ledMatrix.startupAnimation();

    // 2. I2C шина
    Wire.setTimeOut(50);
    if (!Wire.begin(runtimeSettings.i2cSda, runtimeSettings.i2cScl)) {
        Serial.println("Не удалось запустить I2C на сохранённых пинах; повторная попытка на GPIO 21/22.");
        runtimeSettings.i2cSda = I2C_SDA;
        runtimeSettings.i2cScl = I2C_SCL;
        if (!Wire.begin(runtimeSettings.i2cSda, runtimeSettings.i2cScl)) {
            Serial.println("Ошибка: I2C не запустился и на GPIO 21/22; сканирование датчиков может быть недоступно.");
        }
    }
    delay(100);

    // 3. Сервопривод 360° — управление заслонкой
    servo.begin();

    // 4. Модуль двух реле
    relays.begin();

    // 5. Аналоговые входы (MGS-TH50)
    th50.begin();

    // 6. Сканирование и инициализация I2C датчиков
    initI2CSensors();

    // 7. Режим калибровки влажности (10 сек)
    runCalibration();

    // 8. Инициализация веб-сервера
    webServer = new GreenhouseWebServer(
        sensorLogger,
        ledMatrix,
        servo,
        th50,
        relays,
        guva,
        bmp280,
        l75
    );
    webServer->begin();
    
    Serial.println("=== СИСТЕМА ГОТОВА ===");
    Serial.print("Веб-интерфейс: http://");
    Serial.println(webServer->getIP());
}

// ============================================================
// LOOP — обслуживание веб-сервера и таймеров реле, датчики каждые 2 сек
// ============================================================
void loop() {
    // --- Веб-сервер и таймеры реле ---
    if (webServer) {
        webServer->loop();
    }
    relays.update();

    static uint32_t lastSensorRead = 0;
    const uint32_t now = millis();
    if (now - lastSensorRead < LOOP_DELAY_MS) {
        delay(10);
        return;
    }
    lastSensorRead = now;

    // --- Чтение и логирование данных датчиков ---
    readAllSensors();
    sensorLogger.log(th50, l75, bmp280, guva, servo, ledMatrix);

    // --- Автоматика: управление заслонкой по температуре ---
    float temp = th50.getTemperature();
    if (webServer && webServer->isServoAutoMode()) {
        controlVentilation(temp);
    }

    // --- Индикация на LED матрице (только если не установлен пользовательский цвет) ---
    updateDisplay(temp);

    Serial.println("------------------------------------");
}

// ============================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ (логика приложения)
// ============================================================

void initI2CSensors() {
    Serial.println("--- Инициализация I2C датчиков ---");
    auto devices = I2cScanner::scan();

    if (devices.bmp280) {
        Serial.printf("BMP280 инициализация: %s\n", bmp280.begin() ? "успешно" : "ошибка");
    }
    if (devices.mgsL75 && !l75.begin()) {
        Serial.println("MGS-L75 найден на шине, но инициализация не удалась.");
    }
    if (devices.ads1015 && !guva.begin(devices.ads1015Addr)) {
        Serial.println("ADS1015 найден на шине, но инициализация MGS-GUVA не удалась.");
    }
}

void runCalibration() {
    Serial.println("--- КАЛИБРОВКА MGS-TH50 (10 сек) ---");
    ledMatrix.calibrationMode();

    unsigned long start = millis();
    while (millis() - start < CALIBRATION_TIME_MS) {
        Serial.print("Влажность: "); Serial.print(analogRead(runtimeSettings.pinMoisture));
        Serial.print("  Темп: "); Serial.print(analogRead(runtimeSettings.pinTemp50));
        Serial.println();
        delay(300);
    }
    ledMatrix.clear();
    Serial.println("Калибровка завершена.\n");
}

void readAllSensors() {
    if (bmp280.isReady()) bmp280.read();
    if (l75.isReady()) l75.read();
    th50.read();
    if (guva.isReady()) guva.read();
}

void controlVentilation(float temperature) {
    if (!servo.isReady()) {
        Serial.println("Автовентиляция недоступна: сервопривод не инициализирован.");
        return;
    }
    if (isnan(temperature)) {
        Serial.println("Автовентиляция пропущена: TH50 не выдал корректную температуру.");
        return;
    }

    servo.printStatus(temperature);
    if (temperature >= runtimeSettings.tempThreshold) {
        servo.open();   // Открыть заслонку
    } else {
        servo.close();  // Закрыть заслонку
    }
}

void updateDisplay(float temperature) {
    // Цвет матрицы по температуре (только в автоматическом режиме)
    if (webServer && !webServer->isCustomColorActive() && !isnan(temperature)) {
        ledMatrix.displayTemperature(temperature);
    }
    
    // Углы матрицы — состояние заслонки
    if (servo.isReady()) {
        ledMatrix.showServoState(servo.getState());
    }
}