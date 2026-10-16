// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// PocketOBI — screens, V2 UI rendering (split from PocketOBI.ino, DECISIONS.md D21).
// v2.4: rebuilt on the shared component layer (ui_components.h) and the generated design tokens
// (ui_theme.h / ui_palette.h <- design/tokens.json), shared vocabulary (D35).
#include "pocketobi.h"
#include "protocol.h"
#include "unlock.h"
#include "decode.h"
#include "display.h"
#include "ui_nav.h"
#include "ui_components.h"
#include "bridge.h"
#include "icons_bitmaps.h"

// PocketOBI logo, 48x48, 1-bit: battery outline split by a pulse line (house mark, D27).
// Rasterized from the design source PO-logo.svg (2026-09-26, replaces the battery+bolt
// mark); drawn in COL_ORANGE, the logo's own color.
#define LOGO_W 48
#define LOGO_H 48
const uint8_t LOGO_PO[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3F,0xFC,0x00,0x00,
  0x00,0x00,0x7F,0xFE,0x00,0x00,0x00,0x00,0x7F,0xFE,0x00,0x00,
  0x00,0x00,0x7F,0xFE,0x00,0x00,0x00,0x00,0x7F,0xFE,0x00,0x00,
  0x00,0x07,0xFF,0xFF,0xE0,0x00,0x00,0x1F,0xFF,0xFF,0xF8,0x00,
  0x00,0x3F,0xFF,0xFF,0xFC,0x00,0x00,0x7F,0xFF,0xFF,0xFE,0x00,
  0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0xF8,0x00,0x00,0x1F,0x00,
  0x00,0xF8,0x00,0x00,0x0F,0x00,0x00,0xF8,0x00,0x00,0x0F,0x00,
  0x00,0xF8,0x00,0x00,0x0F,0x00,0x00,0xF8,0x00,0x00,0x0F,0x00,
  0x00,0xF8,0x00,0x00,0x0F,0x00,0x00,0xF8,0x03,0x80,0x0F,0x00,
  0x00,0xF8,0x07,0xC0,0x0F,0x00,0x00,0xF8,0x07,0xC0,0x0F,0x00,
  0x00,0xF8,0x07,0xC0,0x0F,0x00,0x00,0xF8,0x0F,0xC0,0x0F,0x00,
  0x00,0xF0,0x0F,0xE0,0x0F,0x00,0x00,0x00,0x1F,0xE0,0x00,0x00,
  0x00,0x00,0x1F,0xE0,0x00,0x00,0x7F,0xFF,0xFE,0xF1,0xFF,0xFE,
  0xFF,0xFF,0xFC,0xF3,0xFF,0xFF,0x7F,0xFF,0xFC,0xF3,0xFF,0xFF,
  0x7F,0xFF,0xFC,0xF7,0xFF,0xFE,0x00,0x00,0xF8,0x7F,0x80,0x00,
  0x00,0x00,0x78,0x7F,0x80,0x00,0x00,0xF8,0x70,0x7F,0x0F,0x00,
  0x00,0xF8,0x00,0x3F,0x0F,0x00,0x00,0xF8,0x00,0x3E,0x0F,0x00,
  0x00,0xF8,0x00,0x3E,0x0F,0x00,0x00,0xF8,0x00,0x1C,0x0F,0x00,
  0x00,0xF8,0x00,0x1C,0x0F,0x00,0x00,0xF8,0x00,0x00,0x0F,0x00,
  0x00,0xF8,0x00,0x00,0x0F,0x00,0x00,0xF8,0x00,0x00,0x0F,0x00,
  0x00,0xF8,0x00,0x00,0x0F,0x00,0x00,0xF8,0x00,0x00,0x1F,0x00,
  0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x7F,0xFF,0xFF,0xFE,0x00,
  0x00,0x3F,0xFF,0xFF,0xFE,0x00,0x00,0x1F,0xFF,0xFF,0xF8,0x00,
  0x00,0x07,0xFF,0xFF,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


// Static QR of github.com/TheRepairforge/PocketOBI (About screen).
#define QR_PX 66
#define QR_BYTES 594
const uint8_t QR_URL[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xFF,0xCF,0x03,
  0x30,0x3C,0xFF,0xFC,0x00,0x0F,0xFF,0xCF,0x03,0x30,0x3C,0xFF,0xFC,0x00,0x0C,0x00,0xCF,0x33,0x00,0x00,
  0xC0,0x0C,0x00,0x0C,0x00,0xCF,0x33,0x00,0x00,0xC0,0x0C,0x00,0x0C,0xFC,0xC0,0xFC,0x0C,0x0C,0xCF,0xCC,
  0x00,0x0C,0xFC,0xC0,0xFC,0x0C,0x0C,0xCF,0xCC,0x00,0x0C,0xFC,0xCF,0xCC,0xF3,0x30,0xCF,0xCC,0x00,0x0C,
  0xFC,0xCF,0xCC,0xF3,0x30,0xCF,0xCC,0x00,0x0C,0xFC,0xC0,0xFC,0xC3,0x30,0xCF,0xCC,0x00,0x0C,0xFC,0xC0,
  0xFC,0xC3,0x30,0xCF,0xCC,0x00,0x0C,0x00,0xC0,0xF3,0x03,0xF0,0xC0,0x0C,0x00,0x0C,0x00,0xC0,0xF3,0x03,
  0xF0,0xC0,0x0C,0x00,0x0F,0xFF,0xCC,0xCC,0xCC,0xCC,0xFF,0xFC,0x00,0x0F,0xFF,0xCC,0xCC,0xCC,0xCC,0xFF,
  0xFC,0x00,0x00,0x00,0x0F,0xCC,0x03,0xF0,0x00,0x00,0x00,0x00,0x00,0x0F,0xCC,0x03,0xF0,0x00,0x00,0x00,
  0x0C,0xF3,0xF0,0x03,0x3F,0xF0,0xC3,0x3C,0x00,0x0C,0xF3,0xF0,0x03,0x3F,0xF0,0xC3,0x3C,0x00,0x03,0xF0,
  0x00,0xF3,0xFC,0xFF,0xFC,0xFC,0x00,0x03,0xF0,0x00,0xF3,0xFC,0xFF,0xFC,0xFC,0x00,0x0F,0x0F,0xF0,0xC3,
  0x03,0xCC,0xF0,0xF0,0x00,0x0F,0x0F,0xF0,0xC3,0x03,0xCC,0xF0,0xF0,0x00,0x00,0xF0,0x0C,0x00,0x3C,0xC0,
  0xF0,0x0C,0x00,0x00,0xF0,0x0C,0x00,0x3C,0xC0,0xF0,0x0C,0x00,0x0C,0xFF,0xCF,0xF0,0xF3,0xCC,0x3F,0xC0,
  0x00,0x0C,0xFF,0xCF,0xF0,0xF3,0xCC,0x3F,0xC0,0x00,0x0C,0xCF,0x03,0x30,0xCC,0x30,0xC3,0xFC,0x00,0x0C,
  0xCF,0x03,0x30,0xCC,0x30,0xC3,0xFC,0x00,0x03,0x0C,0xC3,0xCF,0x03,0xFF,0xFC,0x3C,0x00,0x03,0x0C,0xC3,
  0xCF,0x03,0xFF,0xFC,0x3C,0x00,0x0C,0x3C,0x0C,0x0C,0xCC,0x00,0x33,0x00,0x00,0x0C,0x3C,0x0C,0x0C,0xCC,
  0x00,0x33,0x00,0x00,0x0C,0x00,0xF0,0xCC,0xF3,0xFF,0x3C,0x0C,0x00,0x0C,0x00,0xF0,0xCC,0xF3,0xFF,0x3C,
  0x0C,0x00,0x03,0x3C,0x3F,0x3C,0xC0,0xCF,0xC3,0x00,0x00,0x03,0x3C,0x3F,0x3C,0xC0,0xCF,0xC3,0x00,0x00,
  0x0C,0xF3,0xFC,0x0F,0xF0,0xCF,0x3C,0xC0,0x00,0x0C,0xF3,0xFC,0x0F,0xF0,0xCF,0x3C,0xC0,0x00,0x00,0x33,
  0x03,0xC3,0xCF,0x0C,0xCC,0xFC,0x00,0x00,0x33,0x03,0xC3,0xCF,0x0C,0xCC,0xFC,0x00,0x03,0x33,0xFC,0x0F,
  0xC3,0xFF,0xFF,0xFC,0x00,0x03,0x33,0xFC,0x0F,0xC3,0xFF,0xFF,0xFC,0x00,0x00,0x00,0x0F,0xCC,0x0F,0xCC,
  0x0F,0xFC,0x00,0x00,0x00,0x0F,0xCC,0x0F,0xCC,0x0F,0xFC,0x00,0x0F,0xFF,0xCC,0x3C,0xCC,0xFC,0xCC,0x30,
  0x00,0x0F,0xFF,0xCC,0x3C,0xCC,0xFC,0xCC,0x30,0x00,0x0C,0x00,0xCC,0x0F,0xC0,0xCC,0x0C,0x30,0x00,0x0C,
  0x00,0xCC,0x0F,0xC0,0xCC,0x0C,0x30,0x00,0x0C,0xFC,0xC3,0x3C,0x33,0x0F,0xFC,0xF0,0x00,0x0C,0xFC,0xC3,
  0x3C,0x33,0x0F,0xFC,0xF0,0x00,0x0C,0xFC,0xCF,0xF0,0x3F,0x0F,0x3C,0xCC,0x00,0x0C,0xFC,0xCF,0xF0,0x3F,
  0x0F,0x3C,0xCC,0x00,0x0C,0xFC,0xCC,0x0F,0x03,0x30,0x30,0xCC,0x00,0x0C,0xFC,0xCC,0x0F,0x03,0x30,0x30,
  0xCC,0x00,0x0C,0x00,0xC3,0xF3,0x00,0x3C,0x33,0x30,0x00,0x0C,0x00,0xC3,0xF3,0x00,0x3C,0x33,0x30,0x00,
  0x0F,0xFF,0xCC,0xFC,0x30,0xFF,0x0C,0x30,0x00,0x0F,0xFF,0xCC,0xFC,0x30,0xFF,0x0C,0x30,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
// ---------- Display ----------
// v2.4 (D35): every screen is composed from ui_components.h (the shared vocabulary) and sized from
// the generated tokens (ui_theme.h <- design/tokens.json). Layout literals do not live here.

// Cell color based on its voltage and its position within the pack.
uint16_t cellColor(float v, float minV, float diff) {
  if (v < CELL_V_CRIT) return COL_RED;
  bool isLowest = (v <= minV + 0.001f);
  if (isLowest && diff > DIFF_BAD)  return COL_RED;
  if (isLowest && diff > DIFF_WARN) return COL_YELLOW;
  return COL_GREEN;
}

// Screen orientation (landscape, or 180 deg with cfgFlip). setRotation() writes the
// ST7789's MADCTL; an ILI9341 panel (a CYD fitted with one, or the ES3C28P) reads the MX bit
// the other way and is BGR-wired, so it gets the ILI9341 values instead (PANEL_ILI9341).
void applyRotation() {
  tft.setRotation(cfgFlip ? 3 : 1);
#if PANEL_ILI9341
  uint8_t madctl = cfgFlip ? 0xE8 : 0x28;   // MX|MY|MV|BGR flipped, MV|BGR normal
  tft.sendCommand(ST77XX_MADCTL, &madctl, 1);
#endif
}

#if PANEL_ILI9341_INIT
// ILI9341 power, VCOM, frame-rate and gamma registers, sent after tft.init() (the ST7789 sequence,
// which leaves them at the controller's reset values). Values from the Adafruit_ILI9341 library 1.6.1
// init table (BSD, see THIRD-PARTY.md). MADCTL is left to applyRotation(); pixel format, sleep-out and
// display-on were already sent by tft.init().
void ili9341PanelInit() {
  static const uint8_t seq[] = {
    0xEF, 3, 0x03, 0x80, 0x02,
    0xCF, 3, 0x00, 0xC1, 0x30,
    0xED, 4, 0x64, 0x03, 0x12, 0x81,
    0xE8, 3, 0x85, 0x00, 0x78,
    0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
    0xF7, 1, 0x20,
    0xEA, 2, 0x00, 0x00,
    0xC0, 1, 0x23,                   // power control 1
    0xC1, 1, 0x10,                   // power control 2
    0xC5, 2, 0x3E, 0x28,             // VCOM control 1
    0xC7, 1, 0x86,                   // VCOM control 2
    0xB1, 2, 0x00, 0x18,             // frame rate
    0xB6, 3, 0x08, 0x82, 0x27,       // display function control
    0xF2, 1, 0x00,                   // 3-gamma off
    0x26, 1, 0x01,                   // gamma curve 1
    0xE0, 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
    0xE1, 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
  };
  for (size_t i = 0; i < sizeof(seq); ) {
    const uint8_t cmd = seq[i], n = seq[i + 1];
    tft.sendCommand(cmd, &seq[i + 2], n);
    i += 2 + n;
  }
}
#endif

// ---------- Small helpers shared by the screens ----------
// Header bar with the glyph of the current screen (the component takes the glyph, it does not know
// the screens). LAUNCHER: no glyph.
static void header(const char *title) {
  IconFn g = nullptr;
  switch (state) {
    case BATTERY:        g = iconBattery;  break;
    case REPAIR_DIAG:
    case CONFIRM_UNLOCK:
    case UNLOCK_RESULT:  g = iconKey;      break;
    case TOOLS:          g = iconList;     break;
    case SETTINGS:       g = iconSettings; break;
    case PC_BRIDGE:      g = iconBridge;   break;
    case DEBUG_RAW:      g = iconCode;     break;
    case ABOUT:          g = iconInfo;     break;
    case CONFIRM_RESET:
    case RESET_RESULT:   g = iconRefresh;  break;
    case COMM_ERROR:     g = iconWarning;  break;
    default:             break;
  }
  drawHeader(title, g);
}
// Repair wizard header (the shell, D35 lockstep): the title names the step, the page dots carry
// its position (the step number is not repeated in the title).
static void repairHeader(int step, StrId title) {
  header(tr(title));
  drawPageDots(step - 1, 4);
}
// Footer hints. The encoder board says Turn / Click / Back; the CYD has no encoder and no Back key, so
// its build says what a tap does and that the title bar is Back (D46 rule 1, touch variant). Every
// screen names its encoder hints; hint() swaps them for the touch wording, or drops them (no
// equivalent, e.g. "Turn: select" when the items are tapped directly).
static int hint(StrId id) {
#if BOARD_HAS_TOUCH
  switch (id) {
    case S_H_CLICK_OPEN:     return S_T_TAP_OPEN;
    case S_H_CLICK_BACK:     return S_T_TAP_BACK;
    case S_H_CLICK_CONTINUE: return S_T_TAP_CONTINUE;
    case S_H_CLICK_TOGGLE:
    case S_H_TURN_TOGGLE:    return S_T_TAP_TOGGLE;
    case S_H_CLICK_REFRESH:  return S_T_CENTRE_REFRESH;
    case S_H_CLICK_CONFIRM:  return S_T_TAP_BUTTON;
    case S_H_TURN_PAGE:      return S_T_EDGES_PAGE;
    case S_H_TURN_SELECT:    return -1;
    case S_H_TURN_CANCEL:
    case S_H_BACK_MENU:
    case S_H_BACK_TOOLS:     return S_T_TITLE_BACK;
    default:                 return id;
  }
#else
  return id;
#endif
}
// Up to two hints on the left (joined by two spaces), one on the right. A hint the touch
// build drops is skipped; the same hint twice (Turn: cancel and Back: menu both become "Title: back")
// is shown once.
static void footer(StrId a, StrId b = S_COUNT, StrId right = S_COUNT) {
  int l1 = hint(a), l2 = (b == S_COUNT) ? -1 : hint(b), r = (right == S_COUNT) ? -1 : hint(right);
  if (l1 < 0) { l1 = l2; l2 = -1; }
  if (l2 == l1) l2 = -1;
  if (r == l1 || r == l2) r = -1;
  char h[64];
  if (l1 < 0) h[0] = 0;
  else if (l2 < 0) snprintf(h, sizeof(h), "%s", tr((StrId)l1));
  else snprintf(h, sizeof(h), "%s  %s", tr((StrId)l1), tr((StrId)l2));
  drawFooterHint(h, r < 0 ? nullptr : tr((StrId)r));
}
// Verdict flavour of the status card: the verdict's own word and colour, one line under it.
static void statusCard(int y, const char *line2) {
  const Verdict v = computeVerdict();
  statusCardText(y, verdictText(v), verdictColor(v), line2);
}
// Two half-width columns inside the content width.

// Compact text for a lock-cause bitmask, e.g. "CS0 CS2 N34" or "none".
void lockCausesText(uint8_t causes, char *out, size_t n) {
  out[0] = 0;
  if (causes == 0) { strncpy(out, tr(S_NONE), n); out[n - 1] = 0; return; }
  if (causes & LF_N34) strncat(out, "N34 ", n - strlen(out) - 1);
  if (causes & LF_CS0) strncat(out, "CS0 ", n - strlen(out) - 1);
  if (causes & LF_CS1) strncat(out, "CS1 ", n - strlen(out) - 1);
  if (causes & LF_CS2) strncat(out, "CS2 ", n - strlen(out) - 1);
}
// ---------- Launcher (2x2 big tiles) ----------
// Selection is the ONLY border treatment. One tile is drawn on its own so a selection change
// repaints just the two tiles involved (redrawSelection), never the whole screen.
// 2x2 grid inside the content zone, so the footer hint fits under it (v2.5).
static void drawLauncherTile(int i) {
  const int tw = TILE_W, th = TILE_H, top = CONTENT_Y;
  const int x = (i % 2) ? GUTTER + tw + TILE_GAP : GUTTER, y = (i / 2) ? top + th + TILE_GAP : top;
  const bool sel = (launcherIndex == i);
  drawTile(x, y, tw, th, sel);
  const int cx = x + tw / 2;
  const Verdict v = (i == 0) ? computeVerdict() : V_UNKNOWN;     // only the Battery tile shows it
  const uint8_t *ic = (i == 0) ? ICON_BATTERY : (i == 1) ? ICON_REPAIR : (i == 2) ? ICON_TOOLS : ICON_INFO;
  const uint16_t icc = (i == 0) ? (bat.valid ? verdictColor(v) : COL_MUTED)
                     : (i == 1) ? COL_ORANGE : (i == 2) ? COL_ACCENT : COL_CYAN;
  tft.drawBitmap(cx - ICON_W / 2, y + TILE_ICON_Y, ic, ICON_W, ICON_H, icc);
  if (i == 0) {                                  // Battery: V + verdict under the icon (verdict colour)
    char ms[48];
    if (bat.valid) { snprintf(ms, sizeof(ms), "%.1fV %s", bat.packVoltage, verdictText(v));
                     tft.setTextColor(verdictColor(v)); }
    else           { snprintf(ms, sizeof(ms), "%s", tr(S_NO_PACK)); tft.setTextColor(COL_MUTED); }
    tft.setFont(NULL); tft.setTextSize(1);
    tft.setCursor(cx - txtW(ms, 1) / 2, y + TILE_INFO_Y); tft.print(ms);
  }
  gfxCenter(FONT_LEVEL3, cx, y + TILE_LABEL_Y, tr((StrId)(S_BATTERY + i)), sel ? COL_HEAD : COL_TEXT);
}
void drawLauncher() {
  header("PocketOBI");
  for (int i = 0; i < 4; i++) drawLauncherTile(i);
  footer(S_H_TURN_SELECT, S_H_CLICK_OPEN);
}

// ---------- Battery : Overview page ----------
// What the repair diagnosis concluded, in one place: the Overview status line and the Repair
// wizard read the same finding (single source, never two parallel rules).
struct Finding { const char *title; uint16_t color; char line2[64]; HwFault hw; int grp; uint8_t causes; };
static void diagFinding(Finding &f) {
  const bool isF0513 = strcmp(bat.commandVersion, "F0513") == 0;
  char action[64];
  f.grp = 0;
  f.hw = findHardwareFault(&f.grp, action, sizeof(action));
  f.causes = (bat.valid && !isF0513) ? lockCauses(bat.msg) : 0;
  if (f.hw != HW_NONE) {
    f.color = COL_RED;
    switch (f.hw) {
      case HW_SENSE_WIRE: f.title = tr(S_HW_SENSE);  break;
      case HW_WEAK_CELL:  f.title = tr(S_HW_WEAK);   break;
      case HW_IMBALANCE:  f.title = tr(S_HW_IMB);    break;
      default:            f.title = tr(S_HW_THERM);  break;
    }
    strncpy(f.line2, action, sizeof(f.line2)); f.line2[sizeof(f.line2) - 1] = 0;
  } else if (bat.latchedFault) {
    // Checked BEFORE "not locked": a latched marker means the BMS memorised a fault even if the
    // pack isn't charger-locked right now (matches computeVerdict -> V_SUSPECT).
    f.title = tr(S_BMS_MEMORISED); f.color = COL_ORANGE;
    if (bat.odEventCount)            strcpy(f.line2, tr(S_WHY_OD));
    else if (bat.olEventCount)       strcpy(f.line2, tr(S_WHY_OL));
    else if (bat.healthEstPct < 50)  strcpy(f.line2, tr(S_WHY_WEAR));   // <50% => >~448 cycles
    else                             strcpy(f.line2, tr(S_WHY_UNCLEAR));
  } else if (!f.causes && !bat.locked) {
    f.title = tr(S_NO_LOCK_NO_FAULT); f.color = COL_GREEN;
    strcpy(f.line2, tr(S_PACK_HEALTHY));
  } else {
    // Charger-locked, no fault marker -> a false lockout our unlock should clear.
    f.title = tr(S_FALSE_LOCKOUT); f.color = COL_GREEN;
    strcpy(f.line2, tr(S_UNLOCK_SHOULD_HOLD));
  }
  // Recoverable over-discharge (cells below the healthy minimum, no imbalance): computeVerdict says
  // SUSPECT, so the line must not claim "no fault".
  if (f.hw == HW_NONE && f.color == COL_GREEN && computeVerdict() == V_SUSPECT && !thermistorSuspect()) {
    f.title = tr(S_WHY_OD); strcpy(f.line2, tr(S_WHY_OD)); f.color = COL_ORANGE;
  }
  // Spread-only thermistor suspicion: add a check note on an otherwise-clean finding (the verdict
  // already reflects it via computeVerdict -> V_SUSPECT when not charger-locked).
  if (f.hw == HW_NONE && f.color == COL_GREEN && thermistorSuspect()) {
    strcpy(f.line2, tr(S_WHY_THERM_CHECK));
    f.color = COL_ORANGE;
  }
}
// Temperature value as the pack reports it: "cell/board C", or a single sensor.
static void fmtTemp(char *out, size_t n) {
  if (bat.boardTempValid) snprintf(out, n, "%.0f/%.0f C", bat.tempCell, bat.tempMosfet);
  else                    snprintf(out, n, "%.0f C", bat.tempCell);   // single sensor
}
void drawBatteryOverview() {
  header(bat.valid ? bat.model : tr(S_BATTERY));
  drawPageDots(0, 3);
  Finding f; f.title = ""; f.color = COL_MUTED; f.line2[0] = 0;
  if (bat.valid) diagFinding(f);
  // A warning/fault shows its reason, not the title; no pack says what to do.
  statusCard(CONTENT_Y, !bat.valid ? tr(S_SUB_INSERT_PACK) : (f.color == COL_GREEN) ? f.title : f.line2);
  // Left column: PACK and TEMP KPIs; right column: the cells (D46: same frame with or without a pack,
  // so nothing jumps when one is inserted, and nothing stale is shown when it is gone).
  const int y = CONTENT_Y + STATUS_H + CARD_GAP, rx = GUTTER + COL2_W + CARD_GAP;
  char buf[32], cap[32];
  if (bat.valid) {
    const bool tPinned = tempImplausible(bat.tempCell) || (bat.boardTempValid && tempImplausible(bat.tempMosfet));
    // Pinned sensor = confirmed fault (red); spread-only = empirical suspicion (orange, explained on Health).
    const uint16_t tcol = tPinned ? COL_RED : (thermistorSuspect() ? COL_ORANGE : COL_TEXT);
    snprintf(buf, sizeof(buf), "%.2f V", bat.packVoltage);
    snprintf(cap, sizeof(cap), "%u %s", bat.chargeCount, tr(S_CHARGES));
    kpiCard(GUTTER, y, COL2_W, KPI_H, iconBattery, tr(S_PACK), buf, COL_TEXT, cap);
    fmtTemp(buf, sizeof(buf));
    kpiCard(GUTTER, y + KPI_H + CARD_GAP, COL2_W, KPI_H, iconThermo, tr(S_TEMP), buf, tcol,
            bat.boardTempValid ? tr(S_TEMP_CB) : nullptr);
  } else {
    kpiCard(GUTTER, y, COL2_W, KPI_H, iconBattery, tr(S_PACK), "--", COL_MUTED2);
    kpiCard(GUTTER, y + KPI_H + CARD_GAP, COL2_W, KPI_H, iconThermo, tr(S_TEMP), "--", COL_MUTED2);
  }
  const int ch = 2 * KPI_H + CARD_GAP;
  float cv[sizeof(bat.cell) / sizeof(bat.cell[0])];
  uint16_t cc[sizeof(bat.cell) / sizeof(bat.cell[0])];
  float cmn = 9.0f;
  for (int i = 0; i < cellCount; i++) if (bat.cell[i] > 0.1f && bat.cell[i] < cmn) cmn = bat.cell[i];
  for (int i = 0; i < cellCount; i++) {
    cv[i] = bat.valid ? bat.cell[i] : NAN;
    cc[i] = bat.valid ? cellColor(bat.cell[i], cmn, bat.cellDiff) : COL_MUTED2;
  }
  if (bat.valid) snprintf(cap, sizeof(cap), "%s %.3f V", tr(S_SUB_CELL_DIFF), bat.cellDiff); else cap[0] = 0;
  cellBarGroup(rx, y, COL2_W, ch, tr(S_CELLS), cap, cv, cc, cellCount, CELL_V_MIN, CELL_V_MAX);
}

// ---------- Battery : Health page ----------
void drawBatteryHealth() {
  header(tr(S_HDR_HEALTH));
  drawPageDots(1, 3);
  char buf[40];
  if (!bat.valid) {                                   // no pack: same frame, no stale value
    const int y1 = CONTENT_Y + STATUS_H + CARD_GAP, x2 = GUTTER + COL2_W + CARD_GAP, y2 = y1 + KPI_H + CARD_GAP;
    conditionCard(CONTENT_Y, tr(S_CONDITION), -1, COL_MUTED2, "");
    kpiCard(GUTTER, y1, COL2_W, KPI_H, iconBattery, tr(S_OVERDISCHARGE), "--", COL_MUTED2);
    kpiCard(x2,     y1, COL2_W, KPI_H, iconBanks,   tr(S_OVERLOAD),      "--", COL_MUTED2);
    kpiCard(GUTTER, y2, COL2_W, KPI_H, iconDoc,     tr(S_CELLS),         "--", COL_MUTED2);
    kpiCard(x2,     y2, COL2_W, KPI_H, iconThermo,  tr(S_TEMP_CB),       "--", COL_MUTED2);
    return;
  }
  const uint8_t soh = bat.healthEstPct;
  const uint16_t cc = soh >= 80 ? COL_GREEN : (soh >= 50 ? COL_YELLOW : COL_RED);
  // Condition card: OUR OWN cycle-based estimate (not the BMS's SOH gauge); the raw charge level
  // (D4 0x150 / SOC) is exposed next to it as a diagnostic value.
  const int cx = GUTTER, y0 = CONTENT_Y;
  if (bat.extValid) snprintf(buf, sizeof(buf), "est. from cycles - SOC raw %u", bat.socRaw);
  else              snprintf(buf, sizeof(buf), "est. from cycles");
  conditionCard(y0, tr(S_CONDITION), soh, cc, buf);
  // 2x2 grid: wear counters (D4) with the raw count as the primary figure and the "% of cycles"
  // only as a soft hint in parentheses. When the extended data is unavailable show "-", not "none".
  const int y1 = y0 + STATUS_H + CARD_GAP, kw = COL2_W, kx2 = cx + kw + CARD_GAP;
  uint16_t col;
  if (!bat.extValid) strcpy(buf, "-");
  else if (bat.odEventCount == 0) strcpy(buf, tr(S_NONE));
  else snprintf(buf, sizeof(buf), "%u (%u%%)", bat.odEventCount, bat.odWearPct);
  col = !bat.extValid ? COL_MUTED : (bat.odEventCount ? COL_TEXT : COL_GREEN);
  kpiCard(cx, y1, kw, KPI_H, iconBattery, tr(S_OVERDISCHARGE), buf, col);
  if (!bat.extValid) strcpy(buf, "-");
  else if (bat.olEventCount == 0) strcpy(buf, tr(S_NONE));
  else snprintf(buf, sizeof(buf), "%u (%u%%)", bat.olEventCount, bat.olWearPct);
  col = !bat.extValid ? COL_MUTED : (bat.olEventCount ? COL_TEXT : COL_GREEN);
  kpiCard(kx2, y1, kw, KPI_H, iconBanks, tr(S_OVERLOAD), buf, col);
  float mn = 9, mx = 0;
  for (int i = 0; i < cellCount; i++) { if (bat.cell[i] > 0.1f && bat.cell[i] < mn) mn = bat.cell[i];
                                        if (bat.cell[i] > mx) mx = bat.cell[i]; }
  snprintf(buf, sizeof(buf), "%.2f-%.2f V", mn, mx);
  kpiCard(cx, y1 + KPI_H + CARD_GAP, kw, KPI_H, iconDoc, tr(S_CELLS), buf, bat.cellDiff > DIFF_BAD ? COL_RED : COL_GREEN);
  const uint16_t tc = thermistorFault() ? COL_RED : (thermistorSuspect() ? COL_ORANGE : COL_GREEN);
  fmtTemp(buf, sizeof(buf));
  // The cell/board spread lives here (D46 Q3): it is what makes the thermistor "suspect".
  char cap[24]; cap[0] = 0;
  if (bat.boardTempValid) {
    const float td = bat.tempMosfet > bat.tempCell ? bat.tempMosfet - bat.tempCell : bat.tempCell - bat.tempMosfet;
    snprintf(cap, sizeof(cap), "%s %.1f C", tr(S_SPREAD), td);
  }
  kpiCard(kx2, y1 + KPI_H + CARD_GAP, kw, KPI_H, iconThermo, tr(S_TEMP_CB), buf, tc, cap);
}

// ---------- Battery : Identity page ----------
// Format a pack date (produced / assembled). Valid only if month 1..12, day 1..31 and a plausible
// year: this generation can read the ROM/assembly-date bytes back as all-FF (-> 2255-255-255) or
// leave them unwritten (0). Anything else shows "?".
void fmtPackDate(char *out, size_t n, uint16_t year, uint8_t m, uint8_t d) {
  if (m >= 1 && m <= 12 && d >= 1 && d <= 31 && year >= 2005 && year <= 2099)
    snprintf(out, n, "%04u-%02u-%02u", year, m, d);
  else { strncpy(out, "?", n); out[n - 1] = 0; }
}
void drawBatteryIdentity() {
  header(tr(S_HDR_IDENTITY));
  drawPageDots(2, 3);
  char sn[20], buf[24];
  formatSerial(sn);
  const bool ok = bat.valid;                          // no pack: dashes, never the last pack's identity
  const int x = GUTTER, w = CONTENT_W;
  const int wa = COL2_W, xb = x + COL2_W + CARD_GAP, wb = COL2_W;   // the 2-column grid (D46)
  const int pitch = FIELD_H + FIELD_GAP;
  int y = CONTENT_Y;
  drawField(x, y, wa, FIELD_H, iconBattery, tr(S_MODEL), ok ? bat.model : "--");
  fmtPackDate(buf, sizeof(buf), bat.mfgYear, bat.mfgMonth, bat.mfgDay);
  drawField(xb, y, wb, FIELD_H, iconCalendar, tr(S_PRODUCED), ok ? buf : "--");
  y += pitch;
  drawField(x, y, wa, FIELD_H, iconDoc, tr(S_TYPE), ok ? "LXT 18V" : "--");
  fmtPackDate(buf, sizeof(buf), 2000 + bat.asmY, bat.asmM, bat.asmD);
  drawField(xb, y, wb, FIELD_H, iconCalendar, tr(S_ASSEMBLED), ok ? buf : "--");
  y += pitch;
  snprintf(buf, sizeof(buf), "%.1f Ah", bat.capacityAh);
  drawField(x, y, wa, FIELD_H, iconBanks, tr(S_CAPACITY), ok ? buf : "--");
  snprintf(buf, sizeof(buf), "%u", bat.chargeCount);
  drawField(xb, y, wb, FIELD_H, iconRefresh, tr(S_CHARGES), ok ? buf : "--");
  y += pitch;
  drawField(x, y, w, FIELD_H, iconBarcode, tr(S_SN), ok ? sn : "--");
}

// ---------- Battery : paged ----------
void drawBatteryPage() {
  if (batteryPage == 0)      drawBatteryOverview();
  else if (batteryPage == 1) drawBatteryHealth();
  else                       drawBatteryIdentity();
  footer(S_H_TURN_PAGE, S_H_CLICK_REFRESH, S_H_BACK_MENU);   // click re-reads the pack
}

// ---------- Repair wizard, step 1/4: staged, feasibility-FIRST diagnosis ----------
//  Stage 0 comms -> Stage 1 hardware faults (before lock, they invalidate an unlock)
//  -> Stage 2 lock + prognosis -> Stage 3 the WHY (from the wear counters).
// The status card is computeVerdict() (the exact verdict of the Battery tile and pages, single
// source of truth); its second line is the wizard's detailed prediction (why / what to do).
void drawWizardDiag() {
  repairHeader(1, S_REPAIR);
  if (!bat.valid) {                                                   // Stage 0: comms
    statusCardText(CONTENT_Y, tr(S_NO_DIAGNOSIS), COL_MUTED, tr(S_SUB_NO_REPLY));
    const char *steps[3] = { tr(S_STEP_ADAPTER), tr(S_STEP_DATA_EN), tr(S_STEP_DEAD_BMS) };
    stepListCard(CONTENT_Y + STATUS_H + CARD_GAP, CONTENT_H - STATUS_H - CARD_GAP, tr(S_LBL_TRY), steps, 3);
    footer(S_H_TURN_CANCEL, S_COUNT, S_H_BACK_MENU);
    return;
  }
  Finding f; diagFinding(f);
  statusCard(CONTENT_Y, f.line2);
  // Checklist: the raw signals behind the verdict.
  const int y = CONTENT_Y + STATUS_H + CARD_GAP, ch = CONTENT_H - STATUS_H - CARD_GAP;
  drawCard(GUTTER, y, CONTENT_W, ch, false, RADIUS_MD);
  char cb[24]; lockCausesText(f.causes, cb, sizeof(cb));
  const int ry = y + (ch - 4 * CHECK_ROW_H) / 2;
  // A charger lock is the repairable case (yellow triangle), not a fault (D46 CHECK_WARN).
  drawCheckRow(GUTTER, ry, CONTENT_W, tr(S_CHARGER_LOCK), f.causes ? cb : tr(S_NONE), f.causes ? CHECK_WARN : CHECK_OK);
  // Latched marker is a HINT (orange warning), not a red fault, in the checklist too.
  drawCheckRow(GUTTER, ry + CHECK_ROW_H, CONTENT_W, tr(S_LATCHED_FAULT), bat.latchedFault ? tr(S_YES) : tr(S_NONE),
               bat.latchedFault ? CHECK_SUSPECT : CHECK_OK);
  // Thermistor row is 3-state: confirmed fault (pinned) red, suspect (spread) orange "?", else ok.
  const CheckState th = thermistorFault() ? CHECK_FAULT : (thermistorSuspect() ? CHECK_SUSPECT : CHECK_OK);
  drawCheckRow(GUTTER, ry + 2 * CHECK_ROW_H, CONTENT_W, tr(S_THERMISTOR),
               th == CHECK_FAULT ? tr(S_FAULTV) : (th == CHECK_SUSPECT ? "?" : tr(S_OKV)), th);
  char cv[12]; const bool cbad = (f.hw == HW_SENSE_WIRE || f.hw == HW_WEAK_CELL || f.hw == HW_IMBALANCE);
  if (cbad) snprintf(cv, sizeof(cv), "G%d", f.grp); else strcpy(cv, tr(S_OKV));
  drawCheckRow(GUTTER, ry + 3 * CHECK_ROW_H, CONTENT_W, tr(S_CELLS), cv, cbad ? CHECK_FAULT : CHECK_OK);
  // Only a lock with no hardware fault continues; otherwise the status card already says why (fix
  // the hardware first / nothing to repair) and a click just repeats it as a toast.
  if (f.causes && f.hw == HW_NONE) footer(S_H_CLICK_CONTINUE, S_COUNT, S_H_TURN_CANCEL);
  else                             footer(S_H_TURN_CANCEL, S_COUNT, S_H_BACK_MENU);
}

// ---------- Repair wizard, step 2/4: confirmation before writing to the BMS flash ----------
// Shows the detected charger-lock causes and a clear "writes flash" safety warning. If nothing is
// locked (or F0513), clicking just returns to the menu (no write is performed).
void drawConfirmUnlock() {
  const bool isF0513 = strcmp(bat.commandVersion, "F0513") == 0;
  const uint8_t causes = bat.valid ? lockCauses(bat.msg) : 0;
  // Nothing will be written in the first two branches, so the header does not ask a question.
  repairHeader(2, (isF0513 || causes == 0) ? S_REPAIR : S_HDR_UNLOCK_Q);
  if (isF0513) {
    statusCardText(CONTENT_Y, tr(S_W_NOT_SUPPORTED), COL_YELLOW, tr(S_SUB_F0513));
    footer(S_H_CLICK_BACK);
    return;
  }
  if (causes == 0) {
    statusCardText(CONTENT_Y, tr(S_W_NOTHING_TO_DO), COL_GREEN, tr(S_SUB_FRAME_OK));   // honest: nothing was written
    footer(S_H_CLICK_BACK);
    return;
  }
  // What happens (the shared confirm card): the write, what it changes, the honest limit. The
  // red border keeps the "this writes the BMS flash" warning visible without red small text.
  const int x = GUTTER, w = CONTENT_W;
  int y = CONTENT_Y;
  drawCard(x, y, w, 72, CARD_ERROR, RADIUS_MD);                       // 38..110
  cardLabel(x + SP_3, y + 9, tr(S_LBL_WHAT));
  gfxText(FONT_LEVEL3, x + SP_3, y + 32, tr(S_WHAT_FLASH), COL_TEXT);
  drawCaption(x + SP_3, y + 40, "Sets nybble34=0, recomputes CS0/1/2.");
  drawCaption(x + SP_3, y + 56, tr(S_REPAIRS_FALSE_ONLY), COL_MUTED);
  y += 72 + CARD_GAP;
  char cb[24], line[48];
  lockCausesText(causes, cb, sizeof(cb));
  snprintf(line, sizeof(line), "%s: %s", tr(S_CHARGER_LOCK), cb);
  lineCard(y, 36, nullptr, line, COL_YELLOW, COL_YELLOW);             // 116..152, the lock found
  y += 36 + SP_2;
  drawButtonPair(y, tr(S_BTN_CANCEL), tr(S_BTN_UNLOCK));              // 160..208
  footer(S_H_CLICK_CONFIRM, S_COUNT, S_H_TURN_CANCEL);
}

// ---------- Repair wizard, step 3/4: working (unlockRepair() blocks for a few seconds) ----------
// The shared busy card (card 54..164). The step line and the bar are repainted by
// drawUnlockProgress(), which unlockRepair() calls before each of its 4 steps.
static const int WORK_Y = HEADER_H + 22;
void drawUnlockWorking() {
  tft.fillScreen(COL_BG);
  repairHeader(3, S_HDR_UNLOCKING);
  busyCard(WORK_Y, 110, tr(S_WORKING), tr(S_KEEP_PACK));
  // The sequence is a fact about the work, so it sits in the card; the footer only says the input is ignored.
  const char *seq = tr(S_FRAME_STORE_PC);
  drawCaption((SCREEN_W - txtW(seq, 1)) / 2, WORK_Y + 94, seq);
  busyProgress(WORK_Y, "", 0, 4);
  footer(S_H_PLEASE_WAIT);
}
void drawUnlockProgress(int step, int total) {
  char buf[24]; snprintf(buf, sizeof(buf), tr(S_STEP_FMT), step, total);
  busyProgress(WORK_Y, buf, step, total);
}

// ---------- Repair wizard, step 4/4: lock causes before -> after, plus a verdict ----------
void drawUnlockResult() {
  repairHeader(4, S_HDR_UNLOCK_RESULT);
  const char *word; uint16_t col;
  if (unlockCausesAfter == 0xFF)    { word = tr(S_W_NOT_REREAD);    col = COL_YELLOW; }
  else if (unlockCausesAfter == 0)  { word = tr(S_W_UNLOCKED);      col = COL_GREEN; }
  else                              { word = tr(S_W_STILL_LOCKED);  col = COL_RED; }
  statusCardText(CONTENT_Y, word, col, tr(S_RETRY_REINSERT));
  char b[24], a[24];
  lockCausesText(unlockCausesBefore, b, sizeof(b));
  if (unlockCausesAfter == 0xFF) strcpy(a, "?"); else lockCausesText(unlockCausesAfter, a, sizeof(a));
  const int y = CONTENT_Y + STATUS_H + CARD_GAP;
  beforeAfterCard(y, b, unlockCausesBefore ? COL_YELLOW : COL_GREEN,
                  a, unlockCausesAfter == 0 ? COL_GREEN : COL_YELLOW, tr(S_CHARGER_LOCK), tr(S_CAP_REREAD));
  // Verdict now = computeVerdict() on the re-read unlockRepair() just did (single source); no re-read, no verdict.
  if (unlockCausesAfter != 0xFF) {
    const Verdict v = computeVerdict();
    lineCard(y + STATUS_H + CARD_GAP, 46, tr(S_LBL_VERDICT_NOW), verdictText(v), verdictColor(v), COL_BORDER);
  }
  footer(S_H_CLICK_BACK);
}

// ---------- Tools > Reset error ----------
void drawConfirmReset() {
  header(tr(S_HDR_RESET_Q));
  // Same pieces as the unlock confirm: what happens, the state now, the buttons.
  const int x = GUTTER, w = CONTENT_W;
  int y = CONTENT_Y;
  drawCard(x, y, w, 72, false, RADIUS_MD);                            // 38..110
  cardLabel(x + SP_3, y + 9, tr(S_LBL_WHAT));
  gfxText(FONT_LEVEL3, x + SP_3, y + 32, tr(S_WHAT_RESET), COL_TEXT);
  drawCaption(x + SP_3, y + 40, "TESTMODE + RESET");                 // the command itself, untranslated
  drawCaption(x + SP_3, y + 56, tr(S_SUB_RELOCKS), COL_MUTED);       // the honest limit
  y += 72 + CARD_GAP;
  char line[48];
  snprintf(line, sizeof(line), tr(S_ERR_NOW_FMT), bat.locked ? tr(S_LOCKEDV) : tr(S_OKSTATE));
  const uint16_t nc = bat.locked ? COL_YELLOW : COL_GREEN;
  lineCard(y, 36, nullptr, line, nc, nc);                              // 116..152
  y += 36 + SP_2;
  drawButtonPair(y, tr(S_BTN_CANCEL), tr(S_BTN_RESET));               // 160..208
  footer(S_H_CLICK_CONFIRM, S_COUNT, S_H_TURN_CANCEL);
}
// Visual feedback after an error-reset: BMS fault-register state before -> after, plus a verdict.
// Tracks bat.locked (the register the reset actually targets), not the undecoded status byte 19
// (see resetLockedBefore note).
void drawResetResult() {
  header(tr(S_HDR_RESET_RESULT));
  StrId word, line; uint16_t col;
  if (!resetLockedBefore)      { word = S_W_NO_ERROR;  line = S_SUB_ALREADY_OK; col = COL_MUTED;  }   // nothing to clear
  else if (!resetLockedAfter)  { word = S_W_CLEARED;   line = S_SUB_REREAD_OK;  col = COL_GREEN;  }   // false positive, stayed clear
  else                         { word = S_W_UNCHANGED; line = S_SUB_RELOCKS;    col = COL_YELLOW; }   // BMS re-flagged it: real fault
  statusCardText(CONTENT_Y, tr(word), col, tr(line));
  beforeAfterCard(CONTENT_Y + STATUS_H + CARD_GAP,
                  resetLockedBefore ? tr(S_LOCKEDV) : tr(S_OKSTATE), resetLockedBefore ? COL_YELLOW : COL_GREEN,
                  resetLockedAfter ? tr(S_LOCKEDV) : tr(S_OKSTATE),  resetLockedAfter ? COL_YELLOW : COL_GREEN,
                  tr(S_CAP_ERRFLAG), tr(S_CAP_REREAD));
  // Verdict now: computeVerdict() on the readAllData() that follows the reset (single source).
  const Verdict v = computeVerdict();
  lineCard(CONTENT_Y + 2 * (STATUS_H + CARD_GAP), 46, tr(S_LBL_VERDICT_NOW), verdictText(v), verdictColor(v), COL_BORDER);
  footer(S_H_CLICK_BACK);
}

// ---------- Debug / raw: the bench view, English on purpose (D33) ----------
void drawDebugRaw() {
  header("Debug / raw");
  const int x = GUTTER, w = CONTENT_W;
  drawCard(x, CONTENT_Y, w, CONTENT_H, false, RADIUS_MD);
  if (!bat.valid) {
    gfxText(FONT_LEVEL3, x + SP_3, CONTENT_Y + SP_6, "No data", COL_MUTED);
    footer(S_H_CLICK_BACK);
    return;
  }
  // One key column, one value column (the shared Debug rows); raw values never take a status colour.
  const int kx = GUTTER + SP_3, vx = GUTTER + 84, dy = FONT_LEVEL4_LINE_H + SP_1;
  int y = CONTENT_Y + SP_3;
  char b[48], k[8];
  b[0] = 0; for (int i = 0; i < 8; i++) { char o[4]; snprintf(o, sizeof(o), "%02X ", bat.romId[i]); strcat(b, o); }
  monoKv(kx, y, vx, "ROM ID", b); y += dy;
  for (int r = 0; r < 4; r++) {
    b[0] = 0; for (int i = 0; i < 8; i++) { char o[4]; snprintf(o, sizeof(o), "%02X ", bat.msg[r * 8 + i]); strcat(b, o); }
    snprintf(k, sizeof(k), "MSG %02X", r * 8);
    monoKv(kx, y, vx, k, b); y += dy;
  }
  snprintf(b, sizeof(b), "%.2f V", bat.packVoltage);                                   monoKv(kx, y, vx, "PACK", b); y += dy;
  snprintf(b, sizeof(b), "%.2f %.2f %.2f %.2f %.2f", bat.cell[0], bat.cell[1], bat.cell[2], bat.cell[3], bat.cell[4]);
  monoKv(kx, y, vx, "CELLS", b); y += dy;
  snprintf(b, sizeof(b), "%.0f / %.0f C", bat.tempCell, bat.tempMosfet);              monoKv(kx, y, vx, "TEMP", b); y += dy;
  // Raw D6 fault markers (0x58D/0x309), research only, not interpreted (D24).
  snprintf(b, sizeof(b), "%02X/%02X", bat.faultMkA, bat.faultMkB);                      monoKv(kx, y, vx, "FAULTMK", b); y += dy;
  lockCausesText(lockCauses(bat.msg), b, sizeof(b));                                   monoKv(kx, y, vx, "LOCK/CS", b); y += dy;
  // Raw status byte 19: checksum-covered but not interpreted by any known tool; never a verdict.
  snprintf(b, sizeof(b), "0x%02X (undecoded)", bat.errorCode);                          monoKv(kx, y, vx, "B19 RAW", b);
  footer(S_H_CLICK_BACK);
}

// ---------- Comm error ----------
// The pack did not answer: attention (yellow), not a fault (red is reserved for a real pack fault),
// then what actually fixes it on the bench.
void drawCommError() {
  header(tr(S_HDR_COMM_ERROR));
  statusCardText(CONTENT_Y, tr(S_W_NO_RESPONSE), COL_YELLOW, tr(S_SUB_NO_ANSWER));
  const char *steps[3] = { tr(S_STEP_DATA), tr(S_STEP_PULLUPS), tr(S_STEP_RESEAT) };   // one complete check each
  stepListCard(CONTENT_Y + STATUS_H + CARD_GAP, CONTENT_H - STATUS_H - CARD_GAP, tr(S_LBL_TRY), steps, 3);
  footer(S_H_CLICK_BACK);
}

// ---- About easter egg ----
const char* const ABOUT_EGG[] = {
  "Kickstart detected. Childhood memories loading...",
  "Insert Workbench disk and press any key",
  "A500 mode enabled. Productivity disabled",
  "No Kickstart ROMs were harmed in this process",
  "68000 instructions executed. Mostly useless ones",
  "Loading... Please wait. Like it's 1985",
  "640K should be enough for anybody",
  "Have you tried turning it off and on again? Again?",
  "Insert disk #2 of 47",
  "Memory full. Delete childhood memories?",
  "IRQ received. Nobody knows why !",
  "The scene never died. It just got a broadband connection",
  "There is no place like 127.0.0.1",
  "In space, no one can hear your hard drive click",
  "All your batteries are belong to us",
  "Wake up, Neo. The system has rebooted",
  "[NFO] No copy protection was harmed during the making of this software",
  "42. Obviously.",
  "Resistance is futile. But this software isn't.",
  "Made with love, caffeine, and questionable engineering decisions.",
  "Crafted by humans. Debugged by luck.",
  "Built for people who remember the sound of a 3.5\" floppy drive.",
  "No AI was harmed in the making of this software. ;)",
};
const int ABOUT_EGG_N = sizeof(ABOUT_EGG) / sizeof(ABOUT_EGG[0]);
// Draw a string word-wrapped and horizontally centered, classic font, from row y down.
// Classic font, word-wrapped and centred from row y. UTF-8 safe (shared helper,
// D41): the line grows one character at a time, measured with txtW(), and breaks at the last space
// that fits or, for text with no space (Japanese), at the last whole character. Never cuts one.
void drawWrapCentered(const char* s, int y, uint16_t col, uint8_t size) {
  tft.setFont(NULL); tft.setTextSize(size); tft.setTextColor(col, COL_BG);
  const int maxPx = SCREEN_W - 6 * size, ch = 8 * size;   // = the old "SCREEN_W / cw - 1" characters
  char line[60];
  int n = strlen(s), i = 0;
  while (i < n) {
    int take = 0, lastSpace = -1, w = 0;
    while (i + take < n) {                       // grow the line one character at a time
      int cl = 1;
#if ENABLE_CJK
      cl = cjkCharLen(s + i + take);
#endif
      char one[5]; memcpy(one, s + i + take, cl); one[cl] = 0;
      int cw = txtW(one, size);
      if (w + cw > maxPx || take + cl > (int)sizeof(line) - 1) break;
      if (cl == 1 && s[i + take] == ' ') lastSpace = take;
      w += cw; take += cl;
    }
    if (take == 0) take = 1;                     // pathological: always make progress
    if (i + take < n && s[i + take] != ' ' && lastSpace > 0) take = lastSpace;   // break at the last space that fits (a full line followed by a space breaks there)
    memcpy(line, s + i, take); line[take] = 0;
    int px = (SCREEN_W - txtW(line, size)) / 2; if (px < 0) px = 0;
    tft.setCursor(px, y); tft.print(line);
    y += ch + SP_1; i += take;
    while (i < n && s[i] == ' ') i++;          // eat the break space
  }
}
// Mock Amiga "Guru Meditation": black screen, blinking red border, red text. Deliberately outside the
// design system (a joke screen): its own black and red, centred on the panel.
void drawGuruCrash() {
  const uint16_t BLACK = 0x0000;
  const int bx = SP_4, by = (SCREEN_H - 118) / 2 - SP_4, bw = SCREEN_W - 2 * SP_4, bh = 118;
  if (!aboutCrashDrawn) {                       // blink only on entry, not on every extra turn
    for (int k = 0; k < 4; k++) {
      tft.fillScreen(BLACK);
      uint16_t bc = (k & 1) ? BLACK : COL_RED;
      for (int t = 0; t < 4; t++) tft.drawRect(bx + t, by + t, bw - 2 * t, bh - 2 * t, bc);
      delay(220);
    }
    aboutCrashDrawn = true;
  }
  tft.fillScreen(BLACK);
  for (int t = 0; t < 4; t++) tft.drawRect(bx + t, by + t, bw - 2 * t, bh - 2 * t, COL_RED);
  tft.setFont(NULL); tft.setTextSize(2); tft.setTextColor(COL_RED, BLACK);
  const char* l1 = "Software Failure.";
  const char* l2 = "Click to continue.";
  tft.setCursor((SCREEN_W - (int)strlen(l1) * 12) / 2, by + 20);  tft.print(l1);
  tft.setCursor((SCREEN_W - (int)strlen(l2) * 12) / 2, by + 44); tft.print(l2);
  tft.setTextSize(1);
  const char* l3 = "Guru Meditation #00000003.00C0FFEE";
  tft.setCursor((SCREEN_W - (int)strlen(l3) * 6) / 2, by + 74); tft.print(l3);
}
// About: logo box + wordmark + gamme line, version / maker line, tagline, divider, then the credits and
// the QR code (shared About composition and coordinates; the Makita gammes carry the
// Open Battery Information credit, D27). English on purpose (D33).
void drawAbout() {
  if (aboutEgg > ABOUT_EGG_N) { drawGuruCrash(); return; }   // one turn past the last line
  header(tr(S_ABOUT));
  const int bx0 = 10, by0 = CONTENT_Y;
  drawLogoBox(bx0, by0, LOGO_PO, LOGO_W, LOGO_H);
  const int rx0 = bx0 + LOGO_BOX + 10, rcx = rx0 + (SCREEN_W - rx0) / 2;
  drawWordmark(rcx, by0 + 30);
  gfxCenter(FONT_LEVEL3, rcx, by0 + 50, "Makita LXT", COL_TEXT);
  const char *vl = "v" FW_VERSION "   -   The Repair Forge";
  drawCaption((SCREEN_W - (int)strlen(vl) * 6) / 2, 106, vl, COL_MUTED);
  const char *tag = ". No Guru Meditation required .";
  drawCaption((SCREEN_W - (int)strlen(tag) * 6) / 2, 120, tag, COL_ACCENT);
  const int dy = 136;                                                   // divider
  tft.drawFastHLine(bx0, dy, SCREEN_W - 2 * bx0, COL_BORDER);
  if (aboutEgg == 0) {
    // Credits (left column, 12 px pitch) + QR code (right). The link is muted: no accent on small text.
    const int line = FONT_LEVEL4_LINE_H + SP_1;
    int cy = dy + SP_2;
    cardLabel(SP_2, cy, "Based on");                                       cy += line;
    drawCaption(SP_2, cy, "Open Battery Information (MIT)", COL_TEXT);     cy += line;
    drawCaption(SP_2, cy, "Facts: rosvall, drakosha", COL_MUTED);          cy += line;
    drawCaption(SP_2, cy, "PolyForm Noncommercial 1.0.0", COL_MUTED);      cy += line;
    drawCaption(SP_2, cy, "github.com/TheRepairforge", COL_MUTED);         cy += line;
    drawCaption(SP_2, cy, "/PocketOBI", COL_MUTED);
    qrBlock(SCREEN_W - QR_PX - SP_6 - SP_2, dy + SP_2, QR_URL, QR_PX, tr(S_SCAN));
#if BOARD_HAS_TOUCH
    footer(S_H_BACK_MENU);
#else
    footer(S_H_CLICK_BACK);
#endif
  } else {
    // Easter egg engaged: a big retro one-liner where the credits usually sit.
    drawWrapCentered(ABOUT_EGG[aboutEgg - 1], dy + SP_3, COL_CYAN, 2);
#if BOARD_HAS_TOUCH
    drawFooterHint("keep tapping...");
#else
    drawFooterHint("keep turning...");
#endif
  }
}

// ---------- PC bridge ----------
void drawPcBridge() {
  header(tr(S_PC_BRIDGE));
  statusCardText(CONTENT_Y, bridgeActive ? tr(S_W_ACTIVE) : tr(S_W_OFF), bridgeActive ? COL_GREEN : COL_MUTED,
                 bridgeActive ? tr(S_SUB_BRIDGE_ON) : tr(S_SUB_BRIDGE_OFF));
  // How to (the shared card), then the boot setting it depends on.
  int y = CONTENT_Y + STATUS_H + CARD_GAP;
  drawInfoCard(y, 50, iconBridge, tr(S_LBL_HOWTO), "Open Battery Information", tr(S_HOWTO_CAP));   // 100..150
  y += 50 + CARD_GAP;
  drawField(GUTTER, y, CONTENT_W, FIELD_H, iconSettings, tr(S_BRIDGE_BOOT), cfgBridgeBoot ? "ON" : "OFF");
  footer(S_H_TURN_TOGGLE, S_COUNT, S_H_BACK_TOOLS);
}

// Boot splash: logo, wordmark, gamme line, version, maker (shared splash composition).
void drawSplash() {
  tft.fillScreen(COL_BG);
  tft.drawBitmap((SCREEN_W - LOGO_W) / 2, 20, LOGO_PO, LOGO_W, LOGO_H, COL_ORANGE);
  drawWordmark(SCREEN_W / 2, 116);
  gfxCenter(FONT_LEVEL3, SCREEN_W / 2, 146, "Makita LXT", COL_TEXT);
  char v[24]; snprintf(v, sizeof(v), "v%s", FW_VERSION);
  gfxCenter(FONT_LEVEL3, SCREEN_W / 2, 168, v, COL_MUTED);
  gfxCenter(FONT_LEVEL3, SCREEN_W / 2, 190, "The Repair Forge", COL_ORANGE);
}

// ---------- Tools / Settings lists ----------
// Rows are drawn one at a time (same reason as the launcher tiles): a cursor move repaints two rows.
// The shared row (icon at +22, label at +44, value pill on the right), LIST_ROW_H tall so
// every row is a CYD tap target (D46 rule 4).
static void listRowChrome(int y, bool sel, IconFn icon, const char *label) {
  drawListRow(GUTTER, y, CONTENT_W, LIST_ROW_H, sel);
  icon(GUTTER + 22, y + LIST_ROW_H / 2, sel ? COL_TEXT : COL_ACCENT, ICON_SCALE_HEADER);
  gfxText(FONT_LEVEL3, GUTTER + 44, y + 26, label, sel ? COL_HEAD : COL_TEXT);
}
static void rowBadge(int y, const char *v, uint16_t fill, uint16_t text) {
  drawBadge(SCREEN_W - 2 * SP_3 - badgeWidth(v), y + (LIST_ROW_H - BADGE_H) / 2, v, fill, text);
}
// Tools: two pages of three (D46 Q2). Page 1 = the device tools, the shared Tools rows;
// page 2 = the actions that send a command to the pack, each with a WRITE pill. toolIndex is the
// position on screen; toolAt() maps it to the tool (activate() order, S_PC_BRIDGE + id).
static const int TOOL_ORDER[6] = { 0, 4, 5, 1, 2, 3 };   // bridge, debug, settings | LEDs on, LEDs off, reset
static const int TOOLS_PER_PAGE = 3;
int toolAt(int pos) { return TOOL_ORDER[pos]; }
static int toolPage() { return toolIndex / TOOLS_PER_PAGE; }
static void drawToolRow(int pos) {
  static const IconFn icons[6] = { iconBridge, iconSun, iconSunOff, iconRefresh, iconCode, iconSettings };
  const int id = TOOL_ORDER[pos], y = CONTENT_Y + (pos % TOOLS_PER_PAGE) * LIST_ROW_PITCH;
  listRowChrome(y, toolIndex == pos, icons[id], tr((StrId)(S_PC_BRIDGE + id)));
  if (id >= 1 && id <= 3) rowBadge(y, tr(S_BADGE_WRITE), COL_ORANGE, COL_BG);   // test mode + LED/reset command
}
// The page body: 3 rows (each erases its own rectangle), the legend band, the page dots. Used by
// drawTools() and by a page change, which repaints only this (never the whole screen: no flash).
static void drawToolsPage() {
  drawPageDots(toolPage(), 2);
  const int first = toolPage() * TOOLS_PER_PAGE, ly = CONTENT_Y + TOOLS_PER_PAGE * LIST_ROW_PITCH;
  tft.fillRect(0, ly, SCREEN_W, SCREEN_H - FOOTER_H - ly, COL_BG);       // legend band (empty on page 1)
  for (int p = first; p < first + TOOLS_PER_PAGE && p < toolCount; p++) drawToolRow(p);
  if (toolPage() == 1) {                                  // legend for the pill
    const char *w = tr(S_BADGE_WRITE);
    drawBadge(GUTTER, ly + SP_2, w, COL_ORANGE, COL_BG);
    drawCaption(GUTTER + badgeWidth(w) + SP_2, ly + SP_2 + 3 - jaDy(2), tr(S_WRITE_LEGEND), COL_MUTED);
  }
}
void drawTools() {
  header(tr(S_TOOLS));
  drawToolsPage();
#if BOARD_HAS_TOUCH
  footer(S_H_CLICK_OPEN, S_T_BELOW_PAGE, S_H_BACK_MENU);
#else
  footer(S_H_TURN_SELECT, S_H_CLICK_OPEN, S_H_BACK_MENU);
#endif
}
// Settings: 3 rows (4 on the CYD, with Calibrate touch: then the pitch tightens so the 4th row clears the footer).
static int settingsPitch() { return settingsCount > 3 ? LIST_ROW_H + FIELD_GAP : LIST_ROW_PITCH; }
static void drawSettingsRow(int i) {
  static const IconFn icons[4] = { iconRefresh, iconBridge, iconDoc, iconSettings };
  const int y = CONTENT_Y + i * settingsPitch();
  listRowChrome(y, settingsIndex == i, icons[i], (i <= 2) ? tr((StrId)(S_FLIP + i)) : tr(S_CALIBRATE));
  if (i < 2) {                                 // boolean toggles: ON green / OFF muted (shared pills)
    const bool on = (i == 0) ? cfgFlip : cfgBridgeBoot;
    rowBadge(y, on ? "ON" : "OFF", on ? COL_GREEN : COL_MUTED, COL_BG);
  } else if (i == 2) {                         // Language: current code (click cycles)
    rowBadge(y, langCode(lang), COL_ACCENT, COL_TEXT);
  } else {                                     // Calibrate touch: an action row that opens the calibration
    gfxText(FONT_LEVEL3, SCREEN_W - 2 * SP_3 - SP_2, y + 26, ">", COL_ACCENT);
  }
}
void drawSettings() {
  header(tr(S_SETTINGS));
  for (int i = 0; i < settingsCount; i++) drawSettingsRow(i);
  footer(S_H_TURN_SELECT, S_H_CLICK_TOGGLE, S_H_BACK_TOOLS);   // each toggle is saved at once
}
// Encoder moved the cursor on a list screen: repaint only the item that lost the selection and the
// one that gained it (never fillScreen per detent: it flashes black on the panel).
void redrawSelection(int prev, int cur) {
  switch (state) {
    case LAUNCHER: drawLauncherTile(prev); drawLauncherTile(cur); break;
    case TOOLS:                                       // crossing a page boundary redraws the page
      if (prev / TOOLS_PER_PAGE != cur / TOOLS_PER_PAGE) drawToolsPage();
      else { drawToolRow(prev); drawToolRow(cur); }
      break;
    case SETTINGS: drawSettingsRow(prev);  drawSettingsRow(cur);  break;
    default:       render();                                      break;
  }
}
// Which item a touch at (sx, sy) selects. Geometry stays owned by this file (the same helpers
// that draw the rows), so a layout change can never desynchronise the hit-testing.
int touchHit(UiState s, int sx, int sy) {
  switch (s) {
    case LAUNCHER: {
      const int col = (sx >= SCREEN_W / 2) ? 1 : 0;
      const int row = (sy >= CONTENT_Y + TILE_H + TILE_GAP / 2) ? 1 : 0;
      return row * 2 + col;
    }
    case TOOLS: {                                       // a row of the current page, or the zone under the rows
      const int i = (sy - CONTENT_Y) / LIST_ROW_PITCH;
      if (i >= TOOLS_PER_PAGE) return TOUCH_NEXT_PAGE;
      return toolPage() * TOOLS_PER_PAGE + (i < 0 ? 0 : i);
    }
    case SETTINGS: {
      int i = (sy - CONTENT_Y) / settingsPitch();
      return i < 0 ? 0 : (i >= settingsCount ? settingsCount - 1 : i);
    }
    case CONFIRM_UNLOCK:
    case CONFIRM_RESET: {
      // Only the buttons act (1 = primary, 0 = Cancel, -1 = nowhere): a stray tap must never write
      // to the pack. The unlock screen's no-button branches (F0513, nothing to do) take any tap.
      if (s == CONFIRM_UNLOCK && (strcmp(bat.commandVersion, "F0513") == 0 || !bat.valid || lockCauses(bat.msg) == 0))
        return 1;
      const int by = CONTENT_Y + 72 + CARD_GAP + 36 + SP_2, bw = (CONTENT_W - SP_2) / 2;   // drawButtonPair's row
      if (sy < by || sy >= by + BTN_H) return -1;
      if (sx >= GUTTER && sx < GUTTER + bw) return 0;
      if (sx >= GUTTER + bw + SP_2 && sx < GUTTER + CONTENT_W) return 1;
      return -1;
    }
    default: return 0;
  }
}

void render() {
  // Clear on a screen change, and on a Battery page change (different layouts).
  // Launcher/Tools/Settings selection repaint their items in place (self-erasing), no clear.
  bool clear = ((int)state != lastRenderedState);
  if (state == BATTERY && batteryPage != lastBatteryPage) clear = true;
  if (clear) tft.fillScreen(COL_BG);
  lastRenderedState = (int)state;
  lastBatteryPage = batteryPage;
  switch (state) {
    case LAUNCHER:       drawLauncher();     break;
    case BATTERY:        drawBatteryPage();  break;
    case REPAIR_DIAG:    drawWizardDiag();   break;
    case CONFIRM_UNLOCK: drawConfirmUnlock(); break;
    case UNLOCK_RESULT:  drawUnlockResult();  break;
    case CONFIRM_RESET:  drawConfirmReset(); break;
    case RESET_RESULT:   drawResetResult();  break;
    case TOOLS:          drawTools();        break;
    case SETTINGS:       drawSettings();     break;
    case DEBUG_RAW:      drawDebugRaw();     break;
    case PC_BRIDGE:      drawPcBridge();     break;
    case ABOUT:          drawAbout();        break;
    case COMM_ERROR:     drawCommError();    break;
  }
}
