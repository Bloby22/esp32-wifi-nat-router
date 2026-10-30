#pragma once
#include <cstdint>
#include "config/config.h"

// Configures the BOOT button pin as an input with a pull-up. Call once in setup()
void initBootButton();

// Waits up to windowMs for the BOOT button to be pressed and returns true as
// soon as it is. Press the button AFTER the reset: holding it during the reset
// puts the chip into download mode instead of running this program
bool bootButtonHeld(uint32_t windowsMs = config::BOOT_BUTTON_WINDOW_MS);