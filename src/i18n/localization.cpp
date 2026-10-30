#include "i18n/localization.h"
#include "access_point/settings.h"

bool isCzech() {
    return accessPointSettings.czechLanguage;
}

const char* localizedText(const char* english, const char* czech) {
    return isCzech() ? czech : english;
}