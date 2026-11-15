#pragma once
#include <stdint.h>
#include "net/net.h"

enum Page { PAGE_PET, PAGE_USAGE };

// The bot with its props, effects and the tool name / token count
void drawPetPage(uint32_t now, NetStatus net);
// Boot intro, t = ms since boot
void drawBootPage(uint32_t t);
// Today's tokens + 7-day bars
void drawUsagePage();
