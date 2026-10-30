#include "console/serial_setup.h"

#include <Arduino.h>

#include "access_point/settings.h"
#include "config/config.h"
#include "i18n/localization.h"

namespace {

/// Reads one line from the serial console, waiting for Enter without a timeout.
/// Handles both "\n" and "\r\n" line endings.
String readSerialLine() {
    String input;

    while (true) {
        while (Serial.available() == 0) {
            delay(config::SERIAL_POLL_DELAY_MS);
        }

        const char c = static_cast<char>(Serial.read());

        if (c == '\n' || c == '\r') {
            Serial.printf(
                "\n[INPUT DEBUG] Received %u bytes.\n",
                static_cast<unsigned>(input.length())
            );

            return input;
        }

        input += c;
    }
}

/// A Wi-Fi name must be 1-32 bytes (UTF-8 bytes, not characters).
[[nodiscard]] bool isValidSsid(const String& ssid) {
    return ssid.length() >= MIN_SSID_LENGTH && ssid.length() <= MAX_SSID_LENGTH;
}

/// A password must be 8-63 bytes; it may be empty only for an open network.
[[nodiscard]] bool isValidPassword(const String& password, bool allowOpenNetwork) {
    if (allowOpenNetwork && password.length() == 0) {
        return true;
    }
    return password.length() >= MIN_PASSWORD_LENGTH &&
           password.length() <= MAX_PASSWORD_LENGTH;
}

String readNetworkName(const char* englishPrompt, const char* czechPrompt) {
    while (true) {
        Serial.println(localizedText(englishPrompt, czechPrompt));
        const String networkName = readSerialLine();
        if (isValidSsid(networkName)) {
            return networkName;
        }
        Serial.println(localizedText(
            "🙈 A name needs 1-32 bytes. Heads up: diacritics and emoji take more than one byte each!",
            "🙈 Název musí mít 1-32 bajtů. Pozor: diakritika a emoji zabírají víc než jeden bajt!"));
    }
}

String readNetworkPassword(
    const char* englishPrompt,
    const char* czechPrompt,
    bool allowOpenNetwork) {
    while (true) {
        Serial.println(localizedText(englishPrompt, czechPrompt));
        const String password = readSerialLine();
        if (isValidPassword(password, allowOpenNetwork)) {
            return password;
        }
        if (allowOpenNetwork) {
            Serial.println(localizedText(
                "🙅 Nope! 8-63 bytes, or just hit Enter if the network is open. Give it another shot.",
                "🙅 To ne! 8-63 bajtů, nebo jen Enter, když je síť otevřená. Zkus to znovu."));
        } else {
            Serial.println(localizedText(
                "🙅 Nope! This one needs 8-63 bytes, an empty password is not allowed here. Try again.",
                "🙅 To ne! Tady to chce 8-63 bajtů, prázdné heslo nejde. Zkus to znovu."));
        }
    }
}

}  // namespace

void readLanguageChoice() {
    String languageChoice;
    bool firstTry = true;
    do {
        if (!firstTry) {
            Serial.println("🤔 Hmm, that's not a 1 or a 2. / Hmm, to není 1 ani 2.");
        }
        firstTry = false;
        Serial.println("\n👋 Hey! Pick your language / Čau! Vyber si jazyk:");
        Serial.println("1 - English");
        Serial.println("2 - Česky");
        languageChoice = readSerialLine();
    } while (languageChoice != "1" && languageChoice != "2");

    accessPointSettings.czechLanguage = languageChoice == "2";
    Serial.println(localizedText(
        "✅ English it is!", 
        "✅ Česky, jedeme!"
    ));
}

void readUpstreamSettings() {
    accessPointSettings.upstreamSsid = readNetworkName(
        "🏠 First up: what's your home Wi-Fi called? (1-32 UTF-8 bytes)",
        "🏠 Nejdřív: jak se jmenuje tvoje domácí Wi-Fi? (1-32 bajtů UTF-8)");
    accessPointSettings.upstreamPassword = readNetworkPassword(
        "🔑 And its password? (8-63 bytes)",
        "🔑 A jaké má heslo? (8-63 bajtů)",
        false);
}

void readAccessPointSettings() {
    accessPointSettings.ssid = readNetworkName(
        "📡 Now name the network your ESP will create (1-32 UTF-8 bytes). Be creative!",
        "📡 Teď pojmenuj síť, kterou ESP vytvoří (1-32 bajtů UTF-8). Buď kreativní!");
    accessPointSettings.password = readNetworkPassword(
        "🔒 Pick a password for it (8-63 bytes). No 'password123', please.",
        "🔒 Vymysli k ní heslo (8-63 bajtů). Jen ne 'heslo123', prosím.",
        false);
    Serial.println(localizedText(
        "🎉 Nice! Both networks are noted. Time to make some magic happen...",
        "🎉 Hotovo! Obě sítě mám. Jdeme na to..."));
}
