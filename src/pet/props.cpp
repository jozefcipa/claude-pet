#include "pet/props.h"
#include "pet/scenes.h"
#include "pet/speech.h"
#include "pet/state.h"
#include "display/display.h"

using namespace color;

static void drawBlock(int x, int y) {
  gfx->fillRect(x, y, 14, 14, ORANGE_DARK);
  gfx->fillRect(x + 2, y + 2, 4, 4, ORANGE);
}

static void drawStar(int x, int y, int s, uint16_t c) {
  gfx->fillRect(x - s, y, 3 * s, s, c);
  gfx->fillRect(x, y - s, s, 3 * s, c);
}

static void drawBubble(int x, int y, const char *s) {
  gfx->fillRect(x - 10, y, 20, 22, CREAM);
  gfx->fillRect(x - 3, y + 22, 6, 4, CREAM);
  centerText(s, x + 1, y + 4, 2, BG);
}

// 7x6 pixel heart, s px per pixel
static void drawHeart(int x, int y, int s = 3) {
  static const char *rows[] = { ".XX.XX.", "XXXXXXX", "XXXXXXX", ".XXXXX.", "..XXX..", "...X..." };
  for (int r = 0; r < 6; r++) {
    for (int c = 0; c < 7; c++) {
      if (rows[r][c] == 'X') {
        gfx->fillRect(x + c * s, y + r * s, s, s, RED);
      }
    }
  }
}

static void drawLaptop(uint32_t now) {
  int x = LAPTOP_X, y = GROUND;
  gfx->fillRect(x, y - 3, 48, 3, GRAY);
  gfx->fillRect(x + 6, y - 33, 38, 30, GRAY);
  gfx->fillRect(x + 8, y - 31, 34, 26, BG);
  uint32_t scroll = now / 400;
  for (int i = 0; i < 4; i++) {
    int len = 6 + ((scroll + i) * 2654435761u >> 27); // pseudo-random line widths
    gfx->fillRect(x + 11, y - 28 + i * 5, min(len, 28), 2, GREEN);
  }
  gfx->fillRect(x + 11, y - 8, (now / 60) % 28, 2, CREAM); // line being typed
}

static void drawBlockPiles() {
  drawBlock(PILE_X - 14, GROUND - 14);
  drawBlock(PILE_X, GROUND - 14);
  drawBlock(PILE_X - 7, GROUND - 28);
  for (int i = 0; i < scene.stackN; i++) {
    drawBlock(STACK_X - 7, GROUND - 14 * (i + 1));
  }
}

static void drawMinis(uint32_t now) {
  for (int i = 0; i < 3; i++) {
    Bot m;
    m.x = scene.minis[i];
    m.dir = bot.dir;
    m.lookX = bot.dir * 3;
    m.legs = bot.legs ? 1 + (now / 140 + i) % 2 : 0;
    drawBot(m, 2);
  }
}

static void drawMagnifier() {
  int top = botTop(bot, U);
  int mx = (int)bot.x + bot.dir * (8 * U + 12);
  int my = top + 2 * U;
  gfx->fillRect(mx - bot.dir * 12, my + 10, 6, 4, GRAY);
  gfx->fillRect(mx - bot.dir * 8, my + 7, 4, 4, GRAY);
  for (int r = 7; r <= 9; r++) {
    gfx->drawCircle(mx, my, r, CREAM);
  }
}

static void drawJuggling(uint32_t now) {
  if (fabsf(bot.x - W / 2) > 1) {
    return; // still walking to the middle
  }
  int top = botTop(bot, U);
  const uint16_t colors[] = { CREAM, GREEN, RED };
  for (int i = 0; i < 3; i++) {
    float p = now / 500.0f + i * 2.0f / 3;
    float frac = p - floorf(p);
    int side = ((int)floorf(p)) % 2 ? 1 : -1; // arcs left->right, then right->left
    int bx = W / 2 + side * (int)((frac * 2 - 1) * 60);
    int by = top + 3 * U - (int)(sinf(PI * frac) * 60);
    gfx->fillRect(bx - 3, by - 3, 7, 7, colors[i]);
  }
}

static void drawThought(uint32_t now) {
  int top = botTop(bot, U);
  int side = bot.x + 70 > W ? -1 : 1;
  int bx = (int)bot.x + side * 44;
  gfx->fillRect((int)bot.x + side * 18 - 2, top - 10, 4, 4, CREAM);
  gfx->fillRect((int)bot.x + side * 26 - 3, top - 18, 6, 6, CREAM);
  gfx->fillRect(bx - 22, top - 42, 44, 20, CREAM);
  for (int i = 0; i < 3; i++) {
    int b = sinf(now / 180.0f - i * 0.9f) > 0.3f ? 3 : 0;
    gfx->fillRect(bx - 14 + i * 12, top - 34 - b, 4, 4, BG);
  }
}

static void drawConfetti(uint32_t inState) {
  int top = botTop(bot, U);
  float t = inState / 1000.0f;
  const uint16_t colors[] = { CREAM, ORANGE, GREEN, RED };
  for (int i = 0; i < 12; i++) {
    float a = i * PI / 6 + 0.3f;
    float x = bot.x + cosf(a) * 110 * t;
    float y = top + 20 - 120 * t + 160 * t * t + sinf(a) * 30 * t;
    drawStar(x, y, 2, colors[i % 4]);
  }
}

static void drawFly(uint32_t now) {
  int x, y;
  if (!flyPos(now, x, y)) {
    return;
  }
  gfx->fillRect(x - 2, y - 1, 4, 3, GRAY);
  bool flap = (now / 40) % 2; // fast wing flicker
  gfx->fillRect(x - 3, y - (flap ? 3 : 2), 2, 1, CREAM);
  gfx->fillRect(x + 1, y - (flap ? 3 : 2), 2, 1, CREAM);
}

static void drawNote(int x, int y, uint16_t c) {
  gfx->fillRect(x, y + 6, 4, 3, c);   // head
  gfx->fillRect(x + 3, y, 1, 8, c);   // stem
  gfx->fillRect(x + 4, y, 2, 2, c);   // flag
}

static void drawMusic(uint32_t now) {
  int top = botTop(bot, U);
  for (int i = 0; i < 2; i++) {
    float p = fmodf(now / 1400.0f + i * 0.5f, 1.0f);
    int side = i ? 1 : -1;
    drawNote((int)bot.x + side * (50 + p * 14), top + 10 - p * 50, i ? ORANGE : CREAM);
  }
}

static void drawZzz(uint32_t now) {
  int top = botTop(bot, U);
  float t = now / 1000.0f;
  for (int i = 0; i < 3; i++) {
    float p = fmodf(t * 0.4f + i / 3.0f, 1.0f);
    text("z", (int)bot.x + 40 + p * 22, top - 4 - p * 44, 1 + (p > 0.5f), GRAY);
  }
}

void drawPropsBehind(uint32_t now) {
  for (int x = 0; x < W; x += 8) {
    gfx->fillRect(x, GROUND, 4, 1, GRAY_DARK);
  }
  if (pet.state != WORKING) {
    return;
  }
  switch (pet.activity) {
    case ACT_TYPE: drawLaptop(now); break;
    case ACT_BUILD: drawBlockPiles(); break;
    case ACT_AGENT: drawMinis(now); break;
    default: break;
  }
}

void drawPropsFront(uint32_t now) {
  if (pet.state != WORKING) {
    return;
  }
  if (pet.activity == ACT_SEARCH) {
    drawMagnifier();
  } else if (pet.activity == ACT_JUGGLE) {
    drawJuggling(now);
  } else if (pet.activity == ACT_BUILD && scene.carrying) {
    drawBlock((int)bot.x - 7, botTop(bot, U) - 2 * U - 14);
  }
}

static void drawReaction(uint32_t now, int top) {
  uint32_t t = now - pet.tapAt;
  if (t >= reactionMs() || pet.state == SLEEP) {
    return;
  }
  int x = (int)bot.x;
  int side = x > W / 2 ? -1 : 1; // the side with more room
  switch (scene.reaction) {
    case REACT_HOP: // a small heart floats up beside it
      drawHeart(x + side * 60 - 10, top + 10 - t / 30);
      break;
    case REACT_BLUSH: // pink cheeks under the eyes
      for (int c : { 2, 12 }) {
        gfx->fillRect(x - 8 * U + c * U, top + 5 * U, 2 * U, U, PINK);
      }
      break;
    case REACT_KISS: { // a heart flies out, growing as if coming at you
      if (t < 350) {
        break;
      }
      float p = (t - 350) / (float)(reactionMs() - 350);
      int s = 2 + p * 6;
      drawHeart(x + side * (40 + p * 70) - 7 * s / 2, top + 10 - p * 50 - 3 * s, s);
      break;
    }
    case REACT_GIGGLE: // little ha-ha sparkles
      if ((t / 150) % 2) {
        drawStar(x - side * 58, top + 4, 2, CREAM);
        drawStar(x + side * 58, top + 14, 2, CREAM);
      }
      break;
    case REACT_DIZZY: // stars circling the head
      for (int i = 0; i < 3; i++) {
        float a = t / 200.0f + i * 2 * PI / 3;
        drawStar(x + cosf(a) * 40, top - 6 + sinf(a) * 6, 2, i ? CREAM : ORANGE);
      }
      break;
  }
}

void drawEffects(uint32_t now) {
  int top = botTop(bot, U);
  uint32_t inState = now - pet.since;
  bool talking = speaking(now); // a speech bubble replaces the small thought/!/? bubbles
  if (pet.state == ATTENTION && (now / 500) % 2) {
    for (int i = 0; i < 3; i++) {
      gfx->drawRect(i, i, W - 2 * i, H - 2 * i, ORANGE);
    }
  }
  if (pet.state == THINKING && !talking) {
    drawThought(now);
  } else if (pet.state == ATTENTION && !talking) {
    drawBubble((int)bot.x, top - 48, "!");
  } else if (pet.state == WORKING && pet.activity == ACT_SEARCH && now < scene.pauseUntil && !talking) {
    drawBubble((int)bot.x, top - 34, scene.pauseUntil % 3 ? "?" : "!");
  } else if (pet.state == SLEEP || (pet.state == IDLE && scene.antic == ANT_NAP)) {
    drawZzz(now);
  } else if (pet.state == IDLE && scene.antic == ANT_DANCE) {
    drawMusic(now);
  } else if (pet.state == IDLE && scene.antic == ANT_FLY) {
    drawFly(now);
  } else if (pet.state == DONE && inState < 1800) {
    drawConfetti(inState);
  }
  drawReaction(now, top);
  drawSpeech(now);
}

// Wi-Fi symbol: a dot plus three arcs opening upwards; `bars` of them lit
static void drawWifiIcon(int x, int y, int bars, uint16_t on, uint16_t off) {
  gfx->fillCircle(x, y, 3, bars > 0 ? on : off);
  for (int i = 0; i < 3; i++) {
    int r = 10 + i * 8;
    gfx->fillArc(x, y, r, r - 4, 225, 315, i + 1 < bars ? on : off);
  }
}

void drawOfflineEffects(uint32_t now, bool searching) {
  int top = botTop(bot, U);
  int x = (int)bot.x, y = top - 14;
  if (searching) {
    drawWifiIcon(x, y, (now / 350) % 5, CREAM, GRAY_DARK);
  } else {
    drawWifiIcon(x, y, 4, GRAY, GRAY);
    for (int d = -1; d <= 1; d++) { // thick slash
      gfx->drawLine(x - 18 + d, y - 30, x + 18 + d, y + 4, RED);
    }
  }
  drawSpeech(now);
}
