#include "pet/bot.h"
#include "display/display.h"

using namespace color;

int botTop(const Bot &b, int u) {
  return GROUND - 10 * u - (int)b.y + (b.legs < 0 ? 2 * u : 0);
}

static void drawArm(int x, int oy, int u, int mode) {
  if (mode == 2) {
    gfx->fillRect(x, oy - 2 * u, 2 * u, 6 * u, ORANGE);
  } else {
    gfx->fillRect(x, oy + (4 - mode) * u, 2 * u, 2 * u, ORANGE);
  }
}

static void drawEye(int ex, int ey, int u, Eyes eyes) {
  int h = max(1, u / 2);
  switch (eyes) {
    case EYES_BLINK:
      gfx->fillRect(ex, ey + u + h / 2, u, h, BG);
      break;
    case EYES_CLOSED:
      gfx->fillRect(ex - h / 2, ey + u + h, u + h, max(1, u / 3), BG);
      break;
    case EYES_HAPPY: // ^
      gfx->fillRect(ex - h, ey + u, h, h, BG);
      gfx->fillRect(ex, ey + h, u, h, BG);
      gfx->fillRect(ex + u, ey + u, h, h, BG);
      break;
    case EYES_WIDE:
      gfx->fillRect(ex, ey - h, u, 2 * u + h, BG);
      break;
    default:
      gfx->fillRect(ex, ey, u, 2 * u, BG);
  }
}

void drawBot(const Bot &b, int u) {
  int ox = (int)b.x - 8 * u;
  int oy = botTop(b, u);

  gfx->fillRect(ox + 2 * u, oy, 12 * u, 8 * u, ORANGE);
  drawArm(ox, oy, u, b.armL);
  drawArm(ox + 14 * u, oy, u, b.armR);

  if (b.legs >= 0) {
    const int cols[] = { 3, 5, 10, 12 };
    for (int i = 0; i < 4; i++) {
      bool lifted = (b.legs == 1 && i % 2 == 0) || (b.legs == 2 && i % 2 == 1);
      gfx->fillRect(ox + cols[i] * u, oy + 8 * u, u, lifted ? u : 2 * u, ORANGE);
    }
  }

  for (int c : { 4, 11 }) {
    drawEye(ox + c * u + b.lookX * u / 6, oy + 2 * u + b.lookY * u / 6, u, b.eyes);
  }
}
