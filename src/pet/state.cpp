#include <Arduino.h>
#include <initializer_list>
#include "pet/state.h"
#include "config.h"

Pet pet;

static const char *NAMES[STATE_COUNT] = { "idle", "thinking", "working", "attention", "done", "sleep", "off" };

const char *stateName(State s) {
  return NAMES[s];
}

bool parseState(const char *name, State &out) {
  for (int i = 0; i < STATE_COUNT; i++) {
    if (!strcmp(name, NAMES[i])) {
      out = (State)i;
      return true;
    }
  }
  return false;
}

static bool isAny(const char *t, std::initializer_list<const char *> names) {
  for (const char *n : names) {
    if (!strcmp(t, n)) {
      return true;
    }
  }
  return false;
}

static Activity activityFor(const char *t) {
  if (isAny(t, { "Bash", "BashOutput", "KillShell" })) {
    return ACT_TYPE;
  }
  if (isAny(t, { "Edit", "Write", "MultiEdit", "NotebookEdit" })) {
    return ACT_BUILD;
  }
  if (isAny(t, { "Read", "Grep", "Glob", "LS", "WebFetch", "WebSearch", "ToolSearch" })) {
    return ACT_SEARCH;
  }
  if (isAny(t, { "Task", "Agent" })) {
    return ACT_AGENT;
  }
  return ACT_JUGGLE;
}

static void petSet(State s) {
  uint32_t now = millis();
  pet.lastEvent = now;
  pet.pendingThinkAt = 0;
  Activity a = s == WORKING ? activityFor(pet.tool) : pet.activity;
  if (s == pet.state && a == pet.activity) {
    return; // same scene keeps going (e.g. several Edits in a row keep stacking blocks)
  }
  pet.state = s;
  pet.activity = a;
  pet.since = now;
}

static bool busy(State s) {
  return s == THINKING || s == WORKING || s == ATTENTION;
}

bool petLocked(const char *sid) {
  return sid[0] && pet.owner[0] && strcmp(sid, pet.owner) && busy(pet.state)
    && millis() - pet.lastEvent < STALE_WORK_MS;
}

// Macs seen recently; the slot doubles as the name tag color
struct Mac {
  char name[sizeof(Pet::ownerMac)];
  uint32_t at;
};
static Mac macs[4];

static void noteMac(const char *name) {
  if (!name[0]) {
    return;
  }
  int slot = -1, oldest = 0;
  for (int i = 0; i < 4; i++) {
    if (!strncmp(macs[i].name, name, sizeof(macs[i].name) - 1)) {
      slot = i;
    }
    if (macs[i].at < macs[oldest].at) {
      oldest = i;
    }
  }
  if (slot < 0) {
    slot = oldest;
    strlcpy(macs[slot].name, name, sizeof(macs[slot].name));
  }
  macs[slot].at = millis();
}

int petMacSlot(uint32_t now) {
  if (!pet.ownerMac[0]) {
    return -1;
  }
  int recent = 0, slot = -1;
  for (int i = 0; i < 4; i++) {
    if (macs[i].at && now - macs[i].at < MULTI_MAC_MS) {
      recent++;
    }
    if (!strcmp(macs[i].name, pet.ownerMac)) {
      slot = i;
    }
  }
  return recent > 1 ? slot : -1;
}

bool petEvent(State s, const char *tool, const char *msg, const char *sid, const char *mac) {
  noteMac(mac);
  if (pet.held || petLocked(sid)) {
    return false;
  }
  if (sid[0]) {
    strlcpy(pet.owner, busy(s) ? sid : "", sizeof(pet.owner)); // done/idle hands the pet back
    strlcpy(pet.ownerMac, mac, sizeof(pet.ownerMac));
  }
  if (s == THINKING && pet.state == WORKING) {
    pet.pendingThinkAt = millis() + THINK_DEBOUNCE_MS;
    pet.lastEvent = millis();
    return true;
  }
  strlcpy(pet.tool, tool, sizeof(pet.tool));
  strlcpy(pet.msg, msg, sizeof(pet.msg));
  petSet(s);
  return true;
}

void petPoke(uint32_t now) {
  if (pet.held) {
    return;
  }
  pet.tapAt = now;
  if (pet.state == ATTENTION || pet.state == SLEEP || pet.state == OFF || pet.state == DONE) {
    pet.msg[0] = 0;
    petSet(IDLE);
  }
}

void petToggleOff(uint32_t now) {
  if (pet.held) {
    pet.held = false;
    petPoke(now);
    return;
  }
  pet.held = true;
  pet.heldAt = now;
  pet.msg[0] = 0;
  petSet(OFF);
}

void petTick() {
  // Read the clock here, not in loop(): events stamp millis() during netLoop(),
  // so an older `now` would underflow `now - pet.since` and fire every timeout at once.
  uint32_t now = millis();
  if (pet.held) {
    // stays in OFF standby afterwards, the next hook or tap wakes it as usual
    pet.held = now - pet.heldAt < HOLD_OFF_MS;
    return;
  }
  if (pet.pendingThinkAt && now > pet.pendingThinkAt) {
    pet.tool[0] = 0;
    pet.msg[0] = 0;
    petSet(THINKING);
  }
  uint32_t inState = now - pet.since;
  if (pet.state == DONE && inState > DONE_MS) {
    pet.msg[0] = 0;
    petSet(IDLE);
  } else if (pet.state == IDLE && inState > SLEEP_AFTER_MS) {
    petSet(SLEEP);
  } else if (pet.state == SLEEP && inState > OFF_AFTER_MS - SLEEP_AFTER_MS) {
    petSet(OFF);
  } else if ((pet.state == THINKING || pet.state == WORKING) && now - pet.lastEvent > STALE_WORK_MS) {
    petSet(IDLE);
  }
}
