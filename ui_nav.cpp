// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// PocketOBI — encoder + touch navigation (split from PocketOBI.ino, DECISIONS.md D21).
#include "pocketobi.h"
#include "protocol.h"
#include "unlock.h"
#include "decode.h"
#include "display.h"
#include "ui_nav.h"
#include "bridge.h"

// ---------- Buttons / navigation logic (V2) ----------
// Activate target `idx` on screen `s`. Shared core of every selection: the
// encoder calls it with the item under the cursor, a touch board calls it with
// the item that was tapped (tap-target, no cursor). Screens that carry a cursor
// (LAUNCHER / TOOLS / SETTINGS) act on `idx`; the others ignore it. It does NOT
// render — the caller does, exactly like handleRotate() / handleBack().
void activate(UiState s, int idx) {
  switch (s) {
    case LAUNCHER:
      switch (idx) {
        case 0: // Battery
          if (readAllData()) { readExtended(); batteryPage = 0; state = BATTERY; }
          else state = COMM_ERROR;
          break;
        case 1: // Repair (wizard)
          if (readAllData()) { readExtended(); state = REPAIR_DIAG; }
          else state = COMM_ERROR;
          break;
        case 2: toolIndex = 0; state = TOOLS; break;   // Tools
        case 3: aboutEgg = 0; aboutCrashDrawn = false; state = ABOUT; break;  // About (egg reset)
      }
      break;
    case BATTERY:                                      // click = refresh reading
      if (readAllData()) { readExtended(); lastRenderedState = -1; }  // force a clean redraw
      else state = COMM_ERROR;
      break;
    case REPAIR_DIAG: {                                // continue to confirm
      bool isF0513 = strcmp(bat.commandVersion, "F0513") == 0;
      uint8_t causes = (bat.valid && !isF0513) ? lockCauses(bat.msg) : 0;
      int grp;
      if (findHardwareFault(&grp, nullptr, 0) != HW_NONE)        // feasibility-first: fix HW before unlocking
        toast(tr(S_TOAST_FIXHW), COL_RED);                       // stay on diagnose
      else if (causes == 0) toast(tr(S_TOAST_NOTHING), COL_MUTED);
      else state = CONFIRM_UNLOCK;
      break;
    }
    case CONFIRM_UNLOCK: {
      bool isF0513 = strcmp(bat.commandVersion, "F0513") == 0;
      uint8_t causes = (bat.valid && !isF0513) ? lockCauses(bat.msg) : 0;
      if (causes == 0) { state = BATTERY; break; }     // nothing to repair -> no write
      unlockCausesBefore = causes;
      // step 3/4: working screen (unlockRepair blocks for a few seconds).
      tft.fillScreen(COL_BG); { char h[16]; snprintf(h, sizeof(h), "%s 3/4", tr(S_REPAIR)); drawHeader(h); } drawPageDots(2, 4);
      tft.setTextSize(2); tft.setTextColor(COL_TEXT, COL_BG);
      tft.setCursor(90, 96); tft.print(tr(S_WORKING));
      tft.setTextSize(1); tft.setTextColor(COL_MUTED, COL_BG);
      tft.setCursor(28, 126); tft.print(tr(S_FRAME_STORE_PC));
      unlockCausesAfter = unlockRepair();              // write + commit + reset + re-read
      readExtended();
      state = UNLOCK_RESULT;
      lastRenderedState = -1;                          // force a clean redraw of the result
      break;
    }
    case CONFIRM_RESET:
      resetLockedBefore = bat.locked;
      resetErrors();
      readAllData();
      resetLockedAfter = bat.locked;
      state = RESET_RESULT;
      break;
    case TOOLS:
      switch (idx) {
        case 0: bridgeActive = true; state = PC_BRIDGE; break;   // enter active by default
        case 1: ledsOn();  toast(tr(S_LEDS_ON_MSG), COL_GREEN); break;
        case 2: ledsOff(); toast(tr(S_LEDS_OFF_MSG), COL_MUTED); break;
        case 3: state = readAllData() ? CONFIRM_RESET : COMM_ERROR; break;
        case 4: state = readAllData() ? DEBUG_RAW : COMM_ERROR; break;
        case 5: settingsIndex = 0; state = SETTINGS; break;
      }
      break;
    case SETTINGS:
      if (idx == 0)      { cfgFlip = !cfgFlip; tft.setRotation(cfgFlip ? 3 : 1);
                           prefs.putBool("flip", cfgFlip); lastRenderedState = -1; }
      else if (idx == 1) { cfgBridgeBoot = !cfgBridgeBoot; prefs.putBool("bridge", cfgBridgeBoot); }
      else if (idx == 2) { lang = (lang + 1) % LANG_NUM; prefs.putInt("lang", lang);
                           lastRenderedState = -1; }   // full redraw so all text swaps
#if BOARD_HAS_TOUCH
      else if (idx == 3) { calibrateTouch(); lastRenderedState = -1; }  // Calibrate touch (CYD)
#endif
      break;
    case ABOUT:
      if (aboutEgg > 0) { aboutEgg = 0; aboutCrashDrawn = false; lastRenderedState = -1; }  // click = dismiss egg
      else state = LAUNCHER;
      break;
    case UNLOCK_RESULT:
    case RESET_RESULT:
    case DEBUG_RAW:
    case COMM_ERROR:
      state = LAUNCHER;
      break;
    case PC_BRIDGE:
      state = TOOLS;
      break;
  }
}
// The cursor index the encoder would activate on the current screen. Screens
// without a cursor return 0 (activate() ignores idx for them).
static int currentActivationIndex(UiState s) {
  switch (s) {
    case LAUNCHER: return launcherIndex;
    case TOOLS:    return toolIndex;
    case SETTINGS: return settingsIndex;
    default:       return 0;
  }
}
// Encoder click: activate the item under the cursor on the current screen.
void handleClick() {
  activate(state, currentActivationIndex(state));
  render();
}
void handleRotate(int dir) {
  if (state == LAUNCHER)      { launcherIndex = (launcherIndex + dir + 4) % 4; render(); }
  else if (state == BATTERY)  { batteryPage   = (batteryPage + dir + 3) % 3;   render(); }
  else if (state == TOOLS)    { toolIndex     = (toolIndex + dir + toolCount) % toolCount; render(); }
  else if (state == SETTINGS) { settingsIndex = (settingsIndex + dir + settingsCount) % settingsCount; render(); }
  else if (state == ABOUT)          { aboutEgg++; lastRenderedState = -1; render(); }  // turn = easter egg
  else if (state == PC_BRIDGE)      { bridgeActive = !bridgeActive; render(); }  // turn = toggle bridge
  else if (state == REPAIR_DIAG)    { state = LAUNCHER;     render(); }  // turn = cancel
  else if (state == CONFIRM_UNLOCK) { state = REPAIR_DIAG;  render(); }  // turn = cancel
  else if (state == CONFIRM_RESET)  { state = TOOLS;        render(); }  // turn = cancel
}
// Back button, short press: go one screen back.
void handleBack() {
  switch (state) {
    case LAUNCHER: return;                              // already at the top
    case CONFIRM_UNLOCK:
    case UNLOCK_RESULT:  state = REPAIR_DIAG; break;
    case CONFIRM_RESET:
    case RESET_RESULT:
    case DEBUG_RAW:
    case PC_BRIDGE:
    case SETTINGS:       state = TOOLS; break;
    default:             state = LAUNCHER; break;       // Battery / Repair / Tools / About / error
  }
  render();
}
// Back button, long press: jump straight to the launcher.
void goHome() {
  if (state != LAUNCHER) { state = LAUNCHER; render(); }
}
#if BOARD_HAS_TOUCH
// ---------- Touch input (CYD): tap-target, drives the shared nav core ----------
// Map a raw XPT2046 reading (0..4095 per axis) to screen pixels, correcting axis
// swap/flip and the 180-degree display flip (cfgFlip). Tunable via the TOUCH_* flags.
void mapTouch(int rawX, int rawY, int *sx, int *sy) {
  long x = map(rawY, tcXL, tcXR, 0, 319);   // screen X <- raw Y axis (calibration in NVS)
  long y = map(rawX, tcYT, tcYB, 0, 239);   // screen Y <- raw X axis
  if (cfgFlip) { x = 319 - x; y = 239 - y; }                 // screen rotated 180 in settings
  *sx = constrain((int)x, 0, 319);
  *sy = constrain((int)y, 0, 239);
}
// Brief visual acknowledgement at the tap point: a touch panel gives no mechanical
// click, so a ring is drawn and shown for a moment before the screen reacts. The
// next render() (always called after dispatch) clears it.
void touchFeedback(int x, int y) {
  tft.drawCircle(x, y, 10, COL_ACCENT);
  tft.drawCircle(x, y,  7, COL_ACCENT);
  delay(40);
}
// On-device touch calibration: tap three corner targets, recompute the four corner
// raw values (edge-extrapolated from a 20px inset) and persist them to NVS. Called
// from Settings > Calibrate; blocks until done, then the caller redraws.
void calibrateTouch() {
  const int M = 20;
  const int tx[3] = { M, 319 - M, M };       // TL, TR, BL target centres
  const int ty[3] = { M, M, 239 - M };
  int rx[3], ry[3];
  for (int i = 0; i < 3; i++) {
    tft.fillScreen(COL_BG);
    tft.setTextSize(1); tft.setTextColor(COL_TEXT, COL_BG);
    tft.setCursor(84, 108); tft.print(tr(S_CAL_TAP));
    tft.setTextColor(COL_MUTED, COL_BG);
    tft.setCursor(140, 128); tft.printf("%d / 3", i + 1);
    tft.drawCircle(tx[i], ty[i], 9, COL_ACCENT);
    tft.drawFastHLine(tx[i] - 13, ty[i], 27, COL_ACCENT);
    tft.drawFastVLine(tx[i], ty[i] - 13, 27, COL_ACCENT);
    while (touch.touched()) delay(10);        // release the selecting/previous touch first
    delay(60);
    for (;;) {                                // wait for a firm tap
      if (touch.touched()) { TS_Point p = touch.getPoint();
        if (p.z >= TOUCH_Z_MIN) { rx[i] = p.x; ry[i] = p.y; break; } }
      delay(5);
    }
    tft.fillCircle(tx[i], ty[i], 6, COL_GREEN);
    while (touch.touched()) delay(10);        // wait for release before the next target
    delay(150);
  }
  // Axis map: screen X <- raw Y, screen Y <- raw X. Extrapolate the inset to the edges.
  long rawY_L = (ry[0] + ry[2]) / 2, rawY_R = ry[1];   // left (TL,BL) / right (TR)
  long rawX_T = (rx[0] + rx[1]) / 2, rawX_B = rx[2];   // top (TL,TR) / bottom (BL)
  int spanX = (319 - M) - M, spanY = (239 - M) - M;
  tcXL = rawY_L - (rawY_R - rawY_L) * M / spanX;
  tcXR = rawY_R + (rawY_R - rawY_L) * M / spanX;
  tcYT = rawX_T - (rawX_B - rawX_T) * M / spanY;
  tcYB = rawX_B + (rawX_B - rawX_T) * M / spanY;
  prefs.putInt("tcXL", tcXL); prefs.putInt("tcXR", tcXR);
  prefs.putInt("tcYT", tcYT); prefs.putInt("tcYB", tcYB);
  tft.fillScreen(COL_BG); tft.setTextColor(COL_GREEN, COL_BG); tft.setTextSize(2);
  tft.setCursor(96, 108); tft.print(tr(S_CAL_SAVED)); delay(700);
}
// The tapped target for the current screen. Cursor screens map a tap to the item
// under the finger; the rest ignore the index (tap = their primary action).
int touchHitIndex(UiState s, int sx, int sy) {
  switch (s) {
    case LAUNCHER: {                       // 2x2 tiles: split at the mid-gaps (see drawLauncher)
      int col = (sx >= 160) ? 1 : 0;
      int row = (sy >= 134) ? 1 : 0;
      return row * 2 + col;
    }
    case TOOLS: {                          // vertical list, rows at y+34*i (see drawTools)
      int i = (sy - (HEADER_H + 6)) / 34;
      return constrain(i, 0, toolCount - 1);
    }
    case SETTINGS: {                       // vertical list, rows at y+40*i (see drawSettings)
      int i = (sy - (HEADER_H + 12)) / 40;
      return constrain(i, 0, settingsCount - 1);
    }
    default: return 0;                     // other screens: tap = their single primary action
  }
}
// Poll the touch panel and dispatch one tap on touch-down. Tapping the title bar
// acts as Back (the CYD has no physical back button); everywhere else a tap goes
// straight to activate(), the same nav core the encoder uses.
void serviceTouch() {
  bool now = touch.touched();
  if (now && !touchDown && millis() - lastTouchMs > 200) {
    TS_Point p = touch.getPoint();
    if (p.z < TOUCH_Z_MIN) return;                 // ghost / too-light touch: ignore, stay un-latched
    touchDown = true;
    lastTouchMs = millis();
    int sx, sy;
    mapTouch(p.x, p.y, &sx, &sy);
#if TOUCH_DEBUG
    // Calibration aid: show where the tap lands, print raw+mapped, do NOT navigate.
    tft.fillCircle(sx, sy, 4, COL_RED);
    tft.fillRect(0, 224, 320, 16, COL_BG);
    tft.setTextSize(1); tft.setTextColor(COL_TEXT, COL_BG); tft.setCursor(2, 228);
    tft.printf("raw %4d,%4d  map %3d,%3d", p.x, p.y, sx, sy);
    Serial.printf("raw %d,%d  map %d,%d\n", p.x, p.y, sx, sy);
    return;
#endif
    touchFeedback(sx, sy);                                             // acknowledge the tap
    lastRenderedState = -1;                                            // force a full redraw so the feedback ring is erased
    if (sy < HEADER_H && state != LAUNCHER) { handleBack(); return; }  // tap header = back
    if (state == BATTERY) {                        // page nav: side edges = prev/next, centre = refresh
      if (sx < 64)       { handleRotate(-1); return; }
      else if (sx > 255) { handleRotate(+1); return; }
    }
    if (state == PC_BRIDGE) { handleRotate(1); return; }   // tap body = toggle bridge active
    if (state == ABOUT) {                                  // easter egg
      if (aboutEgg > ABOUT_EGG_N) { activate(ABOUT, 0); render(); }  // in the guru crash: tap dismisses to About
      else handleRotate(1);                                          // otherwise: tap advances the egg
      return;
    }
    activate(state, touchHitIndex(state, sx, sy));
    render();
  } else if (!now) {
    touchDown = false;
  }
}
#endif // BOARD_HAS_TOUCH
