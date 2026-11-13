#pragma once
#include <stdint.h>
#include "pet/bot.h"

// Prop positions, shared with props.cpp
constexpr int LAPTOP_X = 190; // left edge of the laptop
constexpr int PILE_X = 30;
constexpr int STACK_X = W - 30;

// Little things the bot does on its own while Claude is idle
enum Antic { ANT_REST, ANT_WANDER, ANT_WAVE, ANT_STRETCH, ANT_DANCE, ANT_NAP, ANT_HOP, ANT_PEEK, ANT_FLY };

// Scene memory that outlives a single frame
struct Scene {
  float targetX = W / 2;
  float minis[3] = {};       // x of the subagent mini-bots
  uint32_t pauseUntil = 0;
  int stackN = 0;            // blocks on the build stack
  bool carrying = false;
  Antic antic = ANT_REST;
  uint32_t anticStart = 0, anticEnd = 0;
  int anticStep = 0;
};

extern Bot bot;
extern Scene scene;

// Call on every state/activity transition
void scenesReset();
// Moves the bot and sets its pose for this frame, based on pet state
void scenesUpdate(uint32_t now);
// Pose while there is no Wi-Fi: scanning for a signal, or sitting sadly
void scenesOffline(uint32_t now, bool searching);
// A tap wakes it from a nap
void scenesPoke(uint32_t now);
// Where the fly is during ANT_FLY; false when there is no fly on screen
bool flyPos(uint32_t now, int &x, int &y);
