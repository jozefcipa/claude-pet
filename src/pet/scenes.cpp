#include <Arduino.h>
#include "pet/scenes.h"
#include "pet/speech.h"
#include "pet/state.h"

Bot bot;
Scene scene;

static uint32_t nextBlink = 0, blinkStart = 0, nextGlance = 0;
static int glanceX = 0, glanceY = 0;

static void startAntic(Antic a, uint32_t minMs, uint32_t maxMs, uint32_t now) {
  scene.antic = a;
  scene.anticStart = now;
  scene.anticEnd = now + random(minMs, maxMs + 1);
  scene.anticStep = 0;
}

void scenesReset() {
  scene.pauseUntil = 0;
  scene.carrying = false;
  scene.targetX = random(50, W - 50);
  for (float &m : scene.minis) {
    m = bot.x;
  }
  startAntic(ANT_REST, 3000, 6000, millis());
}

uint32_t reactionMs() {
  static const uint32_t MS[] = { 900, 1500, 1300, 1000, 2000 }; // indexed by Reaction
  return MS[scene.reaction];
}

void scenesPoke(uint32_t now) {
  scene.tapStreak = now - scene.lastTap < TAP_STREAK_MS ? scene.tapStreak + 1 : 1;
  scene.lastTap = now;
  if (scene.tapStreak >= DIZZY_TAPS) {
    scene.reaction = REACT_DIZZY;
    scene.tapStreak = 0;
  } else {
    Reaction prev = scene.reaction;
    do {
      scene.reaction = (Reaction)random(REACT_DIZZY); // anything but dizzy, not twice in a row
    } while (scene.reaction == prev);
  }
  if (pet.state == IDLE && scene.antic == ANT_NAP) {
    scene.anticEnd = now;
  }
}

static bool walkTo(float tx, float speed, uint32_t now) {
  float d = tx - bot.x;
  if (fabsf(d) <= speed) {
    bot.x = tx;
    bot.legs = 0;
    return true;
  }
  bot.dir = d > 0 ? 1 : -1;
  bot.x += bot.dir * speed;
  bot.legs = 1 + (now / 140) % 2;
  bot.lookX = bot.dir * 3;
  return false;
}

// Pace between random points, pausing in between. Returns true while paused.
static bool pace(float speed, uint32_t pauseMin, uint32_t pauseMax, uint32_t now) {
  if (now < scene.pauseUntil) {
    return true;
  }
  if (walkTo(scene.targetX, speed, now)) {
    scene.pauseUntil = now + random(pauseMin, pauseMax);
    // next stop on the other half so it actually crosses the screen
    scene.targetX = bot.x < W / 2 ? random(W / 2 + 30, W - 50) : random(50, W / 2 - 30);
  }
  return false;
}

// ---------- idle antics ----------

struct AnticDef {
  Antic antic;
  int weight;
  uint32_t minMs, maxMs; // wander/peek end themselves when done, max is a safety net
};

static const AnticDef ANTICS[] = {
  { ANT_WANDER, 30, 30000, 30000 },
  { ANT_WAVE, 10, 2500, 2500 },
  { ANT_STRETCH, 10, 2200, 2200 },
  { ANT_DANCE, 10, 4000, 6500 },
  { ANT_NAP, 7, 15000, 30000 },
  { ANT_HOP, 10, 1100, 1100 },
  { ANT_PEEK, 8, 40000, 40000 },
  { ANT_FLY, 10, 6000, 9000 },
};
constexpr uint32_t REST_MIN_MS = 4000, REST_MAX_MS = 12000;
constexpr uint32_t FLY_ESCAPE_MS = 700;

// Rest between antics, then pick the next one by weight
static void nextAntic(uint32_t now) {
  if (scene.antic != ANT_REST) {
    speechOnAntic(scene.antic, false, now);
    startAntic(ANT_REST, REST_MIN_MS, REST_MAX_MS, now);
    return;
  }
  int total = 0;
  for (const AnticDef &d : ANTICS) {
    total += d.weight;
  }
  int r = random(total);
  for (const AnticDef &d : ANTICS) {
    r -= d.weight;
    if (r < 0) {
      startAntic(d.antic, d.minMs, d.maxMs, now);
      break;
    }
  }
  scene.targetX = random(40, W - 40);
  speechOnAntic(scene.antic, true, now);
}

static void finishAntic(uint32_t now) {
  scene.anticEnd = now;
}

// Walk off the nearest edge, hang out off-screen, come back
static void scenePeek(uint32_t now) {
  switch (scene.anticStep) {
    case 0:
      if (walkTo(bot.x < W / 2 ? -70 : W + 70, 1.5f, now)) {
        scene.anticStep = 1;
        scene.pauseUntil = now + random(2000, 5000);
      }
      break;
    case 1:
      if (now > scene.pauseUntil) {
        scene.anticStep = 2;
        scene.targetX = random(60, W - 60);
      }
      break;
    default:
      if (walkTo(scene.targetX, 1.5f, now)) {
        finishAntic(now);
      }
  }
}

bool flyPos(uint32_t now, int &x, int &y) {
  if (pet.state != IDLE || scene.antic != ANT_FLY) {
    return false;
  }
  uint32_t t = now - scene.anticStart, dur = scene.anticEnd - scene.anticStart;
  float fx = W / 2 + sinf(t / 700.0f) * 110 + sinf(t / 230.0f) * 20;
  float fy = 62 + sinf(t / 500.0f) * 18 + cosf(t / 170.0f) * 8;
  if (t + FLY_ESCAPE_MS > dur) { // zooms away
    float e = t + FLY_ESCAPE_MS - dur;
    fx += e * 0.5f;
    fy -= e * 0.15f;
  }
  x = fx;
  y = fy;
  return x < W + 10;
}

// Follows a fly with its eyes, then jumps at it and misses
static void sceneFly(uint32_t now, uint32_t t, uint32_t dur) {
  int fx = 0, fy = 0;
  flyPos(now, fx, fy);
  if (t + FLY_ESCAPE_MS > dur) {
    uint32_t e = min<uint32_t>(t + FLY_ESCAPE_MS - dur, 500);
    bot.y = sinf(PI * e / 500.0f) * 18;
    bot.armL = bot.armR = 2;
  } else {
    walkTo(constrain(fx, 50, W - 50), 1.0f, now);
  }
  bot.lookX = constrain((fx - (int)bot.x) / 10, -5, 5);
  bot.lookY = -4;
}

static void sceneIdle(uint32_t now) {
  if (now >= scene.anticEnd) {
    nextAntic(now);
  }
  uint32_t t = now - scene.anticStart;
  uint32_t dur = scene.anticEnd - scene.anticStart;
  bool beat = (t / 300) % 2;
  switch (scene.antic) {
    case ANT_WANDER:
      if (walkTo(scene.targetX, 0.7f, now)) {
        finishAntic(now);
      }
      break;
    case ANT_WAVE:
      bot.lookX = bot.lookY = 0;
      bot.armR = (t / 200) % 2 ? 2 : 1;
      break;
    case ANT_STRETCH:
      bot.armL = bot.armR = 2;
      bot.y = 2;
      if (t > 300 && t < 1800) {
        bot.eyes = EYES_CLOSED;
      }
      break;
    case ANT_DANCE:
      bot.y = fabsf(sinf(t / 150.0f)) * 6;
      bot.armL = beat ? 2 : 0;
      bot.armR = beat ? 0 : 2;
      bot.legs = beat ? 1 : 2;
      bot.lookX = beat ? 3 : -3;
      bot.eyes = EYES_HAPPY;
      break;
    case ANT_NAP:
      bot.legs = -1;
      bot.eyes = EYES_CLOSED;
      bot.y = (now / 1500) % 2;
      break;
    case ANT_HOP:
      if (t < 1050) {
        bot.y = fabsf(sinf(PI * t / 350.0f)) * 16;
        bot.armL = bot.armR = bot.y > 4 ? 2 : 0;
      }
      break;
    case ANT_PEEK:
      scenePeek(now);
      break;
    case ANT_FLY:
      sceneFly(now, t, dur);
      break;
    default:
      break; // ANT_REST: stand, blink, look around
  }
}

static void sceneThink(uint32_t now) {
  bot.armR = 1; // hand on chin
  if (pace(1.1f, 700, 1800, now)) {
    bot.lookX = -3;
    bot.lookY = -4;
  } else {
    bot.lookY = -2;
  }
}

// Bash: types on a laptop
static void sceneType(uint32_t now) {
  if (!walkTo(LAPTOP_X - 4 - 8 * U, 2.5f, now)) {
    return;
  }
  bot.dir = 1;
  bot.lookX = 4;
  bot.lookY = 3;
  bool beat = (now / 90) % 2;
  bot.armL = beat;
  bot.armR = !beat;
  bot.y = beat ? 1 : 0;
}

// Edit/Write: carries blocks from the pile to the stack
static void sceneBuild(uint32_t now) {
  bot.armL = bot.armR = scene.carrying ? 2 : 0;
  if (now < scene.pauseUntil) {
    bot.lookY = 3;
    return;
  }
  float tx = scene.carrying ? STACK_X - 8 * U - 12 : PILE_X + 8 * U + 12;
  if (walkTo(tx, 1.8f, now)) {
    if (scene.carrying) {
      scene.stackN = scene.stackN >= 6 ? 1 : scene.stackN + 1;
    }
    scene.carrying = !scene.carrying;
    scene.pauseUntil = now + 400;
  }
}

// Read/Grep/Web: wanders with a magnifying glass
static void sceneSearch(uint32_t now) {
  if (pace(0.9f, 600, 1600, now)) {
    bot.lookX = bot.dir * 4;
    bot.lookY = 4;
  } else {
    bot.lookY = 2;
  }
  (bot.dir > 0 ? bot.armR : bot.armL) = 1;
}

// Task/Agent: mini-bots follow it around
static void sceneAgent(uint32_t now) {
  pace(1.4f, 300, 900, now);
  for (int i = 0; i < 3; i++) {
    float lead = i == 0 ? bot.x : scene.minis[i - 1];
    float target = lead - bot.dir * (i == 0 ? 62 : 24);
    scene.minis[i] += (target - scene.minis[i]) * 0.08f;
  }
}

// Any other tool: juggles in the middle
static void sceneJuggle(uint32_t now) {
  if (!walkTo(W / 2, 2.5f, now)) {
    return;
  }
  bot.lookY = -4;
  bool beat = (now / 250) % 2;
  bot.armL = beat;
  bot.armR = !beat;
}

static void sceneAttention(uint32_t now) {
  if (!walkTo(W / 2, 3.5f, now)) {
    return;
  }
  bot.y = fabsf(sinf(now / 160.0f)) * 14;
  bool beat = (now / 220) % 2;
  bot.armL = beat ? 2 : 0;
  bot.armR = beat ? 0 : 2;
  bot.eyes = EYES_WIDE;
}

static void sceneDone(uint32_t inState) {
  if (inState < 700) {
    bot.y = sinf(PI * inState / 700.0f) * 26;
  }
  bot.armL = bot.armR = 2;
  bot.eyes = EYES_HAPPY;
}

static void sceneSleep(uint32_t now) {
  bot.legs = -1;
  bot.eyes = EYES_CLOSED;
  bot.y = (now / 1500) % 2; // slow breathing
}

static void baseline(uint32_t now) {
  bot.armL = bot.armR = 0;
  bot.y = 0;
  bot.legs = 0;
  if (now > nextGlance) {
    glanceX = random(-4, 5);
    glanceY = random(-3, 3);
    nextGlance = now + random(1200, 4000);
  }
  bot.lookX = glanceX;
  bot.lookY = glanceY;
  if (now > nextBlink) {
    blinkStart = now;
    nextBlink = now + random(2000, 5500);
  }
  bot.eyes = now - blinkStart < 130 ? EYES_BLINK : EYES_OPEN;
}

// A poke overrides the scene's pose for a moment (hearts, cheeks and stars are in props.cpp)
static void applyPoke(uint32_t now) {
  uint32_t t = now - pet.tapAt;
  if (t >= reactionMs() || pet.state == SLEEP) {
    return;
  }
  bot.eyes = EYES_HAPPY;
  switch (scene.reaction) {
    case REACT_HOP:
      if (t < 400) {
        bot.y += sinf(PI * t / 400.0f) * 12;
      }
      break;
    case REACT_BLUSH: // shy: hands up, looks away
      bot.armL = bot.armR = 1;
      bot.lookX = (t / 400) % 2 ? 3 : -3;
      bot.lookY = 3;
      break;
    case REACT_KISS: // hand to mouth, then throws the heart
      bot.armR = t < 350 ? 1 : 2;
      break;
    case REACT_GIGGLE: // shakes with laughter
      bot.y += (t / 70) % 2 * 3;
      bot.armL = bot.armR = (t / 140) % 2;
      break;
    case REACT_DIZZY:
      bot.eyes = EYES_DIZZY;
      bot.lookX = sinf(t / 120.0f) * 3;
      bot.legs = (t / 300) % 2 + 1; // wobbly
      break;
  }
}

void scenesOffline(uint32_t now, bool searching) {
  baseline(now);
  if (walkTo(W / 2, 2.0f, now)) {
    if (searching) {
      bot.lookX = (int)(sinf(now / 600.0f) * 5); // scanning left and right
      bot.lookY = -4;
      bot.armR = 2;                              // arm up like an antenna
      bot.armL = (now / 400) % 2;
    } else {
      bot.legs = -1;
      bot.lookX = 0;
      bot.lookY = 4;                             // looking down, a bit sad
      bot.y = (now / 2000) % 2;
    }
  }
  applyPoke(now);
}

void scenesUpdate(uint32_t now) {
  baseline(now);
  switch (pet.state) {
    case IDLE: sceneIdle(now); break;
    case THINKING: sceneThink(now); break;
    case ATTENTION: sceneAttention(now); break;
    case DONE: sceneDone(now - pet.since); break;
    case SLEEP: sceneSleep(now); break;
    case WORKING:
      switch (pet.activity) {
        case ACT_TYPE: sceneType(now); break;
        case ACT_BUILD: sceneBuild(now); break;
        case ACT_SEARCH: sceneSearch(now); break;
        case ACT_AGENT: sceneAgent(now); break;
        default: sceneJuggle(now);
      }
      break;
    default: break;
  }

  applyPoke(now);
}
