#include "ui/pages.h"
#include "display/display.h"
#include "net/net.h"
#include "pet/props.h"
#include "pet/scenes.h"
#include "pet/state.h"
#include "usage/usage.h"

using namespace color;

static void drawHud() {
  char buf[24];
  if (pet.state == IDLE || pet.state == SLEEP) {
    if (!netConnected()) {
      text("no wifi", 8, 8, 1, GRAY);
    }
    if (usage.has && pet.state == IDLE) {
      char n[16];
      fmtTokens(usage.today, n, sizeof(n));
      snprintf(buf, sizeof(buf), "%s tokens", n);
      text(buf, W - 8 - textW(buf, 1), H - 11, 1, GRAY);
    }
    return;
  }
  const char *label = pet.state == THINKING ? "thinking"
    : pet.state == WORKING ? "working"
    : pet.state == ATTENTION ? "needs you!"
    : "done!";
  text(label, 10, 10, 2, pet.state == ATTENTION ? ORANGE : CREAM);
  if (pet.state == WORKING && pet.tool[0]) {
    char shortTool[13];
    strlcpy(shortTool, pet.tool, sizeof(shortTool));
    text(shortTool, W - 10 - textW(shortTool, 2), 10, 2, GRAY);
  }
}

void drawPetPage(uint32_t now) {
  scenesUpdate(now);
  drawPropsBehind(now);
  drawBot(bot, U);
  drawPropsFront(now);
  drawEffects(now);
  drawHud();
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
