#include <Arduino.h>
#include "led/led.h"
#include "config.h"

// Claude orange, at full brightness
constexpr uint8_t R = 255, G = 90, B = 20;

static int lastLevel = -1;

static float breathe(uint32_t now, float periodMs) {
  return 0.5f - 0.5f * cosf(TWO_PI * now / periodMs);
}

static void setLevel(float level) {
  // squared: perceived brightness is roughly quadratic
  int l = (int)(level * level * LED_MAX);
  if (l == lastLevel) {
    return;
  }
  lastLevel = l;
  rgbLedWrite(pins::LED, R * l / 255, G * l / 255, B * l / 255);
}

void ledInit() {
  setLevel(0);
}

void ledUpdate(uint32_t now, State state, uint32_t since) {
  switch (state) {
    case THINKING:
      setLevel(0.15f + 0.85f * breathe(now, 3000));
      break;
    case WORKING:
      setLevel(0.15f + 0.85f * breathe(now, 1200));
      break;
    case ATTENTION:
      setLevel((now / 250) % 2 ? 1.0f : 0.0f);
      break;
    case DONE:
      setLevel(max(0.3f, 1.0f - (now - since) / 1500.0f));
      break;
    case SLEEP:
    case OFF:
      setLevel(0);
      break;
    default:
      setLevel(0.3f);
  }
}
