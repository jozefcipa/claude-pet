#pragma once
#include <stdint.h>

enum TouchEvent { TOUCH_NONE, TOUCH_TAP, TOUCH_SWIPE, TOUCH_LONG };

void touchInit();
// Call every loop; reports a gesture once the finger lifts (TOUCH_LONG fires while still held)
TouchEvent touchPoll(uint32_t now);
