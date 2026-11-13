// Claude pet: a desk buddy that acts out what Claude Code is doing.
// main only wires the modules together and runs the frame loop.
#include <Arduino.h>
#include "config.h"
#include "display/display.h"
#include "input/touch.h"
#include "net/net.h"
#include "pet/demo.h"
#include "pet/scenes.h"
#include "pet/speech.h"
#include "pet/state.h"
#include "ui/pages.h"

static Page page = PAGE_PET;
static uint32_t sceneSince = UINT32_MAX;
static State prevState = IDLE;
static NetStatus prevNet = NET_DEMO;

static void onSceneChange(uint32_t now) {
  scenesReset();
  if (pet.state == OFF) {
    displayPower(false);
    netPowerSave(true);
  } else {
    if (prevState == OFF) {
      displayPower(true);
      netPowerSave(false);
    }
    displayBacklight(pet.state == SLEEP ? BL_SLEEP : BL_ON);
  }
  if (pet.state == ATTENTION) {
    page = PAGE_PET;
  }
  speechOnScene(prevState, now);
  prevState = pet.state;
}

// The bot reacts when Wi-Fi comes and goes
static void onNetChange(NetStatus net, uint32_t now) {
  if (net == NET_ONLINE && prevNet != NET_DEMO) {
    say("Connected!", now);
    pet.tapAt = now; // happy hop
  } else if (net == NET_CONNECTING && prevNet == NET_ONLINE) {
    say("Lost Wi-Fi!", now);
  } else if (net == NET_NO_WIFI) {
    say("Can't find Wi-Fi...", now);
  }
  if (net != NET_ONLINE && pet.state == OFF) {
    petPoke(now); // wake the screen so the problem is visible
  }
  prevNet = net;
}

static void handleInput(uint32_t now) {
  switch (touchPoll(now)) {
    case TOUCH_TAP:
      if (page == PAGE_PET || pet.state == OFF) {
        petPoke(now);
        scenesPoke(now);
        speechOnPoke(now);
      }
      break;
    case TOUCH_SWIPE:
      page = page == PAGE_PET ? PAGE_USAGE : PAGE_PET;
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  randomSeed(esp_random());
  displayInit();
  touchInit();
  netInit();
}

void loop() {
  static uint32_t lastFrame = 0;
  uint32_t now = millis();

  netLoop();
  NetStatus net = netStatus(now);
  if (net != prevNet) {
    onNetChange(net, now);
  }
  handleInput(now);
  if (!netConfigured()) {
    demoTick(now);
  }
  petTick(now);
  if (pet.since != sceneSince) {
    sceneSince = pet.since;
    onSceneChange(now);
  }

  if (pet.state == OFF) {
    delay(20); // nothing to draw; keep polling Wi-Fi and touch for a wake-up
    return;
  }
  if (now - lastFrame < FRAME_MS) {
    return;
  }
  lastFrame = now;
  gfx->fillScreen(color::BG);
  if (page == PAGE_PET) {
    drawPetPage(now, net);
  } else {
    drawUsagePage();
  }
  displayFlush();
}
