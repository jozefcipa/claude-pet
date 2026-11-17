#pragma once
#include <stdint.h>
#include "net/net.h"

enum Page { PAGE_PET, PAGE_USAGE };

// The bot with its props, effects and the tool name / session usage
void drawPetPage(uint32_t now, NetStatus net);
// Boot intro, t = ms since boot
void drawBootPage(uint32_t t);
// Current session and weekly plan limits
void drawUsagePage();
