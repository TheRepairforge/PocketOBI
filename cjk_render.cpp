// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// PocketOBI — Japanese (CJK) text engine, compiled only when ENABLE_CJK=1 (DECISIONS.md D19/D33).
// PocketTFT::write() decodes UTF-8 as it is printed: ASCII goes to the normal Adafruit_GFX
// path, a Japanese code point is blitted from the 16x16 or 12x12 bitmap table (glyphs_ja.h),
// scaled up for large classic-font sizes (see cjkCell() in cjk_render.h).
// With ENABLE_CJK=0 this file compiles to nothing.
#include "pocketobi.h"

#if ENABLE_CJK
#include "glyphs_ja.h"

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

size_t PocketTFT::write(uint8_t c) {
  if (need_) {                                    // continuation byte of a multi-byte char
    if ((c & 0xC0) != 0x80) { need_ = 0; return write(c); }   // malformed: resync on c
    cp_ = (cp_ << 6) | (c & 0x3F);
    if (--need_ == 0) drawWide(cp_);
    return 1;
  }
  if (c < 0x80) return Adafruit_ST7789::write(c);
  if      ((c & 0xE0) == 0xC0) { cp_ = c & 0x1F; need_ = 1; }
  else if ((c & 0xF0) == 0xE0) { cp_ = c & 0x0F; need_ = 2; }
  else if ((c & 0xF8) == 0xF0) { cp_ = c & 0x07; need_ = 3; }
  return 1;                                       // stray continuation byte: ignored
}

// Draw one non-ASCII code point at the cursor and advance it, matching the current font.
// A code point missing from glyphs_ja.h draws a hollow box (a forgotten regeneration
// is visible on the device).
void PocketTFT::drawWide(uint32_t cp) {
  int n = gfxFont ? 2 : textsize_y;               // a GFX font uses the 16 px cell, unscaled
  int w = cjkCell(n), sc = gfxFont ? 1 : cjkScale(n), adv = (w + 1) * sc;
  int top = gfxFont ? cursor_y - w + CJK_BASELINE_DROP    // GFX: cursor = baseline
          : (n == 1 ? cursor_y - 2 : cursor_y);           // classic: cursor = top of the 8n line
  const uint8_t* bmp = (w == 16) ? findGlyph(CJK_GLYPHS16, CJK_GLYPH_COUNT, cp)
                                 : findGlyph(CJK_GLYPHS12, CJK_GLYPH_COUNT, cp);
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

bool cjkHasWide(const char* s) {
  for (; *s; s++) if ((uint8_t)*s >= 0x80) return true;
  return false;
}

// Visit each code point of s: ASCII -> fn(c, true), other -> fn(0, false).
template <typename F> static void eachCp(const char* s, F fn) {
  for (const uint8_t* p = (const uint8_t*)s; *p; ) {
    if (*p < 0x80) { fn(*p++, true); continue; }
    p++;
    while ((*p & 0xC0) == 0x80) p++;              // skip continuation bytes
    fn(0, false);
  }
}

int cjkWidth(const GFXfont* f, const char* s) {
  int w = 0;
  eachCp(s, [&](uint8_t c, bool ascii) {
    if (!ascii) { w += CJK_ADV_GFX; return; }
    if (c >= f->first && c <= f->last) w += f->glyph[c - f->first].xAdvance;
  });
  return w;
}

int txtW(const char* s, int size) {
  int w = 0;
  eachCp(s, [&](uint8_t, bool ascii) {
    w += ascii ? 6 * size : cjkAdv(size);
  });
  return w;
}
#endif // ENABLE_CJK
