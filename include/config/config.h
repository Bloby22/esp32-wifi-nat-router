#pragma once

#include <Arduino.h>
#include <WiFi.h>

/// Pin of the BOOT button. It is set per board in platformio.ini with
/// -DBOOT_BUTTON_GPIO=... (ESP32 DevKit V1: GPIO0, ESP32-C3 DevKitM-1: GPIO9).
/// If the flag is missing, GPIO9 (the ESP32-C3 button) is used.
#ifndef BOOT_BUTTON_GPIO
#define BOOT_BUTTON_GPIO 9
#endif

/// Fixed settings of the firmware (pins, timings, limits and addresses).
namespace config {

    // Serial console
    constexpr uint32_t SERIAL_BAUD_RATE = 115200;
    constexpr uint32_t SERIAL_POLL_DELAY_MS = 100;
    constexpr uint32_t MESSAGE_DELAY_MS = 1000;

    // BOOT button (press it within the window after reset to reconfigure) ----
    // The "3 seconds" in the on-screen prompts matches BOOT_BUTTON_WINDOW_MS.
    constexpr uint8_t BOOT_BUTTON_PIN = BOOT_BUTTON_GPIO;
    constexpr uint32_t BOOT_BUTTON_WINDOW_MS = 3000;
    constexpr uint32_t BOOT_BUTTON_POLL_MS = 10;

    /// Only a starting point: the AP follows the home router's channel once the
    /// station connects, because the chip has a single radio.
    constexpr uint8_t ACCESS_POINT_CHANNEL = 1;
    constexpr uint8_t MAX_CONNECTED_CLIENTS = 4;
    constexpr wifi_power_t ACCESS_POINT_TX_POWER = WIFI_POWER_8_5dBm;

    // Addresses of the ESP network (192.168.4.0/24)
    // IPAddress objects cannot be constexpr, so they are defined once in config.cpp.
    extern const IPAddress ACCESS_POINT_IP;           ///< The ESP itself (gateway and management page).
    extern const IPAddress ACCESS_POINT_MASK;         ///< Subnet mask of the ESP network.
    extern const IPAddress ACCESS_POINT_LEASE_START;  ///< First address handed out by DHCP.
    extern const IPAddress CLIENT_DNS_SERVER;         ///< DNS server given to clients.

}  // namespace config