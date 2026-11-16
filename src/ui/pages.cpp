#include "ui/pages.h"
#include "display/display.h"
#include "net/net.h"
#include "pet/props.h"
#include "pet/scenes.h"
#include "pet/speech.h"
#include "pet/state.h"
#include "usage/usage.h"

using namespace color;

static void drawHud(uint32_t now) {
  // Which Mac is in charge, only once more than one is around
  static constexpr uint16_t MAC_COLORS[4] = { PURPLE, GREEN, BLUE, PINK };
  int slot = petMacSlot(now);
  if (slot >= 0) {
    text(pet.ownerMac, 10, 10, 2, MAC_COLORS[slot]);
  }
  if (pet.state == IDLE && usage.has) {
    char n[16], buf[24];
    fmtTokens(usage.today, n, sizeof(n));
    snprintf(buf, sizeof(buf), "%s tokens", n);
    text(buf, W - 8 - textW(buf, 1), H - 11, 1, GRAY);
  }
  if (pet.state == WORKING && pet.tool[0]) {
    char shortTool[13];
    strlcpy(shortTool, pet.tool, sizeof(shortTool));
    text(shortTool, W - 10 - textW(shortTool, 2), 10, 2, GRAY);
  }
}

static void drawOffline(uint32_t now, bool searching) {
  scenesOffline(now, searching);
  drawPropsBehind(now);
  drawBot(bot, U);
  drawOfflineEffects(now, searching);
  if (searching) {
    char label[16] = "connecting";
    strncat(label, "...", (now / 400) % 4); // animated dots
    text(label, 10, 10, 2, CREAM);
  } else {
    text("no wifi", 10, 10, 2, ORANGE);
    text("retrying in the background", 10, 32, 1, GRAY);
  }
}

void drawPetPage(uint32_t now, NetStatus net) {
  if (net == NET_CONNECTING || net == NET_NO_WIFI) {
    drawOffline(now, net == NET_CONNECTING);
    return;
  }
  scenesUpdate(now);
  drawPropsBehind(now);
  drawBot(bot, U);
  drawPropsFront(now);
  drawEffects(now);
  drawHud(now);
}

// The bot gets printed in bottom-up while the ground loads like a progress bar,
// then opens its eyes and hops hello
void drawBootPage(uint32_t t) {
  constexpr uint32_t PRINT_MS = 1100, WAKE_MS = 1500;
  static bool greeted = false;
  int fill = min<uint32_t>(t, PRINT_MS) * W / PRINT_MS;
  for (int x = 0; x < fill; x += 8) {
    gfx->fillRect(x, GROUND, 4, 1, GRAY_DARK);
  }

  bot = Bot();
  bot.eyes = t < WAKE_MS ? EYES_CLOSED : EYES_HAPPY;
  if (t > WAKE_MS) {
    uint32_t h = t - WAKE_MS;
    bot.y = h < 450 ? sinf(PI * h / 450.0f) * 16 : 0;
    bot.armL = bot.armR = 2;
    if (!greeted) {
      greeted = true;
      say("Hello!", millis());
    }
  }
  drawBot(bot, U);

  if (t < PRINT_MS) { // hide what isn't printed yet, with a bright scan line at the edge
    int top = botTop(bot, U), scan = GROUND - (GROUND - top) * t / PRINT_MS;
    gfx->fillRect(0, top, W, scan - top, BG);
    gfx->fillRect((int)bot.x - 8 * U - 4, scan, 16 * U + 8, 2, CREAM);
  }
  drawSpeech(millis());
}

void drawUsagePage() {
  const int leftX = 80, rightX = 244;
  centerText("today", leftX, 26, 2, GRAY);
  if (!usage.has) {
    centerText("no data yet", W / 2, 76, 1, GRAY);
    centerText("finish a Claude turn", W / 2, 92, 1, GRAY);
    return;
  }
  char buf[32];
  fmtTokens(usage.today, buf, sizeof(buf));
  centerText(buf, leftX, 58, 4, ORANGE);
  centerText("tokens", leftX, 96, 2, CREAM);
  snprintf(buf, sizeof(buf), "%d session%s", usage.sessions, usage.sessions == 1 ? "" : "s");
  centerText(buf, leftX, 124, 1, GRAY);

  uint64_t peak = 1;
  for (uint64_t v : usage.week) {
    peak = v > peak ? v : peak;
  }
  const int base = 146, maxH = 116, bw = 18, gap = 4;
  int x0 = rightX - (7 * bw + 6 * gap) / 2;
  for (int i = 0; i < 7; i++) {
    int h = max(2, (int)(usage.week[i] * maxH / peak));
    gfx->fillRect(x0 + i * (bw + gap), base - h, bw, h, i == 6 ? ORANGE : GRAY);
  }
  centerText("last 7 days", rightX, 156, 1, GRAY);
}
