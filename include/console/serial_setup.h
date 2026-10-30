#pragma once

/// @file
/// Interactive first-time setup over the serial console.
///
/// Each function asks questions, validates the answers (asking again on
/// invalid input) and stores the result in accessPointSettings. They block
/// until the user answers, so call them only when a person is expected to be
/// at the serial monitor.

/// @brief Asks for the language: 1 = English, 2 = Czech.
/// @post accessPointSettings.czechLanguage is set.
void readLanguageChoice();

/// @brief Asks for the home Wi-Fi name and password.
/// @note An empty password means an open network.
/// @post accessPointSettings.upstreamSsid and .upstreamPassword are set.
void readUpstreamSettings();

/// @brief Asks for the ESP's own Wi-Fi name and password.
/// @note The password is required (8-63 bytes).
/// @post accessPointSettings.ssid and .password are set.
void readAccessPointSettings();