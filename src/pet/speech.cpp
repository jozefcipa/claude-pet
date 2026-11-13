#include <Arduino.h>
#include "pet/speech.h"
#include "pet/scenes.h"
#include "display/display.h"

using namespace color;

constexpr int TEXT_SIZE = 2;   // 12x16px glyphs
constexpr int LINE_CHARS = 24; // 24 * 12px + padding fits the 320px screen
constexpr int MAX_LINES = 3;

// The terminal's spinner verb isn't passed to hooks, so the pet has its own
static const char *const THINK[] = {
  "Hmm...", "On it!", "Scampering...", "Pondering...", "Noodling...", "Percolating...",
  "Tinkering...", "Mulling...", "Brewing...", "Cogitating...", "Puzzling...", "Wrangling...",
};
static const char *const DONE_LINES[] = { "All done!", "Ta-da!", "Finished!", "Your turn!" };
static const char *const ATTN[] = { "Hey! I need you", "Psst... over here!", "Can you take a look?" };
static const char *const WAKE[] = { "I'm up!", "Oh hi!", "Back to work?" };
static const char *const POKE[] = { "Hey!", "Hehe", "That tickles!", "Hi there!" };
static const char *const HUM[] = { "*hums*", "...", "Need anything?" };
static const char *const WAVE[] = { "Hi!", "Hello!", "Hey you!" };
static const char *const STRETCH[] = { "*stretch*", "Ahh..." };
static const char *const DANCE[] = { "*la la la*", "Dance break!" };
static const char *const NAP_START[] = { "*yawn*", "Quick nap..." };
static const char *const NAP_END[] = { "...huh?", "I'm awake!" };
static const char *const HOP[] = { "Boing!", "Wheee!" };
static const char *const PEEK_START[] = { "brb", "Be right back" };
static const char *const PEEK_END[] = { "I'm back!", "Miss me?" };
static const char *const FLY_START[] = { "A fly!", "Bzzz?" };
static const char *const FLY_END[] = { "Missed...", "Next time!" };
static const char *const TYPE[] = { "Running a command", "Typing away..." };
static const char *const BUILD[] = { "Editing files", "Building stuff!" };
static const char *const SEARCH[] = { "Looking around...", "Where is it..." };
static const char *const AGENT[] = { "Calling my friends!", "Team effort!" };
static const char *const JUGGLE[] = { "Using a tool", "Look, juggling!" };

static char line[96] = "";
static uint32_t until = 0;

template <size_t N>
static const char *pick(const char *const (&options)[N]) {
  return options[random(N)];
}

static bool chance(int pct) {
  return random(100) < pct;
}

void say(const char *msg, uint32_t now, uint32_t durationMs) {
  strlcpy(line, msg, sizeof(line));
  until = now + (durationMs ? durationMs : min<uint32_t>(1500 + strlen(line) * 70, 7000));
}

bool speaking(uint32_t now) {
  return line[0] && now < until;
}

static const char *workLine() {
  switch (pet.activity) {
    case ACT_TYPE: return pick(TYPE);
    case ACT_BUILD: return pick(BUILD);
    case ACT_SEARCH: return pick(SEARCH);
    case ACT_AGENT: return pick(AGENT);
    default: return pick(JUGGLE);
  }
}

void speechOnScene(State prev, uint32_t now) {
  until = 0;
  if (pet.msg[0]) {
    say(pet.msg, now, 8000);
    return;
  }
  switch (pet.state) {
    case THINKING:
      if (chance(prev == WORKING ? 15 : 60)) {
        say(pick(THINK), now);
      }
      break;
    case WORKING:
      if (chance(40)) {
        say(workLine(), now);
      }
      break;
    case ATTENTION: say(pick(ATTN), now, 8000); break;
    case DONE: say(pick(DONE_LINES), now); break;
    case SLEEP: say("*yawn*", now); break;
    case IDLE:
      if (prev == SLEEP || prev == OFF) {
        say(pick(WAKE), now);
      }
      break;
    default: break;
  }
}

void speechOnPoke(uint32_t now) {
  if (!speaking(now) || pet.state == IDLE) {
    say(pick(POKE), now);
  }
}

void speechOnAntic(Antic a, bool starting, uint32_t now) {
  const char *s = nullptr;
  int pct = 50;
  if (starting) {
    switch (a) {
      case ANT_WANDER: s = pick(HUM); pct = 10; break;
      case ANT_WAVE: s = pick(WAVE); pct = 70; break;
      case ANT_STRETCH: s = pick(STRETCH); break;
      case ANT_DANCE: s = pick(DANCE); break;
      case ANT_NAP: s = pick(NAP_START); pct = 60; break;
      case ANT_HOP: s = pick(HOP); pct = 30; break;
      case ANT_PEEK: s = pick(PEEK_START); pct = 40; break;
      case ANT_FLY: s = pick(FLY_START); break;
      default: break;
    }
  } else {
    switch (a) {
      case ANT_NAP: s = pick(NAP_END); pct = 60; break;
      case ANT_PEEK: s = pick(PEEK_END); break;
      case ANT_FLY: s = pick(FLY_END); break;
      default: break;
    }
  }
  if (s && chance(pct)) {
    say(s, now);
  }
}

// Greedy word wrap; words longer than a line get cut
static int wrap(const char *s, char out[MAX_LINES][LINE_CHARS + 1]) {
  char buf[sizeof(line)];
  strlcpy(buf, s, sizeof(buf));
  int n = 0;
  out[0][0] = 0;
  for (char *w = strtok(buf, " "); w; w = strtok(nullptr, " ")) {
    size_t len = strlen(out[n]);
    if (len && len + 1 + strlen(w) > LINE_CHARS) {
      if (++n == MAX_LINES) {
        return MAX_LINES;
      }
      out[n][0] = 0;
      len = 0;
    }
    if (len) {
      strlcat(out[n], " ", LINE_CHARS + 1);
    }
    strlcat(out[n], w, LINE_CHARS + 1);
  }
  return n + 1;
}

void drawSpeech(uint32_t now) {
  if (!speaking(now)) {
    return;
  }
  char lines[MAX_LINES][LINE_CHARS + 1];
  int n = wrap(line, lines);
  int longest = 0;
  for (int i = 0; i < n; i++) {
    longest = max(longest, (int)strlen(lines[i]));
  }
  const int charW = 6 * TEXT_SIZE, lineH = 8 * TEXT_SIZE + 4, pad = 8;
  int w = longest * charW + 2 * pad, h = n * lineH - 4 + 2 * pad;
  int top = botTop(bot, U);
  int bx = constrain((int)bot.x - w / 2, 4, W - 4 - w);
  int minY = pet.state == IDLE || pet.state == SLEEP ? 4 : 30; // stay below the status line
  int by = max(minY, top - 16 - h);

  // pixel-art bubble: clipped corners + a stepped tail toward the bot
  gfx->fillRect(bx + 2, by, w - 4, h, CREAM);
  gfx->fillRect(bx, by + 2, w, h - 4, CREAM);
  int tx = constrain((int)bot.x - 4, bx + 8, bx + w - 16);
  gfx->fillRect(tx, by + h, 8, 4, CREAM);
  gfx->fillRect(tx + 2, by + h + 4, 4, 4, CREAM);

  for (int i = 0; i < n; i++) {
    text(lines[i], bx + pad, by + pad + i * lineH, TEXT_SIZE, BG);
  }
}
