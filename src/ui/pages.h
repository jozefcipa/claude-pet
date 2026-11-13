#pragma once
#include <stdint.h>
#include "net/net.h"

enum Page { PAGE_PET, PAGE_USAGE };

// The bot with its props, effects and the status line/clock
void drawPetPage(uint32_t now, NetStatus net);
// Today's tokens + 7-day bars
void drawUsagePage();
