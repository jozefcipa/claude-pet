#pragma once
#include <stdint.h>

enum NetStatus {
  NET_DEMO,       // no SSID configured
  NET_CONNECTING, // trying (first NO_WIFI_AFTER_MS of being offline)
  NET_NO_WIFI,    // offline for longer, still retrying in the background
  NET_ONLINE,
};

// Wi-Fi, mDNS (claude-pet.local), NTP clock and the HTTP API the hooks call:
//   GET /state?s=<state>&tool=<name>&msg=<text for a speech bubble>
//   GET /usage?today=N&sessions=N&week=a,b,c,d,e,f,g
//   GET /say?msg=<text>[&ms=<duration>]  -> speech bubble without changing state
//   GET /  -> current state name
void netInit();
void netLoop();
bool netConfigured(); // false until secrets.h has an SSID
bool netConnected();
NetStatus netStatus(uint32_t now);
// Modem sleep: stays connected (so a hook can wake the pet) at lower power, ~100ms more latency
void netPowerSave(bool on);
