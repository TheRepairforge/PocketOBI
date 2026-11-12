// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
#pragma once
// Language set + Japanese / Korean / Chinese text seam (DECISIONS.md D19/D33/D58). Included by pocketobi.h,
// so every module sees it; it depends only on the display library and the language enum.
//
// The display type PocketTFT and the measuring helpers (txtW(), cjkWidth() ...) live in narrow_render.h and
// exist in every build: UTF-8 is decoded everywhere, accented Latin / Vietnamese take the narrow path.
//   ENABLE_CJK=1 adds the wide path: a Japanese, Korean or Chinese character is drawn from the current
//                language's bitmap glyph tables (glyphs_ja/ko/zh/zh_tw.h) at the height of the current
//                font: 16 px next to a GFX font; with the classic font (8*size px), 12 px at size 1, 16 at
//                2, then 12 or 16 scaled up.
// So call sites need no change: tr() returns UTF-8, and the only thing a call site must not do is measure
// text with strlen(). Use txtW() (classic font) or gfxWidth() (GFX font).
#include <Adafruit_ST7789.h>
#include "strings_i18n.h"
#include "narrow_render.h"   // PocketTFT, txtW(), cjkWidth(), cjkCharLen() ... in every build

extern int lang;

// Languages the user can pick in Settings, in the order Settings cycles them (D58).
constexpr int LANG_AVAIL[] = {
  LANG_EN,
#if ENABLE_EU
  LANG_FR, LANG_DE, LANG_ES,
#endif
#if ENABLE_PL
  LANG_PL,
#endif
#if ENABLE_JA
  LANG_JA,
#endif
#if ENABLE_KO
  LANG_KO,
#endif
#if ENABLE_ZH
  LANG_ZH,
#endif
#if ENABLE_ZH_TW
  LANG_ZH_TW,
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
// A language drawn from the bitmap glyph tables (Japanese, Korean, Simplified and Traditional Chinese).
inline bool langWide(int l) { return ENABLE_CJK && (l == LANG_JA || l == LANG_KO || l == LANG_ZH || l == LANG_ZH_TW); }

// Vertical nudge for classic-font (size 1) lines in a wide-glyph UI (Japanese, Korean): a 12 px cell
// spans [y-2, y+10) where an ASCII row spans [y, y+8), so a caption squeezed under a 9 pt value needs
// a couple of pixels (shared helper, lockstep D35). 0 in every other language and in the default
// build.
inline int wideDy(int px) { return langWide(lang) ? px : 0; }

inline const char* langCode(int l) {
#if ENABLE_CJK
  if (l == LANG_JA) return "JA";
  if (l == LANG_KO) return "KO";
  if (l == LANG_ZH) return "ZH";
  if (l == LANG_ZH_TW) return "TW";
#endif
  if (l == LANG_VI) return "VI";
  if (l == LANG_PL) return "PL";
  return LANG_CODE[l];
}

#if ENABLE_CJK
// Glyph cell for classic-font text size n (line height 8n): 12 px for odd n, 16 px for
// even n, scaled by (n+1)/2 -> 12, 16, 24, 32 ... Advance = (cell + 1) * scale.
inline int cjkCell(int n)  { return (n & 1) ? 12 : 16; }
inline int cjkScale(int n) { return n < 1 ? 1 : (n + 1) / 2; }
inline int cjkAdv(int n)   { return (cjkCell(n) + 1) * cjkScale(n); }
#define CJK_ADV_GFX 17     // next to a GFX font: always the 16 px cell
#endif
