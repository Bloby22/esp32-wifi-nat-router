#include "network/nat_router.h"

#include <WiFi.h>

#include <atomic>

#include "access_point/settings.h"
#include "config/config.h"
#include "i18n/localization.h"

namespace {

/// Written by the Wi-Fi event task, read by the main loop and the web server.
std::atomic<bool> natEnabled{false};

void onWifiEvent(arduino_event_id_t event, [[maybe_unused]] arduino_event_info_t info) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            natEnabled = WiFi.AP.enableNAPT(true);
            if (natEnabled) {
                Serial.print(localizedText(
                    "Home Wi-Fi connected; NAT is active. Upstream IP: ",
                    "Domácí Wi-Fi připojena; NAT je aktivní. IP adresa: "));
                Serial.println(WiFi.localIP());
            } else {
                Serial.println(localizedText(
                    "[ERROR] Upstream connected, but NAT could not be enabled.",
                    "[CHYBA] Domácí Wi-Fi je připojena, ale NAT se nepodařilo zapnout."));
            }
            break;

        case ARDUINO_EVENT_WIFI_STA_LOST_IP:
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            if (natEnabled && !WiFi.AP.enableNAPT(false)) {
                Serial.println(localizedText(
                    "[WARNING] Could not disable NAT after upstream disconnect.",
                    "[VAROVÁNÍ] Po odpojení domácí Wi-Fi se nepodařilo vypnout NAT."));
            }
            natEnabled = false;
            Serial.println(localizedText(
                "Home Wi-Fi disconnected; waiting to reconnect.",
                "Domácí Wi-Fi odpojena; čekám na opětovné připojení."));
            break;

        default:
            break;
    }
}

}  // namespace

bool isNatEnabled() {
    return natEnabled;
}

bool startNatRouter() {
    Network.onEvent(onWifiEvent);
    WiFi.mode(WIFI_AP_STA);

    if (!WiFi.AP.begin()) {
        Serial.println(localizedText(
            "[ERROR] Failed to initialize the ESP access point.",
            "[CHYBA] Nepodařilo se inicializovat Wi-Fi síť ESP."));
        return false;
    }

    if (!WiFi.AP.config(
            config::ACCESS_POINT_IP,
            config::ACCESS_POINT_IP,
            config::ACCESS_POINT_MASK,
            config::ACCESS_POINT_LEASE_START,
            config::CLIENT_DNS_SERVER)) {
        Serial.println(localizedText(
            "[ERROR] Failed to configure the ESP access point.",
            "[CHYBA] Nepodařilo se nakonfigurovat Wi-Fi síť ESP."));
        return false;
    }

    if (!WiFi.AP.create(
            accessPointSettings.ssid.c_str(),
            accessPointSettings.password.c_str(),
            config::ACCESS_POINT_CHANNEL,
            0,
            config::MAX_CONNECTED_CLIENTS)) {
        Serial.println(localizedText(
            "[ERROR] Failed to start the ESP access point.",
            "[CHYBA] Nepodařilo se spustit Wi-Fi síť ESP."));
        return false;
    }

    if (!WiFi.AP.waitStatusBits(ESP_NETIF_STARTED_BIT, config::MESSAGE_DELAY_MS)) {
        Serial.println(localizedText(
            "[ERROR] Timed out waiting for the ESP access point to start.",
            "[CHYBA] Vypršel čas při čekání na spuštění Wi-Fi sítě ESP."));
        return false;
    }

    WiFi.setTxPower(config::ACCESS_POINT_TX_POWER);
    WiFi.setAutoReconnect(true);

    Serial.println(localizedText(
        "Connecting to the home Wi-Fi network...",
        "Připojuji se k domácí Wi-Fi síti..."));
    if (WiFi.begin(
            accessPointSettings.upstreamSsid.c_str(),
            accessPointSettings.upstreamPassword.c_str()) == WL_CONNECT_FAILED) {
        Serial.println(localizedText(
            "[WARNING] Could not start the home Wi-Fi connection.",
            "[VAROVÁNÍ] Nepodařilo se zahájit připojení k domácí Wi-Fi."));
    }
    return true;
}