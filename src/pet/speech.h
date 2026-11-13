#pragma once
#include <stdint.h>
#include "pet/scenes.h"
#include "pet/state.h"

// Speech bubbles: Claude's own messages (pet.msg) win, otherwise a canned line fitting the scene.
void say(const char *msg, uint32_t now, uint32_t durationMs = 0); // 0 = based on length
void speechOnScene(State prev, uint32_t now);
void speechOnPoke(uint32_t now);
void speechOnAntic(Antic a, bool starting, uint32_t now);
bool speaking(uint32_t now);
void drawSpeech(uint32_t now);
