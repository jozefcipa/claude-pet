#pragma once
#include <Arduino_GFX_Library.h>

// Everything draws into this off-screen canvas; displayFlush() pushes it to the panel.
extern Arduino_Canvas *gfx;

namespace color {
constexpr uint16_t BG = RGB565(0x0B, 0x0B, 0x0D);
constexpr uint16_t ORANGE = RGB565(0xCC, 0x7A, 0x5A);
constexpr uint16_t ORANGE_DARK = RGB565(0x9A, 0x52, 0x38);
constexpr uint16_t CREAM = RGB565(0xF0, 0xEE, 0xE6);
constexpr uint16_t GRAY = RGB565(0x80, 0x7D, 0x76);
constexpr uint16_t GRAY_DARK = RGB565(0x3A, 0x38, 0x35);
constexpr uint16_t GREEN = RGB565(0x7A, 0xC9, 0x6E);
constexpr uint16_t RED = RGB565(0xE0, 0x5A, 0x5A);
}

void displayInit();
void displayBacklight(uint8_t level);
void displayPower(bool on); // panel sleep in/out; set the backlight again after waking
void displayFlush();

// Text in the built-in 6x8 font
void text(const char *s, int x, int y, int size, uint16_t color);
int textW(const char *s, int size);
void centerText(const char *s, int x, int y, int size, uint16_t color);
