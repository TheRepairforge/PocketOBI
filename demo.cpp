// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// Screen demo (UI_DEMO=1 only): see demo.h. The fictional packs and the screen list match the ones the
// project's review renders use; keep the two in step when a pack state or a screen is added.
// Fixtures hold RAW pack facts only: the verdict, colours and wording come from the firmware's own code.
#include "pocketobi.h"
#include "demo.h"

#if UI_DEMO
#include "decode.h"
#include "unlock.h"
#include "display.h"
#include "ui_nav.h"
#include "ui_components.h"

// ---------- fictional packs (raw facts, illustrative values, not captures) ----------
static void frameWithLock(uint8_t *f, bool n34) {
  memset(f, 0, 32);
  if (n34) f[17] = (f[17] & 0xF0) | 0x03;     // nybble 34 = 3: the charger-lock signature (N34 + CS2)
}
static void base() {
  bat = BatteryData();
  bat.valid = true;
  strcpy(bat.model, "BL1850B");
  bat.commandVersion[0] = 0;
  const uint8_t rom[8] = { 0x2C, 0x41, 0x7E, 0x13, 0x00, 0x00, 0x00, 0x9A };
  memcpy(bat.romId, rom, 8);
  bat.chargeCount = 127;
  bat.mfgYear = 2021; bat.mfgMonth = 6; bat.mfgDay = 14;
  bat.asmY = 21; bat.asmM = 6; bat.asmD = 2;
  bat.capacityAh = 5.0f;
  bat.overloadPct = 12; bat.overdischargePct = 8; bat.healthEstPct = 87;
  const float c[5] = { 3.98f, 3.97f, 3.99f, 3.96f, 3.98f };
  for (int i = 0; i < 5; i++) bat.cell[i] = c[i];
  bat.tempCell = 22.4f; bat.tempMosfet = 23.1f; bat.boardTempValid = true;
  bat.extValid = true; bat.socRaw = 812; bat.odEventCount = 3; bat.olEventCount = 5;
  frameWithLock(bat.msg, false);
  cellCount = 5;
}
static void finish() {                         // the derived values the decoder would compute
  float sum = 0, mn = 9, mx = 0;
  for (int i = 0; i < cellCount; i++) { sum += bat.cell[i]; if (bat.cell[i] < mn) mn = bat.cell[i]; if (bat.cell[i] > mx) mx = bat.cell[i]; }
  bat.packVoltage = sum; bat.cellDiff = mx - mn;
  bat.odWearPct = round5up(bat.odEventCount, bat.chargeCount);
  bat.olWearPct = round5up(bat.olEventCount, bat.chargeCount);
}
static void fxHealthy() { base(); finish(); }
static void fxLocked()  { base(); frameWithLock(bat.msg, true); bat.chargerLocked = true; bat.locked = true; bat.errorCode = 0x03; finish(); }
static void fxFault()   {
  base();
  bat.cell[2] = 2.31f; bat.cell[0] = 3.71f; bat.cell[1] = 3.70f; bat.cell[3] = 3.72f; bat.cell[4] = 3.71f;
  bat.tempCell = -30.0f; bat.tempMosfet = 24.0f;
  bat.odEventCount = 41; bat.olEventCount = 9; bat.healthEstPct = 38; bat.chargeCount = 612; finish();
}
static void fxSuspect() {
  base();
  for (int i = 0; i < 5; i++) bat.cell[i] = 2.30f + 0.01f * i;
  bat.tempCell = 21.0f; bat.tempMosfet = 33.0f; bat.odEventCount = 12; finish();
}
static void fxF0513()   { base(); strcpy(bat.commandVersion, "F0513"); strcpy(bat.model, "BL1415");
                          cellCount = 4; bat.cell[4] = 0.0f;   // 14.4 V: 4S, cell 5 reads 0 (#28)
                          bat.boardTempValid = false; bat.extValid = false; bat.chargeCount = 0; finish(); }
static void fxNoPack()  { base(); finish(); bat.valid = false; }

struct DemoPack { const char *name; void (*apply)(); };
static const DemoPack PACKS[] = {
  {"healthy", fxHealthy}, {"locked", fxLocked}, {"fault", fxFault},
  {"suspect", fxSuspect}, {"F0513", fxF0513}, {"no pack", fxNoPack},
};
static const int NPACKS = sizeof(PACKS) / sizeof(PACKS[0]);

// ---------- screens ----------
struct DemoShot { const char *name; void (*prep)(); bool selfDraw; };
static const DemoShot SHOTS[] = {
  {"splash",         [] { state = LAUNCHER; drawSplash(); }, true},
  {"launcher",       [] { state = LAUNCHER; launcherIndex = 0; }, false},
  {"battery 1/3",    [] { state = BATTERY; batteryPage = 0; }, false},
  {"battery 2/3",    [] { state = BATTERY; batteryPage = 1; }, false},
  {"battery 3/3",    [] { state = BATTERY; batteryPage = 2; }, false},
  {"repair 1/4",     [] { state = REPAIR_DIAG; }, false},
  {"repair 2/4",     [] { state = CONFIRM_UNLOCK; }, false},
  {"repair 3/4",     [] { state = REPAIR_DIAG; drawUnlockWorking(); drawUnlockProgress(2, 4); }, true},
  {"repair 4/4 ok",  [] { state = UNLOCK_RESULT; unlockCausesBefore = LF_N34; unlockCausesAfter = 0; }, false},
  {"repair 4/4 still",[] { state = UNLOCK_RESULT; unlockCausesBefore = LF_N34; unlockCausesAfter = LF_N34 | LF_CS2; }, false},
  {"reset confirm",  [] { state = CONFIRM_RESET; }, false},
  {"reset cleared",  [] { state = RESET_RESULT; resetLockedBefore = true; resetLockedAfter = false; resetReread = true; }, false},
  {"reset still",    [] { state = RESET_RESULT; resetLockedBefore = true; resetLockedAfter = true; resetReread = true; }, false},
  {"reset no reread",[] { state = RESET_RESULT; resetLockedBefore = true; resetLockedAfter = true; resetReread = false; }, false},
  {"reset none",     [] { state = RESET_RESULT; resetLockedBefore = false; resetLockedAfter = false; resetReread = true; }, false},
  {"tools 1/2",      [] { state = TOOLS; toolIndex = 0; }, false},
  {"tools 2/2",      [] { state = TOOLS; toolIndex = 3; }, false},
  {"settings",       [] { state = SETTINGS; settingsIndex = 2; }, false},
  {"debug",          [] { state = DEBUG_RAW; }, false},
  {"bridge on",      [] { state = PC_BRIDGE; bridgeActive = true; }, false},
  {"bridge off",     [] { state = PC_BRIDGE; bridgeActive = false; }, false},
  {"about",          [] { state = ABOUT; aboutEgg = 0; }, false},
  {"about egg",      [] { state = ABOUT; aboutEgg = 3; }, false},
  {"comm error",     [] { state = COMM_ERROR; }, false},
};
static const int NSHOTS = sizeof(SHOTS) / sizeof(SHOTS[0]);

static int shot = 0, pack = 0;

static void show() {
  PACKS[pack].apply();
  lastRenderedState = -1; lastBatteryPage = -1;
  aboutCrashDrawn = false; bridgeActive = false;     // never the live bridge: loop() skips it in this build anyway
  tft.setFont(NULL); tft.setTextSize(1); tft.fillScreen(COL_BG);
  SHOTS[shot].prep();
  if (!SHOTS[shot].selfDraw) render();
  Serial.printf("[demo] %s | %s | %s  (%d/%d)\n", PACKS[pack].name, SHOTS[shot].name, langCode(lang), shot + 1, NSHOTS);
}
static void announce(const char *what) { toast(what, COL_ACCENT); show(); }

void demoBegin()          { shot = 0; pack = 0; show(); }
void demoRotate(int dir)  { shot = (shot + dir + NSHOTS) % NSHOTS; show(); }
void demoClick()          { pack = (pack + 1) % NPACKS; announce(PACKS[pack].name); }
void demoBack()           { pack = (pack + NPACKS - 1) % NPACKS; announce(PACKS[pack].name); }
void demoNextLang()       { lang = langNext(lang); announce(langCode(lang)); }

#if BOARD_HAS_TOUCH
// Title bar = next pack, footer = next language, left / right half of the body = previous / next screen.
void demoServiceTouch() {
  static bool down = false; static unsigned long lastMs = 0;
  const bool now = touch.touched();
  if (now && !down && millis() - lastMs > 200) {
    TS_Point p = touch.getPoint();
    if (p.z < TOUCH_Z_MIN) return;
    down = true; lastMs = millis();
    int sx, sy; mapTouch(p.x, p.y, &sx, &sy);
    if (sy < HEADER_H)                    demoClick();
    else if (sy >= SCREEN_H - FOOTER_H)   demoNextLang();
    else                                  demoRotate(sx < SCREEN_W / 2 ? -1 : 1);
  } else if (!now) {
    down = false;
  }
}
#endif
#endif // UI_DEMO
