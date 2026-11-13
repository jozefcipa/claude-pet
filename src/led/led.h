#pragma once
#include <stdint.h>
#include "pet/state.h"

// Status light on an optional WS2812 wired to pins::LED.
// (The board's own LEDs are hardwired power/charge indicators, not controllable.)
//   idle      steady dim orange
//   thinking  slow breathing
//   working   faster breathing
//   attention blinking
//   done      bright flash fading out
//   sleep     off
void ledInit();
void ledUpdate(uint32_t now, State state, uint32_t since);
