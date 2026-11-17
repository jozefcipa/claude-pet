#pragma once

// Copy to secrets.h (gitignored) and fill in.

// 2.4 GHz network only (the C6 has no 5 GHz radio). Leave the SSID empty for demo mode.
#define WIFI_SSID ""
#define WIFI_PASS ""

// POSIX TZ string, e.g. Europe/Prague
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

// Password for OTA updates (POST /update, user "pet"). Empty = anyone on your network can flash.
#define OTA_PASS ""
