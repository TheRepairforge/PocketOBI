// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// The shared drawing vocabulary's implementation (see ui_components.h; docs/platforms/
// UI_DESIGN_SYSTEM.md, DECISIONS.md D35/D46). BYTE-IDENTICAL across the firmware trees:
// edit one, copy it to the other (tools/check_structure.py fails otherwise). The COL_* values come
// from the tree's generated ui_palette.h, every size from the generated ui_theme.h.
#include "ui_components.h"

// smooth-font helpers (labels/chrome). Classic font stays for data/values.
// In the JA build the print draws Japanese itself (PocketTFT); only the width differs, because
// getTextBounds() does not know the bitmap glyphs (it would count every kanji as 0 px).
int gfxWidth(const GFXfont *f, const char *s) {
#if ENABLE_CJK
  if (cjkHasWide(s)) return cjkWidth(f, s);
#endif
  tft.setFont(f); tft.setTextSize(1);
  int16_t x1, y1; uint16_t w, h; tft.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  tft.setFont(NULL);
  return w;
}
int gfxText(const GFXfont *f, int x, int baseY, const char *s, uint16_t col) {
#if ENABLE_CJK
  if (cjkHasWide(s)) {
    tft.setFont(f); tft.setTextSize(1); tft.setTextColor(col);
    tft.setCursor(x, baseY); tft.print(s); tft.setFont(NULL);
    return cjkWidth(f, s);
  }
#endif
  tft.setFont(f); tft.setTextSize(1); tft.setTextColor(col);
  int16_t x1, y1; uint16_t w, h; tft.getTextBounds(s, 0, baseY, &x1, &y1, &w, &h);
  tft.setCursor(x, baseY); tft.print(s); tft.setFont(NULL);
  return w;
}
void gfxCenter(const GFXfont *f, int cx, int baseY, const char *s, uint16_t col) {
#if ENABLE_CJK
  if (cjkHasWide(s)) { gfxText(f, cx - cjkWidth(f, s) / 2, baseY, s, col); return; }
#endif
  tft.setFont(f); tft.setTextSize(1); tft.setTextColor(col);
  int16_t x1, y1; uint16_t w, h; tft.getTextBounds(s, 0, baseY, &x1, &y1, &w, &h);
  tft.setCursor(cx - w / 2, baseY); tft.print(s); tft.setFont(NULL);
}

// Uppercased copy of a UI label (the mockup's small-caps label style). Only ASCII letters change;
// a multi-byte (Japanese) character is copied whole or not at all, never cut by the buffer end.
const char *upperLabel(const char *in, char *out, size_t n) {
  size_t i = 0;
  while (in[i]) {
    int cl = 1;
#if ENABLE_CJK
    cl = cjkCharLen(in + i);
#endif
    if (i + cl + 1 > n) break;
    if (cl == 1) out[i] = (in[i] >= 'a' && in[i] <= 'z') ? in[i] - 32 : in[i];
    else memcpy(out + i, in + i, cl);
    i += cl;
  }
  out[i] = 0;
  return out;
}

void drawCaption(int x, int y, const char *s, uint16_t col) {
  tft.setFont(NULL); tft.setTextSize(1); tft.setTextColor(col);
  tft.setCursor(x, y); tft.print(s);
}

// ---------- Icons ----------
void iconBattery(int cx, int cy, uint16_t c, int scale) {
  tft.drawRect(cx - 8 * scale, cy - 5 * scale, 13 * scale, 10 * scale, c);
  tft.fillRect(cx + 5 * scale, cy - 2 * scale, 2 * scale, 4 * scale, c);   // + terminal nub
  for (int k = 0; k < 3; k++) tft.fillRect(cx + (-6 + k * 3) * scale, cy - 3 * scale, 2 * scale, 6 * scale, c);
}
void iconBanks(int cx, int cy, uint16_t c, int scale) {   // ascending bars = the two-bank/cell view
  tft.fillRect(cx - 7 * scale, cy + 2 * scale, 4 * scale, 6 * scale, c);
  tft.fillRect(cx - 1 * scale, cy - 3 * scale, 4 * scale, 11 * scale, c);
  tft.fillRect(cx + 5 * scale, cy - 7 * scale, 4 * scale, 15 * scale, c);
}
void iconKey(int cx, int cy, uint16_t c, int scale) {     // repair / reset
  tft.drawCircle(cx - 4 * scale, cy, 4 * scale, c);       // bow
  tft.drawFastHLine(cx, cy, 9 * scale, c);                // shaft
  tft.drawFastVLine(cx + 6 * scale, cy, 4 * scale, c);    // tooth
  tft.drawFastVLine(cx + 8 * scale, cy, 3 * scale, c);    // tooth
}
void iconList(int cx, int cy, uint16_t c, int scale) {    // usage table / list
  for (int k = 0; k < 3; k++) {
    int yy = cy + (-5 + k * 5) * scale;
    tft.fillRect(cx - 8 * scale, yy, 2 * scale, 2 * scale, c);
    tft.drawFastHLine(cx - 4 * scale, yy + 1 * scale, 11 * scale, c);
  }
}
void iconInfo(int cx, int cy, uint16_t c, int scale) {    // About
  tft.drawCircle(cx, cy, 7 * scale, c);
  tft.fillRect(cx - 1 * scale, cy - 4 * scale, 2 * scale, 2 * scale, c);   // dot
  tft.fillRect(cx - 1 * scale, cy - 1 * scale, 2 * scale, 5 * scale, c);   // stem
}
void iconBridge(int cx, int cy, uint16_t c, int scale) {  // PC bridge = two arrows
  tft.drawFastHLine(cx - 7 * scale, cy - 3 * scale, 12 * scale, c);
  tft.drawLine(cx + 5 * scale, cy - 3 * scale, cx + 2 * scale, cy - 6 * scale, c);
  tft.drawLine(cx + 5 * scale, cy - 3 * scale, cx + 2 * scale, cy, c);
  tft.drawFastHLine(cx - 5 * scale, cy + 3 * scale, 12 * scale, c);
  tft.drawLine(cx - 5 * scale, cy + 3 * scale, cx - 2 * scale, cy + 6 * scale, c);
  tft.drawLine(cx - 5 * scale, cy + 3 * scale, cx - 2 * scale, cy, c);
}
void iconCode(int cx, int cy, uint16_t c, int scale) {    // Debug / raw = "<>"
  tft.drawLine(cx - 2 * scale, cy - 5 * scale, cx - 7 * scale, cy, c);
  tft.drawLine(cx - 7 * scale, cy, cx - 2 * scale, cy + 5 * scale, c);
  tft.drawLine(cx + 2 * scale, cy - 5 * scale, cx + 7 * scale, cy, c);
  tft.drawLine(cx + 7 * scale, cy, cx + 2 * scale, cy + 5 * scale, c);
}
void iconSettings(int cx, int cy, uint16_t c, int scale) {  // sliders glyph
  int gx = cx, gy = cy;
  tft.drawFastHLine(gx - 8 * scale, gy - 4 * scale, 16 * scale, c); tft.fillCircle(gx - 2 * scale, gy - 4 * scale, 2 * scale, c);
  tft.drawFastHLine(gx - 8 * scale, gy,             16 * scale, c); tft.fillCircle(gx + 4 * scale, gy,             2 * scale, c);
  tft.drawFastHLine(gx - 8 * scale, gy + 4 * scale, 16 * scale, c); tft.fillCircle(gx - 4 * scale, gy + 4 * scale, 2 * scale, c);
}
void iconThermo(int cx, int cy, uint16_t c, int scale) {  // thermometer = Temperature KPI
  tft.drawRoundRect(cx - 2 * scale, cy - 6 * scale, 4 * scale, 9 * scale, 2 * scale, c);
  tft.fillCircle(cx, cy + 5 * scale, 3 * scale, c);
}
void iconRefresh(int cx, int cy, uint16_t c, int scale) {  // cycle arrow = Charges KPI
  tft.drawCircle(cx, cy, 6 * scale, c);
  tft.fillTriangle(cx + 6 * scale, cy - 3 * scale, cx + 10 * scale, cy, cx + 6 * scale, cy + 3 * scale, c);
}
void iconDoc(int cx, int cy, uint16_t c, int scale) {      // document/tag = Model KPI
  tft.drawRoundRect(cx - 5 * scale, cy - 6 * scale, 10 * scale, 12 * scale, 2 * scale, c);
  tft.drawFastHLine(cx - 2 * scale, cy - 2 * scale, 4 * scale, c);
  tft.drawFastHLine(cx - 2 * scale, cy + 1 * scale, 4 * scale, c);
}
void iconCalendar(int cx, int cy, uint16_t c, int scale) {  // Identity "Produced"
  tft.drawRoundRect(cx - 6 * scale, cy - 5 * scale, 12 * scale, 11 * scale, 1 * scale, c);
  tft.drawFastHLine(cx - 6 * scale, cy - 2 * scale, 12 * scale, c);   // header strip
  tft.drawFastVLine(cx - 3 * scale, cy - 6 * scale, 2 * scale, c);    // binding tabs
  tft.drawFastVLine(cx + 3 * scale, cy - 6 * scale, 2 * scale, c);
}
void iconWarning(int cx, int cy, uint16_t c, int scale) {   // Faults list entries
  tft.drawTriangle(cx, cy - 6 * scale, cx - 6 * scale, cy + 5 * scale, cx + 6 * scale, cy + 5 * scale, c);
  tft.fillRect(cx - scale, cy - 2 * scale, 2 * scale, 4 * scale, c);   // "!" stem
  tft.fillRect(cx - scale, cy + 3 * scale, 2 * scale, scale, c);       // "!" dot
}
void iconWarningFilled(int cx, int cy, uint16_t c, uint16_t bg, int scale) {
  tft.fillTriangle(cx, cy - 7 * scale, cx - 8 * scale, cy + 6 * scale, cx + 8 * scale, cy + 6 * scale, c);
  tft.fillRect(cx - scale, cy - 3 * scale, 2 * scale, 5 * scale, bg);   // stem
  tft.fillRect(cx - scale, cy + 3 * scale, 2 * scale, 2 * scale, bg);   // dot
}
void iconBarcode(int cx, int cy, uint16_t c, int scale) {    // Identity: Serial
  static const int8_t bars[][2] = { {-8, 2}, {-5, 1}, {-3, 3}, {1, 1}, {3, 2}, {6, 1}, {8, 1} };
  for (auto &b : bars) tft.fillRect(cx + b[0] * scale, cy - 6 * scale, b[1] * scale, 12 * scale, c);
}
void iconClock(int cx, int cy, uint16_t c, int scale) {     // Usage: Wake/Work time
  tft.drawCircle(cx, cy, 7 * scale, c);
  tft.drawFastVLine(cx, cy - 4 * scale, 4 * scale, c);   // hour hand
  tft.drawFastHLine(cx, cy, 3 * scale, c);               // minute hand
}

void iconBrush(int cx, int cy, uint16_t c, int scale) {     // Usage: 18V work-time cards
  tft.fillRect(cx - 1 * scale, cy - 7 * scale, 3 * scale, 5 * scale, c);        // handle
  tft.fillRect(cx - 5 * scale, cy - 2 * scale, 11 * scale, 3 * scale, c);       // ferrule
  for (int k = 0; k < 4; k++)                                                    // bristles
    tft.fillRect(cx + (-5 + k * 3) * scale, cy + 2 * scale, 2 * scale, 5 * scale, c);
}

void iconSun(int cx, int cy, uint16_t c, int scale) {        // Tools: pack LEDs on
  tft.fillCircle(cx, cy, 3 * scale, c);
  tft.drawFastVLine(cx, cy - 8 * scale, 3 * scale, c);   tft.drawFastVLine(cx, cy + 6 * scale, 3 * scale, c);
  tft.drawFastHLine(cx - 8 * scale, cy, 3 * scale, c);   tft.drawFastHLine(cx + 6 * scale, cy, 3 * scale, c);
  tft.drawLine(cx - 6 * scale, cy - 6 * scale, cx - 4 * scale, cy - 4 * scale, c);
  tft.drawLine(cx + 4 * scale, cy + 4 * scale, cx + 6 * scale, cy + 6 * scale, c);
  tft.drawLine(cx - 6 * scale, cy + 6 * scale, cx - 4 * scale, cy + 4 * scale, c);
  tft.drawLine(cx + 4 * scale, cy - 4 * scale, cx + 6 * scale, cy - 6 * scale, c);
}
void iconSunOff(int cx, int cy, uint16_t c, int scale) {     // Tools: pack LEDs off
  tft.drawCircle(cx, cy, 4 * scale, c);
}
void iconEraser(int cx, int cy, uint16_t c, int scale) {     // "acts on the pack" marker
  tft.drawLine(cx - 7 * scale, cy + 7 * scale, cx + 6 * scale, cy - 6 * scale, c);
  tft.drawLine(cx - 6 * scale, cy + 7 * scale, cx + 7 * scale, cy - 6 * scale, c);
  tft.drawLine(cx + 3 * scale, cy - 7 * scale, cx + 8 * scale, cy - 2 * scale, c);
  tft.fillTriangle(cx - 7 * scale, cy + 7 * scale, cx - 3 * scale, cy + 7 * scale, cx - 7 * scale, cy + 3 * scale, c);
}

// ---------- Header ----------
// Filled COL_ACCENT bar, the optional screen glyph and the title (level 3). Paints the bar only.
void drawHeader(const char *title, IconFn glyph) {
  tft.fillRect(0, 0, SCREEN_W, HEADER_H, COL_ACCENT);
  if (glyph) glyph(SP_4 - 1, HEADER_H / 2, COL_HEAD, ICON_SCALE_HEADER);
  gfxText(FONT_LEVEL3, glyph ? 2 * SP_4 - 2 : SP_2, HEADER_H / 2 + SP_1, title, COL_HEAD);
}

// ---------- Footer ----------
// Page dots at the top-right of the header; the active one punches through to COL_BG, the others get
// the dim tint (a same-colour dot would vanish on the bar).
void drawPageDots(int active, int count, int rightInset) {
  const int dw = SP_2, gap = 5, tot = count * dw + (count - 1) * gap;
  const int x0 = SCREEN_W - rightInset - tot, y = (HEADER_H - dw) / 2;
  for (int i = 0; i < count; i++)
    tft.fillCircle(x0 + i * (dw + gap) + dw / 2, y + dw / 2, dw / 2, i == active ? COL_BG : COL_DOT_DIM);
}

// A real footer BAR (full-width strip, left hint / optional right hint), not a bare line of text.
void drawFooterHint(const char *first, const char *second) {
  const int fy = SCREEN_H - FOOTER_H;
  tft.fillRect(0, fy, SCREEN_W, FOOTER_H, COL_PANEL);
  tft.drawFastHLine(0, fy, SCREEN_W, COL_BORDER);
  tft.setFont(NULL); tft.setTextSize(1); tft.setTextColor(COL_MUTED, COL_PANEL);
  tft.setCursor(SP_3, fy + SP_2); tft.print(first);
  if (second) {
    const int w = txtW(second, 1);
    tft.setCursor(SCREEN_W - SP_3 - w, fy + SP_2); tft.print(second);
  }
}

// ---------- Card ----------
// Selection is the ONLY border treatment (thick accent + lit fill).
void drawCard(int x, int y, int w, int h, bool selected, int radius) {
  tft.fillRoundRect(x, y, w, h, radius, selected ? COL_PANEL2 : COL_PANEL);
  tft.drawRoundRect(x, y, w, h, radius, selected ? COL_ACCENT : COL_BORDER);
  if (selected) tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, radius - 1, COL_ACCENT);
}
// WARNING/ERROR keep the normal fill (they are not a selection state) and border in the semantic colour.
void drawCard(int x, int y, int w, int h, CardStyle style, int radius) {
  if (style == CARD_SELECTED) { drawCard(x, y, w, h, true, radius); return; }
  uint16_t bc = COL_BORDER;
  switch (style) {
    case CARD_WARNING: bc = COL_YELLOW; break;
    case CARD_ERROR:   bc = COL_RED;    break;
    case CARD_OK:      bc = COL_GREEN;  break;
    case CARD_SUSPECT: bc = COL_ORANGE; break;
    case CARD_NEUTRAL: bc = COL_MUTED2; break;
    default:           break;
  }
  tft.fillRoundRect(x, y, w, h, radius, COL_PANEL);
  tft.drawRoundRect(x, y, w, h, radius, bc);
}
void drawTile(int x, int y, int w, int h, bool selected) {
  drawCard(x, y, w, h, selected, RADIUS_LG);
}
void cardLabel(int x, int y, const char *text) {
  char lb[40];
  tft.setFont(NULL); tft.setTextSize(1); tft.setTextColor(COL_MUTED);
  tft.setCursor(x, y - wideDy(2)); tft.print(upperLabel(text, lb, sizeof(lb)));
}
void drawIconWell(int wx, int wy, IconFn icon, uint16_t color) {
  tft.fillRoundRect(wx, wy, WELL_SIZE, WELL_SIZE, RADIUS_SM, COL_PANEL2);
  tft.drawRoundRect(wx, wy, WELL_SIZE, WELL_SIZE, RADIUS_SM, COL_BORDER);
  icon(wx + WELL_SIZE / 2, wy + WELL_SIZE / 2, color, ICON_SCALE_HEADER);
}

// ---------- KPI ----------
int drawKpi(int x, int y, const char *label, const char *value, uint16_t valueColor) {
  gfxText(FONT_LEVEL3, x, y + 8, label, COL_MUTED);
  gfxText(FONT_LEVEL1, x, y + 40, value, valueColor);
  return y + 48;
}
// Icon KPI card: icon in a left column, small uppercase muted label (classic size 1), level-3 value,
// optional muted caption under it. Label slot = 18 chars for a 144 px card. Offsets checked on the
// host render: no caption (h 48) label y+11 / value base y+37; with caption (h 52) label y+9 /
// value base y+31 / caption y+37.
void kpiCard(int x, int y, int w, int h, IconFn icon, const char *label, const char *value,
             uint16_t valueColor, const char *caption) {
  char lb[24];
  const bool cap = caption && caption[0];
  drawCard(x, y, w, h, false, RADIUS_MD);
  icon(x + SP_4 - 1, y + h / 2, COL_ACCENT, ICON_SCALE_HEADER);
  tft.setFont(NULL); tft.setTextSize(1); tft.setTextColor(COL_MUTED);
  tft.setCursor(x + 30, y + (cap ? 9 - wideDy(2) : 11)); tft.print(upperLabel(label, lb, sizeof(lb)));
  gfxText(FONT_LEVEL3, x + 30, y + (cap ? 31 : 37), value, valueColor);
  if (cap) drawCaption(x + 30, y + 37, caption);
}

// ---------- Badge ----------
int badgeWidth(const char *text) { return SP_2 + txtW(text, 1); }
void drawBadge(int x, int y, const char *text, uint16_t accentColor, uint16_t textColor) {
  const int w = badgeWidth(text);
  // A 12 px kanji cell overflows the 13 px pill: in that case the pill grows 4 px, centred on the
  // same row, and the text drops 1 px (the cell sits 2 px above the classic baseline). ASCII: unchanged.
  bool wide = false;
#if ENABLE_CJK
  wide = cjkHasWide(text);
#endif
  const int h = wide ? BADGE_H + 4 : BADGE_H, yy = y - (h - BADGE_H) / 2;
  tft.fillRoundRect(x, yy, w, h, RADIUS_SM, accentColor);
  tft.setFont(NULL); tft.setTextSize(1); tft.setTextColor(textColor);
  tft.setCursor(x + SP_1, yy + (wide ? 4 : 3)); tft.print(text);
}

// ---------- ListRow ----------
// Dense, non-selectable key/value pair: level-3 muted label above a classic size-2 value (the value
// is the thing that dominates). Needs >= 30 px before the next row.
void drawKvRow(int x, int y, const char *label, const char *value, uint16_t valueColor) {
  gfxText(FONT_LEVEL3, x, y + 9, label, COL_MUTED);
  tft.setFont(NULL); tft.setTextSize(2);
  tft.setTextColor(valueColor); tft.setCursor(x, y + 12); tft.print(value);
}
// Selectable row chrome only; the caller draws the content.
void drawListRow(int x, int y, int w, int h, bool selected, int radius) {
  if (selected) {
    tft.fillRoundRect(x, y, w, h, radius, COL_PANEL2);
    tft.drawRoundRect(x, y, w, h, radius, COL_ACCENT);
  } else {
    tft.fillRoundRect(x, y, w, h, radius, COL_PANEL);
    tft.drawRoundRect(x, y, w, h, radius, COL_BORDER);
  }
}

// ---------- Field ----------
void drawField(int x, int y, int w, int h, IconFn icon, const char *label, const char *value,
               uint16_t valueColor) {
  drawCard(x, y, w, h, false, RADIUS_MD);
  const int wx = x + SP_2, wy = y + (h - WELL_SIZE) / 2;
  drawIconWell(wx, wy, icon);
  const int tx = wx + WELL_SIZE + SP_3;
  cardLabel(tx, y + 9, label);
  gfxText(FONT_LEVEL3, tx, y + 32, value, valueColor);
}

// ---------- Check row ----------
uint16_t checkColor(CheckState s) {
  return s == CHECK_OK ? COL_GREEN : (s == CHECK_FAULT ? COL_RED : (s == CHECK_WARN ? COL_YELLOW : COL_ORANGE));
}
// 16 px status glyph centred on (cx, cy): check in green, cross in red, warning triangle in orange
// (suspect) or yellow (repairable).
void drawCheckGlyph(int cx, int cy, CheckState s) {
  const uint16_t c = checkColor(s);
  if (s == CHECK_OK) {
    tft.drawLine(cx - 7, cy,     cx - 2, cy + 6, c); tft.drawLine(cx - 2, cy + 6, cx + 8, cy - 6, c);
    tft.drawLine(cx - 7, cy + 1, cx - 2, cy + 7, c); tft.drawLine(cx - 2, cy + 7, cx + 8, cy - 5, c);
  } else if (s == CHECK_FAULT) {
    tft.drawLine(cx - 6, cy - 6, cx + 7, cy + 7, c); tft.drawLine(cx + 7, cy - 6, cx - 6, cy + 7, c);
    tft.drawLine(cx - 5, cy - 6, cx + 8, cy + 7, c); tft.drawLine(cx + 8, cy - 6, cx - 5, cy + 7, c);
  } else {
    tft.drawTriangle(cx, cy - 7, cx - 7, cy + 7, cx + 7, cy + 7, c);
    tft.fillRect(cx - 1, cy - 3, 2, 6, c); tft.fillRect(cx - 1, cy + 4, 2, 2, c);
  }
}
void drawCheckRow(int x, int y, int w, const char *label, const char *value, CheckState s) {
  const uint16_t c = checkColor(s);
  const int gx = x + w - SP_3 - 8, base = y + CHECK_ROW_H / 2 + 5;
  gfxText(FONT_LEVEL3, x + SP_3, base, label, COL_MUTED);
  gfxText(FONT_LEVEL3, gx - SP_3 - gfxWidth(FONT_LEVEL3, value), base, value, c);
  drawCheckGlyph(gx, y + CHECK_ROW_H / 2, s);
}

// ---------- Progress bar ----------
void drawProgress(int x, int y, int w, int h, int pct, uint16_t color) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  tft.fillRoundRect(x, y, w, h, RADIUS_SM, COL_PANEL2);
  tft.drawRoundRect(x, y, w, h, RADIUS_SM, COL_BORDER);
  const int fw = (w - 4) * pct / 100;
  if (fw > 0) tft.fillRoundRect(x + 2, y + 2, fw, h - 4, RADIUS_SM - 1, color);
}

// ---------- Button ----------
void drawButton(int x, int y, int w, int h, const char *label, bool primary, uint16_t primaryColor) {
  if (primary) {
    tft.fillRoundRect(x, y, w, h, RADIUS_MD, primaryColor);
  } else {
    tft.fillRoundRect(x, y, w, h, RADIUS_MD, COL_PANEL2);
    tft.drawRoundRect(x, y, w, h, RADIUS_MD, COL_BORDER);
  }
  const uint16_t col = primary ? COL_TEXT : COL_MUTED;
  if (gfxWidth(FONT_LEVEL3, label) <= w - 2 * SP_2) {
    gfxCenter(FONT_LEVEL3, x + w / 2, y + h / 2 + 5, label, col);
  } else {                                       // a long translation: classic caption font, centred
    tft.setFont(NULL); tft.setTextSize(1); tft.setTextColor(col);
    tft.setCursor(x + (w - txtW(label, 1)) / 2, y + (h - 8) / 2); tft.print(label);
  }
}
void drawButtonPair(int y, const char *cancel, const char *primary, uint16_t primaryColor) {
  const int bw = (CONTENT_W - SP_2) / 2;
  drawButton(GUTTER, y, bw, BTN_H, cancel, false);
  drawButton(GUTTER + bw + SP_2, y, bw, BTN_H, primary, true, primaryColor);
}

// ---------- Status banner / card ----------
int drawStatusBanner(int x, int y, int w, int h, const char *label, const char *value, uint16_t valueColor) {
  tft.fillRoundRect(x, y, w, h, RADIUS_MD, COL_PANEL);
  tft.drawRoundRect(x, y, w, h, RADIUS_MD, valueColor);
  tft.setFont(NULL); tft.setTextColor(COL_MUTED); tft.setTextSize(1);
  tft.setCursor(x + SP_2, y + 5); tft.print(label);
  tft.setTextColor(valueColor); tft.setTextSize(3);
  tft.setCursor(x + SP_2, y + 17); tft.print(value);
  return y + h;
}
// Height and line positions are fixed so the layout never jumps between verdict states.
void statusCardText(int y, const char *word, uint16_t vc, const char *line2, int count) {
  const int cx = GUTTER, cw = CONTENT_W;
  tft.fillRoundRect(cx, y, cw, STATUS_H, RADIUS_MD, COL_PANEL);
  tft.drawRoundRect(cx, y, cw, STATUS_H, RADIUS_MD, vc);
  tft.fillCircle(cx + 22, y + STATUS_H / 2, 9, vc);
  const int wx = cx + 42;
  int right = cx + cw - SP_3;
  if (count > 0) {
    char cb[4]; snprintf(cb, sizeof(cb), "%d", count);
    const int bw = badgeWidth(cb);
    drawBadge(right - bw, y + SP_2, cb, COL_RED, COL_TEXT);
    right -= bw + SP_1;
  }
  if (gfxWidth(FONT_LEVEL2, word) <= right - wx) {
    gfxText(FONT_LEVEL2, wx, y + 31, word, vc);
  } else {                                       // too wide at 18 pt: classic size 2
    tft.setFont(NULL); tft.setTextColor(vc); tft.setTextSize(2);
    tft.setCursor(wx, y + 15); tft.print(word);
  }
  // The second line: 9 pt when it fits, otherwise the classic caption font (long finding/action text).
  if (gfxWidth(FONT_LEVEL3, line2) <= cx + cw - SP_3 - wx) gfxText(FONT_LEVEL3, wx, y + 49, line2, COL_MUTED);
  else drawCaption(wx, y + 44 - wideDy(2), line2, COL_MUTED);
}

// ---------- Line card ----------
void lineCard(int y, int h, const char *label, const char *text, uint16_t color, uint16_t border, bool dot) {
  const int cx = GUTTER, cw = CONTENT_W;
  tft.fillRoundRect(cx, y, cw, h, RADIUS_MD, COL_PANEL);
  tft.drawRoundRect(cx, y, cw, h, RADIUS_MD, border);
  if (label) cardLabel(cx + SP_3, y + 9, label);
  const int ry = label ? 29 : 18;
  if (dot) tft.fillCircle(cx + 17, y + ry, 4, color);
  gfxText(FONT_LEVEL3, dot ? cx + 28 : cx + SP_3, y + ry + 5, text, color);
}

// ---------- Before / after card ----------
void beforeAfterCard(int y, const char *before, uint16_t bcol, const char *after, uint16_t acol,
                     const char *capBefore, const char *capAfter) {
  const int cx = GUTTER, cw = CONTENT_W, h = STATUS_H;
  const int ax = cx + cw / 2, rx = cx + cw / 2 + SP_4;
  const bool caps = (capBefore && capBefore[0]) || (capAfter && capAfter[0]);
  const int base = caps ? 34 : 38;                               // the captions take the bottom line
  drawCard(cx, y, cw, h, false, RADIUS_MD);
  cardLabel(cx + SP_3, y + 9, tr(S_BEFORE));
  const int maxW = ax - 8 - (cx + SP_3) - SP_1;                  // room left of the arrow
  if (gfxWidth(FONT_LEVEL3, before) <= maxW) gfxText(FONT_LEVEL3, cx + SP_3, y + base, before, bcol);
  else drawCaption(cx + SP_3, y + base - 4, before, bcol);       // a long cause list: classic caption font
  if (capBefore && capBefore[0]) drawCaption(cx + SP_3, y + 41, capBefore);
  tft.drawFastHLine(ax - 8, y + h / 2, 14, COL_MUTED); tft.drawFastHLine(ax - 8, y + h / 2 + 1, 14, COL_MUTED);
  tft.fillTriangle(ax + 4, y + h / 2 - 5, ax + 10, y + h / 2, ax + 4, y + h / 2 + 6, COL_MUTED);
  cardLabel(rx, y + 9, tr(S_AFTER));
  if (gfxWidth(FONT_LEVEL3, after) <= cx + cw - SP_3 - rx) gfxText(FONT_LEVEL3, rx, y + base, after, acol);
  else drawCaption(rx, y + base - 4, after, acol);
  if (capAfter && capAfter[0]) drawCaption(rx, y + 41, capAfter);
}

// ---------- Step list card ----------
void stepListCard(int y, int h, const char *label, const char *const *steps, int n) {
  const int cx = GUTTER, cw = CONTENT_W;
  drawCard(cx, y, cw, h, false, RADIUS_MD);
  cardLabel(cx + SP_3, y + 9, label);
  for (int i = 0; i < n; i++) {
    const int ty = y + 26 + i * 24;
    tft.fillCircle(cx + SP_3 + 7, ty + 6, 7, COL_PANEL2);
    tft.drawCircle(cx + SP_3 + 7, ty + 6, 7, COL_BORDER);
    char num[2] = { (char)('1' + i), 0 };
    drawCaption(cx + SP_3 + 5, ty + 3, num, COL_ACCENT);
    drawCaption(cx + SP_3 + 22, ty + 3, steps[i], COL_TEXT);
  }
}

// ---------- Cell bar group ----------
void cellBarGroup(int x, int y, int w, int h, const char *label, const char *caption,
                  const float *v, const uint16_t *col, int n, float vMin, float vMax) {
  drawCard(x, y, w, h, false, RADIUS_MD);
  cardLabel(x + SP_2, y + SP_2, label);
  if (caption && caption[0]) drawCaption(x + w - SP_2 - txtW(caption, 1), y + SP_2 - wideDy(2), caption);
  if (n <= 0) return;
  const bool big = (n <= 5);                                     // 9 pt values, else classic size 1
  const int top = y + SP_5, pitch = (y + h - SP_1 - top) / n;
  const int ix = x + SP_2, iw = w - 2 * SP_2;
  const int vw = big ? gfxWidth(FONT_LEVEL3_REG, "0.00") : txtW("0.00", 1);
  const int barx = ix + SP_5, barw = iw - SP_5 - vw - SP_2, bh = pitch - 4 < 8 ? pitch - 4 : 8;
  for (int i = 0; i < n; i++) {
    const int ry = top + i * pitch, mid = ry + pitch / 2;
    char t[8];
    snprintf(t, sizeof(t), "C%d", i + 1);
    drawCaption(ix, mid - 4, t, COL_MUTED);
    const bool none = (v[i] != v[i]);                            // NaN: no reading
    int pct = none ? 0 : (int)((v[i] - vMin) / (vMax - vMin) * 100.0f);
    drawProgress(barx, mid - bh / 2, barw, bh, pct, col[i]);
    const uint16_t vc = none ? COL_MUTED2 : (col[i] == COL_GREEN ? COL_TEXT : col[i]);
    if (none) strcpy(t, "--"); else snprintf(t, sizeof(t), "%.2f", v[i]);
    if (big) gfxText(FONT_LEVEL3_REG, ix + iw - gfxWidth(FONT_LEVEL3_REG, t), mid + 5, t, vc);
    else     drawCaption(ix + iw - txtW(t, 1), mid - 4, t, vc);
  }
}

// ---------- Busy card ----------
void busyCard(int y, int h, const char *title, const char *below) {
  const int cx = GUTTER, cw = CONTENT_W;
  tft.fillRoundRect(cx, y, cw, h, RADIUS_MD, COL_PANEL);
  tft.drawRoundRect(cx, y, cw, h, RADIUS_MD, COL_ACCENT);
  gfxCenter(FONT_LEVEL2, SCREEN_W / 2, y + 44, title, COL_TEXT);
  if (below && below[0]) drawCaption((SCREEN_W - txtW(below, 1)) / 2, y + h + SP_4, below, COL_MUTED);
}
void busyProgress(int y, const char *caption, int done, int total) {
  const int cx = GUTTER, cw = CONTENT_W;
  tft.fillRect(cx + SP_3, y + 52, cw - 2 * SP_3, 10, COL_PANEL);   // the caption band, cleared then redrawn
  if (caption && caption[0]) drawCaption((SCREEN_W - txtW(caption, 1)) / 2, y + 53, caption);
  const int bx = cx + SP_6, by = y + 76, bw = cw - 2 * SP_6;
  tft.fillRoundRect(bx, by, bw, 12, RADIUS_SM, COL_PANEL);
  tft.drawRoundRect(bx, by, bw, 12, RADIUS_SM, COL_BORDER);
  if (done > 0 && total > 0) tft.fillRoundRect(bx + 2, by + 2, (bw - 4) * done / total, 8, 3, COL_ACCENT);
}

// ---------- Condition card ----------
void conditionCard(int y, const char *label, int pct, uint16_t color, const char *caption) {
  const int cx = GUTTER, cw = CONTENT_W;
  char buf[8];
  drawCard(cx, y, cw, STATUS_H, false, RADIUS_MD);
  cardLabel(cx + SP_3, y + 9, label);
  if (pct < 0) strcpy(buf, "--"); else snprintf(buf, sizeof(buf), "%d%%", pct);   // < 0: no reading
  gfxText(FONT_LEVEL2, cx + SP_3, y + 44, buf, color);
  const int bx = cx + 96;
  drawProgress(bx, y + SP_3, cx + cw - SP_3 - bx, 14, pct < 0 ? 0 : pct, color);
  drawCaption(bx, y + 34, caption);
}

// ---------- Identity blocks ----------
void drawLogoBox(int x, int y, const uint8_t *logo, int lw, int lh) {
  tft.fillRoundRect(x, y, LOGO_BOX, LOGO_BOX, RADIUS_LG, COL_PANEL2);
  tft.drawRoundRect(x, y, LOGO_BOX, LOGO_BOX, RADIUS_LG, COL_ACCENT);
  tft.drawBitmap(x + (LOGO_BOX - lw) / 2, y + (LOGO_BOX - lh) / 2, logo, lw, lh, COL_ORANGE);
}
void drawWordmark(int cx, int baseY) {
  tft.setFont(FONT_LEVEL2); tft.setTextSize(1);
  int16_t bx, by; uint16_t w1, w2, hh;
  tft.getTextBounds("Pocket", 0, 0, &bx, &by, &w1, &hh);
  tft.getTextBounds("OBI", 0, 0, &bx, &by, &w2, &hh);
  const int sx = cx - (int)(w1 + w2 + SP_2) / 2;
  tft.setTextColor(COL_ACCENT); tft.setCursor(sx, baseY); tft.print("Pocket");
  tft.setTextColor(COL_ORANGE); tft.setCursor(sx + w1 + SP_2, baseY); tft.print("OBI");
  tft.setFont(NULL);
}
void qrBlock(int x, int y, const uint8_t *bmp, int px, const char *label) {
  tft.fillRect(x - 3, y - 3, px + 6, px + 6, 0xFFFF);           // white quiet zone
  tft.drawBitmap(x, y, bmp, px, px, 0x0000);                    // black modules
  if (label && label[0]) drawCaption(x - 3 - SP_2 - txtW(label, 1), y + px - FONT_LEVEL4_LINE_H, label, COL_MUTED);
}

// ---------- Info card / data row ----------
void drawInfoCard(int y, int h, IconFn icon, const char *label, const char *value, const char *caption) {
  const int cx = GUTTER, cw = CONTENT_W, wx = cx + SP_2, tx = wx + WELL_SIZE + SP_3;
  drawCard(cx, y, cw, h, false, RADIUS_MD);
  drawIconWell(wx, y + (h - WELL_SIZE) / 2, icon);
  cardLabel(tx, y + 9, label);
  gfxText(FONT_LEVEL3, tx, y + 32, value, COL_TEXT);
  if (caption && caption[0]) drawCaption(tx, y + 38, caption);
}
void monoKv(int kx, int y, int vx, const char *k, const char *v) {
  drawCaption(kx, y, k, COL_MUTED);
  drawCaption(vx, y, v, COL_TEXT);
}

// ---------- Fault card ----------
int faultCard(int y, uint16_t color, const char *title, const char *ref, const char *ctx) {
  const int x = GUTTER, w = CONTENT_W;
  drawCard(x, y, w, FAULT_ROW_H, false, RADIUS_MD);
  iconWarningFilled(x + SP_4, y + FAULT_ROW_H / 2, color, COL_PANEL);
  gfxText(FONT_LEVEL3, x + 34, y + 16, title, COL_TEXT);
  drawCaption(x + 34, y + 20, ref, COL_MUTED);
  if (ctx && ctx[0]) drawCaption(x + 34 + txtW(ref, 1) + SP_3, y + 20, ctx, COL_MUTED2);
  return y + FAULT_ROW_H;
}

// ---------- Toast ----------
// Brief centred confirmation banner (blocking ~0.9 s), then forces a redraw. Non-blocking = a
// "working" banner left up while the caller talks to the pack; the next render() replaces it.
void toast(const char *msg, uint16_t col, bool blocking) {
  const int x = (SCREEN_W - TOAST_W) / 2, y = (SCREEN_H - TOAST_H) / 2;
  tft.fillRect(x, y, TOAST_W, TOAST_H, col);
  tft.drawRect(x, y, TOAST_W, TOAST_H, COL_BG);
  gfxCenter(FONT_LEVEL3, SCREEN_W / 2, y + TOAST_H / 2 + 6, msg, COL_BG);   // cap height ~13 px
  if (blocking) delay(900);
  lastRenderedState = -1;   // force a full redraw on the next render()
}
