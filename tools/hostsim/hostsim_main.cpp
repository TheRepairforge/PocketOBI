// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// hostsim driver: loads a fixture pack, then renders every screen of the REAL firmware UI
// code (display.cpp & co., compiled for the PC) in every language, one image per screen.
// Usage: hostsim <out_dir>   -> <out_dir>/<LANG>/<nn>_<screen>.ppm  (hostsim.py makes PNGs)
// Guarded whole: PlatformIO builds every .cpp under src_dir = . (D14), this one included.
#ifdef POCKETOBI_HOSTSIM
#include "../../pocketobi.h"
#include "../../decode.h"
#include "../../display.h"
#include "../../cjk_render.h"
#include "../../ui_nav.h"
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#define MKDIR(p) mkdir(p, 0755)
#endif

// A healthy-looking 5-cell 18 V pack: illustrative values only, not a real capture.
static void fixturePack() {
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
  bat.batteryType = 0;
  bat.overloadPct = 12; bat.overdischargePct = 8; bat.healthEstPct = 87;
  const float c[5] = { 3.980f, 3.978f, 3.991f, 3.962f, 3.986f };
  float sum = 0, mn = 9, mx = 0;
  for (int i = 0; i < 5; i++) { bat.cell[i] = c[i]; sum += c[i]; if (c[i] < mn) mn = c[i]; if (c[i] > mx) mx = c[i]; }
  bat.packVoltage = sum; bat.cellDiff = mx - mn;
  bat.tempCell = 22.4f; bat.tempMosfet = 23.1f; bat.boardTempValid = true;
  bat.extValid = true; bat.socRaw = 812; bat.odEventCount = 3; bat.olEventCount = 5;
  bat.odWearPct = round5up(bat.odEventCount, bat.chargeCount);
  bat.olWearPct = round5up(bat.olEventCount, bat.chargeCount);
  cellCount = 5;
}
// *****************************************
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
  const float c[5] = { 3.980f, 3.978f, 3.991f, 3.962f, 3.986f };
  for (int i = 0; i < 5; i++) bat.cell[i] = c[i];
  bat.tempCell = 22.4f; bat.tempMosfet = 23.1f; bat.boardTempValid = true;
  bat.extValid = true; bat.socRaw = 812; bat.odEventCount = 3; bat.olEventCount = 5;
  //    frameWithLock(bat.msg, false);
  cellCount = 5;
}
//******************************************
static void savePpm(const char* path) {
  FILE* f = fopen(path, "wb");
  if (!f) { fprintf(stderr, "cannot write %s\n", path); exit(1); }
  int w = tft.width(), h = tft.height();
  fprintf(f, "P6\n%d %d\n255\n", w, h);
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
      uint16_t p = tft.getPixel(x, y);
      uint8_t rgb[3] = { (uint8_t)(((p >> 11) & 0x1F) * 255 / 31),
                         (uint8_t)(((p >> 5) & 0x3F) * 255 / 63),
                         (uint8_t)((p & 0x1F) * 255 / 31) };
      fwrite(rgb, 1, 3, f);
    }
  fclose(f);
}

// A screen either sets the nav state for render(), or draws itself (selfDraw).
struct Screen { const char* name; void (*setup)(); bool selfDraw; };

static char g_dir[512];
static int  g_index = 0;
static void saveFrame(const char* name) {
  char path[600]; snprintf(path, sizeof(path), "%s/%02d_%s.ppm", g_dir, g_index, name);
  savePpm(path);
}

#if BOARD_HAS_TOUCH
// Calibration blocks on the touch panel: grab the first target screen from inside it
// (on the first getPoint()), then the "saved" screen it ends on is the normal frame.
static bool g_calShot = false;
static void calHook() {
  if (g_calShot) return;
  g_calShot = true;
  saveFrame("calibrate_tap");
}
static void sCalibrate() {
  state = SETTINGS; g_calShot = false;
  hostsimTouchHook = calHook;
  calibrateTouch();
  hostsimTouchHook = nullptr;
}
#endif

static void sBattery(int page) { state = BATTERY; batteryPage = page; }
// A 14.4 V LXT pack (#28): 4S, cell 5 reads 0, read-only. Illustrative values, not a capture.
static void sPack14(bool locked) {
  strcpy(bat.model, "BL1415N"); bat.capacityAh = 1.5f; cellCount = 4;
  const float c[4] = { 3.950f, 3.961f, 3.953f, 3.964f };
  float sum = 0;
  for (int i = 0; i < 4; i++) { bat.cell[i] = c[i]; sum += c[i]; }
  bat.cell[4] = 0.0f; bat.packVoltage = sum; bat.cellDiff = 0.014f;
  memset(bat.msg, 0, 32);
  if (locked) { bat.msg[21] = 0x7E; bat.chargerLocked = true; }
}
static const Screen SCREENS[] = {
  {"splash",         [] { state = LAUNCHER; drawSplash(); }, true},
  {"launcher",       [] { state = LAUNCHER; launcherIndex = 0; }, false},
  {"battery_1",      [] { state = BATTERY; batteryPage = 0; }, false},
  {"battery_2",      [] { state = BATTERY; batteryPage = 1; }, false},
  {"battery_3",      [] { state = BATTERY; batteryPage = 2; }, false},
  {"repair_1",       [] { state = REPAIR_DIAG; }, false},
  {"repair_2",       [] { state = CONFIRM_UNLOCK; }, false},
  {"repair_3",       [] { state = REPAIR_DIAG; drawUnlockWorking(); drawUnlockProgress(2, 4); }, true},
  {"repair_4_ok",    [] { state = UNLOCK_RESULT; unlockCausesBefore = LF_N34; unlockCausesAfter = 0; }, false},
  {"repair_4_still", [] { state = UNLOCK_RESULT; unlockCausesBefore = LF_N34; unlockCausesAfter = LF_N34 | LF_CS2; }, false},
  {"reset_confirm",  [] { state = CONFIRM_RESET; }, false},
  {"reset_cleared",  [] { state = RESET_RESULT; resetLockedBefore = true; resetLockedAfter = false; resetReread = true; }, false},
  {"reset_still",    [] { state = RESET_RESULT; resetLockedBefore = true; resetLockedAfter = true; resetReread = true; }, false},
  {"reset_no_reread",[] { state = RESET_RESULT; resetLockedBefore = true; resetLockedAfter = true; resetReread = false; }, false},
  {"reset_none",     [] { state = RESET_RESULT; resetLockedBefore = false; resetLockedAfter = false; resetReread = true; }, false},
  {"tools_1",        [] { state = TOOLS; toolIndex = 0; }, false},
  {"tools_2",        [] { state = TOOLS; toolIndex = 3; }, false},
  {"settings",       [] { state = SETTINGS; settingsIndex = 2; }, false},
  {"debug",          [] { state = DEBUG_RAW; }, false},
  {"bridge_on",      [] { state = PC_BRIDGE; bridgeActive = true; }, false},
  {"bridge_off",     [] { state = PC_BRIDGE; bridgeActive = false; }, false},
  {"about",          [] { state = ABOUT; aboutEgg = 0; }, false},
  {"about_egg",      [] { state = ABOUT; aboutEgg = 3; }, false},
  {"comm_error",     [] { state = COMM_ERROR; }, false},


  { "bl14_overview",  [] { sPack14(false); sBattery(0); } },
  { "bl14_health",    [] { sPack14(false); sBattery(1); } },
  { "bl14_identity",  [] { sPack14(false); sBattery(2); } },
  { "bl14_repair_locked", [] { sPack14(true); state = REPAIR_DIAG; } },
  { "bl14_confirm_unlock", [] { sPack14(true); state = CONFIRM_UNLOCK; } },
  { "nopack_overview", [] { bat.valid = false; sBattery(0); } },
  { "nopack_repair",  [] { bat.valid = false; state = REPAIR_DIAG; } },
#if BOARD_HAS_TOUCH
  { "calibrate_saved", sCalibrate, true },
#endif
};

  
  
  /*
  
  { "splash",         [] { state = LAUNCHER; drawSplash(); }, true },
  { "launcher",       [] { state = LAUNCHER; launcherIndex = 0; } },
  { "battery_overview", [] { sBattery(0); } },
  { "battery_health", [] { sBattery(1); } },
  { "battery_identity", [] { sBattery(2); } },
  { "repair_diag",    [] { state = REPAIR_DIAG; } },
  // A charger lock on byte 21 (nyb 42/43 mismatch) -> lockCauses != 0, so the confirm screen
  // draws the real unlock card. Modern pack (boardTempValid stays true from the fixture).
  { "confirm_unlock", [] { memset(bat.msg, 0, 32); bat.msg[21] = 0x7E; state = CONFIRM_UNLOCK; } },
  // Same lock on the F0513 cell path (boardTempValid=false): the 2.5.1 honest caveat replaces
  // the scope line. Longest-language render here is what the warning string must fit.
  { "confirm_unlock_f0513", [] { memset(bat.msg, 0, 32); bat.msg[21] = 0x7E; bat.boardTempValid = false; state = CONFIRM_UNLOCK; } },
  { "unlock_result",  [] { state = UNLOCK_RESULT; unlockCausesBefore = LF_N34; unlockCausesAfter = 0; } },
  { "confirm_reset",  [] { state = CONFIRM_RESET; } },
  { "reset_result",   [] { state = RESET_RESULT; resetLockedBefore = true; resetLockedAfter = false; } },
  { "tools",          [] { state = TOOLS; toolIndex = 0; } },
  { "settings",       [] { state = SETTINGS; settingsIndex = 2; } },
  { "debug_raw",      [] { state = DEBUG_RAW; } },
  { "pc_bridge",      [] { state = PC_BRIDGE; bridgeActive = true; } },
  { "about",          [] { state = ABOUT; aboutEgg = 0; } },
  { "comm_error",     [] { state = COMM_ERROR; } },
  { "bl14_overview",  [] { sPack14(false); sBattery(0); } },
  { "bl14_health",    [] { sPack14(false); sBattery(1); } },
  { "bl14_identity",  [] { sPack14(false); sBattery(2); } },
  { "bl14_repair_locked", [] { sPack14(true); state = REPAIR_DIAG; } },
  { "bl14_confirm_unlock", [] { sPack14(true); state = CONFIRM_UNLOCK; } },
  { "nopack_overview", [] { bat.valid = false; sBattery(0); } },
  { "nopack_repair",  [] { bat.valid = false; state = REPAIR_DIAG; } },
#if BOARD_HAS_TOUCH
  { "calibrate_saved", sCalibrate, true },
#endif
};
*/
// *********************************************************

//-------------------------------------------------------
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
  bat.cell[2] = 2.317f; bat.cell[0] = 3.717f; bat.cell[1] = 3.707f; bat.cell[3] = 3.727f; bat.cell[4] = 3.717f;
  bat.tempCell = -30.0f; bat.tempMosfet = 24.0f;
  bat.odEventCount = 41; bat.olEventCount = 9; bat.healthEstPct = 38; bat.chargeCount = 612; finish();
}
static void fxSuspect() {
  base();
  for (int i = 0; i < 5; i++) bat.cell[i] = 2.301f + 0.011f * i;
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

//----------------------------------------------------------

// // fxHealthy() 等から呼ばれる base() を fixturePack() のエイリアスとして定義
// static void base() {
//     fixturePack();
// }

static bool isSinglePackScreen(int idx) {
    switch (idx) {
        case 0:  case 7:  case 15: case 16: case 17:
        case 19: case 20: case 21: case 22: case 23:
        case 31: case 32: case 33:
            return true;
                    default:
            return false;
    }
}

int main(int argc, char** argv) {
    const char* out = argc > 1 ? argv[1] : "out";
    MKDIR(out);

    tft.init(240, 320);
// #if BOARD_TOUCH_DRIVER == TOUCH_GT911
//     touch.begin();                            // the shim's fake GT911 (shim/Wire.h) must be found first
// #endif
    tft.setRotation(1);                       // landscape 320x240, as setup() does

    int n = (int)(sizeof(SCREENS) / sizeof(SCREENS[0]));

    for (int k = 0; k < LANG_NUM; k++) {
        lang = LANG_AVAIL[k];
        snprintf(g_dir, sizeof(g_dir), "%s/%s", out, langCode(lang));
        MKDIR(g_dir);

        // 画面のループ
        for (int i = 0; i < n; i++) {
            g_index = i;
            bool single_pack = isSinglePackScreen(i);

            for (int p = 0; p < NPACKS; p++) {
                lastRenderedState = -1;
                lastBatteryPage = -1;
                tft.setFont(NULL);
                tft.setTextSize(1);
                tft.fillScreen(COL_BG);

                SCREENS[i].setup();
                PACKS[p].apply();
                if (!SCREENS[i].selfDraw) render();

                char pack_name[64];
                snprintf(pack_name, sizeof(pack_name), "%s", PACKS[p].name);

                char tag[128];
                snprintf(tag, sizeof(tag), "%s_%s", SCREENS[i].name, pack_name);
                saveFrame(tag);

                if (single_pack)  break;
            }
        }
    }

    printf("hostsim: %d screens x %d languages -> %s\n", n, LANG_NUM, out);
    return 0;
}

#endif // POCKETOBI_HOSTSIM
