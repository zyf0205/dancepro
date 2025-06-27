#ifndef WIFI_UI_H
#define WIFI_UI_H

#include <M5Unified.h>
#include "wifi/my_wifi.h"

// Display WiFi connection UI with built-in anti-flicker logic
// Call this function repeatedly in your main loop
void displayWiFiUI(WiFiStatus status);

// AP name to display in UI (defined in link.cpp)
extern const char* AP_NAME;

#endif
