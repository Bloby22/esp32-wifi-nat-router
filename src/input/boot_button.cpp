#include "input/boot_button.h"
#include <Arduino.h>

void initBootButton() {
    pinMode(config::BOOT_BUTTON_PIN, INPUT_PULLUP);
}

bool bootButtonHeld(uint32_t windowMs) {
    const uint32_t start = millis();
    while (millis() - start < windowMs) {
        if (digitalRead(config::BOOT_BUTTON_PIN) == LOW) {
            return true;
        } 
        delay(config::BOOT_BUTTON_POLL_MS);
    }
    return false;
}