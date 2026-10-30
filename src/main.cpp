#include <LittleFS.h>
#include <WiFi.h>

#include "access_point/access_point_server.h"
#include "access_point/settings.h"
#include "config/config.h"
#include "console/serial_setup.h"
#include "i18n/localization.h"
#include "input/boot_button.h"
#include "network/nat_router.h"

namespace {

bool webServerReady = false;

/// Mounts the LittleFS partition that holds the web page files.
/// @return false (after printing a hint) if the partition cannot be mounted.
[[nodiscard]] bool mountFilesystem() {
    if (LittleFS.begin(false)) {
        return true;
    }
    Serial.println(
        "[ERROR] Failed to mount LittleFS. Upload web files with "
        "'pio run --target uploadfs'.");
    return false;
}

/// Loads the saved Wi-Fi settings, or asks for them over the serial console
/// when nothing is saved, the settings are incomplete, or BOOT is pressed.
/// @return false if the settings cannot be read or saved.
[[nodiscard]] bool configureSettings() {
    const SettingsLoadResult settingsResult = loadAccessPointSettings();
    if (settingsResult == SettingsLoadResult::StorageError) {
        Serial.println("[ERROR] Failed to read Wi-Fi settings from NVS.");
        return false;
    }

    // On first boot everything is asked anyway, so no BOOT window is needed.
    bool bootButtonPressed = false;
    if (settingsResult != SettingsLoadResult::NotConfigured) {
        Serial.println(localizedText(
            "Hold BOOT within 3 seconds to reconfigure...",
            "Pro přenastavení podržte BOOT do 3 sekund..."));
        bootButtonPressed = bootButtonHeld();
    }

    if (settingsResult == SettingsLoadResult::NotConfigured) {
        Serial.println(
            "\nNo saved settings. Configure both Wi-Fi networks in the serial monitor.");
        readLanguageChoice();
        readUpstreamSettings();
        readAccessPointSettings();
    } else if (settingsResult == SettingsLoadResult::NeedsUpstream) {
        Serial.println(localizedText(
            "\nLegacy settings found. Enter the home Wi-Fi credentials.",
            "\nBylo nalezeno původní nastavení. Zadejte údaje domácí Wi-Fi."));
        readUpstreamSettings();
        if (bootButtonPressed) {
            readAccessPointSettings();
        }
    } else if (bootButtonPressed) {
        Serial.println(localizedText(
            "\nBOOT button held. Re-enter both Wi-Fi configurations.",
            "\nTlačítko BOOT je stisknuté. Zadejte znovu nastavení obou Wi-Fi sítí."));
        readUpstreamSettings();
        readAccessPointSettings();
    } else {
        Serial.println(localizedText(
            "\nLoaded saved Wi-Fi settings.",
            "\nNačetl jsem uložené nastavení Wi-Fi."));
    }

    if (settingsResult != SettingsLoadResult::Loaded || bootButtonPressed) {
        if (!saveAccessPointSettings(accessPointSettings)) {
            Serial.println(localizedText(
                "[ERROR] Failed to save Wi-Fi settings to NVS.",
                "[CHYBA] Nepodařilo se uložit nastavení Wi-Fi do NVS."));
            return false;
        }
    }
    return true;
}

void printReadyMessage() {
    Serial.println(localizedText(
        "Wi-Fi NAT router is ready.",
        "Wi-Fi NAT router je připraven."));
    Serial.print(localizedText("ESP network name: ", "Název sítě ESP: "));
    Serial.println(accessPointSettings.ssid);
    Serial.print(localizedText("Management IP address: ", "Adresa pro správu: "));
    Serial.println(WiFi.softAPIP());
}

}  // namespace

void setup() {
    Serial.begin(config::SERIAL_BAUD_RATE);
    initBootButton();
    delay(config::MESSAGE_DELAY_MS);

    if (!mountFilesystem() || !configureSettings() || !startNatRouter()) {
        return;
    }

    startAccessPointWebServer();
    webServerReady = true;
    printReadyMessage();
}

void loop() {
    if (!webServerReady) {
        delay(config::MESSAGE_DELAY_MS);
        return;
    }

    webServer.handleClient();
    delay(2);
}