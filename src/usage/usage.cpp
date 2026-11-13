#include <stdio.h>
#include <stdlib.h>
#include "usage/usage.h"

Usage usage;

void usageSetWeek(const char *csv) {
  const char *p = csv;
  for (int i = 0; i < 7; i++) {
    char *end;
    usage.week[i] = strtoull(p, &end, 10);
    if (*end != ',') {
      break;
    }
    p = end + 1;
  }
}

void fmtTokens(uint64_t n, char *out, size_t len) {
  if (n < 1000) {
    snprintf(out, len, "%llu", n);
  } else if (n < 1000000) {
    snprintf(out, len, "%.1fk", n / 1e3);
  } else if (n < 1000000000) {
    snprintf(out, len, "%.1fM", n / 1e6);
  } else {
    snprintf(out, len, "%.2fB", n / 1e9);
  }
}
