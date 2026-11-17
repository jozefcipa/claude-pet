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
    char buf[24];
    snprintf(buf, sizeof(buf), "%d%% used", usagePct(usage.session));
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

// Card with a big percentage, a label pill, a progress bar and the reset countdown
static void drawLimit(const Limit &l, const char *label, int y, uint16_t fill) {
  constexpr uint16_t CARD = RGB565(0x1E, 0x1D, 0x20);
  const int x = 8, w = W - 16, h = 74, pad = 10;
  gfx->fillRoundRect(x, y, w, h, 8, CARD);

  int pct = usagePct(l);
  char buf[24];
  snprintf(buf, sizeof(buf), "%d%%", pct);
  text(buf, x + pad, y + 7, 3, CREAM);

  int pillW = textW(label, 2) + 16;
  gfx->fillRoundRect(x + w - pad - pillW, y + 8, pillW, 22, 11, GRAY_DARK);
  text(label, x + w - pad - pillW + 8, y + 12, 2, CREAM);

  const int barW = w - 2 * pad;
  gfx->fillRoundRect(x + pad, y + 36, barW, 10, 5, GRAY_DARK);
  if (pct > 0) {
    gfx->fillRoundRect(x + pad, y + 36, max(10, barW * pct / 100), 10, 5, pct >= 90 ? RED : fill);
  }

  uint32_t left = usageResetIn(l);
  if (left) {
    char d[16];
    fmtDuration(left, d, sizeof(d));
    snprintf(buf, sizeof(buf), "Resets in %s", d);
  } else {
    strlcpy(buf, "Fresh window", sizeof(buf));
  }
  text(buf, x + pad, y + 53, 2, GRAY);
}

void drawUsagePage() {
  if (!usage.has) {
    centerText("no data yet", W / 2, 76, 1, GRAY);
    centerText("finish a Claude turn", W / 2, 92, 1, GRAY);
    return;
  }
  drawLimit(usage.session, "Current", 8, ORANGE);
  drawLimit(usage.week, "Weekly", 90, GREEN);
}
