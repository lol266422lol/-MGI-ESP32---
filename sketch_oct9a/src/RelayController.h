#pragma once

#include <Arduino.h>
#include "config.h"

class RelayController {
private:
    struct RelayState {
        uint8_t pin;
        bool active;
        uint32_t startedAt;
        uint32_t durationMs;
    };

    RelayState relays[2] = {
        { PIN_RELAY_1, false, 0, 0 },
        { PIN_RELAY_2, false, 0, 0 }
    };

    RelayState* getRelay(uint8_t relayNumber) {
        if (relayNumber < 1 || relayNumber > 2) return nullptr;
        return &relays[relayNumber - 1];
    }

public:
    void begin() {
        relays[0].pin = runtimeSettings.pinRelay1;
        relays[1].pin = runtimeSettings.pinRelay2;
        for (RelayState& relay : relays) {
            digitalWrite(relay.pin, RELAY_INACTIVE_LEVEL);
            pinMode(relay.pin, OUTPUT);
            digitalWrite(relay.pin, RELAY_INACTIVE_LEVEL);
        }
    }

    bool activate(uint8_t relayNumber, uint32_t durationMs) {
        RelayState* relay = getRelay(relayNumber);
        if (!relay || durationMs == 0) return false;

        digitalWrite(relay->pin, RELAY_ACTIVE_LEVEL);
        relay->active = true;
        relay->startedAt = millis();
        relay->durationMs = durationMs;
        return true;
    }

    bool deactivate(uint8_t relayNumber) {
        RelayState* relay = getRelay(relayNumber);
        if (!relay) return false;

        digitalWrite(relay->pin, RELAY_INACTIVE_LEVEL);
        relay->active = false;
        relay->durationMs = 0;
        return true;
    }

    void update() {
        const uint32_t now = millis();
        for (uint8_t i = 0; i < 2; i++) {
            RelayState& relay = relays[i];
            if (relay.active && now - relay.startedAt >= relay.durationMs) {
                deactivate(i + 1);
                Serial.printf("Реле %u выключено по таймеру\n", i + 1);
            }
        }
    }

    bool isActive(uint8_t relayNumber) const {
        return relayNumber >= 1 && relayNumber <= 2 && relays[relayNumber - 1].active;
    }

    uint32_t remainingMs(uint8_t relayNumber) const {
        if (relayNumber < 1 || relayNumber > 2) return 0;
        const RelayState& relay = relays[relayNumber - 1];
        if (!relay.active) return 0;

        const uint32_t elapsed = millis() - relay.startedAt;
        return elapsed >= relay.durationMs ? 0 : relay.durationMs - elapsed;
    }
};
