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
  const float c[5] = { 3.98f, 3.97f, 3.99f, 3.96f, 3.98f };
  float sum = 0, mn = 9, mx = 0;
  for (int i = 0; i < 5; i++) { bat.cell[i] = c[i]; sum += c[i]; if (c[i] < mn) mn = c[i]; if (c[i] > mx) mx = c[i]; }
  bat.packVoltage = sum; bat.cellDiff = mx - mn;
  bat.tempCell = 22.4f; bat.tempMosfet = 23.1f; bat.boardTempValid = true;
  bat.extValid = true; bat.socRaw = 812; bat.odEventCount = 3; bat.olEventCount = 5;
  bat.odWearPct = round5up(bat.odEventCount, bat.chargeCount);
  bat.olWearPct = round5up(bat.olEventCount, bat.chargeCount);
  cellCount = 5;
}

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
static const Screen SCREENS[] = {
  { "splash",         [] { state = LAUNCHER; drawSplash(); }, true },
  { "launcher",       [] { state = LAUNCHER; launcherIndex = 0; } },
  { "battery_overview", [] { sBattery(0); } },
  { "battery_health", [] { sBattery(1); } },
  { "battery_identity", [] { sBattery(2); } },
  { "repair_diag",    [] { state = REPAIR_DIAG; } },
  { "confirm_unlock", [] { state = CONFIRM_UNLOCK; } },
  { "unlock_result",  [] { state = UNLOCK_RESULT; unlockCausesBefore = LF_N34; unlockCausesAfter = 0; } },
  { "confirm_reset",  [] { state = CONFIRM_RESET; } },
  { "reset_result",   [] { state = RESET_RESULT; resetLockedBefore = true; resetLockedAfter = false; } },
  { "tools",          [] { state = TOOLS; toolIndex = 0; } },
  { "settings",       [] { state = SETTINGS; settingsIndex = 2; } },
  { "debug_raw",      [] { state = DEBUG_RAW; } },
  { "pc_bridge",      [] { state = PC_BRIDGE; bridgeActive = true; } },
  { "about",          [] { state = ABOUT; aboutEgg = 0; } },
  { "comm_error",     [] { state = COMM_ERROR; } },
  { "nopack_overview", [] { bat.valid = false; sBattery(0); } },
  { "nopack_repair",  [] { bat.valid = false; state = REPAIR_DIAG; } },
#if BOARD_HAS_TOUCH
  { "calibrate_saved", sCalibrate, true },
#endif
};

int main(int argc, char** argv) {
  const char* out = argc > 1 ? argv[1] : "out";
  MKDIR(out);
  tft.init(240, 320);
  tft.setRotation(1);                       // landscape 320x240, as setup() does
  int n = (int)(sizeof(SCREENS) / sizeof(SCREENS[0]));
  for (int l = 0; l < LANG_NUM; l++) {
    lang = l;
    snprintf(g_dir, sizeof(g_dir), "%s/%s", out, langCode(l));
    MKDIR(g_dir);
    for (int i = 0; i < n; i++) {
      g_index = i;
      fixturePack();
      lastRenderedState = -1; lastBatteryPage = -1;
      tft.setFont(NULL); tft.setTextSize(1); tft.fillScreen(COL_BG);
      SCREENS[i].setup();
      if (!SCREENS[i].selfDraw) render();
      saveFrame(SCREENS[i].name);
    }
  }
  printf("hostsim: %d screens x %d languages -> %s\n", n, LANG_NUM, out);
  return 0;
}
#endif // POCKETOBI_HOSTSIM
