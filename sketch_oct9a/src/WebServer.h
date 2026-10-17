#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <math.h>
#include <stdlib.h>
#include "WebConfig.h"
#include "SensorLogger.h"
#include "config.h"
#include "LedMatrix.h"
#include "ServoController.h"
#include "RelayController.h"
#include "MgsTh50.h"
#include "MgsGuva.h"
#include "Bmp280.h"
#include "MgsL75.h"
#include "WebPage.h"
#include "I2cScanner.h"

class GreenhouseWebServer {
private:
    WebServer server;
    SensorLogger& logger;
    
    // Hardware references
    LedMatrix& ledMatrix;
    ServoController& servo;
    RelayController& relays;
    MgsTh50& th50;
    MgsGuva& guva;
    Bmp280& bmp280;
    MgsL75& l75;
    
    bool apMode = false;
    
    // Settings storage (ESP32 Preferences / NVS)
    Preferences prefs;
    
    // Matrix color control
    uint8_t customMatrixR = 0;
    uint8_t customMatrixG = 255;
    uint8_t customMatrixB = 0;
    bool useCustomColor = false;
    
    // Servo control mode
    bool servoAutoMode = true;

    static bool readJsonNumber(const String& body, const char* key, double& value) {
        const String marker = String("\"") + key + "\"";
        const int keyIndex = body.indexOf(marker);
        if (keyIndex < 0) return false;

        const int colonIndex = body.indexOf(':', keyIndex + marker.length());
        if (colonIndex < 0) return false;

        int endIndex = body.indexOf(',', colonIndex + 1);
        if (endIndex < 0) endIndex = body.indexOf('}', colonIndex + 1);
        if (endIndex < 0) return false;

        String token = body.substring(colonIndex + 1, endIndex);
        token.trim();
        if (!token.length()) return false;

        char* parseEnd = nullptr;
        value = strtod(token.c_str(), &parseEnd);
        return parseEnd != token.c_str() && *parseEnd == '\0' && isfinite(value);
    }

    static bool readIntSetting(const String& body, const char* key, int minValue, int maxValue, int& value) {
        double parsed = 0;
        if (!readJsonNumber(body, key, parsed) || parsed < minValue || parsed > maxValue ||
            parsed != static_cast<int>(parsed)) {
            return false;
        }
        value = static_cast<int>(parsed);
        return true;
    }
    
    bool checkAuth() {
        if (!server.authenticate("root", "admin")) {
            server.send(403, "application/json", "{\"error\":\"Unauthorized\"}");
            return false;
        }
        return true;
    }
    
    void handleRoot() {
        server.send_P(200, "text/html", htmlPageContent);
    }
    
    void handleApiSensors() {
        if (!checkAuth()) return;
        String json;
        logger.getLatestJson(json);
        server.send(200, "application/json", json);
    }
    
    void handleApiMatrix() {
        if (!checkAuth()) return;
        if (server.method() == HTTP_GET) {
            String json = "{";
            json += "\"r\":" + String(customMatrixR) + ",";
            json += "\"g\":" + String(customMatrixG) + ",";
            json += "\"b\":" + String(customMatrixB) + ",";
            json += "\"custom\":" + String(useCustomColor ? "true" : "false");
            json += "}";
            server.send(200, "application/json", json);
        } else if (server.method() == HTTP_POST) {
            if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {
                customMatrixR = constrain(server.arg("r").toInt(), 0, 255);
                customMatrixG = constrain(server.arg("g").toInt(), 0, 255);
                customMatrixB = constrain(server.arg("b").toInt(), 0, 255);
                useCustomColor = true;
                ledMatrix.fill(ledMatrix.Color(customMatrixR, customMatrixG, customMatrixB));
                server.send(200, "application/json", "{\"status\":\"ok\"}");
            } else {
                server.send(400, "application/json", "{\"error\":\"Missing r,g,b parameters\"}");
            }
        } else if (server.method() == HTTP_DELETE) {
            // Reset to automatic temperature-based color
            useCustomColor = false;
            float temp = th50.getTemperature();
            if (!isnan(temp)) {
                ledMatrix.displayTemperature(temp);
            }
            server.send(200, "application/json", "{\"status\":\"reset to auto\"}");
        }
    }
    
    void handleApiServo() {
        if (!checkAuth()) return;
        if (server.method() == HTTP_GET) {
            String json = "{";
            json += "\"open\":" + String(servo.isReady() ? (servo.getState() ? "true" : "false") : "false") + ",";
            json += "\"ready\":" + String(servo.isReady() ? "true" : "false") + ",";
            json += "\"auto\":" + String(servoAutoMode ? "true" : "false");
            json += "}";
            server.send(200, "application/json", json);
        } else if (server.method() == HTTP_POST) {
            if (!servo.isReady()) {
                server.send(503, "application/json", "{\"error\":\"Servo not ready\"}");
                return;
            }
            
            if (server.hasArg("action")) {
                String action = server.arg("action");
                if (action == "open") {
                    servoAutoMode = false;
                    servo.open();
                    server.send(200, "application/json", "{\"status\":\"opened\"}");
                } else if (action == "close") {
                    servoAutoMode = false;
                    servo.close();
                    server.send(200, "application/json", "{\"status\":\"closed\"}");
                } else if (action == "stop") {
                    servoAutoMode = false;
                    servo.stop();
                    server.send(200, "application/json", "{\"status\":\"stopped\"}");
                } else if (action == "auto") {
                    servoAutoMode = true;
                    server.send(200, "application/json", "{\"status\":\"auto mode enabled\"}");
                } else {
                    server.send(400, "application/json", "{\"error\":\"Invalid action\"}");
                }
            } else {
                server.send(400, "application/json", "{\"error\":\"Missing action parameter\"}");
            }
        }
    }

    void handleApiRelays() {
        if (!checkAuth()) return;
        if (server.method() == HTTP_GET) {
            String json = "{";
            for (uint8_t relay = 1; relay <= 2; relay++) {
                if (relay > 1) json += ",";
                json += "\"relay" + String(relay) + "\":{";
                json += "\"on\":" + String(relays.isActive(relay) ? "true" : "false") + ",";
                json += "\"remaining_ms\":" + String(relays.remainingMs(relay));
                json += "}";
            }
            json += "}";
            server.send(200, "application/json", json);
            return;
        }

        if (!server.hasArg("relay") || !server.hasArg("action")) {
            server.send(400, "application/json", "{\"error\":\"Missing relay or action parameter\"}");
            return;
        }

        const String relayValue = server.arg("relay");
        if (relayValue != "1" && relayValue != "2") {
            server.send(400, "application/json", "{\"error\":\"Relay must be 1 or 2\"}");
            return;
        }

        const uint8_t relay = static_cast<uint8_t>(relayValue.toInt());
        const String action = server.arg("action");
        if (action == "off") {
            relays.deactivate(relay);
            server.send(200, "application/json", "{\"status\":\"off\"}");
            return;
        }

        if (action != "on" || !server.hasArg("seconds")) {
            server.send(400, "application/json", "{\"error\":\"Use action=on with seconds, or action=off\"}");
            return;
        }

        const String secondsValue = server.arg("seconds");
        for (size_t i = 0; i < secondsValue.length(); i++) {
            if (secondsValue[i] < '0' || secondsValue[i] > '9') {
                server.send(400, "application/json", "{\"error\":\"Duration must be an integer from 1 to 86400 seconds\"}");
                return;
            }
        }

        const long seconds = secondsValue.toInt();
        if (seconds < 1 || seconds > 86400) {
            server.send(400, "application/json", "{\"error\":\"Duration must be from 1 to 86400 seconds\"}");
            return;
        }

        relays.activate(relay, static_cast<uint32_t>(seconds) * 1000UL);
        server.send(200, "application/json", "{\"status\":\"on\",\"seconds\":" + String(seconds) + "}");
    }
    
    void handleApiStatus() {
        if (!checkAuth()) return;
        String json = "{";
        json += "\"wifi\":{";
        json += "\"mode\":\"" + String(apMode ? "AP" : "STA") + "\",";
        json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI());
        json += "},";
        json += "\"sensors\":{";
        json += "\"th50\":" + String(th50.isReady() ? "true" : "false") + ",";
        json += "\"mgsL75\":" + String(l75.isReady() ? "true" : "false") + ",";
        json += "\"bmp280\":" + String(bmp280.isReady() ? "true" : "false") + ",";
        json += "\"guva\":" + String(guva.isReady() ? "true" : "false");
        json += "},";
        json += "\"uptime\":" + String(millis() / 1000);
        json += "}";
        server.send(200, "application/json", json);
    }
    
    void handleApiSettings() {
        if (!checkAuth()) return;
        if (server.method() == HTTP_GET) {
            String json = "{";
            json += "\"pinServo\":" + String(prefs.getInt("pinServo", 19)) + ",";
            json += "\"pinRelay1\":" + String(prefs.getInt("pinRelay1", 13)) + ",";
            json += "\"pinRelay2\":" + String(prefs.getInt("pinRelay2", 14)) + ",";
            json += "\"pinMoisture\":" + String(prefs.getInt("pinMoisture", 32)) + ",";
            json += "\"pinTemp50\":" + String(prefs.getInt("pinTemp50", 33)) + ",";
            json += "\"pinLedMatrix\":" + String(prefs.getInt("pinLedMatrix", 18)) + ",";
            json += "\"i2cSda\":" + String(prefs.getInt("i2cSda", 21)) + ",";
            json += "\"i2cScl\":" + String(prefs.getInt("i2cScl", 22)) + ",";
            json += "\"servoOpenTimeMs\":" + String(prefs.getInt("servoOpenTimeMs", 800)) + ",";
            json += "\"servoCloseTimeMs\":" + String(prefs.getInt("servoCloseMs", 420)) + ",";
            json += "\"tempThreshold\":" + String(prefs.getFloat("tempThreshold", 30.0f));
            json += "}";
            server.send(200, "application/json", json);
        } else if (server.method() == HTTP_POST) {
            // Expect JSON body
            if (!server.hasArg("plain")) {
                server.send(400, "application/json", "{\"error\":\"Missing JSON body\"}");
                return;
            }
            String body = server.arg("plain");
            int pinServo, pinRelay1, pinRelay2, pinMoisture, pinTemp50;
            int pinLedMatrix, i2cSda, i2cScl, servoOpenTimeMs, servoCloseTimeMs;
            double tempThreshold;
            if (!readIntSetting(body, "pinServo", 0, 33, pinServo) ||
                !readIntSetting(body, "pinRelay1", 0, 33, pinRelay1) ||
                !readIntSetting(body, "pinRelay2", 0, 33, pinRelay2) ||
                !readIntSetting(body, "pinMoisture", 0, 39, pinMoisture) ||
                !readIntSetting(body, "pinTemp50", 0, 39, pinTemp50) ||
                !readIntSetting(body, "pinLedMatrix", 0, 33, pinLedMatrix) ||
                !readIntSetting(body, "i2cSda", 0, 33, i2cSda) ||
                !readIntSetting(body, "i2cScl", 0, 33, i2cScl) ||
                i2cSda == i2cScl ||
                !readIntSetting(body, "servoOpenTimeMs", 100, 10000, servoOpenTimeMs) ||
                !readIntSetting(body, "servoCloseTimeMs", 100, 10000, servoCloseTimeMs) ||
                !readJsonNumber(body, "tempThreshold", tempThreshold) ||
                tempThreshold < 0 || tempThreshold > 100) {
                server.send(400, "application/json", "{\"error\":\"Invalid or missing setting values\"}");
                return;
            }

            const char* failedSetting = nullptr;
            const auto saveInt = [this, &failedSetting](const char* key, int value) {
                if (prefs.putInt(key, value) > 0) return true;
                failedSetting = key;
                return false;
            };
            const bool saved =
                saveInt("pinServo", pinServo) &&
                saveInt("pinRelay1", pinRelay1) &&
                saveInt("pinRelay2", pinRelay2) &&
                saveInt("pinMoisture", pinMoisture) &&
                saveInt("pinTemp50", pinTemp50) &&
                saveInt("pinLedMatrix", pinLedMatrix) &&
                saveInt("i2cSda", i2cSda) &&
                saveInt("i2cScl", i2cScl) &&
                saveInt("servoOpenTimeMs", servoOpenTimeMs) &&
                saveInt("servoCloseMs", servoCloseTimeMs) &&
                prefs.putFloat("tempThreshold", static_cast<float>(tempThreshold)) > 0;
            if (!saved) {
                if (!failedSetting) failedSetting = "tempThreshold";
                Serial.printf("Не удалось сохранить настройку NVS: %s\n", failedSetting);
                server.send(500, "application/json",
                    String("{\"error\":\"Failed to store setting: ") + failedSetting + "\"}");
                return;
            }

            server.send(200, "application/json", "{\"status\":\"saved_restart\"}");
            delay(500);
            ESP.restart();
        } else {
            server.send(405, "application/json", "{\"error\":\"Method not allowed\"}");
        }
    }
    
    void handleApiSensorScan() {
        if (!checkAuth()) return;
        if (server.method() != HTTP_POST) {
            server.send(405, "application/json", "{\"error\":\"Method not allowed\"}");
            return;
        }
        
        String result = "{";
        result += "\"scan\":{";
        
        I2cScanner::FoundDevices devices = I2cScanner::scan();
        const bool bmpInit = bmp280.begin();
        const bool l75Init = l75.begin();
        const bool guvaInit = guva.begin(devices.ads1015 ? devices.ads1015Addr : ADDR_ADS1015_MIN);

        result += "\"attempts\":1,";
        result += "\"bmp280\":" + String(devices.bmp280 ? "true" : "false") + ",";
        result += "\"mgsL75\":" + String(devices.mgsL75 ? "true" : "false") + ",";
        result += "\"ads1015\":" + String(devices.ads1015 ? "true" : "false") + ",";
        result += "\"bmp280Addr\":" + String(devices.bmp280Addr) + ",";
        result += "\"bmpInit\":" + String(bmpInit ? "true" : "false") + ",";
        result += "\"l75Init\":" + String(l75Init ? "true" : "false") + ",";
        result += "\"guvaInit\":" + String(guvaInit ? "true" : "false");
        result += "}";
        result += "}";
        
        server.send(200, "application/json", result);
    }
    
    void handleNotFound() {
        String message = "File Not Found\n\n";
        message += "URI: " + server.uri() + "\n";
        message += "Method: " + String((server.method() == HTTP_GET) ? "GET" : "POST") + "\n";
        message += "Arguments: " + String(server.args()) + "\n";
        for (uint8_t i = 0; i < server.args(); i++) {
            message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
        }
        server.send(404, "text/plain", message);
    }
    
public:
    
    GreenhouseWebServer(
        SensorLogger& log,
        LedMatrix& matrix,
        ServoController& servoCtrl,
        MgsTh50& th50Sensor,
        RelayController& relayController,
        MgsGuva& guvaSensor,
        Bmp280& bmpSensor,
        MgsL75& l75Sensor
    ) : server(WEB_SERVER_PORT), logger(log),
        ledMatrix(matrix), servo(servoCtrl), relays(relayController), th50(th50Sensor),
        guva(guvaSensor), bmp280(bmpSensor), l75(l75Sensor) {}
    
    void begin() {
        // Try to connect to WiFi
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        
        Serial.print("Connecting to WiFi");
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        Serial.println();
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("WiFi connected: %s\n", WiFi.localIP().toString().c_str());
        } else {
            // Fallback to AP mode
            Serial.println("WiFi failed, starting AP mode...");
            WiFi.mode(WIFI_AP);
            WiFi.softAP(AP_SSID, AP_PASSWORD);
            apMode = true;
            Serial.printf("AP started: %s\n", WiFi.softAPIP().toString().c_str());
        }
        
        // Setup routes
        prefs.begin("gh", false);
        server.on("/", HTTP_GET, [this]() { handleRoot(); });
        server.on("/img/logo", HTTP_GET, [this]() {
            String svg = R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="180" height="180" viewBox="0 0 180 180"><rect width="180" height="180" fill="#1e293b"/><text x="90" y="95" font-family="Arial" font-size="24" fill="#22d3ee" text-anchor="middle">🌱</text></svg>)svg";
            server.send(200, "image/svg+xml", svg);
        });
        server.on("/api/sensors", HTTP_GET, [this]() { handleApiSensors(); });
        server.on("/api/matrix", HTTP_GET, [this]() { handleApiMatrix(); });
        server.on("/api/matrix", HTTP_POST, [this]() { handleApiMatrix(); });
        server.on("/api/matrix", HTTP_DELETE, [this]() { handleApiMatrix(); });
        server.on("/api/servo", HTTP_GET, [this]() { handleApiServo(); });
        server.on("/api/servo", HTTP_POST, [this]() { handleApiServo(); });
        server.on("/api/relays", HTTP_GET, [this]() { handleApiRelays(); });
        server.on("/api/relays", HTTP_POST, [this]() { handleApiRelays(); });
        server.on("/api/status", HTTP_GET, [this]() { handleApiStatus(); });
        server.on("/api/settings", HTTP_GET, [this]() { handleApiSettings(); });
        server.on("/api/settings", HTTP_POST, [this]() { handleApiSettings(); });
        server.on("/api/sensors/scan", HTTP_POST, [this]() { handleApiSensorScan(); });
        server.onNotFound([this]() { handleNotFound(); });
        
        // CORS headers
        server.enableCORS(true);
        
        server.begin();
        
        Serial.println("Web server started");
    }
    
    void loop() {
        server.handleClient();
    }
    
    void setMatrixColor(uint8_t r, uint8_t g, uint8_t b) {
        customMatrixR = r;
        customMatrixG = g;
        customMatrixB = b;
        useCustomColor = true;
        ledMatrix.fill(ledMatrix.Color(r, g, b));
    }
    
    void setMatrixAuto() {
        useCustomColor = false;
    }
    
    bool isApMode() const { return apMode; }
    IPAddress getIP() const { return apMode ? WiFi.softAPIP() : WiFi.localIP(); }
    bool isCustomColorActive() const { return useCustomColor; }
    bool isServoAutoMode() const { return servoAutoMode; }
};

extern GreenhouseWebServer* webServer;