// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
#pragma once
// The text seam of every build (docs/NARROW_GLYPHS.md; DECISIONS.md D19/D33/D58 for the wide path).
//
// The display type is PocketTFT, an Adafruit_ST7789 whose write() decodes UTF-8 as it is printed:
//   - ASCII goes to the normal font path (classic 5x7 or the current GFX font), unchanged;
//   - an accented Latin or Vietnamese letter (glyphs_narrow.h) is the current font's own ASCII base
//     letter plus small mark bitmaps: same advance as the base, so the text stays proportional;
//   - with ENABLE_CJK=1, any other code point is a wide glyph (cjk_render.cpp); without it, a hollow box.
// So call sites print UTF-8 with no change. What a call site must still do: measure with txtW() (classic
// font) or gfxWidth() / cjkWidth() (GFX font), never strlen() * 6 * size, and size buffers in bytes.
//
// getTextBounds(const char*) is redeclared here: Adafruit's skips every byte >= 0x80, so it would measure
// "Réglages" without its é. It hides the base overloads, which no call site uses.
#include <Adafruit_ST7789.h>

// Panels that are not the 320x240 ST7789 (BOARD_S3_ST7796U: 320x480 native, 480x320 landscape).
// Adafruit_ST7789::init(w, h) only knows its own small panels: for any other size it computes address
// offsets that are wrong (stored as uint8_t, so large), and init(240, 320) would clip the drawing area to
// 320x240. A board with a bigger panel therefore calls init(<real native size>) and then zeroOffsets().
// A UI smaller than the panel is centred through PANEL_X_OFFSET / PANEL_Y_OFFSET (0 when it fills it).
#if defined(PANEL_W) && !defined(POCKETOBI_HOSTSIM)
#define PANEL_CUSTOM 1
class PanelST7789 : public Adafruit_ST7789 {
public:
  using Adafruit_ST7789::Adafruit_ST7789;
  void zeroOffsets() { _colstart = _rowstart = _colstart2 = _rowstart2 = 0; }
#if PANEL_X_OFFSET || PANEL_Y_OFFSET
  void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    Adafruit_ST7789::setAddrWindow(x + PANEL_X_OFFSET, y + PANEL_Y_OFFSET, w, h);
  }
#endif
  // Fills the whole PANEL_W x PANEL_H panel (landscape, no UI offset) with one colour.
  void clearPanel(uint16_t color) {
    startWrite();
    Adafruit_ST7789::setAddrWindow(0, 0, PANEL_W, PANEL_H);
    writeColor(color, (uint32_t)PANEL_W * PANEL_H);
    endWrite();
  }
};
typedef PanelST7789 PanelBaseTFT;
#else
#define PANEL_CUSTOM 0
typedef Adafruit_ST7789 PanelBaseTFT;
#endif

class PocketTFT : public PanelBaseTFT {
public:
  using PanelBaseTFT::PanelBaseTFT;
  size_t write(uint8_t c) override;
  // Every print()/printf() of a whole string arrives here: the UTF-8 decoder starts clean for each one,
  // so a string cut inside a character cannot leak into the next print.
  size_t write(const uint8_t *buf, size_t n) override;
  size_t write(const char *s) { return s ? write((const uint8_t *)s, strlen(s)) : 0; }
  void getTextBounds(const char *s, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);
  // Advance of code point cp in the current font and size (what write() moves the cursor by).
  int advanceOf(uint32_t cp);
private:
  void drawCp(uint32_t cp);
  bool drawNarrow(uint32_t cp);
  void drawMissing();
#if ENABLE_CJK
  void drawWide(uint32_t cp);                     // cjk_render.cpp
#endif
  uint32_t cp_ = 0;      // UTF-8 decoder state within ONE write(buf, n) call
  uint8_t need_ = 0;     // continuation bytes still expected
};

// Measuring (every build). The names keep their CJK-era spelling because the shared ui_components.cpp
// calls them (lockstep, D35).
int  cjkWidth(const GFXfont* f, const char* s);  // GFX-font pixel width (advances), every script
int  txtW(const char* s, int size);              // classic-font pixel width, every script
bool cjkHasWide(const char* s);                  // any non-ASCII byte in s
int  cjkCharLen(const char* s);                  // byte length of the UTF-8 char at s (1 at a NUL)
// Uppercase the UTF-8 character of cl bytes at p in place when its capital has the same length
// (é -> É, ơ -> Ơ); anything else is left as it is.
void narrowUpper(char* p, int cl);
