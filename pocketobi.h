// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
#pragma once
// PocketOBI shared spine (DECISIONS.md D21): library includes, shared constants,
// types, and extern declarations of the globals defined across the modules.
// Every .cpp includes this first, which also gives each command-array / family /
// singleton definition external linkage (the extern decls below precede them).
#include <Arduino.h>
#include <SPI.h>
#include <string.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <RotaryEncoder.h>
#include <Preferences.h>
#include "OneWire2.h"
#include "board_config.h"
#include "cjk_config.h"   // ENABLE_CJK (D19); before strings_i18n.h
#include "strings_i18n.h"
#include "cjk_render.h"   // PocketTFT (the display type), LANG_NUM, txtW() (D19/D33)
#if BOARD_HAS_TOUCH
#include <XPT2046_Touchscreen.h>
#endif

#define TCAL_DEF_X_LEFT   3765   // raw Y at screen x=0   (left edge)
#define TCAL_DEF_X_RIGHT   235   // raw Y at screen x=319 (right edge)
#define TCAL_DEF_Y_TOP     415   // raw X at screen y=0   (top edge)
#define TCAL_DEF_Y_BOTTOM 3751   // raw X at screen y=239 (bottom edge)
#define TOUCH_Z_MIN 300          // reject ghost / very-light touches below this pressure
#define TOUCH_DEBUG 0
// Firmware version (see CHANGELOG.md). The numeric triplet is the single source
// of truth reported by the PC bridge (interface-version query); keep FW_VERSION
// consistent with it.
#define FW_VER_MAJOR 2
#define FW_VER_MINOR 3
#define FW_VER_PATCH 0
#define FW_VERSION "2.3.1"

// Companion-app compatibility-contract version. Distinct from FW_VERSION: it bumps
// ONLY when the coupling with the companion app changes — a bridge command is
// added/altered, a decode offset moves, or a mirrored verdict rule/threshold changes.
// The app queries it (bridge opcode 0x02) and warns on a mismatch. The 0x02 response is
// 3 bytes — [PROTOCOL_VERSION, gammeId, cellCount] — so the app routes to the right
// decoder from the device-reported family id and learns the cell count up front; rsp[0]
// stays the version, so an older 1-byte reader still parses it.
#define PROTOCOL_VERSION 2
// ---------- Color palette (dark dashboard theme) ----------
// Compile-time RGB888 -> RGB565 conversion.
#define RGB565(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

#define COL_BG      RGB565(0x0C, 0x15, 0x24)  // page background (dark navy)
#define COL_ACCENT  RGB565(0x0E, 0x7C, 0x86)  // header bars, highlight (teal)
#define COL_CYAN    RGB565(0x17, 0xB3, 0xC4)  // bright cyan accent (flashy) - About icon
#define COL_PANEL   RGB565(0x1A, 0x28, 0x3A)  // bar tracks, chips
#define COL_TEXT    RGB565(0xE6, 0xED, 0xF5)  // primary text
#define COL_MUTED   RGB565(0x82, 0x98, 0xB0)  // secondary text
#define COL_HEAD    RGB565(0xEA, 0xFB, 0xFC)  // text on accent header
#define COL_GREEN   RGB565(0x22, 0xC5, 0x5E)  // normal cell / OK
#define COL_YELLOW  RGB565(0xEA, 0xB3, 0x08)  // warning
#define COL_RED     RGB565(0xEF, 0x44, 0x44)  // critical / locked
#define COL_ORANGE  RGB565(0xF2, 0x66, 0x22)  // RepairForge brand spark

#define HEADER_H 28  // height of the colored title bar
// ---------- UI state ----------
// V2 navigation: 2x2 launcher -> sections (Battery paged / Repair wizard / Tools / About).
enum UiState { LAUNCHER, BATTERY, REPAIR_DIAG, CONFIRM_UNLOCK, UNLOCK_RESULT,
               CONFIRM_RESET, RESET_RESULT, TOOLS, SETTINGS, DEBUG_RAW, PC_BRIDGE, ABOUT, COMM_ERROR };
// Traffic-light verdict. Defined BEFORE the first function definition (tr, below)
// so the Arduino IDE's auto-generated prototypes (which reference Verdict) see the type.
enum Verdict { V_UNKNOWN, V_HEALTHY, V_REPAIRABLE, V_SUSPECT, V_FAULT };
// Stage-1 hardware-fault classes (returned by findHardwareFault). Hoisted here for the
// same reason as Verdict: the auto-generated prototypes must see the type.
enum HwFault { HW_NONE, HW_SENSE_WIRE, HW_WEAK_CELL, HW_IMBALANCE, HW_THERMISTOR };
#define BACK_LONG_MS 600
// Set to 1 to trace the encoder over the serial port (diagnostic).
#define ENC_DEBUG 0
// Set to 1 to trace OneWire battery transactions over serial (diagnostic).
// IMPORTANT: keep this 0 when using PC bridge mode — the debug prints share the
// USB serial port and would corrupt the binary protocol the PC app expects.
#define COMM_DEBUG 0
// How many times to retry a key read before giving up. Old / marginal packs answer
// intermittently; each attempt is a full ENABLE power-cycle.
#define READ_RETRIES 4
// ---------- Battery family profile (XGT seam: per-family DATA, not logic) ----------
// What differs between LXT and XGT is data, not logic (REPO_MAP.md "Adding XGT"):
// the cell count and the BMS address map. LXT is the only family today; these are the
// seams so the XGT port stays a port. Do NOT build a multi-family abstraction now —
// the XGT protocol is not settled, so any architecture designed today is a guess.
//
// Cell count: MAX_CELLS sizes the arrays (XGT is up to 10S); cellCount is the ACTIVE
// count and every cell loop / render drives off it, never a literal 5. LXT = 5.
// (The Overview still lays out 5 rows; a 10-bar layout is the one genuine UI rework
// left to the XGT episode, deliberately not done here.)
#define MAX_CELLS 10
// Family id, reported to the companion app in the bridge contract (opcode 0x02) so it
// routes to the right decoder instead of guessing from the model string. Reserved codes:
// 1 = LXT, 2 = XGT, 3 = M18 (a separate firmware). This is per-family DATA (an XGT build
// sets GAMME_XGT), like cellCount and bmsAddr.
#define GAMME_LXT 1
// BMS memory address map, as data. readExtended() reads THESE, not hard-coded hex
// literals — an XGT profile supplies its own map (its counters live in the C0/DD
// space, not D4/D6). Addresses recovered on real LXT packs; see the field comments
// in readExtended() for the decode of each.
struct BmsAddrMap {
  uint16_t asmDate;   // D4: assembly date, 3 bytes YY MM DD (year binary)
  uint16_t soc;       // D4: state of charge / charge level (u16 LE)
  uint16_t odCount;   // D4: over-discharge event count (u8)
  uint16_t olBlock;   // D4: over-load block (7 bytes, bit-packed)
  uint16_t faultMkA;  // D6: latched-fault marker A
  uint16_t faultMkB;  // D6: latched-fault marker B
};
// ---------- Battery data ----------
struct BatteryData {
  bool valid = false;
  char model[10];
  char commandVersion[8]; // "" = standard, "F0513" = older generation
  uint8_t romId[8];
  uint8_t msg[32];        // raw "battery message" frame (after ROM ID)
  uint16_t chargeCount;
  bool locked;          // failure code (nybble 40) > 0 (OBI meaning)
  bool chargerLocked;   // charger will refuse: nybble34 / CS0 / CS2 (lockCauses != 0)
  uint8_t errorCode;   // msg byte 19. OBI's raw "Status code": checksum-covered but
                       // NOT decoded anywhere (OBI prints it raw, the BMS emulator
                       // never sets it). Kept for Debug display only; never a verdict.
  uint8_t mfgDay, mfgMonth;
  uint16_t mfgYear;
  float capacityAh;
  uint8_t batteryType;
  // Protection thresholds + state-of-health, all decoded from the ROM message
  // frame (no extra bus transaction). See readStaticInfo() for the decode.
  uint8_t overloadPct;      // over-current protection threshold, % (0 = disabled)
  uint8_t overdischargePct; // over-discharge (undervoltage) protection threshold, %
  uint8_t healthEstPct;     // cycle-based state-of-health ESTIMATE, % (see note)

  float packVoltage;
  float cell[MAX_CELLS];
  float cellDiff;
  float tempCell;
  float tempMosfet; // board/MOSFET sensor; valid only if boardTempValid
  bool  boardTempValid = false; // false = single-sensor read (F0513 cell path): ignore tempMosfet
  bool  latchedFault = false; // DISABLED (D24): D6 0x58D/0x309 is the model's resting constant, not a latch. Always false. Raw bytes in faultMkA/B.
  uint8_t asmY = 0, asmM = 0, asmD = 0;  // assembly date (D4 0x000-0x002, YY MM DD, year binary)

  // --- Extended D4 diagnostics (family A packs), read in readExtended() ---
  // Addresses/decodes from the D4 memory map recovered on 4 real packs (family A = D4 space).
  bool     extValid = false;    // extended D4 reads ran (standard pack, not F0513)
  uint16_t socRaw = 0;          // D4 0x150 (u16 LE): current CHARGE LEVEL (SOC), NOT a health metric
  uint8_t  odEventCount = 0;    // D4 0x0BA (u8): over-discharge event count (wear counter)
  uint16_t olEventCount = 0;    // D4 0x08D (7B, bit-packed): over-load event count (wear counter)
  uint8_t  odWearPct = 0;       // over-discharge %: round5up(odEventCount*100/charges)
  uint8_t  olWearPct = 0;       // over-load %:      round5up(olEventCount*100/charges)
  uint8_t  faultMkA = 0, faultMkB = 0;  // raw D6 0x58D / 0x309 (kept for Debug)
};
enum { LF_CS0 = 0x01, LF_CS2 = 0x02, LF_N34 = 0x04, LF_CS1 = 0x08 };
// Cell diagnostic thresholds (Makita 18V Li-ion):
//  - bar scale : 2.5 V (empty) to 4.2 V (full)
//  - red    : critical cell (< 3.0 V), or the lowest cell of a badly
//             imbalanced pack (spread > 0.30 V)
//  - yellow : lowest cell of a moderately imbalanced pack (spread > 0.15 V)
//  - green  : normal
#define CELL_V_MIN   2.5f
// Below CELL_V_DEAD a cell is treated as genuinely dead/unrecoverable (-> FAULT). The
// [CELL_V_DEAD, CELL_V_MIN) band is a recoverable over-discharge (-> SUSPECT, not FAULT):
// a uniformly ~2.2 V/cell pack recharges, so treating <2.5 V as a hard FAULT is too
// aggressive. A truly bad cell still shows up as an imbalance (spread > DIFF_BAD) or
// below this floor.
#define CELL_V_DEAD  2.0f
#define CELL_V_MAX   4.2f
#define CELL_V_CRIT  3.0f
#define DIFF_WARN    0.15f
#define DIFF_BAD     0.30f
// A cell reading near 0 V while the pack voltage is normal = a broken SENSE wire on
// that group (the cell itself is almost never truly at 0 V in a live pack).
#define CELL_V_SENSE 0.50f

// Plausible temperature window. A reading outside it is almost certainly a faulty
// thermistor, not a real extreme temperature (a dead sensor pins near -30 C). The MIN is
// -20 C; the MAX is kept deliberately tight at 80 C so a sensor that pins HIGH (~99 C) is
// still caught, and a genuine 80-100 C pack at rest is abnormal anyway.
#define TEMP_MIN_PLAUS  -20.0f
#define TEMP_MAX_PLAUS   80.0f
// A gap between the two sensors on the same pack points to a faulty thermistor. Empirical:
// a charger was seen refusing packs at only ~7-14 C of divergence, so 10 C is the bound
// here (at rest a healthy pack's two sensors sit within a few C).
#define TEMP_SPREAD_BAD  10.0f

// ===================== extern globals =====================
// Hardware singletons (defined in PocketOBI.ino)
extern OneWire makita;
extern PocketTFT tft;
#if BOARD_HAS_TOUCH
extern SPIClass touchSPI;
extern XPT2046_Touchscreen touch;
extern int tcXL, tcXR, tcYT, tcYB;
extern bool touchDown;
extern unsigned long lastTouchMs;
#endif

// UI / navigation state (PocketOBI.ino)
extern UiState state;
extern int lastRenderedState;
extern int launcherIndex, batteryPage, toolIndex, aboutEgg;
extern bool aboutCrashDrawn;
extern int lastBatteryPage;
extern int lang;
extern const int toolCount;
extern const int settingsCount;

// Settings / NVS (PocketOBI.ino)
extern Preferences prefs;
extern bool cfgFlip, cfgBridgeBoot;
extern int settingsIndex, _ry;
extern bool resetLockedBefore, resetLockedAfter;
extern uint8_t unlockCausesBefore, unlockCausesAfter;

// Encoder + secondary "back" button (PocketOBI.ino)
extern RotaryEncoder *encoder;
extern long lastEncPos;
extern bool btnPressed;
extern unsigned long lastBtnTime;
extern bool backDown, backLongFired;
extern unsigned long backStart;

// Battery data (PocketOBI.ino)
extern BatteryData bat;

// PC bridge running state (PocketOBI.ino)
extern bool bridgeActive;

// Family profile data (decode.cpp)
extern uint8_t cellCount;
extern uint8_t gammeId;
extern const BmsAddrMap *bmsAddr;

// Protocol command frames (protocol.cpp)
extern const uint8_t MODEL_CMD[];
extern const uint8_t READ_DATA_CMD[];
extern const uint8_t TESTMODE_CMD[];
extern const uint8_t LEDS_ON_CMD[];
extern const uint8_t LEDS_OFF_CMD[];
extern const uint8_t RESET_ERROR_CMD[];
extern const uint8_t READ_MSG_CMD[];
extern const uint8_t CLEAR_CMD[];
extern const uint8_t TESTMODE_EXIT_CMD[];
extern const uint8_t ARM_CMD[];
extern const uint8_t F0513_VCELL1_CMD[];
extern const uint8_t F0513_VCELL2_CMD[];
extern const uint8_t F0513_VCELL3_CMD[];
extern const uint8_t F0513_VCELL4_CMD[];
extern const uint8_t F0513_VCELL5_CMD[];
extern const uint8_t F0513_TEMP_CMD[];

// About easter-egg text (defined in display.cpp; used by the touch nav too)
extern const char* const ABOUT_EGG[];
extern const int ABOUT_EGG_N;

// i18n accessor (defined in PocketOBI.ino; STRTAB stays single-TU there)
const char* tr(StrId id);
