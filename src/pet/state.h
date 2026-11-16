#pragma once
#include <stdint.h>

// What Claude is doing, as reported by the Claude Code hooks
// OFF = standby, screen off; any hook event or a tap wakes it (unless held off by a long press).
enum State { IDLE, THINKING, WORKING, ATTENTION, DONE, SLEEP, OFF, STATE_COUNT };

// What the bot acts out while WORKING, picked from the tool name
enum Activity { ACT_TYPE, ACT_BUILD, ACT_SEARCH, ACT_AGENT, ACT_JUGGLE };

struct Pet {
  State state = IDLE;
  Activity activity = ACT_JUGGLE;
  uint32_t since = 0;          // when the current scene started; changes on every transition
  uint32_t lastEvent = 0;
  uint32_t pendingThinkAt = 0;
  uint32_t tapAt = 0;
  bool held = false;           // long-press off: ignores hooks and taps until HOLD_OFF_MS or another long press
  uint32_t heldAt = 0;
  char tool[24] = "";
  char msg[96] = "";           // what Claude wants to tell you (e.g. the Notification text)
  char owner[40] = "";         // session_id holding the pet while it's busy; others are ignored until it's done
  char ownerMac[12] = "";      // name of the Mac that last controlled the pet
};

extern Pet pet;

const char *stateName(State s);
bool parseState(const char *name, State &out);

// A hook event. "thinking" right after a tool is debounced.
// sid/mac identify the Claude session and Mac. The first busy session takes the pet;
// events from other sessions are dropped (returns false) until it is done/idle or goes quiet
// for STALE_WORK_MS. An empty sid (manual curl) always goes through.
bool petEvent(State s, const char *tool, const char *msg = "", const char *sid = "", const char *mac = "");
// true if a session other than sid currently holds the pet
bool petLocked(const char *sid);
// Color slot (0-3) of the controlling Mac, or -1 unless several Macs talked within MULTI_MAC_MS
int petMacSlot(uint32_t now);
void petPoke(uint32_t now);
// Long press: force OFF for HOLD_OFF_MS, or wake if already held off
void petToggleOff(uint32_t now);
// Timeouts: done -> idle -> sleep, stale busy -> idle
void petTick();
