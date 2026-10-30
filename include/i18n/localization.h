#pragma once

// True when the saved language setting is Czech.
bool isCzech();

// Returns the Czech or the English text, depending on the saved language.
// Both arguments must be string literals (or otherwise outlive the call).
const char* localizedText(const char* english, const char* czech);