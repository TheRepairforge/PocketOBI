// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// PocketOBI — screens, icons, V2 UI rendering (split from PocketOBI.ino, DECISIONS.md D21).
#include "pocketobi.h"
#include "protocol.h"
#include "unlock.h"
#include "decode.h"
#include "display.h"
#include "ui_nav.h"
#include "bridge.h"
#include "icons_bitmaps.h"
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

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
// Cell color based on its voltage and its position within the pack.
uint16_t cellColor(float v, float minV, float diff) {
  if (v < CELL_V_CRIT) return COL_RED;
  bool isLowest = (v <= minV + 0.001f);
  if (isLowest && diff > DIFF_BAD)  return COL_RED;
  if (isLowest && diff > DIFF_WARN) return COL_YELLOW;
  return COL_GREEN;
}
// Colored title bar at the top of every screen, with a small per-screen glyph
// (based on the current state) to the left of the title.
// Screen orientation (landscape, or 180 deg with cfgFlip). setRotation() writes the
// ST7789's MADCTL; a CYD fitted with an ILI9341 panel reads the MX bit the other way
// and is BGR-wired, so it gets the ILI9341 values instead (see CYD_ILI9341).
void applyRotation() {
  tft.setRotation(cfgFlip ? 3 : 1);
#if POCKETOBI_BOARD == BOARD_CYD && CYD_ILI9341
  uint8_t madctl = cfgFlip ? 0xE8 : 0x28;   // MX|MY|MV|BGR flipped, MV|BGR normal
  tft.sendCommand(ST77XX_MADCTL, &madctl, 1);
#endif
}

void drawHeader(const char* title) {
  tft.fillRect(0, 0, tft.width(), HEADER_H, COL_ACCENT);
  int gx = 15, gy = HEADER_H / 2; uint16_t gc = COL_HEAD;
  bool glyph = true;
  switch (state) {
    case BATTERY:        iconBattery(gx, gy, gc); break;
    case REPAIR_DIAG:
    case CONFIRM_UNLOCK:
    case UNLOCK_RESULT:  iconKey(gx, gy, gc);     break;
    case TOOLS:          iconList(gx, gy, gc);    break;
    case SETTINGS:                                          // sliders glyph
      tft.drawFastHLine(gx - 8, gy - 4, 16, gc); tft.fillCircle(gx - 2, gy - 4, 2, gc);
      tft.drawFastHLine(gx - 8, gy,     16, gc); tft.fillCircle(gx + 4, gy,     2, gc);
      tft.drawFastHLine(gx - 8, gy + 4, 16, gc); tft.fillCircle(gx - 4, gy + 4, 2, gc);
      break;
    case PC_BRIDGE:      iconBridge(gx, gy, gc);  break;
    case DEBUG_RAW:      iconCode(gx, gy, gc);    break;
    case ABOUT:          iconInfo(gx, gy, gc);    break;
    case CONFIRM_RESET:
    case RESET_RESULT:   iconRefresh(gx, gy, gc); break;
    default:             glyph = false;           break;   // LAUNCHER / COMM_ERROR: no glyph
  }
  tft.setFont(&FreeSansBold9pt7b);      // smoother title
  tft.setTextSize(1);
  tft.setTextColor(COL_HEAD, COL_ACCENT);
  tft.setCursor(glyph ? 30 : 8, 20);    // shift title right when a glyph is shown
  tft.print(title);
  tft.setFont(NULL);                    // restore the classic font for the rest of the screen
}
// ---------- Menu icons (drawn with primitives, ~16px, centered on cx,cy) ----------
void iconBattery(int cx, int cy, uint16_t c) {
  tft.drawRect(cx - 8, cy - 5, 13, 10, c);
  tft.fillRect(cx + 5, cy - 2, 2, 4, c);            // + terminal nub
  for (int k = 0; k < 3; k++) tft.fillRect(cx - 6 + k * 3, cy - 3, 2, 6, c);
}
void iconList(int cx, int cy, uint16_t c) {
  for (int k = 0; k < 3; k++) {
    int yy = cy - 5 + k * 5;
    tft.fillRect(cx - 8, yy, 2, 2, c);
    tft.drawFastHLine(cx - 4, yy + 1, 11, c);
  }
}
void iconRefresh(int cx, int cy, uint16_t c) {
  tft.drawCircle(cx, cy, 6, c);
  tft.fillTriangle(cx + 2, cy - 9, cx + 2, cy - 2, cx + 8, cy - 5, c); // arrowhead
}
void iconSun(int cx, int cy, uint16_t c) {                            // LEDs on
  tft.fillCircle(cx, cy, 3, c);
  tft.drawFastVLine(cx, cy - 8, 3, c);   tft.drawFastVLine(cx, cy + 6, 3, c);
  tft.drawFastHLine(cx - 8, cy, 3, c);   tft.drawFastHLine(cx + 6, cy, 3, c);
  tft.drawLine(cx - 6, cy - 6, cx - 4, cy - 4, c);
  tft.drawLine(cx + 4, cy + 4, cx + 6, cy + 6, c);
  tft.drawLine(cx - 6, cy + 6, cx - 4, cy + 4, c);
  tft.drawLine(cx + 4, cy - 4, cx + 6, cy - 6, c);
}
void iconSunOff(int cx, int cy, uint16_t c) {                         // LEDs off
  tft.drawCircle(cx, cy, 4, c);
}
void iconCode(int cx, int cy, uint16_t c) {
  tft.drawLine(cx - 2, cy - 5, cx - 7, cy, c); tft.drawLine(cx - 7, cy, cx - 2, cy + 5, c);
  tft.drawLine(cx + 2, cy - 5, cx + 7, cy, c); tft.drawLine(cx + 7, cy, cx + 2, cy + 5, c);
}
void iconKey(int cx, int cy, uint16_t c) {                           // unlock / repair
  tft.drawCircle(cx - 4, cy, 4, c);        // bow
  tft.drawFastHLine(cx, cy, 9, c);         // shaft
  tft.drawFastVLine(cx + 6, cy, 4, c);     // tooth
  tft.drawFastVLine(cx + 8, cy, 3, c);     // tooth
}
void iconInfo(int cx, int cy, uint16_t c) {
  tft.drawCircle(cx, cy, 7, c);
  tft.fillRect(cx - 1, cy - 4, 2, 2, c);   // dot
  tft.fillRect(cx - 1, cy - 1, 2, 5, c);   // stem
}
void iconBridge(int cx, int cy, uint16_t c) {                        // PC bridge = two arrows
  tft.drawFastHLine(cx - 7, cy - 3, 12, c);
  tft.drawLine(cx + 5, cy - 3, cx + 2, cy - 6, c);
  tft.drawLine(cx + 5, cy - 3, cx + 2, cy, c);
  tft.drawFastHLine(cx - 5, cy + 3, 12, c);
  tft.drawLine(cx - 5, cy + 3, cx - 2, cy + 6, c);
  tft.drawLine(cx - 5, cy + 3, cx - 2, cy, c);
}
void drawConfirmReset() {
  drawHeader(tr(S_RESET_ERROR_Q));
  int y = HEADER_H + 12;
  tft.setTextSize(2);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setCursor(6, y);          tft.print(tr(S_CMD_SENT));
  tft.setTextColor(COL_MUTED, COL_BG);
  tft.setCursor(6, y + 24);     tft.print("TESTMODE + RESET");
  tft.setTextColor(COL_GREEN, COL_BG);
  tft.setCursor(6, y + 66);     tft.print(tr(S_CLICK_CONFIRM));
  tft.setTextColor(COL_RED, COL_BG);
  tft.setCursor(6, y + 92);     tft.print(tr(S_TURN_CANCEL));
}
// Visual feedback after an error-reset: BMS fault-register state before -> after,
// plus a verdict. Tracks bat.locked (the register the reset actually targets), not
// the undecoded status byte 19 (see resetLockedBefore note).
void drawResetResult() {
  drawHeader(tr(S_RESET_DONE));
  int y = HEADER_H + 10;
  tft.setTextSize(2);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setCursor(6, y);      tft.printf("%s: %s", tr(S_BEFORE), resetLockedBefore ? tr(S_LOCKEDV) : tr(S_OKSTATE));
  tft.setCursor(6, y + 24); tft.printf("%s: %s", tr(S_AFTER),  resetLockedAfter  ? tr(S_LOCKEDV) : tr(S_OKSTATE));

  tft.setCursor(6, y + 60);
  if (!resetLockedBefore) {
    // Nothing was flagged, so there was nothing to clear.
    tft.setTextColor(COL_MUTED, COL_BG);
    tft.print(tr(S_NO_ERROR));
  } else if (!resetLockedAfter) {
    // False positive: the fault register cleared and stayed clear.
    tft.setTextColor(COL_GREEN, COL_BG);
    tft.print(tr(S_ERR_CLEARED));
  } else {
    // Real fault: the BMS re-flagged it -> the reset did not hold.
    tft.setTextColor(COL_YELLOW, COL_BG);
    tft.print(tr(S_UNCHANGED));
  }

  tft.setTextColor(COL_MUTED, COL_BG);
  tft.setCursor(6, 220);
  tft.print(tr(S_HINT_CLICK_BACK_P));
}
// Compact text for a lock-cause bitmask, e.g. "CS0 CS2 N34" or "none".
void lockCausesText(uint8_t causes, char *out, size_t n) {
  out[0] = 0;
  if (causes == 0) { strncpy(out, tr(S_NONE), n); out[n - 1] = 0; return; }
  if (causes & LF_N34) strncat(out, "N34 ", n - strlen(out) - 1);
  if (causes & LF_CS0) strncat(out, "CS0 ", n - strlen(out) - 1);
  if (causes & LF_CS1) strncat(out, "CS1 ", n - strlen(out) - 1);
  if (causes & LF_CS2) strncat(out, "CS2 ", n - strlen(out) - 1);
}
// Confirmation before writing to the BMS flash. Shows the detected charger-lock
// causes and a clear "writes flash" safety warning. If nothing is
// locked (or F0513), clicking just returns to the menu (no write is performed).
void drawConfirmUnlock() {
  { char h[16]; snprintf(h, sizeof(h), "%s 2/4", tr(S_REPAIR)); drawHeader(h); }
  drawPageDots(1, 4);
  int y = HEADER_H + 8;
  bool isF0513 = strcmp(bat.commandVersion, "F0513") == 0;

  tft.setTextSize(2);
  if (isF0513) {
    tft.setTextColor(COL_YELLOW, COL_BG);
    tft.setCursor(6, y);      tft.print(tr(S_NOT_SUPPORTED));
    tft.setTextColor(COL_MUTED, COL_BG);
    tft.setCursor(6, 220);    tft.print(tr(S_HINT_CLICK_TURN_BACK));
    return;
  }

  uint8_t causes = bat.valid ? lockCauses(bat.msg) : 0;
  char cbuf[24];
  lockCausesText(causes, cbuf, sizeof(cbuf));

  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setCursor(6, y);        tft.print(tr(S_CHARGER_LOCK)); tft.print(":");
  tft.setTextColor(causes ? COL_RED : COL_GREEN, COL_BG);
  tft.setCursor(6, y + 22);   tft.print(cbuf);

  if (causes == 0) {
    tft.setTextColor(COL_GREEN, COL_BG);
    tft.setCursor(6, y + 56);  tft.print(tr(S_FRAME_ALREADY_VALID));
    tft.setTextColor(COL_MUTED, COL_BG);
    tft.setTextSize(1);
    tft.setCursor(6, y + 82);  tft.print(tr(S_NOTHING_NO_WRITE));
    tft.setTextSize(2);
    tft.setTextColor(COL_MUTED, COL_BG);
    tft.setCursor(6, 220);     tft.print(tr(S_HINT_CLICK_TURN_BACK));
    return;
  }

  // Warning block (writes flash, untested)
  tft.setTextSize(1);
  tft.setTextColor(COL_RED, COL_BG);
  tft.setCursor(6, y + 52);   tft.print(tr(S_WARN_WRITES_FLASH));
  tft.setCursor(6, y + 64);   tft.print("Sets nybble34=0, recomputes CS0/1/2.");
  tft.setCursor(6, y + 76);   tft.print(tr(S_REPAIRS_FALSE_ONLY));

  tft.setTextSize(2);
  tft.setTextColor(COL_GREEN, COL_BG);
  tft.setCursor(6, y + 100);  tft.print(tr(S_CLICK_WRITE));
  tft.setTextColor(COL_RED, COL_BG);
  tft.setCursor(6, y + 124);  tft.print(tr(S_TURN_CANCEL));
}
// Result after an unlock attempt: lock causes before -> after, plus a verdict.
void drawUnlockResult() {
  { char h[16]; snprintf(h, sizeof(h), "%s 4/4", tr(S_REPAIR)); drawHeader(h); }
  drawPageDots(3, 4);
  int y = HEADER_H + 10;
  char b[24], a[24];
  lockCausesText(unlockCausesBefore, b, sizeof(b));
  lockCausesText(unlockCausesAfter, a, sizeof(a));

  tft.setTextSize(2);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setCursor(6, y);        tft.printf("%s: %s", tr(S_BEFORE), b);
  tft.setCursor(6, y + 24);   tft.printf("%s: %s", tr(S_AFTER), a);

  tft.setCursor(6, y + 60);
  if (unlockCausesAfter == 0xFF) {
    tft.setTextColor(COL_YELLOW, COL_BG);
    tft.print(tr(S_NO_READ_AFTER));
  } else if (unlockCausesAfter == 0) {
    tft.setTextColor(COL_GREEN, COL_BG);
    tft.print(tr(S_UNLOCKED_RES));
  } else {
    tft.setTextColor(COL_RED, COL_BG);
    tft.print(tr(S_STILL_LOCKED));
  }

  tft.setTextColor(COL_MUTED, COL_BG);
  tft.setTextSize(1);
  tft.setCursor(6, y + 92);   tft.print(tr(S_RETRY_REINSERT));

  tft.setTextColor(COL_MUTED, COL_BG);
  tft.setTextSize(2);
  tft.setCursor(6, 220);      tft.print(tr(S_HINT_CLICK_BACK_P));
}
void drawDebugRaw() {
  drawHeader("DEBUG / RAW");
  int y = HEADER_H + 6;
  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED, COL_BG);
  if (!bat.valid) {
    tft.setTextSize(2);
    tft.setCursor(6, y);
    tft.print("No data");
    return;
  }
  tft.setCursor(6, y);
  tft.print("ROM ID");
  tft.setTextColor(COL_GREEN, COL_BG);
  tft.setCursor(6, y + 12);
  for (int i = 0; i < 8; i++) tft.printf("%02X ", bat.romId[i]);
  tft.setTextColor(COL_MUTED, COL_BG);
  tft.setCursor(6, y + 32);
  tft.print("Message (32B)");
  tft.setTextColor(COL_GREEN, COL_BG);
  for (int i = 0; i < 32; i++) {
    if (i % 8 == 0) tft.setCursor(6, y + 44 + (i / 8) * 12);
    tft.printf("%02X ", bat.msg[i]);
  }
  int ly = y + 98;
  tft.setTextColor(COL_MUTED, COL_BG); tft.setCursor(6, ly); tft.print("Live");
  tft.setTextColor(COL_TEXT, COL_BG);  tft.setCursor(6, ly + 12);
  tft.printf("Pk %.2fV C %.2f/%.2f/%.2f/%.2f/%.2f", bat.packVoltage,
             bat.cell[0], bat.cell[1], bat.cell[2], bat.cell[3], bat.cell[4]);
  tft.setCursor(6, ly + 24);
  // Raw D6 fault markers (0x58D/0x309), shown for research only - not interpreted as a fault
  // (D24: 0x0B/0x4x is the healthy BL1850B constant, not a latch).
  tft.printf("T %.0f/%.0f  faultMk %02X/%02X", bat.tempCell, bat.tempMosfet, bat.faultMkA, bat.faultMkB);
  uint8_t causes = lockCauses(bat.msg);
  char cb[24]; lockCausesText(causes, cb, sizeof(cb));
  tft.setTextColor(COL_MUTED, COL_BG); tft.setCursor(6, ly + 40); tft.print("Lock/CS: ");
  tft.setTextColor(causes ? COL_RED : COL_GREEN, COL_BG); tft.print(cb);
  // Raw status byte 19: checksum-covered but not interpreted by any known tool.
  // Shown here for the curious only; never used as a verdict.
  tft.setTextColor(COL_MUTED, COL_BG); tft.setCursor(6, ly + 52);
  tft.printf("b19 raw: 0x%02X (undecoded)", bat.errorCode);
}
void drawCommError() {
  drawHeader(tr(S_COMM_ERROR_HDR));
  int y = HEADER_H + 12;
  tft.setTextSize(2);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setCursor(6, y);          tft.print(tr(S_NO_RESPONSE));
  tft.setTextColor(COL_MUTED, COL_BG);
  tft.setCursor(6, y + 30);     tft.print(tr(S_CHECK_DATA));
  tft.setCursor(6, y + 52);     tft.print(tr(S_CHECK_PULLUPS));
  tft.setCursor(6, 220);        tft.print(tr(S_HINT_CLICK_BACK_P));
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
void drawWrapCentered(const char* s, int y, uint16_t col, uint8_t size) {
  tft.setTextSize(size); tft.setTextColor(col, COL_BG);
  const int cw = 6 * size, ch = 8 * size, maxc = 320 / cw - 1;
  int n = strlen(s), i = 0;
  char line[52];
  while (i < n) {
    int take = (n - i > maxc) ? maxc : (n - i);
    if (i + take < n) {                       // break at the last space that fits
      int br = take; while (br > 0 && s[i + br] != ' ') br--;
      if (br > 0) take = br;
    }
    int len = take < 51 ? take : 51;
    memcpy(line, s + i, len); line[len] = 0;
    int px = (320 - (int)strlen(line) * cw) / 2; if (px < 0) px = 0;
    tft.setCursor(px, y); tft.print(line);
    y += ch + 4; i += take;
    while (i < n && s[i] == ' ') i++;          // eat the break space
  }
}
// Mock Amiga "Guru Meditation": black screen, blinking red border, red text.
void drawGuruCrash() {
  const uint16_t BLACK = 0x0000;
  if (!aboutCrashDrawn) {                       // blink only on entry, not on every extra turn
    for (int k = 0; k < 4; k++) {
      tft.fillScreen(BLACK);
      uint16_t bc = (k & 1) ? BLACK : COL_RED;
      for (int t = 0; t < 4; t++) tft.drawRect(16 + t, 60 + t, 288 - 2 * t, 118 - 2 * t, bc);
      delay(220);
    }
    aboutCrashDrawn = true;
  }
  tft.fillScreen(BLACK);
  for (int t = 0; t < 4; t++) tft.drawRect(16 + t, 60 + t, 288 - 2 * t, 118 - 2 * t, COL_RED);
  tft.setTextSize(2); tft.setTextColor(COL_RED, BLACK);
  const char* l1 = "Software Failure.";
  const char* l2 = "Click to continue.";
  tft.setCursor((320 - (int)strlen(l1) * 12) / 2, 80);  tft.print(l1);
  tft.setCursor((320 - (int)strlen(l2) * 12) / 2, 104); tft.print(l2);
  tft.setTextSize(1);
  const char* l3 = "Guru Meditation #00000003.00C0FFEE";
  tft.setCursor((320 - (int)strlen(l3) * 6) / 2, 134); tft.print(l3);
}
void drawAbout() {
  if (aboutEgg > ABOUT_EGG_N) { drawGuruCrash(); return; }   // one turn past the last line
  drawHeader(tr(S_ABOUT));
  // Logo mark on the LEFT, name to its RIGHT (side by side). The old stacked layout
  // (centered box above a centered name) clipped the top of "PocketOBI"; keeping the
  // logo hard-left and the name in the space to its right removes the overlap.
  tft.fillRoundRect(10, 33, 56, 56, 10, RGB565(0x12, 0x30, 0x39));
  tft.drawRoundRect(10, 33, 56, 56, 10, COL_ACCENT);
  tft.drawBitmap(14, 37, LOGO_PO, LOGO_W, LOGO_H, COL_ORANGE);
  // Name: Pocket (teal) + OBI (orange), smooth GFX, centered in the space right of the logo.
  tft.setFont(&FreeSansBold18pt7b); tft.setTextSize(1);
  int16_t bx, by; uint16_t w1, w2, hh;
  tft.getTextBounds("Pocket", 0, 0, &bx, &by, &w1, &hh);
  tft.getTextBounds("OBI", 0, 0, &bx, &by, &w2, &hh);
  int nameW = (int)(w1 + w2 + 8);
  int sx = 76 + ((320 - 76) - nameW) / 2;   // centered in the region to the right of the logo
  int ny = 30 + (56 + (int)hh) / 2;         // baseline vertically centers the name on the logo box
  tft.setTextColor(COL_ACCENT); tft.setCursor(sx, ny); tft.print("Pocket");
  tft.setTextColor(COL_ORANGE); tft.setCursor(sx + w1 + 8, ny); tft.print("OBI");
  tft.setFont(NULL);
  tft.setTextSize(1);
  const char* vl  = "v" FW_VERSION "   -   The Repair Forge";
  tft.setTextColor(COL_MUTED, COL_BG); tft.setCursor((320 - (int)strlen(vl) * 6) / 2, 112); tft.print(vl);

  if (aboutEgg == 0) {
    const char* tag = ". No Guru Meditation required .";
    tft.setTextColor(COL_ACCENT, COL_BG); tft.setCursor((320 - (int)strlen(tag) * 6) / 2, 126); tft.print(tag);
    tft.drawFastHLine(10, 142, 300, COL_PANEL);
    // Credits (left column) + QR code (right).
    tft.setTextColor(COL_MUTED, COL_BG);
    tft.setCursor(8, 154); tft.print("Based on Open Battery Info (MIT)");
    tft.setCursor(8, 168); tft.print("Facts: rosvall, drakosha");
    tft.setCursor(8, 182); tft.print("PolyForm Noncommercial 1.0.0");
    tft.setTextColor(COL_ACCENT, COL_BG);
    tft.setCursor(8, 200); tft.print("github.com/TheRepairforge");
    tft.setCursor(8, 212); tft.print("/PocketOBI");
    int qx = 244, qy = 150;
    tft.fillRect(qx - 3, qy - 3, QR_PX + 6, QR_PX + 6, 0xFFFF);   // white quiet zone
    tft.drawBitmap(qx, qy, QR_URL, QR_PX, QR_PX, 0x0000);         // black modules
    tft.setTextColor(COL_MUTED, COL_BG);
    tft.setCursor(qx + 18, qy + QR_PX + 6); tft.print(tr(S_SCAN));
    tft.setCursor(8, 228); tft.print(tr(S_HINT_CLICK_BACK));
  } else {
    // Easter egg engaged: a big retro one-liner where the credits usually sit.
    drawWrapCentered(ABOUT_EGG[aboutEgg - 1], 148, COL_CYAN, 2);
    tft.setTextSize(1); tft.setTextColor(COL_MUTED, COL_BG);
#if BOARD_HAS_TOUCH
    const char* h = "keep tapping...";
#else
    const char* h = "keep turning...";
#endif
    tft.setCursor((320 - (int)strlen(h) * 6) / 2, 226); tft.print(h);
  }
}
void drawPcBridge() {
  drawHeader("PC BRIDGE");
  tft.setTextSize(3);
  tft.setTextColor(COL_ACCENT, COL_BG);
  tft.setCursor(30, 60);
  tft.print(tr(S_PC_MODE));
  // Live status line (cleared each redraw so ACTIVE <-> INACTIVE swaps cleanly).
  tft.fillRect(0, 100, 320, 26, COL_BG);
  uint16_t sc = bridgeActive ? COL_GREEN : COL_MUTED;
  tft.fillCircle(14, 114, 6, sc);
  tft.setTextSize(2); tft.setTextColor(sc, COL_BG);
  tft.setCursor(28, 108);  tft.print(bridgeActive ? tr(S_BRIDGE_ACTIVE) : tr(S_BRIDGE_INACTIVE));
  tft.setTextColor(COL_MUTED, COL_BG);
  tft.setTextSize(1);
  tft.setCursor(6, 138);  tft.print(tr(S_PC_HELP1));
  tft.setCursor(6, 150);  tft.print(tr(S_PC_HELP2));
  tft.setCursor(6, 162);  tft.print(tr(S_PC_HELP3));
  tft.setCursor(6, 220);  tft.print(tr(S_HINT_TURN_TOGGLE_EXIT));
}
// Boot splash: name in big two-tone letters, "Pocket" (teal) + "OBI" (orange).
void drawSplash() {
  tft.fillScreen(COL_BG);
  tft.drawBitmap((320 - LOGO_W) / 2, 26, LOGO_PO, LOGO_W, LOGO_H, COL_ORANGE);
  tft.setFont(&FreeSansBold18pt7b); tft.setTextSize(1);
  int16_t bx, by; uint16_t w1, w2, hh;
  tft.getTextBounds("Pocket", 0, 0, &bx, &by, &w1, &hh);
  tft.getTextBounds("OBI", 0, 0, &bx, &by, &w2, &hh);
  int sx = (320 - (int)(w1 + w2 + 8)) / 2;
  tft.setTextColor(COL_ACCENT); tft.setCursor(sx, 128); tft.print("Pocket");
  tft.setTextColor(COL_ORANGE); tft.setCursor(sx + w1 + 8, 128); tft.print("OBI");
  tft.setFont(NULL);
  gfxCenter(&FreeSansBold9pt7b, 160, 162, "standalone OBI client", COL_MUTED);
  char v[24]; snprintf(v, sizeof(v), "v%s", FW_VERSION);
  gfxCenter(&FreeSansBold9pt7b, 160, 192, v, COL_MUTED);
}
// Brief centered confirmation banner (blocking ~0.9s), then forces a redraw.
void toast(const char* msg, uint16_t col) {
  int w = txtW(msg, 2);
  tft.fillRect(30, 96, 260, 48, col);
  tft.drawRect(30, 96, 260, 48, COL_BG);
  tft.setTextSize(2); tft.setTextColor(COL_BG, col);
  tft.setCursor((320 - w) / 2, 112); tft.print(msg);
  delay(900);
  lastRenderedState = -1;  // force a full redraw on the next render()
}
// key/value row (size 2, value right-aligned) using the shared _ry cursor.
// smooth-font helpers (labels/chrome). Classic font stays for data/values.
// NOTE: GFX custom fonts are scaled by textSize in Adafruit_GFX. Callers often
// leave textSize at 2 (for classic values), which would DOUBLE the smooth font.
// Always force textSize(1) so a GFX label renders at its true point size.
// In the JA build the print draws Japanese itself (PocketTFT); only the width differs,
// because getTextBounds() does not know the bitmap glyphs.
int gfxText(const GFXfont* f, int x, int baseY, const char* s, uint16_t col) {
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
void gfxCenter(const GFXfont* f, int cx, int baseY, const char* s, uint16_t col) {
#if ENABLE_CJK
  if (cjkHasWide(s)) { gfxText(f, cx - cjkWidth(f, s) / 2, baseY, s, col); return; }
#endif
  tft.setFont(f); tft.setTextSize(1); tft.setTextColor(col);
  int16_t x1, y1; uint16_t w, h; tft.getTextBounds(s, 0, baseY, &x1, &y1, &w, &h);
  tft.setCursor(cx - w / 2, baseY); tft.print(s); tft.setFont(NULL);
}
// Pixel width gfxText() would draw s at (for centering a label next to an icon).
int gfxWidth(const GFXfont* f, const char* s) {
#if ENABLE_CJK
  if (cjkHasWide(s)) return cjkWidth(f, s);
#endif
  tft.setFont(f); tft.setTextSize(1);
  int16_t x1, y1; uint16_t w, h; tft.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  tft.setFont(NULL);
  return w;
}
// Dense-row label (topY = value top of the row).
// Returns the label pixel width so inline layouts can place the value after it.
int rowLabel(int x, int topY, const char* s) {
  // baseline at topY+13 aligns the label's bottom with the classic size-2 value's bottom
  return gfxText(&FreeSansBold9pt7b, x, topY + 13, s, COL_MUTED);
}
// key/value row: label (A/B font) + value in the classic font, right-aligned.
void kvRow(const char* k, const char* val, uint16_t col) {
  rowLabel(6, _ry, k);
  tft.setTextSize(2);
  int vw = txtW(val, 2);
  tft.setTextColor(col, COL_BG); tft.setCursor(314 - vw, _ry); tft.print(val);
  _ry += 24;
}
// Draw a proper "degree + C" at the current text cursor (avoids the broken font glyph).
void degC(uint16_t col) {
  int x = tft.getCursorX(), y = tft.getCursorY();
  tft.drawCircle(x + 3, y + 2, 2, col);
  tft.setTextColor(col, COL_BG); tft.setCursor(x + 8, y); tft.print("C");
}
// Small page-position dots at the top-right of the header (e.g. Battery 3 pages).
void drawPageDots(int active, int count) {
  int dw = 8, gap = 5, tot = count * dw + (count - 1) * gap;
  int x0 = 320 - tot - 8, y = (HEADER_H - dw) / 2;
  for (int i = 0; i < count; i++)
    tft.fillRoundRect(x0 + i * (dw + gap), y, dw, dw, 2,
                      i == active ? COL_BG : RGB565(0x0A, 0x4A, 0x52));
}
// Colored verdict banner across the bottom, with a status icon (dark on the color):
// HEALTHY = check in a circle, REPAIRABLE = warning triangle "!", REAL FAULT = X in a circle.
void drawVerdictBanner(Verdict v) {
  uint16_t c = verdictColor(v);
  tft.fillRect(0, 208, 320, 32, c);
  const char* t = verdictText(v);
  int bw = gfxWidth(&FreeSansBold9pt7b, t);
  bool hasIcon = (v == V_HEALTHY || v == V_REPAIRABLE || v == V_SUSPECT || v == V_FAULT);
  int iconW = hasIcon ? 24 : 0, gap = hasIcon ? 10 : 0;
  int sx = (320 - (iconW + gap + bw)) / 2;
  int yc = 224, r = 10, cx = sx + 11;
  uint16_t fg = COL_BG;
  if (v == V_HEALTHY) {                                  // check in a circle
    tft.drawCircle(cx, yc, r, fg); tft.drawCircle(cx, yc, r - 1, fg);
    tft.drawLine(cx - 5, yc,     cx - 1, yc + 5, fg); tft.drawLine(cx - 1, yc + 5, cx + 6, yc - 5, fg);
    tft.drawLine(cx - 5, yc + 1, cx - 1, yc + 6, fg); tft.drawLine(cx - 1, yc + 6, cx + 6, yc - 4, fg);
  } else if (v == V_FAULT) {                             // X in a circle
    tft.drawCircle(cx, yc, r, fg); tft.drawCircle(cx, yc, r - 1, fg);
    tft.drawLine(cx - 4, yc - 4, cx + 4, yc + 4, fg); tft.drawLine(cx - 4, yc + 4, cx + 4, yc - 4, fg);
    tft.drawLine(cx - 5, yc - 4, cx + 3, yc + 4, fg); tft.drawLine(cx - 5, yc + 4, cx + 3, yc - 4, fg);
  } else if (v == V_REPAIRABLE || v == V_SUSPECT) {      // warning triangle with "!"
    tft.drawTriangle(cx, yc - 9, cx - 10, yc + 8, cx + 10, yc + 8, fg);
    tft.drawTriangle(cx, yc - 8, cx - 9,  yc + 7, cx + 9,  yc + 7, fg);
    tft.fillRect(cx - 1, yc - 3, 3, 6, fg); tft.fillRect(cx - 1, yc + 4, 3, 3, fg);
  }
  gfxText(&FreeSansBold9pt7b, sx + iconW + gap, 228, t, COL_BG);
}
// ---- Launcher (2x2 big tiles) ----
// Selection is the ONLY border treatment (thick cyan + lit bg) so it's unambiguous;
// the Battery tile's verdict is shown by its icon + info-text color, not a border ring.
void drawTile(int x, int y, int w, int h, bool sel) {
  tft.fillRoundRect(x, y, w, h, 8, sel ? RGB565(0x12, 0x30, 0x39) : COL_PANEL);
  tft.drawRoundRect(x, y, w, h, 8, sel ? COL_ACCENT : RGB565(0x26, 0x30, 0x40));
  if (sel)  tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 7, COL_ACCENT); // 2px cyan
}
void drawLauncher() {
  drawHeader("PocketOBI");
  Verdict v = computeVerdict();
  const int gap = 8, top = HEADER_H + 8;
  int tw = (320 - gap * 3) / 2;
  int th = (240 - top - gap * 2) / 2;
  int xs[2] = { gap, gap * 2 + tw };
  int ys[2] = { top, top + th + gap };
  for (int i = 0; i < 4; i++) {
    int x = xs[i % 2], y = ys[i / 2];
    bool sel = (launcherIndex == i);
    drawTile(x, y, tw, th, sel);
    int cx = x + tw / 2;
    const uint8_t* ic = (i == 0) ? ICON_BATTERY : (i == 1) ? ICON_REPAIR
                       : (i == 2) ? ICON_TOOLS  : ICON_INFO;
    uint16_t icc = (i == 0) ? (bat.valid ? verdictColor(v) : COL_MUTED)
                 : (i == 1) ? COL_ORANGE : (i == 2) ? COL_ACCENT : COL_CYAN;
    // Icons and labels align across all tiles; the Battery tile adds a V+verdict line between them.
    tft.drawBitmap(cx - ICON_W / 2, y + 14, ic, ICON_W, ICON_H, icc);
    if (i == 0) {                               // Battery: V + verdict under the icon (verdict color)
      char ms[48];
      if (bat.valid) { snprintf(ms, sizeof(ms), "%.1fV %s", bat.packVoltage, verdictText(v));
                       tft.setTextColor(verdictColor(v)); }
      else           { strcpy(ms, "no pack"); tft.setTextColor(COL_MUTED); }
      tft.setTextSize(1);
      // centered in the gap between the icon's visible bottom (~y+48) and the label (~y+70)
      tft.setCursor(cx - txtW(ms, 1) / 2, y + 55); tft.print(ms);
    }
    gfxCenter(&FreeSansBold9pt7b, cx, y + 82, tr((StrId)(S_BATTERY + i)), sel ? COL_HEAD : COL_TEXT);
  }
}
// ---- Battery : Health page ----
void drawBatteryHealth() {
  drawHeader(tr(S_HDR_HEALTH));
  drawPageDots(1, 3);
  int y = HEADER_H + 8;
  char buf[24];
  uint8_t soh = bat.healthEstPct;
  uint16_t cc = soh >= 80 ? COL_GREEN : (soh >= 50 ? COL_YELLOW : COL_RED);
  tft.setTextSize(2);
  tft.setTextColor(COL_TEXT, COL_BG); tft.setCursor(6, y); tft.print(tr(S_CONDITION));
  snprintf(buf, sizeof(buf), "%u%%", soh);
  int vw = strlen(buf) * 12;
  tft.setTextColor(cc, COL_BG); tft.setCursor(314 - vw, y); tft.print(buf);
  tft.drawRect(6, y + 22, 308, 14, COL_PANEL);
  int fw = (304 * (soh > 100 ? 100 : soh)) / 100;
  tft.fillRect(8, y + 24, fw, 10, cc);

  // "Condition" is OUR OWN cycle-based estimate, not the BMS's SOH gauge. The raw
  // charge level (D4 0x150 / SOC) is exposed alongside it as a diagnostic value.
  tft.setTextSize(1); tft.setTextColor(COL_MUTED, COL_BG);
  tft.setCursor(6, y + 38);
  if (bat.extValid) tft.printf("est. from cycles  -  SOC raw %u", bat.socRaw);
  else              tft.print("est. from cycles");

  float mn = 9, mx = 0;
  for (int i = 0; i < cellCount; i++) { if (bat.cell[i] > 0.1f && bat.cell[i] < mn) mn = bat.cell[i];
                                if (bat.cell[i] > mx) mx = bat.cell[i]; }
  _ry = y + 54;
  // Wear counters (D4): RAW count is the primary figure; the "% of cycles" is SECONDARY
  // (unproven) so it is shown only in parentheses. Never presented as a saturated fact.
  // Extended wear data lives on the CC-addressed D4 path; when that is unavailable (old
  // packs readable only via the AA frame + F0513 cells) show "-" rather than a false "none".
  if (!bat.extValid) strcpy(buf, "-");
  else if (bat.odEventCount == 0) strcpy(buf, tr(S_NONE));
  else snprintf(buf, sizeof(buf), "%u (%u%%)", bat.odEventCount, bat.odWearPct);
  kvRow(tr(S_OVERDISCHARGE), buf, !bat.extValid ? COL_MUTED : (bat.odEventCount ? COL_TEXT : COL_GREEN));
  if (!bat.extValid) strcpy(buf, "-");
  else if (bat.olEventCount == 0) strcpy(buf, tr(S_NONE));
  else snprintf(buf, sizeof(buf), "%u (%u%%)", bat.olEventCount, bat.olWearPct);
  kvRow(tr(S_OVERLOAD), buf, !bat.extValid ? COL_MUTED : (bat.olEventCount ? COL_TEXT : COL_GREEN));
  snprintf(buf, sizeof(buf), "%.2f-%.2fV", mn, mx);
  kvRow(tr(S_CELLS), buf, bat.cellDiff > DIFF_BAD ? COL_RED : COL_GREEN);
  { uint16_t tc = thermistorFault() ? COL_RED : (thermistorSuspect() ? COL_ORANGE : COL_GREEN);
    rowLabel(6, _ry, tr(S_TEMP_CB));                       // "Cell/Board": value order = cell then board
    char tb[16];
    if (bat.boardTempValid) snprintf(tb, sizeof(tb), "%.0f/%.0f", bat.tempCell, bat.tempMosfet);
    else                    snprintf(tb, sizeof(tb), "%.0f", bat.tempCell);   // single sensor
    tft.setTextSize(2);
    int vw = strlen(tb) * 12 + 18;
    tft.setTextColor(tc, COL_BG); tft.setCursor(300 - vw, _ry); tft.print(tb); degC(tc);
    _ry += 24; }
  if (!bat.extValid) kvRow(tr(S_LATCHED), "-", COL_MUTED);   // markers on the D4/D6 path
  else kvRow(tr(S_LATCHED), bat.latchedFault ? tr(S_YES) : tr(S_NONE), bat.latchedFault ? COL_ORANGE : COL_GREEN);
  drawVerdictBanner(computeVerdict());
}
// Format a pack date (produced / assembled). Valid only if month 1..12, day 1..31 and a
// plausible year: this generation can read the ROM/assembly-date bytes back as all-FF
// (-> 2255-255-255) or leave them unwritten (0). Anything else shows "?".
void fmtPackDate(char *out, size_t n, uint16_t year, uint8_t m, uint8_t d) {
  if (m >= 1 && m <= 12 && d >= 1 && d <= 31 && year >= 2005 && year <= 2099)
    snprintf(out, n, "%04u-%02u-%02u", year, m, d);
  else { strncpy(out, "?", n); out[n - 1] = 0; }
}
// ---- Battery : Identity page ----
void drawBatteryIdentity() {
  drawHeader(tr(S_HDR_IDENTITY));
  drawPageDots(2, 3);
  char sn[20], buf[24];
  formatSerial(sn);
  _ry = HEADER_H + 8;
  kvRow(tr(S_MODEL), bat.model, COL_TEXT);
  kvRow(tr(S_SN), sn, COL_TEXT);
  snprintf(buf, sizeof(buf), "%.1f Ah", bat.capacityAh); kvRow(tr(S_CAPACITY), buf, COL_TEXT);
  kvRow(tr(S_TYPE), "LXT 18V", COL_TEXT);
  fmtPackDate(buf, sizeof(buf), bat.mfgYear, bat.mfgMonth, bat.mfgDay);
  kvRow(tr(S_PRODUCED), buf, COL_TEXT);
  fmtPackDate(buf, sizeof(buf), 2000 + bat.asmY, bat.asmM, bat.asmD);
  kvRow(tr(S_ASSEMBLED), buf, COL_TEXT);
  snprintf(buf, sizeof(buf), "%u", bat.chargeCount); kvRow(tr(S_NUM_CHARGES), buf, COL_TEXT);
  tft.setTextSize(1); tft.setTextColor(COL_MUTED, COL_BG);
  tft.setCursor(6, 228); tft.print(tr(S_DATES_NOTE));
}
// ---- Battery : Overview page ----
void drawBatteryOverview() {
  drawHeader(bat.valid ? bat.model : tr(S_BATTERY));
  drawPageDots(0, 3);
  Verdict v = computeVerdict();
  int y = HEADER_H + 8;
  char l[28];
  float td = bat.tempMosfet > bat.tempCell ? bat.tempMosfet - bat.tempCell : bat.tempCell - bat.tempMosfet;
  // Board sensor only present on the D7 path (boardTempValid); ignore it otherwise.
  bool spreadBad = bat.boardTempValid && td > TEMP_SPREAD_BAD;
  bool tPinned = tempImplausible(bat.tempCell) || (bat.boardTempValid && tempImplausible(bat.tempMosfet));
  // Pinned sensor = confirmed fault (red); spread-only = empirical suspicion (orange).
  uint16_t tcol = tPinned ? COL_RED : (spreadBad ? COL_ORANGE : COL_TEXT);
  int lw2;
  // "PACK" label + hero voltage (smooth GFX) + classic "V".
  gfxText(&FreeSansBold9pt7b, 8, y + 12, tr(S_PACK), COL_MUTED);
  tft.setFont(&FreeSansBold24pt7b); tft.setTextSize(1); tft.setTextColor(COL_TEXT);
  snprintf(l, sizeof(l), "%.2f", bat.packVoltage);
  tft.setCursor(6, y + 52); tft.print(l);
  int vx = tft.getCursorX(); tft.setFont(NULL);
  tft.setTextSize(2); tft.setTextColor(COL_TEXT, COL_BG); tft.setCursor(vx + 3, y + 36); tft.print("V");
  // Measures: smooth label + classic value + proper degree.
  lw2 = rowLabel(6, y + 66, tr(S_TEMP));
  tft.setTextSize(2); tft.setTextColor(tcol, COL_BG); tft.setCursor(6 + lw2 + 8, y + 66);
  if (bat.boardTempValid) tft.printf("%.0f/%.0f", bat.tempCell, bat.tempMosfet);
  else                    tft.printf("%.0f", bat.tempCell);   // single sensor
  degC(tcol);
  lw2 = rowLabel(6, y + 92, tr(S_SPREAD));
  uint16_t scol = spreadBad ? COL_ORANGE : COL_MUTED;
  tft.setTextSize(2); tft.setTextColor(scol, COL_BG); tft.setCursor(6 + lw2 + 8, y + 92);
  tft.printf("%.1f", td); degC(scol);
  lw2 = rowLabel(6, y + 118, tr(S_CHARGES));
  tft.setTextSize(2); tft.setTextColor(COL_TEXT, COL_BG); tft.setCursor(6 + lw2 + 8, y + 118);
  tft.printf("%u", bat.chargeCount);
  // Right column: cell bars (LXT = 5, laid out on a fixed 24px pitch). Bar height =
  // value font height; value in size-2 with "V" attached; fill mapped 2.5V (empty) ->
  // 4.2V (full). NOTE: 10 cells (XGT) don't fit this pitch — a compact 2-column layout
  // is the one real UI rework the XGT episode owns; here we just iterate cellCount.
  int bx = 158, rowH = 24, bh = 16, barx = 176, barw = 72;
  float cmn = 9.0f;
  for (int i = 0; i < cellCount; i++) if (bat.cell[i] > 0.1f && bat.cell[i] < cmn) cmn = bat.cell[i];
  for (int i = 0; i < cellCount; i++) {
    int cy = y + i * rowH;
    tft.setTextSize(1); tft.setTextColor(COL_MUTED, COL_BG);
    tft.setCursor(bx, cy + 4); tft.printf("C%d", i + 1);
    tft.drawRect(barx, cy, barw, bh, COL_PANEL);
    int fw = (int)((bat.cell[i] - 2.5f) / (4.2f - 2.5f) * (barw - 2));
    if (fw < 0) fw = 0; if (fw > barw - 2) fw = barw - 2;
    tft.fillRect(barx + 1, cy + 1, fw, bh - 2, cellColor(bat.cell[i], cmn, bat.cellDiff));
    char cvs[8]; snprintf(cvs, sizeof(cvs), "%.2fV", bat.cell[i]);
    int vw = strlen(cvs) * 12;
    tft.setTextSize(2); tft.setTextColor(COL_TEXT, COL_BG);
    tft.setCursor(316 - vw, cy + 1); tft.print(cvs);
  }
  drawVerdictBanner(v);
}
// ---- Battery : paged ----
void drawBatteryPage() {
  if (batteryPage == 0)      drawBatteryOverview();
  else if (batteryPage == 1) drawBatteryHealth();
  else                       drawBatteryIdentity();
}
// ---- Repair : diagnose + prediction (wizard step 1) ----
// Diagnose row (3-state): label + status text + a glyph on the right.
//   st 0 = ok      -> green check
//   st 1 = fault   -> red cross
//   st 2 = suspect -> orange warning triangle "!" (empirical / possible, not confirmed)
void drawDiagRowSt(const char* k, const char* val, int st) {
  rowLabel(6, _ry, k);                                   // 9pt label like Battery rows
  uint16_t col = (st == 0) ? COL_GREEN : (st == 1) ? COL_RED : COL_ORANGE;
  tft.setTextSize(2);
  int vw = txtW(val, 2);
  tft.setTextColor(col, COL_BG); tft.setCursor(272 - vw, _ry); tft.print(val);
  int ix = 288, yy = _ry;
  if (st == 0) {
    tft.drawLine(ix, yy + 8, ix + 5, yy + 14, col); tft.drawLine(ix + 5, yy + 14, ix + 15, yy + 2, col);
    tft.drawLine(ix, yy + 9, ix + 5, yy + 15, col); tft.drawLine(ix + 5, yy + 15, ix + 15, yy + 3, col);
  } else if (st == 1) {
    tft.drawLine(ix, yy + 2, ix + 13, yy + 15, col); tft.drawLine(ix + 13, yy + 2, ix, yy + 15, col);
    tft.drawLine(ix + 1, yy + 2, ix + 14, yy + 15, col); tft.drawLine(ix + 14, yy + 2, ix + 1, yy + 15, col);
  } else {                                               // warning triangle with "!"
    tft.drawTriangle(ix + 7, yy + 1, ix, yy + 15, ix + 14, yy + 15, col);
    tft.fillRect(ix + 6, yy + 5, 2, 6, col); tft.fillRect(ix + 6, yy + 12, 2, 2, col);
  }
  _ry += 26;
}
void drawDiagRow(const char* k, const char* val, bool ok) { drawDiagRowSt(k, val, ok ? 0 : 1); }
// A bottom prognosis banner: fill + centered bold caption (dark on the color). When `hint`
// is set, a small "HINT" tag is drawn at the left — the whole Repair prognosis is a
// prediction from reverse-engineered / empirical signals, not a measured guarantee.
void drawPrognosisBanner(uint16_t bc, const char* bt, bool hint) {
  tft.fillRect(0, 208, 320, 32, bc);
  if (hint) {
    tft.setTextSize(1); tft.setTextColor(bc == COL_MUTED ? COL_TEXT : COL_BG);
    tft.setCursor(6, 213); tft.print(tr(S_HINT_TAG));
  }
  int pbw = gfxWidth(&FreeSansBold9pt7b, bt);
  gfxText(&FreeSansBold9pt7b, (320 - pbw) / 2, 228, bt, COL_BG);
}
// Repair wizard, step 1: staged, feasibility-FIRST diagnosis.
//  Stage 0 comms -> Stage 1 hardware faults (before lock, they invalidate an unlock)
//  -> Stage 2 lock + prognosis -> Stage 3 the WHY (from the wear counters).
// Every finding names the specific group / sensor; Stage 2 is phrased as a prediction.
void drawWizardDiag() {
  { char h[16]; snprintf(h, sizeof(h), "%s 1/4", tr(S_REPAIR)); drawHeader(h); }
  drawPageDots(0, 4);
  int y = HEADER_H + 8;

  // --- Stage 0: comms ---
  if (!bat.valid) {
    tft.setTextSize(2); tft.setTextColor(COL_TEXT, COL_BG);
    tft.setCursor(6, y);      tft.print(tr(S_CANNOT_DIAG));
    tft.setTextSize(1); tft.setTextColor(COL_MUTED, COL_BG);
    tft.setCursor(6, y + 28); tft.print(tr(S_NO_PACK_REPLY1));
    tft.setCursor(6, y + 40); tft.print(tr(S_NO_PACK_REPLY2));
    drawPrognosisBanner(COL_MUTED, tr(S_NO_DIAGNOSIS), false);   // a state, not a prediction
    return;
  }
  bool isF0513 = strcmp(bat.commandVersion, "F0513") == 0;

  // --- Stage 1: hardware faults (evaluated before lock) ---
  int grp = 0; char action[64];
  HwFault hw = findHardwareFault(&grp, action, sizeof(action));

  // Checklist (the raw signals behind the verdict).
  uint8_t causes = !isF0513 ? lockCauses(bat.msg) : 0;
  char cb[24]; lockCausesText(causes, cb, sizeof(cb));
  _ry = y;
  rowLabel(6, _ry, tr(S_CHARGER_LOCK));
  { const char* s = causes ? cb : tr(S_NONE); tft.setTextSize(2); int vw = txtW(s, 2);
    tft.setTextColor(causes ? COL_YELLOW : COL_GREEN, COL_BG);
    tft.setCursor(272 - vw, _ry); tft.print(s);           // value in the same column as the diag rows
    if (!causes) {                                        // "none" -> a green OK check, like the other rows
      int ix = 288, yy = _ry;
      tft.drawLine(ix, yy + 8, ix + 5, yy + 14, COL_GREEN); tft.drawLine(ix + 5, yy + 14, ix + 15, yy + 2, COL_GREEN);
      tft.drawLine(ix, yy + 9, ix + 5, yy + 15, COL_GREEN); tft.drawLine(ix + 5, yy + 15, ix + 15, yy + 3, COL_GREEN);
    } }
  _ry += 24;
  // Latched marker is a HINT (orange warning), not a red fault, in the checklist too.
  drawDiagRowSt(tr(S_LATCHED_FAULT), bat.latchedFault ? tr(S_YES) : tr(S_NONE), bat.latchedFault ? 2 : 0);
  // Thermistor row is 3-state: confirmed fault (pinned) red, suspect (spread) orange "?", else ok.
  int thSt = thermistorFault() ? 1 : (thermistorSuspect() ? 2 : 0);
  drawDiagRowSt(tr(S_THERMISTOR), thSt == 1 ? tr(S_FAULTV) : (thSt == 2 ? "?" : tr(S_OKV)), thSt);
  { char cv[12]; bool cbad = (hw == HW_SENSE_WIRE || hw == HW_WEAK_CELL || hw == HW_IMBALANCE);
    if (cbad) snprintf(cv, sizeof(cv), "G%d", grp); else strcpy(cv, tr(S_OKV));
    drawDiagRow(tr(S_CELLS), cv, !cbad); }

  // --- Finding + action / prognosis text (names the specific group or sensor) ---
  // The finding TITLE + line2 carry the wizard's detailed prediction ("why" + should/unlikely/
  // check). The bottom BANNER is NOT computed here: it is drawVerdictBanner(computeVerdict()), the
  // exact same verdict (word + colour + icon) as the Battery tile/screens, so the two can never
  // disagree (single source of truth).
  int fy = _ry + 4;
  const char* title; uint16_t tcol; char line2[64];

  if (hw != HW_NONE) {
    tcol = COL_RED;
    switch (hw) {
      case HW_SENSE_WIRE: title = tr(S_HW_SENSE);  break;
      case HW_WEAK_CELL:  title = tr(S_HW_WEAK);   break;
      case HW_IMBALANCE:  title = tr(S_HW_IMB);    break;
      default:            title = tr(S_HW_THERM);  break;
    }
    strncpy(line2, action, sizeof(line2)); line2[sizeof(line2) - 1] = 0;
  } else if (bat.latchedFault) {
    // Checked BEFORE "not locked": a latched marker means the BMS memorised a fault even if the
    // pack isn't charger-locked right now (matches computeVerdict -> V_SUSPECT).
    title = tr(S_BMS_MEMORISED); tcol = COL_ORANGE;
    if (bat.odEventCount)                             strcpy(line2, tr(S_WHY_OD));
    else if (bat.olEventCount)                        strcpy(line2, tr(S_WHY_OL));
    else if (bat.healthEstPct < 50)                  strcpy(line2, tr(S_WHY_WEAR)); // <50% => >~448 cycles
    else                                             strcpy(line2, tr(S_WHY_UNCLEAR));
  } else if (!causes && !bat.locked) {
    title = tr(S_NO_LOCK_NO_FAULT); tcol = COL_GREEN;
    strcpy(line2, tr(S_PACK_HEALTHY));
  } else {
    // Charger-locked, no fault marker -> a false lockout our unlock should clear.
    title = tr(S_FALSE_LOCKOUT); tcol = COL_GREEN;
    strcpy(line2, tr(S_UNLOCK_SHOULD_HOLD));
  }

  // Spread-only thermistor suspicion: add a check note on an otherwise-clean finding (the banner
  // already reflects it via computeVerdict -> V_SUSPECT when not charger-locked).
  if (hw == HW_NONE && tcol == COL_GREEN && thermistorSuspect()) {
    strcpy(line2, tr(S_WHY_THERM_CHECK));
    tcol = COL_ORANGE;
  }

  tft.setTextSize(1); tft.setTextColor(tcol, COL_BG);
  tft.setCursor(6, fy);      tft.print(title);
  tft.setTextColor(COL_MUTED, COL_BG);
  tft.setCursor(6, fy + 12); tft.print(line2);
  tft.setCursor(6, fy + 28);
  if      (causes && hw == HW_NONE) tft.print(tr(S_HINT_CONTINUE_BACK));
  else if (hw != HW_NONE)           tft.print(tr(S_HINT_FIXHW_BACK));
  else                              tft.print(tr(S_HINT_NOTHING_BACK));
  drawVerdictBanner(computeVerdict());   // same verdict as the Battery tile/screens
}
// ---- Tools list ----
void drawTools() {
  drawHeader(tr(S_TOOLS));
  int y = HEADER_H + 6;
  for (int i = 0; i < toolCount; i++) {
    int iy = y + i * 34;
    bool sel = (toolIndex == i);
    if (sel) {
      tft.fillRoundRect(4, iy, 312, 30, 6, RGB565(0x12, 0x30, 0x39));
      tft.drawRoundRect(4, iy, 312, 30, 6, COL_ACCENT);
      tft.drawRoundRect(5, iy + 1, 310, 28, 5, COL_ACCENT);
    } else {
      tft.fillRoundRect(4, iy, 312, 30, 6, COL_BG);   // erase any previous highlight
    }
    static const uint16_t tcol[] = { COL_ACCENT, COL_YELLOW, COL_MUTED, COL_ORANGE, COL_GREEN, COL_TEXT };
    uint16_t ig = tcol[i];
    int cx = 24, cy = iy + 15;
    switch (i) {
      case 0: iconBridge(cx, cy, ig);  break;
      case 1: iconSun(cx, cy, ig);     break;
      case 2: iconSunOff(cx, cy, ig);  break;
      case 3: iconRefresh(cx, cy, ig); break;
      case 4: iconCode(cx, cy, ig);    break;
      case 5:                                          // settings = sliders glyph
        tft.drawFastHLine(cx - 8, cy - 4, 16, ig); tft.fillCircle(cx - 2, cy - 4, 2, ig);
        tft.drawFastHLine(cx - 8, cy,     16, ig); tft.fillCircle(cx + 4, cy,     2, ig);
        tft.drawFastHLine(cx - 8, cy + 4, 16, ig); tft.fillCircle(cx - 4, cy + 4, 2, ig);
        break;
    }
    gfxText(&FreeSansBold9pt7b, 46, iy + 20, tr((StrId)(S_PC_BRIDGE + i)), sel ? COL_HEAD : COL_TEXT);
    if (i >= 1 && i <= 3) {                       // "acts on the pack" marker (test-mode + LED/reset
                                                  // command; only Reset persists) vs read-only tools
      int px = 292, py = iy + 6;
      tft.drawLine(px, py + 14, px + 13, py + 1, COL_YELLOW);       // body
      tft.drawLine(px + 1, py + 14, px + 14, py + 1, COL_YELLOW);
      tft.drawLine(px + 10, py, px + 15, py + 5, COL_YELLOW);       // eraser end
      tft.fillTriangle(px, py + 14, px + 4, py + 14, px, py + 10, COL_YELLOW); // tip
    }
  }
}
void drawSettings() {
  drawHeader(tr(S_SETTINGS));
  int y = HEADER_H + 12;
  bool vals[2] = { cfgFlip, cfgBridgeBoot };
  for (int i = 0; i < settingsCount; i++) {
    int iy = y + i * 40;
    bool sel = (settingsIndex == i);
    if (sel) {
      tft.fillRoundRect(4, iy, 312, 34, 6, RGB565(0x12, 0x30, 0x39));
      tft.drawRoundRect(4, iy, 312, 34, 6, COL_ACCENT);
      tft.drawRoundRect(5, iy + 1, 310, 32, 5, COL_ACCENT);
    } else {
      tft.fillRoundRect(4, iy, 312, 34, 6, COL_BG);
    }
    const char* lbl = (i <= 2) ? tr((StrId)(S_FLIP + i)) : tr(S_CALIBRATE);
    gfxText(&FreeSansBold9pt7b, 14, iy + 22, lbl, sel ? COL_HEAD : COL_TEXT);
    tft.setTextSize(2);
    if (i < 2) {                               // boolean toggles
      const char* on = vals[i] ? "ON" : "OFF";
      tft.setTextColor(vals[i] ? COL_GREEN : COL_MUTED);
      int vw = strlen(on) * 12; tft.setCursor(300 - vw, iy + 10); tft.print(on);
    } else if (i == 2) {                       // Language: current code (click cycles)
      const char* code = langCode(lang);
      tft.setTextColor(COL_ACCENT);
      int vw = strlen(code) * 12; tft.setCursor(300 - vw, iy + 10); tft.print(code);
    } else {                                   // Calibrate touch: an action row (chevron)
      tft.setTextColor(COL_ACCENT); tft.setCursor(300 - 12, iy + 10); tft.print(">");
    }
  }
  tft.setTextSize(1); tft.setTextColor(COL_MUTED, COL_BG);
  tft.setCursor(6, 224); tft.print(tr(S_HINT_TOGGLE_SAVE));
}
void render() {
  // Clear on a screen change, and on a Battery page change (different layouts).
  // Launcher/Tools selection redraw their tiles/rows in place (self-erasing), no clear.
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
