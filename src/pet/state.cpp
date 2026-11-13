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

void petEvent(State s, const char *tool, const char *msg) {
  if (s == THINKING && pet.state == WORKING) {
    pet.pendingThinkAt = millis() + THINK_DEBOUNCE_MS;
    pet.lastEvent = millis();
    return;
  }
  strlcpy(pet.tool, tool, sizeof(pet.tool));
  strlcpy(pet.msg, msg, sizeof(pet.msg));
  petSet(s);
}

void petPoke(uint32_t now) {
  pet.tapAt = now;
  if (pet.state == ATTENTION || pet.state == SLEEP || pet.state == OFF || pet.state == DONE) {
    pet.msg[0] = 0;
    petSet(IDLE);
  }
}

void petTick() {
  // Read the clock here, not in loop(): events stamp millis() during netLoop(),
  // so an older `now` would underflow `now - pet.since` and fire every timeout at once.
  uint32_t now = millis();
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
