#pragma once
#include <stdint.h>

enum TouchEvent { TOUCH_NONE, TOUCH_TAP, TOUCH_SWIPE };

void touchInit();
// Call every loop; reports a gesture once the finger lifts
TouchEvent touchPoll(uint32_t now);
