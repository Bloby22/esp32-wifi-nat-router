#include "access_point/settings.h"

#include <Preferences.h>
#include <cstring>

namespace {

constexpr char PREFERENCES_NAMESPACE[] = "wifi-config";
constexpr char CREDENTIALS_KEY[] = "credentials";
constexpr uint8_t CREDENTIALS_FORMAT_VERSION = 0xA1;
constexpr size_t CREDENTIALS_HEADER_SIZE = 6;
constexpr size_t CREDENTIALS_BUFFER_SIZE =
    CREDENTIALS_HEADER_SIZE + MAX_SSID_LENGTH + MAX_PASSWORD_LENGTH +
    MAX_SSID_LENGTH + MAX_PASSWORD_LENGTH;

}  // namespace

AccessPointSettings accessPointSettings;

bool isValidCredentials(const String& ssid, const String& password) {
    return ssid.length() >= MIN_SSID_LENGTH &&
           ssid.length() <= MAX_SSID_LENGTH &&
           password.length() >= MIN_PASSWORD_LENGTH &&
           password.length() <= MAX_PASSWORD_LENGTH;
}

bool isValidUpstreamCredentials(
    const String& ssid,
    const String& password) {
    const bool validPassword =
        password.isEmpty() ||
        (password.length() >= MIN_PASSWORD_LENGTH &&
         password.length() <= MAX_PASSWORD_LENGTH);
    return ssid.length() >= MIN_SSID_LENGTH &&
           ssid.length() <= MAX_SSID_LENGTH &&
           validPassword;
}

SettingsLoadResult loadAccessPointSettings() {
    Preferences preferences;
    if (!preferences.begin(PREFERENCES_NAMESPACE, true)) {
        return SettingsLoadResult::StorageError;
    }

    const size_t storedLength = preferences.getBytesLength(CREDENTIALS_KEY);
    if (storedLength == 0) {
        preferences.end();
        return SettingsLoadResult::NotConfigured;
    }

    if (storedLength < CREDENTIALS_HEADER_SIZE ||
        storedLength > CREDENTIALS_BUFFER_SIZE) {
        preferences.end();
        return SettingsLoadResult::NotConfigured;
    }

    uint8_t credentials[CREDENTIALS_BUFFER_SIZE] = {};
    const size_t bytesRead =
        preferences.getBytes(CREDENTIALS_KEY, credentials, storedLength);
    preferences.end();

    if (bytesRead != storedLength) {
        return SettingsLoadResult::StorageError;
    }

    if (credentials[0] != CREDENTIALS_FORMAT_VERSION) {
        const size_t legacySsidLength = credentials[0];
        const size_t legacyPasswordLength = credentials[1];
        const size_t legacyLength = 3 + legacySsidLength + legacyPasswordLength;
        if (legacySsidLength < MIN_SSID_LENGTH ||
            legacySsidLength > MAX_SSID_LENGTH ||
            legacyPasswordLength < MIN_PASSWORD_LENGTH ||
            legacyPasswordLength > MAX_PASSWORD_LENGTH ||
            storedLength != legacyLength) {
            return SettingsLoadResult::NotConfigured;
        }

        accessPointSettings.ssid.reserve(legacySsidLength);
        for (size_t index = 0; index < legacySsidLength; ++index) {
            accessPointSettings.ssid +=
                static_cast<char>(credentials[3 + index]);
        }
        accessPointSettings.password.reserve(legacyPasswordLength);
        for (size_t index = 0; index < legacyPasswordLength; ++index) {
            accessPointSettings.password += static_cast<char>(
                credentials[3 + legacySsidLength + index]);
        }
        accessPointSettings.czechLanguage = credentials[2] != 0;
        return isValidCredentials(
                   accessPointSettings.ssid,
                   accessPointSettings.password)
                   ? SettingsLoadResult::NeedsUpstream
                   : SettingsLoadResult::NotConfigured;
    }

    const size_t upstreamSsidLength = credentials[1];
    const size_t upstreamPasswordLength = credentials[2];
    const size_t apSsidLength = credentials[3];
    const size_t apPasswordLength = credentials[4];
    const size_t expectedLength =
        CREDENTIALS_HEADER_SIZE + upstreamSsidLength +
        upstreamPasswordLength + apSsidLength + apPasswordLength;

    if (upstreamSsidLength < MIN_SSID_LENGTH ||
        upstreamSsidLength > MAX_SSID_LENGTH ||
        (upstreamPasswordLength != 0 &&
         (upstreamPasswordLength < MIN_PASSWORD_LENGTH ||
          upstreamPasswordLength > MAX_PASSWORD_LENGTH)) ||
        apSsidLength < MIN_SSID_LENGTH || apSsidLength > MAX_SSID_LENGTH ||
        apPasswordLength < MIN_PASSWORD_LENGTH ||
        apPasswordLength > MAX_PASSWORD_LENGTH ||
        storedLength != expectedLength) {
        return SettingsLoadResult::NotConfigured;
    }

    size_t offset = CREDENTIALS_HEADER_SIZE;
    String upstreamSsid;
    upstreamSsid.reserve(upstreamSsidLength);
    for (size_t index = 0; index < upstreamSsidLength; ++index) {
        upstreamSsid += static_cast<char>(credentials[offset++]);
    }

    String upstreamPassword;
    upstreamPassword.reserve(upstreamPasswordLength);
    for (size_t index = 0; index < upstreamPasswordLength; ++index) {
        upstreamPassword += static_cast<char>(credentials[offset++]);
    }

    String apSsid;
    apSsid.reserve(apSsidLength);
    for (size_t index = 0; index < apSsidLength; ++index) {
        apSsid += static_cast<char>(credentials[offset++]);
    }

    String apPassword;
    apPassword.reserve(apPasswordLength);
    for (size_t index = 0; index < apPasswordLength; ++index) {
        apPassword += static_cast<char>(credentials[offset++]);
    }

    if (!isValidUpstreamCredentials(upstreamSsid, upstreamPassword) ||
        !isValidCredentials(apSsid, apPassword)) {
        return SettingsLoadResult::NotConfigured;
    }

    accessPointSettings.upstreamSsid = upstreamSsid;
    accessPointSettings.upstreamPassword = upstreamPassword;
    accessPointSettings.ssid = apSsid;
    accessPointSettings.password = apPassword;
    accessPointSettings.czechLanguage = credentials[5] != 0;
    return SettingsLoadResult::Loaded;
}

bool saveAccessPointSettings(const AccessPointSettings& settings) {
    if (!isValidUpstreamCredentials(
            settings.upstreamSsid,
            settings.upstreamPassword) ||
        !isValidCredentials(settings.ssid, settings.password)) {
        return false;
    }

    uint8_t credentials[CREDENTIALS_BUFFER_SIZE] = {};
    credentials[0] = CREDENTIALS_FORMAT_VERSION;
    credentials[1] = static_cast<uint8_t>(settings.upstreamSsid.length());
    credentials[2] = static_cast<uint8_t>(settings.upstreamPassword.length());
    credentials[3] = static_cast<uint8_t>(settings.ssid.length());
    credentials[4] = static_cast<uint8_t>(settings.password.length());
    credentials[5] = settings.czechLanguage ? 1 : 0;

    size_t offset = CREDENTIALS_HEADER_SIZE;
    memcpy(
        credentials + offset,
        settings.upstreamSsid.c_str(),
        settings.upstreamSsid.length());
    offset += settings.upstreamSsid.length();
    memcpy(
        credentials + offset,
        settings.upstreamPassword.c_str(),
        settings.upstreamPassword.length());
    offset += settings.upstreamPassword.length();
    memcpy(
        credentials + offset,
        settings.ssid.c_str(),
        settings.ssid.length());
    offset += settings.ssid.length();
    memcpy(
        credentials + offset,
        settings.password.c_str(),
        settings.password.length());
    offset += settings.password.length();

    Preferences preferences;
    if (!preferences.begin(PREFERENCES_NAMESPACE, false)) {
        return false;
    }

    const size_t bytesWritten = preferences.putBytes(
        CREDENTIALS_KEY,
        credentials,
        offset);
    preferences.end();

    return bytesWritten == offset;
}
