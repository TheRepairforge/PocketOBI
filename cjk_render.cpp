// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// PocketOBI — Japanese / Korean / Chinese (CJK) text engine, compiled only when ENABLE_CJK=1 (DECISIONS.md
// D19/D33/D58). PocketTFT::write() (narrow_render.cpp) decodes UTF-8 as it is printed: ASCII goes to the normal
// Adafruit_GFX path, a wide code point is blitted from the current language's 16x16 or 12x12
// bitmap table (glyphs_ja/ko/zh/zh_tw.h), scaled up for large classic-font sizes (see cjkCell()
// in cjk_render.h).
// With ENABLE_CJK=0 this file compiles to nothing.
#include "pocketobi.h"

#if ENABLE_CJK
#if ENABLE_JA
#include "glyphs_ja.h"
#endif
#if ENABLE_KO
#include "glyphs_ko.h"
#endif
#if ENABLE_ZH
#include "glyphs_zh.h"
#endif
#if ENABLE_ZH_TW
#include "glyphs_zh_tw.h"
#endif

// Vertical nudge of the 16 px cell below a GFX-font baseline, so kanji sit level with the
// FreeSans labels (tuned on the bench with the 2026-09-10 spike).
#ifndef CJK_BASELINE_DROP
#define CJK_BASELINE_DROP 3
#endif

// Binary search a code-point-sorted glyph table; nullptr if absent.
static const uint8_t* findGlyph(const CjkGlyph16* t, int n, uint32_t cp) {
  int lo = 0, hi = n - 1;
  while (lo <= hi) {
    int mid = (lo + hi) >> 1;
    if (t[mid].cp == cp) return t[mid].bmp;
    if (t[mid].cp < cp) lo = mid + 1; else hi = mid - 1;
  }
  return nullptr;
}
static const uint8_t* findGlyph(const CjkGlyph12* t, int n, uint32_t cp) {
  int lo = 0, hi = n - 1;
  while (lo <= hi) {
    int mid = (lo + hi) >> 1;
    if (t[mid].cp == cp) return t[mid].bmp;
    if (t[mid].cp < cp) lo = mid + 1; else hi = mid - 1;
  }
  return nullptr;
}

// UTF-8 decoding (PocketTFT::write) and the measuring helpers are in narrow_render.cpp: a code point
// the narrow tables do not hold arrives at drawWide() below.

// The glyph of cp in a cell, from the current language's table first, then the other ones
// (each table only holds the characters its own strings use).
struct GlyphTable { int lang; const CjkGlyph16* g16; const CjkGlyph12* g12; int n; };
static const GlyphTable TABLES[] = {
#if ENABLE_JA
  { LANG_JA, CJK_GLYPHS16_JA, CJK_GLYPHS12_JA, CJK_GLYPH_COUNT_JA },
#endif
#if ENABLE_KO
  { LANG_KO, CJK_GLYPHS16_KO, CJK_GLYPHS12_KO, CJK_GLYPH_COUNT_KO },
#endif
#if ENABLE_ZH
  { LANG_ZH, CJK_GLYPHS16_ZH, CJK_GLYPHS12_ZH, CJK_GLYPH_COUNT_ZH },
#endif
#if ENABLE_ZH_TW
  { LANG_ZH_TW, CJK_GLYPHS16_ZH_TW, CJK_GLYPHS12_ZH_TW, CJK_GLYPH_COUNT_ZH_TW },
#endif
};
static const uint8_t* wideGlyph(uint32_t cp, int cell) {
  for (int pass = 0; pass < 2; pass++)
    for (const GlyphTable& t : TABLES) {
      if ((t.lang == lang) != (pass == 0)) continue;
      const uint8_t* b = (cell == 16) ? findGlyph(t.g16, t.n, cp) : findGlyph(t.g12, t.n, cp);
      if (b) return b;
    }
  return nullptr;
}

// Draw one non-ASCII code point at the cursor and advance it, matching the current font.
// A code point missing from the glyph tables draws a hollow box (a forgotten regeneration
// is visible on the device).
void PocketTFT::drawWide(uint32_t cp) {
  int n = gfxFont ? 2 : textsize_y;               // a GFX font uses the 16 px cell, unscaled
  int w = cjkCell(n), sc = gfxFont ? 1 : cjkScale(n), adv = (w + 1) * sc;
  int top = gfxFont ? cursor_y - w + CJK_BASELINE_DROP    // GFX: cursor = baseline
          : (n == 1 ? cursor_y - 2 : cursor_y);           // classic: cursor = top of the 8n line
  const uint8_t* bmp = wideGlyph(cp, w);
  if (!gfxFont && textbgcolor != textcolor)       // classic font with a background: opaque
    fillRect(cursor_x, top, adv, w * sc, textbgcolor);
  if (!bmp)            drawRect(cursor_x, top, w * sc, w * sc, textcolor);
  else if (sc == 1)    drawBitmap(cursor_x, top, bmp, w, w, textcolor);
  else {                                          // scaled up: one sc x sc block per pixel
    const int rb = (w + 7) / 8;
    for (int y = 0; y < w; y++)
      for (int x = 0; x < w; x++)
        if (bmp[y * rb + (x >> 3)] & (0x80 >> (x & 7)))
          fillRect(cursor_x + x * sc, top + y * sc, sc, sc, textcolor);
  }
  cursor_x += adv;
}

#endif // ENABLE_CJK
