#pragma once
#include <stdint.h>

// Board: Waveshare ESP32-C6-Touch-LCD-1.47
// Pins come from the vendor demo + schematic (the wiki pin table is wrong).

// Landscape. ROTATION 1 or 3 picks which side the USB port ends up on.
#define ROTATION 1
constexpr int W = 320;
constexpr int H = 172;

namespace pins {
constexpr int LCD_DC = 15;
constexpr int LCD_CS = 14;
constexpr int LCD_SCK = 1;
constexpr int LCD_MOSI = 2;
constexpr int LCD_RST = 22;
constexpr int LCD_BL = 23;
constexpr int I2C_SDA = 18;
constexpr int I2C_SCL = 19;
constexpr int TP_RST = 20;
constexpr int TP_INT = 21;
}

// Tuning knobs
constexpr int U = 6;               // bot pixel size, the sprite is 16x10 units
constexpr int GROUND = H - 16;     // y of the bot's feet
constexpr uint32_t FRAME_MS = 33;
constexpr uint32_t SLEEP_AFTER_MS = 5 * 60 * 1000;  // idle this long -> dozes off (dimmed)
constexpr uint32_t OFF_AFTER_MS = 20 * 60 * 1000;   // idle this long in total -> standby: screen off, Wi-Fi + CPU power save
constexpr uint32_t HOLD_OFF_MS = 60 * 60 * 1000;    // long press keeps it off (ignoring hooks) this long
constexpr uint32_t LONG_PRESS_MS = 2000;
constexpr uint32_t STALE_WORK_MS = 10 * 60 * 1000; // no hook this long while busy -> idle (Esc interrupt skips Stop)
constexpr uint32_t DONE_MS = 3000;
constexpr uint32_t THINK_DEBOUNCE_MS = 1500;       // PostToolUse -> thinking waits so back-to-back tools don't flicker
constexpr uint32_t DEMO_STEP_MS = 7000;
constexpr uint32_t WIFI_RETRY_MS = 15000;    // re-run WiFi.begin this often while offline
constexpr uint32_t NO_WIFI_AFTER_MS = 20000; // "connecting" turns into "no wifi" after this
constexpr uint8_t BL_ON = 255;
constexpr uint8_t BL_SLEEP = 25;
constexpr const char *HOSTNAME = "claude-pet";
