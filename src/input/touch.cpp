#include <Arduino.h>
#include <Wire.h>
#include "esp_lcd_touch_axs5106l.h"
#include "input/touch.h"
#include "config.h"

constexpr uint32_t RELEASE_MS = 80; // no reports for this long = finger lifted
constexpr int SWIPE_PX = 40;
constexpr int TAP_PX = 15;

static bool touching = false;
static bool longFired = false;
static uint32_t downAt = 0;
static int startX = 0, startY = 0, endX = 0, endY = 0;
static uint32_t lastSeen = 0;

void touchInit() {
  Wire.begin(pins::I2C_SDA, pins::I2C_SCL);
  bsp_touch_init(&Wire, pins::TP_RST, pins::TP_INT, ROTATION, W, H);
}

TouchEvent touchPoll(uint32_t now) {
  touch_data_t td;
  bsp_touch_read();
  if (bsp_touch_get_coordinates(&td)) {
    if (!touching) {
      touching = true;
      longFired = false;
      downAt = now;
      startX = td.coords[0].x;
      startY = td.coords[0].y;
    }
    endX = td.coords[0].x;
    endY = td.coords[0].y;
    lastSeen = now;
    if (!longFired && now - downAt > LONG_PRESS_MS && abs(endX - startX) < TAP_PX && abs(endY - startY) < TAP_PX) {
      longFired = true;
      return TOUCH_LONG;
    }
    return TOUCH_NONE;
  }
  if (!touching || now - lastSeen <= RELEASE_MS) {
    return TOUCH_NONE;
  }
  touching = false;
  if (longFired) {
    return TOUCH_NONE;
  }
  int dx = endX - startX, dy = endY - startY;
  if (abs(dx) > SWIPE_PX && abs(dx) > abs(dy)) {
    return TOUCH_SWIPE;
  }
  if (abs(dx) < TAP_PX && abs(dy) < TAP_PX) {
    return TOUCH_TAP;
  }
  return TOUCH_NONE;
}
