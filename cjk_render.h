// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
#pragma once
// Japanese text seam (DECISIONS.md D19/D33). Included by pocketobi.h, so every module sees
// it; it depends only on the display library and the language enum.
//
//   ENABLE_CJK=0 : PocketTFT is plain Adafruit_ST7789 and every helper below is an inline
//                  that reduces to the EN/FR/DE/ES code (LANG_NUM == LANG_COUNT,
//                  txtW() == strlen * 6 * size).
//   ENABLE_CJK=1 : PocketTFT overrides write(): UTF-8 text printed through ANY print()/
//                  printf() is decoded, ASCII keeps the normal font path, and a Japanese
//                  character is drawn from the bitmap glyph tables (glyphs_ja.h) at the
//                  height of the current font: 16 px next to a GFX font; with the classic
//                  font (8*size px), 12 px at size 1, 16 at 2, then 12 or 16 scaled up.
//                  So call sites need no change to show Japanese: tr() returns it, and
//                  the only thing a call site must not do is measure text with strlen().
//
// Measuring text: use txtW() (classic font) or gfxWidth() (GFX font) instead of strlen().
#include <Adafruit_ST7789.h>
#include "strings_i18n.h"

extern int lang;

// Languages the user can pick in Settings (STRTAB columns + Japanese in the JA build).
constexpr int LANG_NUM = LANG_COUNT + (ENABLE_CJK ? 1 : 0);

inline const char* langCode(int l) {
#if ENABLE_CJK
  if (l == LANG_JA) return "JA";
#endif
  return LANG_CODE[l];
}

#if ENABLE_CJK
// Glyph cell for classic-font text size n (line height 8n): 12 px for odd n, 16 px for
// even n, scaled by (n+1)/2 -> 12, 16, 24, 32 ... Advance = (cell + 1) * scale.
inline int cjkCell(int n)  { return (n & 1) ? 12 : 16; }
inline int cjkScale(int n) { return n < 1 ? 1 : (n + 1) / 2; }
inline int cjkAdv(int n)   { return (cjkCell(n) + 1) * cjkScale(n); }
#define CJK_ADV_GFX 17     // next to a GFX font: always the 16 px cell

class PocketTFT : public Adafruit_ST7789 {
public:
  using Adafruit_ST7789::Adafruit_ST7789;
  size_t write(uint8_t c) override;
  using Print::write;
private:
  void drawWide(uint32_t cp);
  uint32_t cp_ = 0;      // UTF-8 decoder state across write() calls
  uint8_t need_ = 0;     // continuation bytes still expected
};

int  cjkWidth(const GFXfont* f, const char* s);   // GFX-font pixel width, JA included
int  txtW(const char* s, int size);              // classic-font pixel width, JA included
bool cjkHasWide(const char* s);                  // any non-ASCII byte in s
#else
using PocketTFT = Adafruit_ST7789;
inline int txtW(const char* s, int size) { return (int)strlen(s) * 6 * size; }
#endif
