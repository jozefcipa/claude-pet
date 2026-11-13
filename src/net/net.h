#pragma once

// Wi-Fi, mDNS (claude-pet.local), NTP clock and the HTTP API the hooks call:
//   GET /state?s=<state>&tool=<name>&msg=<text for a speech bubble>
//   GET /usage?today=N&sessions=N&week=a,b,c,d,e,f,g
//   GET /say?msg=<text>[&ms=<duration>]  -> speech bubble without changing state
//   GET /  -> current state name
void netInit();
void netLoop();
bool netConfigured(); // false until secrets.h has an SSID
bool netConnected();
// Modem sleep: stays connected (so a hook can wake the pet) at lower power, ~100ms more latency
void netPowerSave(bool on);
