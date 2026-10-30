#ifndef ESP_CONNECT_ACCESS_POINT_SETTINGS_H
#define ESP_CONNECT_ACCESS_POINT_SETTINGS_H

#include <Arduino.h>

constexpr size_t MIN_SSID_LENGTH = 1;
constexpr size_t MAX_SSID_LENGTH = 32;
constexpr size_t MIN_PASSWORD_LENGTH = 8;
constexpr size_t MAX_PASSWORD_LENGTH = 63;

struct AccessPointSettings {
    String upstreamSsid;
    String upstreamPassword;
    String ssid;
    String password;
    bool czechLanguage = false;
};

enum class SettingsLoadResult {
    Loaded,
    NeedsUpstream,
    NotConfigured,
    StorageError
};

extern AccessPointSettings accessPointSettings;

const char* localizedText(const char* english, const char* czech);
bool isValidCredentials(const String& ssid, const String& password);
bool isValidUpstreamCredentials(
    const String& ssid,
    const String& password);
SettingsLoadResult loadAccessPointSettings();
bool saveAccessPointSettings(const AccessPointSettings& settings);

#endif
