// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
#pragma once
// The shared drawing vocabulary (docs/platforms/UI_DESIGN_SYSTEM.md §3, DECISIONS.md D35/D46).
// BYTE-IDENTICAL across the firmware trees (v2.5 M3, checked by tools/check_structure.py):
// one source text, copied, never compiled across trees (D27/D14). What differs per tree lives
// outside: ui_host.h (which firmware header to include), ui_palette.h (the COL_* values).
//
// Everything is sized from the generated tokens (ui_theme.h <- design/tokens.json). A component
// knows no screen, no UiState and no data model: the caller passes words, values and colours.
#include "ui_host.h"

// ---------- Text primitives ----------
// GFX custom fonts are scaled by textSize in Adafruit_GFX: every helper forces textSize(1) so a GFX
// label renders at its true point size. Return/measure the width in px (JA-aware, D33).
int  gfxText(const GFXfont *f, int x, int baseY, const char *s, uint16_t col);
int  gfxWidth(const GFXfont *f, const char *s);
const char *upperLabel(const char *in, char *out, size_t n);   // ASCII-uppercase copy, UTF-8-safe
void gfxCenter(const GFXfont *f, int cx, int baseY, const char *s, uint16_t col);
// Level-4 caption: classic font, size 1, left at (x, y); `col` defaults to the muted caption colour.
void drawCaption(int x, int y, const char *s, uint16_t col = COL_MUTED2);

// ---------- Icons ----------
typedef void (*IconFn)(int cx, int cy, uint16_t c, int scale);
// Hand-drawn vector glyphs, shared set (D27 shared look). scale 1 = ICON_SCALE_HEADER
// (~16 px), scale 2 = ICON_SCALE_TILE (~32 px).
void iconBattery(int cx, int cy, uint16_t c, int scale = ICON_SCALE_HEADER);
void iconBanks(int cx, int cy, uint16_t c, int scale = ICON_SCALE_HEADER);
void iconKey(int cx, int cy, uint16_t c, int scale = ICON_SCALE_HEADER);
void iconList(int cx, int cy, uint16_t c, int scale = ICON_SCALE_HEADER);
void iconInfo(int cx, int cy, uint16_t c, int scale = ICON_SCALE_HEADER);
void iconBridge(int cx, int cy, uint16_t c, int scale = ICON_SCALE_HEADER);
void iconCode(int cx, int cy, uint16_t c, int scale = ICON_SCALE_HEADER);
void iconSettings(int cx, int cy, uint16_t c, int scale = ICON_SCALE_HEADER);
void iconThermo(int cx, int cy, uint16_t c, int scale = ICON_SCALE_HEADER);
void iconRefresh(int cx, int cy, uint16_t c, int scale = ICON_SCALE_HEADER);
void iconDoc(int cx, int cy, uint16_t c, int scale = ICON_SCALE_HEADER);
void iconCalendar(int cx, int cy, uint16_t c, int scale = ICON_SCALE_HEADER);
void iconWarning(int cx, int cy, uint16_t c, int scale = ICON_SCALE_HEADER);
void iconWarningFilled(int cx, int cy, uint16_t c, uint16_t bg, int scale = ICON_SCALE_HEADER);
void iconBarcode(int cx, int cy, uint16_t c, int scale = ICON_SCALE_HEADER);
void iconClock(int cx, int cy, uint16_t c, int scale = ICON_SCALE_HEADER);
void iconBrush(int cx, int cy, uint16_t c, int scale = ICON_SCALE_HEADER);
// Tools list (LXT): pack LEDs on/off, "acts on the pack" marker.
void iconSun(int cx, int cy, uint16_t c, int scale = ICON_SCALE_HEADER);
void iconSunOff(int cx, int cy, uint16_t c, int scale = ICON_SCALE_HEADER);
void iconEraser(int cx, int cy, uint16_t c, int scale = ICON_SCALE_HEADER);

// ---------- Header ----------
// Filled COL_ACCENT bar: optional 16 px glyph (the screen's), then the title. Paints the bar only;
// clearing the screen, choosing the glyph from the screen state, any per-tree chip: the caller.
void drawHeader(const char *title, IconFn glyph = nullptr);

// ---------- Footer ----------
// Page-position dots at the top-right of the header: the active dot punches through to COL_BG.
// `rightInset` = gap to the right edge (a tree that puts a chip there passes a larger one).
void drawPageDots(int active, int count, int rightInset = SP_2);
// The single bottom-of-screen nav hint bar every screen shares: left hint, optional right hint.
void drawFooterHint(const char *first, const char *second = nullptr);

// ---------- Card ----------
void drawCard(int x, int y, int w, int h, bool selected, int radius = RADIUS_LG);
enum CardStyle { CARD_NORMAL, CARD_SELECTED, CARD_WARNING, CARD_ERROR,
                 CARD_OK, CARD_SUSPECT, CARD_NEUTRAL };   // status-card borders (v2.5 guideline M1)
void drawCard(int x, int y, int w, int h, CardStyle style, int radius = RADIUS_LG);
void drawTile(int x, int y, int w, int h, bool selected);
// Small uppercase muted card label (classic size 1), the kpiCard/drawField label style.
void cardLabel(int x, int y, const char *text);
// Icon in its tinted rounded well (WELL_SIZE px square, top-left at wx,wy).
void drawIconWell(int wx, int wy, IconFn icon, uint16_t color = COL_ACCENT);

// ---------- KPI ----------
int drawKpi(int x, int y, const char *label, const char *value, uint16_t valueColor);
// Icon KPI card (Overview, Health): icon left, small uppercase label, level-3 value, optional caption.
void kpiCard(int x, int y, int w, int h, IconFn icon, const char *label, const char *value,
             uint16_t valueColor, const char *caption = nullptr);

// ---------- Badge ----------
void drawBadge(int x, int y, const char *text, uint16_t accentColor, uint16_t textColor = COL_BG);
int  badgeWidth(const char *text);

// ---------- ListRow ----------
void drawKvRow(int x, int y, const char *label, const char *value, uint16_t valueColor);
void drawListRow(int x, int y, int w, int h, bool selected, int radius = RADIUS_MD);

// ---------- Field ----------
void drawField(int x, int y, int w, int h, IconFn icon, const char *label, const char *value,
               uint16_t valueColor = COL_TEXT);

// ---------- Check row ----------
// A labelled finding with a status glyph on its right (Repair diagnosis): label (level 3, muted) on the
// left, value (level 3, in the state colour) right-aligned before the glyph. Fixed CHECK_ROW_H pitch.
enum CheckState { CHECK_OK, CHECK_FAULT, CHECK_SUSPECT, CHECK_WARN };   // WARN = repairable (yellow triangle)
uint16_t checkColor(CheckState s);
void drawCheckGlyph(int cx, int cy, CheckState s);
void drawCheckRow(int x, int y, int w, const char *label, const char *value, CheckState s);

// ---------- Progress bar ----------
// Track + fill (0..100 %), RADIUS_SM corners.
void drawProgress(int x, int y, int w, int h, int pct, uint16_t color);

// ---------- Button ----------
void drawButton(int x, int y, int w, int h, const char *label, bool primary, uint16_t primaryColor = COL_ACCENT);
// Cancel (secondary, left) + primary action (right), each half the content width, BTN_H tall, at y.
// The labels are the actions ("Cancel", "Reset"), never the controls (D46).
void drawButtonPair(int y, const char *cancel, const char *primary, uint16_t primaryColor = COL_ACCENT);

// ---------- Status banner / card ----------
int drawStatusBanner(int x, int y, int w, int h, const char *label, const char *value, uint16_t valueColor);
// Verdict status card: bordered in `vc`, big dot, the word (18 pt when it fits, classic size 2
// otherwise), one level-3 muted line under it; `count` > 0 adds a red count badge top-right.
// STATUS_H tall, full content width. (The verdict flavour statusCard(y, line2) lives in display.cpp.)
void statusCardText(int y, const char *word, uint16_t vc, const char *line2, int count = 0);

// One fact on one line in a full-width card: optional small-caps label, a small dot and a level-3
// text in `color`, border `border` (the colour itself for an expectation, COL_BORDER for a fact).
// Rows sit at y+18 (no label) or y+29 (label). No dot: the text starts at the label's x.
void lineCard(int y, int h, const char *label, const char *text, uint16_t color, uint16_t border,
              bool dot = true);
// Before -> after (result screens): two labelled level-3 values around an arrow, STATUS_H tall,
// full width. Optional captions (register / source) under the values; a value too wide for 9 pt
// drops to the caption font.
void beforeAfterCard(int y, const char *before, uint16_t bcol, const char *after, uint16_t acol,
                     const char *capBefore = nullptr, const char *capAfter = nullptr);
// "Try this": a labelled card of numbered steps (ring + number + classic-font text), 24 px pitch.
void stepListCard(int y, int h, const char *label, const char *const *steps, int n);
// Cell bars (LXT Overview): small-caps label + right caption (the spread), then one row per cell:
// "C<n>", bar (vMin empty .. vMax full), value with 2 decimals at 9 pt REGULAR (lighter than the bold
// values: five numbers side by side read heavy in bold; classic font past 5 cells so a 10-cell pack compacts). Colours come from the caller (the verdict code's thresholds);
// a NaN value draws "--" over an empty track (no pack: nothing stale is shown).
void cellBarGroup(int x, int y, int w, int h, const char *label, const char *caption,
                  const float *v, const uint16_t *col, int n, float vMin, float vMax);
// Busy card (a blocking write, e.g. the LXT unlock): accent-bordered card, the activity in 18 pt
// (sentence case: an activity, not a status), and `below` centred under the card. busyProgress()
// repaints only the progress band (caption line + bar) so a step change never flashes the screen;
// done/total = 0 draws an empty bar, an empty caption clears the line.
void busyCard(int y, int h, const char *title, const char *below);
void busyProgress(int y, const char *caption, int done, int total);
// Condition (LXT Health): label, hero % at 18 pt, bar and the caption that states the source. pct < 0 = no reading ("--").
void conditionCard(int y, const char *label, int pct, uint16_t color, const char *caption);

// ---------- Identity blocks (About, Splash) ----------
// The logo mark (a 1-bit bitmap the tree owns) in its LOGO_BOX well: COL_PANEL2, accent border, mark in
// COL_ORANGE (the logo's own colour).
void drawLogoBox(int x, int y, const uint8_t *logo, int lw, int lh);
// "Pocket" (accent) + "OBI" (orange) at 18 pt, centred on cx, on baseline baseY. Fixed brand: the gamme
// line under it is the caller's.
void drawWordmark(int cx, int baseY);
// A QR bitmap on its white quiet zone, black modules; `label` (e.g. "scan") bottom-left of it.
void qrBlock(int x, int y, const uint8_t *bmp, int px, const char *label);

// ---------- Info card / data row ----------
// Icon well + small-caps label + level-3 value + optional caption, full width, h tall (PC bridge
// "How to"). The well is vertically centred.
void drawInfoCard(int y, int h, IconFn icon, const char *label, const char *value, const char *caption);
// One bench data row (Debug, English): key in COL_MUTED at kx, value in COL_TEXT at vx, classic font.
// Raw values never take a status colour.
void monoKv(int kx, int y, int vx, const char *k, const char *v);

// ---------- Fault card ----------
// One fault entry: filled warning triangle in the entry's severity colour, level-3 title, muted
// reference, dimmer context. Returns the bottom y.
int faultCard(int y, uint16_t color, const char *title, const char *ref, const char *ctx = nullptr);

// ---------- Toast ----------
void toast(const char *msg, uint16_t col, bool blocking = true);
