#pragma once
#include "config.h"

enum Eyes { EYES_OPEN, EYES_BLINK, EYES_HAPPY, EYES_CLOSED, EYES_WIDE, EYES_DIZZY };

// One frame's pose. Scenes fill it in, drawBot renders it.
struct Bot {
  float x = W / 2;
  float y = 0;            // height above the ground (jumps)
  int dir = 1;            // -1 facing left, 1 facing right
  int lookX = 0, lookY = 0;
  int armL = 0, armR = 0; // 0 down, 1 raised, 2 straight up
  int legs = 0;           // 0 standing, 1/2 walk frames, -1 sitting
  Eyes eyes = EYES_OPEN;
};

// Pixel art on a 16x10 grid of u-sized pixels:
// body cols 2-13 rows 0-7, arms cols 0-1/14-15 rows 4-5, eyes cols 4/11 rows 2-3, legs cols 3,5,10,12 rows 8-9
void drawBot(const Bot &b, int u);
int botTop(const Bot &b, int u);
