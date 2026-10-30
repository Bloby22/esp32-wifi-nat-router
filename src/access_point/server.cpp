#include "access_point/access_point_server.h"
#include "access_point/settings.h"
#include "network/nat_router.h"
#include <LittleFS.h>
#include <WiFi.h>
#include <esp_system.h>

namespace {

String csrfToken;

String escapeHtml(const String& value) {
    String escaped;
    escaped.reserve(value.length());

    for (size_t index = 0; index < value.length(); ++index) {
        switch (value[index]) {
            case '&':
                escaped += F("&amp;");
                break;
            case '<':
                escaped += F("&lt;");
                break;
            case '>':
                escaped += F("&gt;");
                break;
            case '"':
                escaped += F("&quot;");
                break;
            case '\'':
                escaped += F("'");
                break;
            default:
                escaped += value[index];
                break;
        }
    }

    return escaped;
}

String generateCsrfToken() {
    char token[33] = {};
    for (size_t index = 0; index < 4; ++index) {
        snprintf(
            token + index * 8,
            sizeof(token) - index * 8,
            "%08lx",
            static_cast<unsigned long>(esp_random()));
    }

    return String(token);
}

bool matchesCsrfToken(const String& submittedToken) {
    if (submittedToken.length() != csrfToken.length()) {
        return false;
    }

    uint8_t difference = 0;
    for (size_t index = 0; index < csrfToken.length(); ++index) {
        difference |= static_cast<uint8_t>(submittedToken[index] ^ csrfToken[index]);
    }
    return difference == 0;
}

bool readLocalizedPage(
    const char* englishPath,
    const char* czechPath,
    String& page) {
    File file = LittleFS.open(
        localizedText(englishPath, czechPath),
        "r");
    if (!file) {
        return false;
    }

    page = file.readString();
    file.close();
    return page.length() > 0;
}

void populatePage(String& page, const String& ssid) {
    page.replace("{{SSID}}", escapeHtml(ssid));
    page.replace(
        "{{UPSTREAM_SSID}}",
        escapeHtml(accessPointSettings.upstreamSsid));
    page.replace("{{CSRF_TOKEN}}", csrfToken);
    page.replace("{{AP_IP}}", WiFi.softAPIP().toString());
    page.replace("{{STA_IP}}", WiFi.localIP().toString());
    page.replace("{{CLIENTS}}", String(WiFi.softAPgetStationNum()));
    page.replace("{{CHANNEL}}", String(WiFi.channel()));
    page.replace("{{UPTIME}}", String(millis() / 1000));
    page.replace("{{FREE_HEAP}}", String(ESP.getFreeHeap() / 1024));
    page.replace(
        "{{UPSTREAM_STATUS}}",
        WiFi.status() == WL_CONNECTED
            ? localizedText("Connected", "Připojeno")
            : localizedText("Not connected", "Nepřipojeno"));
    page.replace(
        "{{NAT_STATUS}}",
        isNatEnabled() // ZMĚNA: Přidány závorky pro volání funkce z nat_router.h
            ? localizedText("NAPT forwarding active", "Přesměrování NAPT aktivní")
            : localizedText("Waiting for upstream", "Čeká se na domácí Wi-Fi"));
}

bool sendLocalizedPage(
    const char* englishPath,
    const char* czechPath,
    const String& ssid) {
    String page;
    if (!readLocalizedPage(englishPath, czechPath, page)) {
        webServer.send(
            500,
            "text/plain",
            localizedText(
                "Failed to read the page from LittleFS.",
                "Nepodařilo se načíst stránku z LittleFS."));
        return false;
    }

    populatePage(page, ssid);
    webServer.sendHeader("Cache-Control", "no-store");
    webServer.send(200, "text/html; charset=UTF-8", page);
    return true;
}

}  // namespace

WebServer webServer(80);

void startAccessPointWebServer() {
    csrfToken = generateCsrfToken();
    webServer.on("/", HTTP_GET, handleRoot);
    webServer.on("/style.css", HTTP_GET, handleStylesheet);
    webServer.on("/save", HTTP_POST, handleSave);
    webServer.onNotFound(handleNotFound);
    webServer.begin();
}

void handleRoot() {
    sendLocalizedPage("/index.en.html", "/index.cs.html", accessPointSettings.ssid);
}

void handleStylesheet() {
    File file = LittleFS.open("/style.css", "r");
    if (!file) {
        webServer.send(404, "text/plain", "Stylesheet not found.");
        return;
    }

    webServer.streamFile(file, "text/css; charset=UTF-8");
    file.close();
}

void handleSave() {
    if (!webServer.hasArg("csrf_token") ||
        !matchesCsrfToken(webServer.arg("csrf_token"))) {
        webServer.send(
            403,
            "text/plain",
            localizedText(
                "Request verification failed. Reload the page and try again.",
                "Ověření požadavku selhalo. Obnovte stránku a zkuste to znovu."));
        return;
    }

    if (!webServer.hasArg("upstream_ssid") ||
        !webServer.hasArg("upstream_password") ||
        !webServer.hasArg("new_ssid") ||
        !webServer.hasArg("new_password")) {
        webServer.send(
            400,
            "text/plain",
            localizedText(
                "Invalid input. Check the network name and password.",
                "Neplatné údaje. Zkontrolujte název sítě a heslo."));
        return;
    }

    AccessPointSettings nextSettings = accessPointSettings;
    nextSettings.upstreamSsid = webServer.arg("upstream_ssid");
    const String submittedUpstreamPassword = webServer.arg("upstream_password");
    const String submittedApPassword = webServer.arg("new_password");
    const bool isOpenNetwork = webServer.hasArg("upstream_open");
    const bool canKeepExistingPassword =
        nextSettings.upstreamSsid == accessPointSettings.upstreamSsid;

    if (isOpenNetwork) {
        nextSettings.upstreamPassword = "";
    } else if (submittedUpstreamPassword.length() == 0 &&
               canKeepExistingPassword) {
        nextSettings.upstreamPassword = accessPointSettings.upstreamPassword;
    } else {
        nextSettings.upstreamPassword = submittedUpstreamPassword;
    }
    nextSettings.ssid = webServer.arg("new_ssid");
    if (submittedApPassword.length() == 0 &&
        nextSettings.ssid == accessPointSettings.ssid) {
        nextSettings.password = accessPointSettings.password;
    } else {
        nextSettings.password = submittedApPassword;
    }

    if (!isValidUpstreamCredentials(
            nextSettings.upstreamSsid,
            nextSettings.upstreamPassword) ||
        (!isOpenNetwork &&
         submittedUpstreamPassword.length() == 0 &&
         !canKeepExistingPassword) ||
        !isValidCredentials(nextSettings.ssid, nextSettings.password)) {
        webServer.send(
            400,
            "text/plain",
            localizedText(
                "Check both network names and passwords. Network names must be 1-32 UTF-8 bytes; passwords must be empty or 8-63 bytes.",
                "Zkontrolujte názvy obou sítí a hesla. Název musí mít 1-32 bajtů UTF-8; heslo musí být prázdné nebo mít 8-63 bajtů."));
        return;
    }

    String confirmationPage;
    if (!readLocalizedPage(
            "/saved.en.html",
            "/saved.cs.html",
            confirmationPage)) {
        webServer.send(
            500,
            "text/plain",
            localizedText(
                "Failed to read the confirmation page from LittleFS.",
                "Nepodařilo se načíst potvrzovací stránku z LittleFS."));
        return;
    }

    if (!saveAccessPointSettings(nextSettings)) {
        webServer.send(
            500,
            "text/plain",
            localizedText(
                "Failed to save the Wi-Fi settings.",
                "Nepodařilo se uložit nastavení Wi-Fi."));
        return;
    }

    accessPointSettings = nextSettings;
    populatePage(confirmationPage, nextSettings.ssid);
    webServer.sendHeader("Cache-Control", "no-store");
    webServer.send(200, "text/html; charset=UTF-8", confirmationPage);
    delay(1000);
    ESP.restart();
}

void handleNotFound() {
    webServer.sendHeader("Location", "http://192.168.4.1/");
    webServer.send(302, "text/plain", "Redirecting to the access point setup page.");
}
