#include <Arduino.h>
#include "usage/usage.h"

Usage usage;

static uint32_t nowSecs() {
  return millis() / 1000;
}

void usageSet(Limit &l, int pct, uint32_t resetIn) {
  l.pct = constrain(pct, 0, 100);
  l.resetAt = nowSecs() + resetIn;
}

uint32_t usageResetIn(const Limit &l) {
  int32_t left = (int32_t)(l.resetAt - nowSecs());
  return left > 0 ? left : 0;
}

int usagePct(const Limit &l) {
  return usageResetIn(l) ? l.pct : 0;
}

void fmtDuration(uint32_t secs, char *out, size_t len) {
  uint32_t m = secs / 60, h = m / 60, d = h / 24;
  if (d) {
    snprintf(out, len, "%lud %luh", d, h % 24);
  } else if (h) {
    snprintf(out, len, "%luh %lum", h, m % 60);
  } else {
    snprintf(out, len, "%lum", max<uint32_t>(m, 1));
  }
}
