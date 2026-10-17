#pragma once

#include <Arduino.h>

// WiFi Configuration
// Change these to your network credentials
#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

// Web Server Configuration
#define WEB_SERVER_PORT 80
// Sensor readings refresh interval
#define LOG_INTERVAL_MS 2000

// Access Point fallback (if WiFi fails to connect)
#define AP_SSID         "Greenhouse"
#define AP_PASSWORD     "123456789"
#define AP_IP           IPAddress(192, 168, 4, 1)