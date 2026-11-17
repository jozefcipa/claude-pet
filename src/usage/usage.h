#pragma once
#include <stddef.h>
#include <stdint.h>

// One plan limit (5-hour session or weekly), pushed by scripts/usage.sh
struct Limit {
  int pct = 0;
  uint32_t resetAt = 0; // in millis() / 1000 seconds, so no NTP needed
};

struct Usage {
  Limit session, week;
  bool has = false;
};

extern Usage usage;

// pct + seconds until reset, as sent by the Mac
void usageSet(Limit &l, int pct, uint32_t resetIn);
// Seconds until the limit resets, 0 once it has
uint32_t usageResetIn(const Limit &l);
// Percent used, 0 once the window has reset
int usagePct(const Limit &l);
// 4920 -> "1h 22m", 547200 -> "6d 8h"
void fmtDuration(uint32_t secs, char *out, size_t len);
