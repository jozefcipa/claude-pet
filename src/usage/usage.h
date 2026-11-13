#pragma once
#include <stddef.h>
#include <stdint.h>

// Token usage pushed by scripts/usage.sh
struct Usage {
  uint64_t today = 0;
  int sessions = 0;
  uint64_t week[7] = {}; // oldest first, today last
  bool has = false;
};

extern Usage usage;

// Parses "1,2,3,4,5,6,7" into usage.week
void usageSetWeek(const char *csv);
// 2578200 -> "2.6M"
void fmtTokens(uint64_t n, char *out, size_t len);
