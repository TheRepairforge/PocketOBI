// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
#pragma once
// Japanese / Korean text seam (DECISIONS.md D19/D33/D58). Included by pocketobi.h, so every module sees
// it; it depends only on the display library and the language enum.
//
//   ENABLE_CJK=0 : PocketTFT is plain Adafruit_ST7789 and every helper below is an inline
//                  that reduces to the EN/FR/DE/ES code (txtW() == strlen * 6 * size).
//   ENABLE_CJK=1 : PocketTFT overrides write(): UTF-8 text printed through ANY print()/
//                  printf() is decoded, ASCII keeps the normal font path, and a Japanese or
//                  Korean character is drawn from the current language's bitmap glyph
//                  tables (glyphs_ja.h / glyphs_ko.h) at the
//                  height of the current font: 16 px next to a GFX font; with the classic
//                  font (8*size px), 12 px at size 1, 16 at 2, then 12 or 16 scaled up.
//                  So call sites need no change to show Japanese: tr() returns it, and
//                  the only thing a call site must not do is measure text with strlen().
//
// Measuring text: use txtW() (classic font) or gfxWidth() (GFX font) instead of strlen().
#include <Adafruit_ST7789.h>
#include "strings_i18n.h"

extern int lang;

// Languages the user can pick in Settings, in the order Settings cycles them (D58).
constexpr int LANG_AVAIL[] = {
  LANG_EN,
#if ENABLE_EU
  LANG_FR, LANG_DE, LANG_ES,
#endif
#if ENABLE_CJK
  LANG_JA, LANG_KO,
#endif
#if ENABLE_VI
  LANG_VI,
#endif
};
constexpr int LANG_NUM = (int)(sizeof(LANG_AVAIL) / sizeof(LANG_AVAIL[0]));

inline bool langAvailable(int l) {
  for (int i = 0; i < LANG_NUM; i++) if (LANG_AVAIL[i] == l) return true;
  return false;
}
// The language after l in LANG_AVAIL (wraps; an unknown l gives the first one).
inline int langNext(int l) {
  for (int i = 0; i < LANG_NUM; i++) if (LANG_AVAIL[i] == l) return LANG_AVAIL[(i + 1) % LANG_NUM];
  return LANG_AVAIL[0];
}
// A language drawn from the bitmap glyph tables (Japanese, Korean).
inline bool langWide(int l) { return ENABLE_CJK && (l == LANG_JA || l == LANG_KO); }

// Vertical nudge for classic-font (size 1) lines in a wide-glyph UI (Japanese, Korean): a 12 px cell
// spans [y-2, y+10) where an ASCII row spans [y, y+8), so a caption squeezed under a 9 pt value needs
// a couple of pixels (shared helper, lockstep D35). 0 in every other language and in the default
// build.
inline int wideDy(int px) { return langWide(lang) ? px : 0; }

inline const char* langCode(int l) {
#if ENABLE_CJK
  if (l == LANG_JA) return "JA";
  if (l == LANG_KO) return "KO";
#endif
  if (l == LANG_VI) return "VI";
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
  // Every print()/printf() of a whole string arrives here: the UTF-8 decoder starts clean
  // for each one, so a string cut inside a character cannot leak into the next print.
  size_t write(const uint8_t *buf, size_t n) override;
  size_t write(const char *s) { return s ? write((const uint8_t *)s, strlen(s)) : 0; }
private:
  void drawWide(uint32_t cp);
  uint32_t cp_ = 0;      // UTF-8 decoder state within ONE write(buf, n) call
  uint8_t need_ = 0;     // continuation bytes still expected
};

int  cjkWidth(const GFXfont* f, const char* s);   // GFX-font pixel width, JA/KO included
int  txtW(const char* s, int size);              // classic-font pixel width, JA/KO included
bool cjkHasWide(const char* s);                  // any non-ASCII byte in s
int  cjkCharLen(const char* s);                  // byte length of the UTF-8 char at s (1 at a NUL)
#else
using PocketTFT = Adafruit_ST7789;
inline int txtW(const char* s, int size) { return (int)strlen(s) * 6 * size; }
#endif
