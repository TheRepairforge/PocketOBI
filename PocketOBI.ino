// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
/*
 * PocketOBI - Standalone Makita LXT battery reader / diagnostic
 * ESP32-C3 SuperMini + SPI TFT display + EC11 rotary encoder
 *
 * Required libraries:
 *  - OneWire2: bundled directly in this folder (OneWire2.h/.cpp + util/).
 *    This is the modified library from the open-battery-information project,
 *    with bit-level timings that differ from the standard OneWire library:
 *      reset      : 750 us (vs 480 us standard)
 *      write-1 slot: ~132 us (vs ~65 us)
 *      write-0 slot: ~130 us (vs ~70 us)
 *      read pulse  : 10 us  (vs 3 us)
 *    These deviations are intentional: the Makita BMS does not respond with
 *    standard Maxim OneWire timings.
 *  - Adafruit GFX Library + Adafruit ST7735 and ST7789 Library
 *    (Arduino IDE Library Manager). Display config is 100% in this file
 *    (pins + driver); no external User_Setup.h to edit. Hardware SPI.
 *  - RotaryEncoder (Matthias Hertel): software encoder decoding, compatible
 *    with the ESP32-C3 (which has no hardware pulse-counter peripheral).
 *
 * Battery protocol wiring (reference: appositeit/obi-esp32 project):
 *  GPIO3  -> Battery pin 2 (DATA, OneWire)   + 470 ohm pull-up to 3.3V
 *  GPIO4  -> Battery pin 6 (ENABLE)          + 470 ohm pull-up to 3.3V
 *  (470 ohm on both: DATA needs the strong pull-up for the 3.3V input
 *   threshold; ENABLE is driven, so same value for a single-value BOM)
 *  GND    -> Battery B- (main negative terminal; simpler and more reliable
 *            than signal pin 5, they share the same ground)
 *  NEVER connect B+ (18V) to the ESP32.
 *
 * TFT SPI display wiring (configured directly in this file):
 *  GPIO0  -> SCL/SCK
 *  GPIO1  -> SDA/MOSI
 *  GPIO10 -> RES
 *  GPIO20 -> DC
 *  GPIO21 -> CS
 *  3.3V   -> VCC + BLK
 *
 * EC11 encoder wiring:
 *  GPIO5  -> A
 *  GPIO6  -> B
 *  GPIO7  -> PUSH (built-in button, to GND, internal pull-up)
 *  GPIO2  -> KO   (module secondary button; short = back, long = home).
 *                 GPIO2 is a strapping pin: do not hold KO while powering on.
 *
 * PROTOCOL:
 * Commands taken verbatim from the official source file
 * OpenBatteryInformation/modules/makita_lxt.py (MIT project, Martin Jansson).
 * Command layout [0x01, len, rsp_len, cmd, data...], identical to the
 * ArduinoOBI USB protocol:
 *  - cmd 0xCC: reset, write 0xCC, write `len` data bytes, read rsp_len
 *  - cmd 0x33: reset, write 0x33, read 8 ROM ID bytes, write `len` data bytes,
 *              read (rsp_len - 8) remaining bytes (rsp_len includes the 8 ROM ID)
 *
 * UNLOCK / FRAME REPAIR (v0.9.0):
 * The write-back / charger-unlock capability (writeFrame(), the CS0/CS2 checksum
 * math, the nybble-34 charger-lock and the arm/write/store opcodes) is a
 * CLEAN-ROOM reimplementation from the publicly documented protocol facts of the
 * synrais/Makita-LXT-Battery-Monitor-Unlocker project. That repository ships
 * with NO license (all rights reserved), so NONE of its source code is copied
 * here; only the unprotectable protocol facts (opcodes, checksum formula, byte
 * map) are reused, cross-checked against real battery dumps. Credit to synrais
 * for the frame-repair research, to the rosvall/makita-lxt-protocol project for
 * the root protocol reverse-engineering (frame byte map, checksum ranges), and
 * to Open Battery Information (Martin Jansson, MIT) for the base protocol.
 * See README.md and CHANGELOG.md.
 *
 * Charger acceptance depends on exactly three frame fields (empirically
 * established by synrais over 200+ tests): nybble 34 (byte 17 low = charger
 * lock, must be 0), CS0 (nybble 41 = sum(nybbles 0-15) & 0x0F) and CS2
 * (nybble 43 = sum(nybbles 32-40) & 0x0F). Byte 19 (status, e.g. 0xA5) and the
 * cell temperatures are NOT part of the charger's frame check.
 */

#include "OneWire2.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <RotaryEncoder.h>
#include <SPI.h>
#include <string.h>
#include <Preferences.h>
#include <Fonts/FreeSansBold9pt7b.h>    // smooth font: chrome + dense-row labels
#include <Fonts/FreeSansBold18pt7b.h>   // smooth font: About / splash name
#include <Fonts/FreeSansBold24pt7b.h>   // smooth font for the hero pack voltage
#include "strings_i18n.h"           // i18n string table (data only; tr()/lang stay below)
#include "icons_bitmaps.h"          // 40x40 launcher icon bitmaps (data only)

// ================= BOARD HEADER: PocketOBI-LXT (ESP32-C3 SuperMini) =================
// One self-contained pin map for this carrier board. This is an XGT seam
// (REPO_MAP.md "Adding XGT"): a different board (different MCU or wiring) swaps ONLY
// this block, not the logic below. Keep every GPIO assignment here, nowhere else.
// --- Battery bus ---
#define ONEWIRE_PIN 3
#define ENABLE_PIN  4
// --- Rotary encoder (EC11) ---
#define ENC_A    5
#define ENC_B    6
#define ENC_BTN  7
#define BACK_BTN 2   // module "KO" secondary button: short = back, long = home
// --- ST7789 TFT display (full config kept in this sketch) ---
#define TFT_CS   21
#define TFT_DC   20
#define TFT_RST  10
#define TFT_MOSI 1   // SDA
#define TFT_SCLK 0   // SCL
// =================================================================================

OneWire makita(ONEWIRE_PIN);
// Hardware-SPI constructor (much faster than software SPI): (&SPI, cs, dc, rst).
// SCLK/MOSI pins are assigned via SPI.begin() in setup().
Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, TFT_CS, TFT_DC, TFT_RST);

// Firmware version (see CHANGELOG.md). The numeric triplet is the single source
// of truth reported by the PC bridge (interface-version query); keep FW_VERSION
// consistent with it.
#define FW_VER_MAJOR 2
#define FW_VER_MINOR 1
#define FW_VER_PATCH 0
#define FW_VERSION "2.1.0"

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

// PocketOBI logo (battery + bolt), 48x48, drawn in teal.
#define LOGO_W 48
#define LOGO_H 48
const uint8_t LOGO_OBI[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x70,0x00,0x00,
  0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x00,0x00,0xE0,0x00,0x00,0x3F,0xFF,0xF9,0xE7,0xFF,0xE0,0x7F,0xFF,
  0xF3,0xE7,0xFF,0xF0,0xFF,0xFF,0xE3,0xCF,0xFF,0xF0,0xF0,0x00,0x07,0xC0,0x00,0x78,0xE0,0x00,0x0F,0xC0,
  0x00,0x38,0xE0,0x00,0x1F,0x80,0x00,0x38,0xE0,0x00,0x1F,0x80,0x00,0x3F,0xE0,0x00,0x3F,0x80,0x00,0x3F,
  0xE0,0x00,0x7F,0xFF,0xC0,0x3F,0xE0,0x00,0x7F,0xFF,0x80,0x3F,0xE0,0x00,0xFF,0xFF,0x00,0x3F,0xE0,0x01,
  0xFF,0xFF,0x00,0x3F,0xE0,0x03,0xFF,0xFE,0x00,0x3F,0xE0,0x03,0xFF,0xFC,0x00,0x3F,0xE0,0x07,0xFF,0xF8,
  0x00,0x3F,0xE0,0x00,0x07,0xF8,0x00,0x3F,0xE0,0x00,0x07,0xF0,0x00,0x3F,0xE0,0x00,0x07,0xE0,0x00,0x3F,
  0xE0,0x00,0x07,0xC0,0x00,0x38,0xE0,0x00,0x0F,0xC0,0x00,0x38,0xE0,0x00,0x0F,0x80,0x00,0x78,0xF8,0x00,
  0x0F,0x00,0x00,0xF8,0x7F,0xFF,0xCF,0x3F,0xFF,0xF0,0x7F,0xFF,0x9E,0x7F,0xFF,0xE0,0x1F,0xFF,0x9C,0x7F,
  0xFF,0xC0,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x00,0x00,
  0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
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


// ---------- Protocol commands (from makita_lxt.py) ----------
const uint8_t MODEL_CMD[]       = {0x01, 0x02, 0x10, 0xCC, 0xDC, 0x0C};
const uint8_t READ_DATA_CMD[]   = {0x01, 0x04, 0x1D, 0xCC, 0xD7, 0x00, 0x00, 0xFF};
const uint8_t TESTMODE_CMD[]    = {0x01, 0x03, 0x09, 0x33, 0xD9, 0x96, 0xA5};
const uint8_t LEDS_ON_CMD[]     = {0x01, 0x02, 0x09, 0x33, 0xDA, 0x31};
const uint8_t LEDS_OFF_CMD[]    = {0x01, 0x02, 0x09, 0x33, 0xDA, 0x34};
const uint8_t RESET_ERROR_CMD[] = {0x01, 0x02, 0x09, 0x33, 0xDA, 0x04};
const uint8_t READ_MSG_CMD[]    = {0x01, 0x02, 0x28, 0x33, 0xAA, 0x00};
const uint8_t CLEAR_CMD[]       = {0x01, 0x02, 0x00, 0xCC, 0xF0, 0x00};

// Exit test mode (skip-ROM CC + D9 FF FF), and "arm" for a frame write
// (skip-ROM CC + F0 00, reading back 32 bytes). Used by the unlock/repair path.
const uint8_t TESTMODE_EXIT_CMD[] = {0x01, 0x03, 0x00, 0xCC, 0xD9, 0xFF, 0xFF};
const uint8_t ARM_CMD[]           = {0x01, 0x02, 0x20, 0xCC, 0xF0, 0x00};

// Commands specific to older F0513 batteries
const uint8_t F0513_VCELL1_CMD[] = {0x01, 0x01, 0x02, 0xCC, 0x31};
const uint8_t F0513_VCELL2_CMD[] = {0x01, 0x01, 0x02, 0xCC, 0x32};
const uint8_t F0513_VCELL3_CMD[] = {0x01, 0x01, 0x02, 0xCC, 0x33};
const uint8_t F0513_VCELL4_CMD[] = {0x01, 0x01, 0x02, 0xCC, 0x34};
const uint8_t F0513_VCELL5_CMD[] = {0x01, 0x01, 0x02, 0xCC, 0x35};
const uint8_t F0513_TEMP_CMD[]   = {0x01, 0x01, 0x02, 0xCC, 0x52};

// ---------- UI state ----------
// V2 navigation: 2x2 launcher -> sections (Battery paged / Repair wizard / Tools / About).
enum UiState { LAUNCHER, BATTERY, REPAIR_DIAG, CONFIRM_UNLOCK, UNLOCK_RESULT,
               CONFIRM_RESET, RESET_RESULT, TOOLS, SETTINGS, DEBUG_RAW, PC_BRIDGE, ABOUT, COMM_ERROR };
UiState state = LAUNCHER;
int lastRenderedState = -1; // so the screen is only cleared when the screen changes

// ---------- V2 navigation indices ----------
int launcherIndex = 0;   // 0=Battery 1=Repair 2=Tools 3=About
int batteryPage   = 0;   // 0=Overview 1=Health 2=Identity
int toolIndex     = 0;
// About easter egg: rotate the encoder on the About screen. 0 = off; each detent
// advances one retro one-liner; one turn past the last line = the mock Guru crash.
int  aboutEgg = 0;
bool aboutCrashDrawn = false;
// Traffic-light verdict. Defined BEFORE the first function definition (tr, below)
// so the Arduino IDE's auto-generated prototypes (which reference Verdict) see the type.
enum Verdict { V_UNKNOWN, V_HEALTHY, V_REPAIRABLE, V_SUSPECT, V_FAULT };
// Stage-1 hardware-fault classes (returned by findHardwareFault). Hoisted here for the
// same reason as Verdict: the auto-generated prototypes must see the type.
enum HwFault { HW_NONE, HW_SENSE_WIRE, HW_WEAK_CELL, HW_IMBALANCE, HW_THERMISTOR };

// ---------- i18n: string table lives in strings_i18n.h (data only). The tr()
// accessor and the mutable `lang` selector are logic/state and stay here. ----------
int lang = LANG_EN;
inline const char* tr(StrId id) { return STRTAB[id][lang]; }

const int   toolCount = 6;
const int   settingsCount = 3;   // Flip screen, PC bridge at boot, Language

// ---------- Settings (persisted in NVS) ----------
Preferences prefs;
bool cfgFlip = false;        // rotate the screen 180 deg
bool cfgBridgeBoot = false;  // boot straight into PC bridge
int  settingsIndex = 0;
int _ry = 0;             // shared vertical cursor for key/value rows


// Reset visual feedback. The error-reset (TESTMODE + RESET_ERROR) targets the BMS
// FAULT register = bat.locked (msg byte-20 low nibble / nybble 40), so the before ->
// after tracks THAT, not the undecoded status byte 19. See the byte-19 note on the
// errorCode field: byte 19 is checksum-covered but carries no interpreted meaning in
// any known tool -> it is not a verdict input.
bool resetLockedBefore = false;
bool resetLockedAfter = false;

// Unlock/repair feedback: charger-lock causes before -> after (bitmask, see LF_*).
uint8_t unlockCausesBefore = 0;
uint8_t unlockCausesAfter = 0;

// ---------- Encoder ----------
// Software decoding via RotaryEncoder (Matthias Hertel). Robust state machine:
// LatchMode::FOUR3 = rests at a detent when A and B are both high (typical EC11),
// 1 mechanical detent = 1 position step, direction handled correctly.
// Polled from loop() (tick()) to avoid any interrupt/IRAM concern.
RotaryEncoder *encoder = nullptr;
long lastEncPos = 0;
bool btnPressed = false;
unsigned long lastBtnTime = 0;

// Secondary "back" button state (short press = back, long press = home).
#define BACK_LONG_MS 600
bool backDown = false;
bool backLongFired = false;
unsigned long backStart = 0;

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
uint8_t cellCount = 5;   // active family cell count (LXT)

// Family id, reported to the companion app in the bridge contract (opcode 0x02) so it
// routes to the right decoder instead of guessing from the model string. Reserved codes:
// 1 = LXT, 2 = XGT, 3 = M18 (a separate firmware). This is per-family DATA (an XGT build
// sets GAMME_XGT), like cellCount and bmsAddr.
#define GAMME_LXT 1
uint8_t gammeId = GAMME_LXT;   // active family (XGT seam)

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
const BmsAddrMap LXT_ADDR = {
  /*asmDate */ 0x000,
  /*soc     */ 0x150,
  /*odCount */ 0x0BA,
  /*olBlock */ 0x08D,
  /*faultMkA*/ 0x58D,
  /*faultMkB*/ 0x309,
};
const BmsAddrMap *bmsAddr = &LXT_ADDR;   // active family address map (LXT)

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
  bool  latchedFault = false; // D6 0x58D/0x309 != 0 -> latched-fault HINT (seen on 3 packs; unlock may not hold)
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
BatteryData bat;

// ---------- Makita OneWire low level ----------
// Inter-byte timing taken from the official ArduinoOBI firmware (main.cpp):
// 90 us between each byte written or read, 400 us after each reset.
// The intra-bit timing (slots) is handled by OneWire2 itself.
// ENABLE_PIN is driven HIGH + 400 ms wait before every transaction.

void mkWrite(uint8_t b) {
  delayMicroseconds(90);
  makita.write(b, 0);
}

uint8_t mkRead() {
  delayMicroseconds(90);
  return makita.read();
}

uint8_t nibbleSwap(uint8_t b) {
  return ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
}

uint16_t le16(const uint8_t *buf, int idx) {
  return buf[idx] | (buf[idx + 1] << 8);
}

// Run a command in the [0x01, len, rsp_len, cmd, data...] layout (identical to
// the makita_lxt.py arrays). Fills outPayload with the payload, and romIdOut
// (optional, 8 bytes) if cmd == 0x33. Returns false only if the response is
// entirely 0xFF (nothing connected / no answer), NOT based on presence pulse.
bool sendCommand(const uint8_t *c, uint8_t *outPayload, uint8_t *romIdOut = nullptr) {
  uint8_t dataLen = c[1];
  uint8_t rspLen  = c[2];
  uint8_t cmdByte = c[3];
  const uint8_t *data = &c[4];

  digitalWrite(ENABLE_PIN, HIGH);
  delay(400);

  bool present = makita.reset();
  delayMicroseconds(400);

#if COMM_DEBUG
  Serial.printf("cmd 0x%02X len=%d rsp=%d present=%d\n", cmdByte, dataLen, rspLen, present ? 1 : 0);
#endif

  // NOTE: the Makita BMS does not assert a standard presence pulse, so we do
  // NOT bail on present==0 (the official ArduinoOBI ignores reset()'s return
  // too). We always talk, then judge success from the response content below.

  uint8_t payloadLen;
  if (cmdByte == 0x33) {
    mkWrite(0x33);
    uint8_t rid[8];
    for (int i = 0; i < 8; i++) rid[i] = mkRead();
    if (romIdOut) memcpy(romIdOut, rid, 8);
    for (int i = 0; i < dataLen; i++) mkWrite(data[i]);
    payloadLen = (rspLen >= 8) ? (rspLen - 8) : 0; // guard uint8_t underflow (rspLen includes the 8 ROM ID)
    for (int i = 0; i < payloadLen; i++) outPayload[i] = mkRead();
#if COMM_DEBUG
    Serial.print("  rom:");
    for (int i = 0; i < 8; i++) Serial.printf(" %02X", rid[i]);
    Serial.println();
#endif
  } else { // 0xCC
    mkWrite(0xCC);
    for (int i = 0; i < dataLen; i++) mkWrite(data[i]);
    payloadLen = rspLen;
    for (int i = 0; i < rspLen; i++) outPayload[i] = mkRead();
  }

#if COMM_DEBUG
  Serial.print("  rx:");
  for (int i = 0; i < payloadLen; i++) Serial.printf(" %02X", outPayload[i]);
  Serial.println();
#endif

  digitalWrite(ENABLE_PIN, LOW);

  // "Nothing connected / no answer" = the whole payload reads back 0xFF.
  for (int i = 0; i < payloadLen; i++) {
    if (outPayload[i] != 0xFF) return true;
  }
  return false;
}

bool isPrintableAscii(const uint8_t *b, int n) {
  for (int i = 0; i < n; i++) {
    if (b[i] < 0x20 || b[i] > 0x7E) return false;
  }
  return true;
}

// Retry a command until it returns real data (not all-FF) or `tries` attempts are spent.
// Very old / marginal packs (e.g. a locked 2010 pack sitting at 18 V) answer only
// intermittently — READ_MSG can succeed 1 read in 3 — so a single attempt reports a false
// comms failure. Each sendCommand() already does its own full ENABLE power-cycle, so a retry
// is a fresh transaction.
bool sendCommandRetry(const uint8_t *c, uint8_t *outPayload, uint8_t *romIdOut, uint8_t tries) {
  for (uint8_t i = 0; i < tries; i++)
    if (sendCommand(c, outPayload, romIdOut)) return true;
  return false;
}

// Special F0513 transaction (the "raw" 0x31/0x32 cases from main.cpp):
// reset, CC, 99, 400 ms delay, reset, cmd, read 2 bytes.
// The storage order is reversed in the official firmware (rsp[3] then rsp[2]),
// hence the model shown as "BL" + byte2 + byte1.
bool readF0513Raw(uint8_t cmdByte, uint8_t *byte1, uint8_t *byte2) {
  digitalWrite(ENABLE_PIN, HIGH);
  delay(400);

  makita.reset(); // presence pulse ignored (see sendCommand note)
  delayMicroseconds(400);

  mkWrite(0xCC);
  mkWrite(0x99);
  delay(400);
  makita.reset();
  delayMicroseconds(400);
  mkWrite(cmdByte);
  *byte1 = mkRead();
  *byte2 = mkRead();

  digitalWrite(ENABLE_PIN, LOW);

  // Both bytes 0xFF => nothing answered.
  return !(*byte1 == 0xFF && *byte2 == 0xFF);
}

// Read static info with automatic standard/F0513 detection, mirroring the
// on_read_static_click() logic: try the standard model command -> if the
// response is not ASCII, fall back to F0513.
bool readStaticInfo() {
  uint8_t modelPayload[16];
  bool gotModel = sendCommandRetry(MODEL_CMD, modelPayload, nullptr, READ_RETRIES);
  bool modelAscii = gotModel && isPrintableAscii(modelPayload, 7);

  // The standard static frame (0x33 AA, READ_MSG_CMD) decodes independently of the MODEL
  // command. Some very old packs (e.g. 2013 BL18xx at 362 cycles) answer READ_MSG and the
  // live read perfectly but return all-FF to MODEL (CC DC 0C). Gating identification on an
  // ASCII model would reject a fully readable pack and — worse — flip it into the F0513
  // path, which zeroes every field and shows a false "healthy 0.0 V". So we
  // try the frame regardless of the model, and only fall back to F0513 when the frame
  // itself is silent (all-FF).
  uint8_t payload[32];
  uint8_t romId[8];
  bool gotMsg = sendCommandRetry(READ_MSG_CMD, payload, romId, READ_RETRIES);

  if (gotMsg) {
    strcpy(bat.commandVersion, "");
    if (modelAscii) {
      memcpy(bat.model, modelPayload, 7);
      bat.model[7] = 0;
    } else {
      // MODEL command silent on this generation: mark the pack as an unidentified standard
      // LXT pack. Everything below is real, decoded from the 0x33 frame.
      strcpy(bat.model, "LXT ?");
    }

    memcpy(bat.romId, romId, 8);
    memcpy(bat.msg, payload, 32);

    // chargeCount: nibble-swap of payload[26] (MSB) and payload[27] (LSB),
    // big-endian order matching makita_lxt.py (bytearray[::-1] + int.from_bytes 'big').
    uint16_t swapped = (nibbleSwap(payload[26]) << 8) | nibbleSwap(payload[27]);
    bat.chargeCount = swapped & 0x0FFF;
    bat.locked = (payload[20] & 0x0F) > 0;
    bat.chargerLocked = (lockCauses(payload) != 0);
    bat.errorCode = payload[19];
    // Capacity (byte 16) has two encodings, per drakosha/makita-battery-tools:
    // newer packs store it directly in Ah (raw 1..8, nibble-swap > 60), older
    // packs store nibble-swap in tenths of an Ah. Detect and decode accordingly.
    uint8_t capRaw = payload[16];
    uint8_t capSw  = nibbleSwap(capRaw);
    if (capRaw >= 1 && capRaw <= 8 && capSw > 60) {
      bat.capacityAh = capRaw;         // newer format: whole Ah
    } else {
      bat.capacityAh = capSw / 10.0;   // legacy format: tenths of an Ah
    }
    bat.batteryType = nibbleSwap(payload[11]);
    bat.mfgYear = 2000 + romId[0];
    bat.mfgMonth = romId[1];
    bat.mfgDay = romId[2];

    // Protection thresholds + SoH, decoded from the ROM frame (no extra bus
    // traffic). These decodes are corroborated by BOTH sides of the m5din-makita
    // fork: its reader (getMsg) and its BMS emulator (Makita.h set_overload /
    // set_overdischarge / set_cycle_count store the same bytes the same way).
    // Overload: msg[25], nibble-swapped; bit 0x20 is the "enabled" flag, low 5
    // bits are the value in steps of 5 %.
    uint8_t ol = nibbleSwap(payload[25]);
    bat.overloadPct = (ol & 0xE0) ? (uint8_t)((ol & 0x1F) * 5) : 0;
    // Over-discharge: msg[24], stored INVERTED in the high nibble, step 5.33 %.
    uint8_t odNib = (uint8_t)(~payload[24]) >> 4;
    bat.overdischargePct = (uint8_t)(odNib * 5.33f + 0.5f);
    // State-of-health ESTIMATE from cycle count: older packs lose ~1 bar (25 %)
    // every 224 cycles => ~ -1 %/8.96 cycles. This is an estimate, not the BMS's
    // own gauge (the extended D4 health command is deliberately not used here).
    int h = 100 - (int)(bat.chargeCount / 8.96f + 0.5f);
    bat.healthEstPct = (uint8_t)(h < 0 ? 0 : (h > 100 ? 100 : h));

  } else {
    // No standard static frame at all (READ_MSG silent) -> try the older F0513 generation
    uint8_t b1, b2;
    if (!readF0513Raw(0x31, &b1, &b2)) return false;

    strcpy(bat.commandVersion, "F0513");
    snprintf(bat.model, sizeof(bat.model), "BL%X%X", b2, b1);

    uint8_t tmp[8];
    sendCommand(CLEAR_CMD, tmp); // reset state, like get_f0513_model()

    bat.chargeCount = 0;
    bat.locked = false;
    bat.chargerLocked = false;
    bat.errorCode = 0;
    bat.capacityAh = 0;
    bat.batteryType = 0;
    bat.overloadPct = 0;
    bat.overdischargePct = 0;
    bat.healthEstPct = 0;
    bat.mfgYear = 0;
    bat.mfgMonth = 0;
    bat.mfgDay = 0;
    // F0513 has no ROM-ID message frame; clear these so Debug/raw does not
    // show stale data from a previously-read standard pack.
    memset(bat.romId, 0, sizeof(bat.romId));
    memset(bat.msg, 0, sizeof(bat.msg));
  }

  bat.valid = true;
  return true;
}

// Read the 5 cell voltages + temperature over the F0513 command set (CC 31..35, CC 52).
// Used for genuine F0513 packs AND as a live fallback for old (2010-era) packs that answer the
// standard AA static frame but are silent on the D7 live read: their cells live here.
bool readF0513Cells() {
  uint8_t tmp[8];
  sendCommand(CLEAR_CMD, tmp);
  sendCommand(CLEAR_CMD, tmp);

  uint8_t c1[2], c2[2], c3[2], c4[2], c5[2], t[2];
  if (!sendCommandRetry(F0513_VCELL1_CMD, c1, nullptr, READ_RETRIES)) return false;
  sendCommand(F0513_VCELL2_CMD, c2);
  sendCommand(F0513_VCELL3_CMD, c3);
  sendCommand(F0513_VCELL4_CMD, c4);
  sendCommand(F0513_VCELL5_CMD, c5);
  sendCommand(F0513_TEMP_CMD, t);

  bat.cell[0] = le16(c1, 0) / 1000.0;
  bat.cell[1] = le16(c2, 0) / 1000.0;
  bat.cell[2] = le16(c3, 0) / 1000.0;
  bat.cell[3] = le16(c4, 0) / 1000.0;
  bat.cell[4] = le16(c5, 0) / 1000.0;
  // Reject a partial / mid-dropout read: a Li-ion cell is < 4.3 V, and a dropped read comes back
  // 0xFFFF -> 65.5 V. Any cell above 5 V means the burst was not fully answered.
  for (int i = 0; i < cellCount; i++) if (bat.cell[i] > 5.0f) return false;

  float sum = 0, mn = 99, mx = 0;
  for (int i = 0; i < cellCount; i++) {
    sum += bat.cell[i];
    if (bat.cell[i] < mn) mn = bat.cell[i];
    if (bat.cell[i] > mx) mx = bat.cell[i];
  }
  bat.packVoltage = sum;
  bat.cellDiff = mx - mn;
  // F0513 temperature: same 1/10 K encoding as the standard path (raw/10 - 273.15).
  // Confirmed on a real F0513 pack (raw 2972 -> 24 C; a /100 decode gives an implausible
  // 29.7 C). tempMosfet has no F0513 equivalent.
  bat.tempCell = le16(t, 0) / 10.0 - 273.15;
  bat.tempMosfet = -1;
  bat.boardTempValid = false;   // single sensor on this path
  return true;
}

// Read live data (voltages, temperatures) -> on_read_data_click().
// F0513 packs use the F0513 cell set; standard packs use the D7 live read, with a fall back to
// the F0513 cell set when D7 is silent (old packs that answer the AA frame but not D7).
bool readLiveData() {
  if (strcmp(bat.commandVersion, "F0513") == 0) return readF0513Cells();

  // Standard path
  uint8_t payload[29];
  if (!sendCommandRetry(READ_DATA_CMD, payload, nullptr, READ_RETRIES)) return readF0513Cells();

  bat.packVoltage = le16(payload, 0) / 1000.0;
  bat.cell[0] = le16(payload, 2) / 1000.0;
  bat.cell[1] = le16(payload, 4) / 1000.0;
  bat.cell[2] = le16(payload, 6) / 1000.0;
  bat.cell[3] = le16(payload, 8) / 1000.0;
  bat.cell[4] = le16(payload, 10) / 1000.0;

  float mn = 99, mx = 0;
  for (int i = 0; i < cellCount; i++) {
    if (bat.cell[i] < mn) mn = bat.cell[i];
    if (bat.cell[i] > mx) mx = bat.cell[i];
  }
  bat.cellDiff = mx - mn;
  // Temperature is 1/10 K: raw = (T_C + 273.15) * 10, so T_C = raw/10 - 273.15
  // (rosvall / obi-esp32, and both sides of the m5din-makita fork). Confirmed on real
  // packs. A faulty internal thermistor reads a pinned absurd value (e.g. ~ -30 C) that
  // the charger refuses as a "temperature" fault.
  // The BMS reports two sensors (offsets 14 and 16); which one is physically the cell vs
  // the board is not certain (upstream OBI just labels them Sensor 1/2), so the UI shows
  // both values without a hard label.
  bat.tempCell = le16(payload, 14) / 10.0 - 273.15;
  bat.tempMosfet = le16(payload, 16) / 10.0 - 273.15;
  bat.boardTempValid = true;    // D7 path exposes both sensors
  return true;
}

// Read the static 0x33 message before the live data. Some packs stop answering the live
// read after a 0x33 read; here that does not happen because sendCommand() power-cycles
// ENABLE around every command, resetting that state, so static-first is safe.
bool readAllData() {
  bool ok1 = readStaticInfo();
  bool ok2 = readLiveData();
  // A reading is only trustworthy with plausible live cell data. Very old / marginal packs
  // can answer the static frame (or the F0513 fallback) while the live read is silent or
  // all-FF; without this guard that surfaced as a false "0.0 V healthy/unlock" tile. A 5S
  // LXT pack reads well above 5 V even deeply discharged, so packVoltage ~0 means "no live
  // data", not "empty pack".
  if (!ok1 || !ok2 || bat.packVoltage < 5.0f) { bat.valid = false; return false; }
  return true;
}

// Power-cycle the OneWire bus: drop ENABLE, wait, raise, settle, drop again.
// Lets the BMS commit a written frame / settle after a reset. Note: this toggles
// the ENABLE line only; it does NOT reset the BMS's own state (a true reset needs
// physically removing the pack).
void busPowerCycle() {
  digitalWrite(ENABLE_PIN, LOW);
  delay(100);
  digitalWrite(ENABLE_PIN, HIGH);
  delay(150);
  digitalWrite(ENABLE_PIN, LOW);
}

// Error reset. Base sequence from on_reset_errors_click() (TESTMODE + RESET),
// extended with the test-mode exit + bus power-cycle so the BMS actually commits
// the internal error-register clear (mirrors the documented full DA 04 flow).
void resetErrors() {
  uint8_t tmp[8];
  sendCommand(TESTMODE_CMD, tmp);       // enter test mode
  delay(30);
  sendCommand(RESET_ERROR_CMD, tmp);    // 0xDA 0x04 -> clear internal error register
  delay(30);
  sendCommand(TESTMODE_EXIT_CMD, tmp);  // exit test mode (CC D9 FF FF)
  busPowerCycle();                      // let the BMS settle / commit
}

// LED test. TESTMODE then the LED command MUST stay in the SAME ENABLE-high session,
// otherwise dropping ENABLE between the two exits test mode and the LED command is ignored.
// Both are 0x33-style: reset, write 0x33, read 8 ROM bytes, write data, read 1.
void ledsSet(bool on) {
  digitalWrite(ENABLE_PIN, HIGH);
  delay(400);
  makita.reset(); delayMicroseconds(400);               // TESTMODE
  mkWrite(0x33); for (int i = 0; i < 8; i++) mkRead();
  mkWrite(0xD9); mkWrite(0x96); mkWrite(0xA5); mkRead();
  delay(30);
  makita.reset(); delayMicroseconds(400);               // LED on/off (DA 31 / DA 34)
  mkWrite(0x33); for (int i = 0; i < 8; i++) mkRead();
  mkWrite(0xDA); mkWrite(on ? 0x31 : 0x34); mkRead();
  digitalWrite(ENABLE_PIN, LOW);
}
void ledsOn()  { ledsSet(true); }
void ledsOff() { ledsSet(false); }

// ---------- Unlock / frame repair (clean-room, see header credit) ----------
// The Makita charger gates on exactly three fields of the 32-byte frame:
//   - nybble 34 (byte 17 low)  = charger lock, must be 0
//   - CS0 (nybble 41)          = sum(nybbles 0-15) & 0x0F
//   - CS2 (nybble 43)          = sum(nybbles 32-40) & 0x0F
// A pack whose cells are healthy but whose frame trips one of these can be
// unlocked by rewriting the lock nybble and recomputing the checksums.
//
// NOTE: writeFrame() writes to the BMS flash, gated behind a confirmation screen.
// It clears a false charger lockout on an otherwise-healthy pack; it never
// overrides the BMS's own fault protection. Only nybble 34 (charger lock) and the
// three frame checksums CS0/CS1/CS2 (nybbles 41/42/43) are ever modified — the
// checksums are recomputed so the cleared lock stays consistent. All manufacturing
// and status bytes (0-4, 12, 19, ...) and the failure code (nybble 40) are untouched.

// Lock-cause bits. CS0/CS2 + N34 gate the CHARGER (empirically, synrais); CS1
// additionally gates the battery's own internal lock (rosvall root protocol doc:
// locked if checksums for ranges 0-15, 16-31 or 32-40 mismatch).
enum { LF_CS0 = 0x01, LF_CS2 = 0x02, LF_N34 = 0x04, LF_CS1 = 0x08 };

// Read one 4-bit nybble n from a byte buffer (n even = low nybble, n odd = high).
uint8_t nybGet(const uint8_t *d, uint8_t n) {
  return (n & 1) ? ((d[n >> 1] >> 4) & 0x0F) : (d[n >> 1] & 0x0F);
}

// Write one 4-bit nybble n into a byte buffer.
void nybSet(uint8_t *d, uint8_t n, uint8_t v) {
  v &= 0x0F;
  if (n & 1) d[n >> 1] = (d[n >> 1] & 0x0F) | (v << 4);
  else       d[n >> 1] = (d[n >> 1] & 0xF0) | v;
}

// Makita frame checksum: sum of the nybbles in [s, e] (inclusive), low nybble.
uint8_t csCalc(const uint8_t *d, uint8_t s, uint8_t e) {
  uint8_t sum = 0;
  for (uint8_t i = s; i <= e; i++) sum += nybGet(d, i);
  return sum & 0x0F;
}

// Which of the three charger-lock conditions a frame currently trips (LF_* mask).
uint8_t lockCauses(const uint8_t *frame) {
  uint8_t c = 0;
  if (nybGet(frame, 41) != csCalc(frame, 0, 15))  c |= LF_CS0;
  if (nybGet(frame, 42) != csCalc(frame, 16, 31)) c |= LF_CS1;
  if (nybGet(frame, 43) != csCalc(frame, 32, 40)) c |= LF_CS2;
  if (nybGet(frame, 34) != 0)                     c |= LF_N34;
  return c;
}

// Build a repaired copy: clear the charger-lock nybble and recompute all three
// checksums. Nybble 34 lives in the CS2 range, so CS2 is recomputed AFTER
// clearing it. The failure code (nybble 40, e.g. 0xF = dead) is deliberately
// NOT touched: we never force a genuinely-dead pack back into service.
void buildRepairedFrame(const uint8_t *in, uint8_t *out) {
  memcpy(out, in, 32);
  nybSet(out, 34, 0);                     // charger lock -> unlocked
  nybSet(out, 41, csCalc(out, 0, 15));    // CS0
  nybSet(out, 42, csCalc(out, 16, 31));   // CS1 (battery internal lock, rosvall)
  nybSet(out, 43, csCalc(out, 32, 40));   // CS2
}

// SECONDARY CHECKSUMS (documentation only - not needed by the current unlock).
// Beyond the three primary checksums above, the frame carries two more in byte 31
// (corroborated by the drakosha/makita-battery-tools MIT project, whose decode of
// the three primary checksums matches ours nybble-for-nybble):
//   - CS3 (nybble 62 = byte 31 low)  = csCalc(frame, 44, 47)  -> covers bytes 22-23
//   - CS4 (nybble 63 = byte 31 high) = csCalc(frame, 48, 61)  -> covers bytes 24-30
// Same formula (sum of the nybbles in range, low nybble). CS4 notably covers the
// overload (byte 25), over-discharge (byte 24) and cycle-count (bytes 26-27)
// fields. Our unlock rewrites only nybble 34 and CS0/CS1/CS2 (nybbles 34, 41-43) —
// all within nybbles 0-43, none of which are covered by byte 31's CS3/CS4 (nybbles
// 44-61) — so byte 31 stays valid and we never recompute it. IMPORTANT: any FUTURE feature that writes to
// bytes 22-30 (e.g. resetting the cycle count) MUST also recompute byte 31, i.e.
//   nybSet(out, 62, csCalc(out, 44, 47));
//   nybSet(out, 63, csCalc(out, 48, 61));
// or the BMS will reject the frame as corrupt.

// Low-level read-ROM (0x33) transaction: reset, write 0x33, read+discard the 8
// ROM bytes, write `dataLen` bytes, read `rspLen` bytes. Mirrors the documented
// cmd_33 flow (the ROM is always clocked out after 0x33, even when unused).
void ow33(const uint8_t *data, uint8_t dataLen, uint8_t *rsp, uint8_t rspLen) {
  digitalWrite(ENABLE_PIN, HIGH);
  delay(400);
  makita.reset();
  delayMicroseconds(400);
  mkWrite(0x33);
  for (int i = 0; i < 8; i++) mkRead();               // ROM ID, discarded
  for (int i = 0; i < dataLen; i++) mkWrite(data[i]);
  delayMicroseconds(400);
  for (int i = 0; i < rspLen; i++) rsp[i] = mkRead();
  digitalWrite(ENABLE_PIN, LOW);

#if COMM_DEBUG
  Serial.print("ow33 wr:");
  for (int i = 0; i < dataLen; i++) Serial.printf(" %02X", data[i]);
  Serial.println();
#endif
}

// Write a 32-byte frame back to the BMS and commit it. (see NOTE above).
// Sequence: arm (CC F0 00) -> write (33 0F 00 + 32 bytes) -> store (33 55 A5).
// The arm is accepted only once per pack insertion; to retry, remove/reinsert.
void writeFrame(const uint8_t *frame) {
  uint8_t junk[32];
  sendCommand(ARM_CMD, junk);               // arm the charger-write
  delay(30);

  uint8_t payload[34];
  payload[0] = 0x0F;                         // frame-write opcode
  payload[1] = 0x00;                         // pad
  memcpy(&payload[2], frame, 32);
  ow33(payload, 34, nullptr, 0);            // write frame
  delay(30);

  uint8_t store[2] = {0x55, 0xA5};
  ow33(store, 2, nullptr, 0);               // store / commit
  delay(30);
}

// Full unlock/repair operation on the currently-read battery. Returns the lock
// causes still present after the attempt (0 = fully unlocked). Assumes bat.msg
// holds a fresh, standard-battery frame.
uint8_t unlockRepair() {
  uint8_t repaired[32];
  buildRepairedFrame(bat.msg, repaired);
  writeFrame(repaired);
  busPowerCycle();     // let the BMS commit to flash
  resetErrors();       // also clear the internal error register
  readAllData();       // re-read to verify
  return bat.valid ? lockCauses(bat.msg) : 0xFF;
}

// ---------- Display ----------
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
  if (causes == 0) { strncpy(out, "none", n); return; }
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
  tft.printf("T %.0f/%.0f  latched %s", bat.tempCell, bat.tempMosfet, bat.latchedFault ? "YES" : "no");
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
  tft.drawBitmap(14, 37, LOGO_OBI, LOGO_W, LOGO_H, COL_ACCENT);
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
    const char* h = "keep turning...";
    tft.setCursor((320 - (int)strlen(h) * 6) / 2, 226); tft.print(h);
  }
}

// PC bridge mode: the tool acts as a USB<->OneWire bridge for the Open Battery
// Information PC app (drop-in ArduinoOBI replacement). Serial debug is suppressed
// while in this state (it would corrupt the binary protocol).
// PC bridge running state; the encoder toggles it live on the PC bridge screen.
bool bridgeActive = true;

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
  tft.drawBitmap((320 - LOGO_W) / 2, 26, LOGO_OBI, LOGO_W, LOGO_H, COL_ACCENT);
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

// ================= V2 UI ==================
// (enum Verdict is defined near the top, with the other enums.)

int lastBatteryPage = -1;  // force a screen clear when the Battery page changes

// Brief centered confirmation banner (blocking ~0.9s), then forces a redraw.
void toast(const char* msg, uint16_t col) {
  int w = strlen(msg) * 12;
  tft.fillRect(30, 96, 260, 48, col);
  tft.drawRect(30, 96, 260, 48, COL_BG);
  tft.setTextSize(2); tft.setTextColor(COL_BG, col);
  tft.setCursor((320 - w) / 2, 112); tft.print(msg);
  delay(900);
  lastRenderedState = -1;  // force a full redraw on the next render()
}

bool tempImplausible(float t) { return t < TEMP_MIN_PLAUS || t > TEMP_MAX_PLAUS; }

// CONFIRMED thermistor fault: a sensor pinned OUTSIDE the plausible window (validated on
// real dead-NTC packs, which pin near -30 C). Strong evidence -> red verdict, and it gates
// the unlock. Not applicable to F0513 (single sensor, unverified unit).
bool thermistorFault() {
  if (strcmp(bat.commandVersion, "F0513") == 0) return false;
  // The board sensor is only present on the D7 path; on a single-sensor (F0513 cell) read
  // tempMosfet is a sentinel and must not be tested.
  return tempImplausible(bat.tempCell) ||
         (bat.boardTempValid && tempImplausible(bat.tempMosfet));
}
// SUSPECTED thermistor issue: both sensors IN range but disagreeing by more than
// TEMP_SPREAD_BAD. EMPIRICAL / unproven (a warm pack fresh off a tool can legitimately show
// a gap), so it is a soft "possible" signal only: orange V_SUSPECT verdict, and it does NOT
// gate the unlock. A pinned sensor is reported by thermistorFault(), not here.
bool thermistorSuspect() {
  if (strcmp(bat.commandVersion, "F0513") == 0) return false;
  if (!bat.boardTempValid) return false;   // single sensor -> nothing to compare against
  if (tempImplausible(bat.tempCell) || tempImplausible(bat.tempMosfet)) return false;
  float ts = bat.tempMosfet > bat.tempCell ? bat.tempMosfet - bat.tempCell
                                           : bat.tempCell - bat.tempMosfet;
  return ts > TEMP_SPREAD_BAD;
}

// Stage-1 hardware faults, evaluated BEFORE the lock state (they invalidate any unlock).
// Feasibility-first: the first one found is the primary finding. Fills `group` (1-based
// cell index) when the fault is cell-specific, and an action string for the user.
// (enum HwFault is declared near the top, with Verdict, for the auto-prototype ordering.)
HwFault findHardwareFault(int *group, char *action, size_t n) {
  *group = 0; if (action && n) action[0] = 0;
  bool isF0513 = strcmp(bat.commandVersion, "F0513") == 0;
  // Broken sense wire: a cell near 0 V while the pack as a whole is clearly alive.
  if (!isF0513 && bat.packVoltage > 10.0f) {
    for (int i = 0; i < cellCount; i++)
      if (bat.cell[i] < CELL_V_SENSE) {
        *group = i + 1;
        if (action) snprintf(action, n, tr(S_ACT_SENSE), i + 1);
        return HW_SENSE_WIRE;
      }
  }
  // Weak / dead group: a cell genuinely below the dead floor (but not a broken sense line).
  // The [CELL_V_DEAD, CELL_V_MIN) band is a recoverable over-discharge, not a hardware fault,
  // so it does NOT land here and does NOT block the unlock.
  for (int i = 0; i < cellCount; i++)
    if (bat.cell[i] >= CELL_V_SENSE && bat.cell[i] < CELL_V_DEAD) {
      *group = i + 1;
      if (action) snprintf(action, n, tr(S_ACT_WEAK), i + 1);
      return HW_WEAK_CELL;
    }
  // Imbalance: spread too wide -> name the lowest group.
  if (bat.cellDiff > DIFF_BAD) {
    int lo = 0; float mn = 9.0f;
    for (int i = 0; i < cellCount; i++) if (bat.cell[i] > 0.1f && bat.cell[i] < mn) { mn = bat.cell[i]; lo = i; }
    *group = lo + 1;
    if (action) snprintf(action, n, tr(S_ACT_IMB), lo + 1);
    return HW_IMBALANCE;
  }
  // Thermistor: only a CONFIRMED fault (pinned sensor) gates the unlock. A mere sensor
  // disagreement (thermistorSuspect()) is handled as a soft V_SUSPECT signal, not here.
  if (thermistorFault()) {
    if (action) snprintf(action, n, "%s", tr(S_ACT_THERM));
    return HW_THERMISTOR;
  }
  return HW_NONE;
}

// Traffic-light verdict from the decoded data + the latched-fault markers.
Verdict computeVerdict() {
  if (!bat.valid) return V_UNKNOWN;
  // Defence in depth: a reading with no plausible live data (all-FF / zero cells) must never
  // read as healthy or repairable — the "dead cell" test below skips a 0.0 V cell, so without
  // this a pack with no live data would fall through to HEALTHY. readAllData() already
  // gates on this; keep it here so every caller of computeVerdict() is safe.
  if (bat.packVoltage < 5.0f) return V_UNKNOWN;
  bool red = (bat.cellDiff > DIFF_BAD);
  for (int i = 0; i < cellCount; i++)
    if (bat.cell[i] > 0.1f && bat.cell[i] < CELL_V_DEAD) red = true;  // genuinely dead cell
  if (thermistorFault()) red = true;                                 // thermistor pinned = confirmed fault
  if (red) return V_FAULT;
  // Soft / empirical signals -> "possible" HINT, never a firm fault. The latched marker
  // (D6 0x58D/0x309, seen on 3 packs) and the sensor-spread are both empirical: they
  // surface as an orange V_SUSPECT, not a red verdict.
  if (bat.latchedFault) return V_SUSPECT;                            // latched marker (hint)
  if (bat.chargerLocked || bat.locked) return V_REPAIRABLE;
  // Recoverable over-discharge: a cell below the healthy minimum but above the dead floor,
  // with no imbalance (that would have gone red above). Not a fault - it charges back up - but
  // not healthy either. Orange: a uniform ~2.2 V is recoverable, not dead.
  for (int i = 0; i < cellCount; i++)
    if (bat.cell[i] > 0.1f && bat.cell[i] < CELL_V_MIN) return V_SUSPECT;
  if (thermistorSuspect()) return V_SUSPECT;                         // sensors disagree (hint)
  return V_HEALTHY;
}

#if COMM_DEBUG
// Bench-validation dump: human-readable decode + verdict over Serial, so a captured
// log shows exactly what the firmware decided for each pack (no hand-decoding of raw
// bytes). Behind COMM_DEBUG - never ships. Called at the end of readExtended().
void dumpDecoded() {
  static const char *VW[] = {"UNKNOWN", "HEALTHY", "REPAIRABLE", "SUSPECT", "FAULT"};
  Serial.println("==== DECODED ====");
  Serial.printf("model=%s (%s)\n", bat.model, bat.commandVersion[0] ? bat.commandVersion : "std");
  Serial.printf("packV=%.3f cells=", bat.packVoltage);
  for (int i = 0; i < cellCount; i++) Serial.printf("%.3f ", bat.cell[i]);
  Serial.printf("\nspread=%.3f tCell=%.1f tBoard=%.1f\n", bat.cellDiff, bat.tempCell, bat.tempMosfet);
  Serial.printf("charges=%u cap=%.1f locked=%d chargerLocked=%d latched=%d err=0x%02X\n",
                bat.chargeCount, bat.capacityAh, bat.locked, bat.chargerLocked,
                bat.latchedFault, bat.errorCode);
  Serial.printf("odEvents=%u olEvents=%u extValid=%d healthEst=%u%% odThr=%u%% olThr=%u%%\n",
                bat.odEventCount, bat.olEventCount, bat.extValid, bat.healthEstPct,
                bat.overdischargePct, bat.overloadPct);
  Serial.printf("faultMk=%02X/%02X VERDICT=%s\n", bat.faultMkA, bat.faultMkB, VW[computeVerdict()]);
  Serial.println("=================");
}
#endif
const char* verdictText(Verdict v) {
  // V_REPAIRABLE = false lock our unlock clears (software fix); V_FAULT = genuine hardware
  // issue the unlock won't hold (bench repair). Wording chosen to stay "everything is fixable".
  switch (v) { case V_HEALTHY: return tr(S_HEALTHY); case V_REPAIRABLE: return tr(S_UNLOCK);
               case V_SUSPECT: return tr(S_SUSPECT_HW);
               case V_FAULT: return tr(S_HARDWARE_FIX); default: return tr(S_NO_PACK); }
}
uint16_t verdictColor(Verdict v) {
  switch (v) { case V_HEALTHY: return COL_GREEN; case V_REPAIRABLE: return COL_YELLOW;
               case V_SUSPECT: return COL_ORANGE;
               case V_FAULT: return COL_RED; default: return COL_MUTED; }
}
// Serial number = the 8-byte ROM ID as a 16-char uppercase hex string (Makita format).
void formatSerial(char *out) {
  for (int i = 0; i < 8; i++) sprintf(out + i * 2, "%02X", bat.romId[i]);
  out[16] = 0;
}

// One D6 addressed read (1 data byte), used for the latched-fault markers.
uint8_t d6ReadByte(uint16_t addr) {
  makita.reset(); delayMicroseconds(400);
  mkWrite(0xCC); mkWrite(0xD6);
  mkWrite(addr & 0xFF); mkWrite((addr >> 8) & 0xFF); mkWrite(0x01);
  uint8_t v = mkRead(); mkRead();    // data byte + ACK
  return v;
}
// One D4 addressed read (1 data byte).
uint8_t d4ReadByte(uint16_t addr) {
  makita.reset(); delayMicroseconds(400);
  mkWrite(0xCC); mkWrite(0xD4);
  mkWrite(addr & 0xFF); mkWrite((addr >> 8) & 0xFF); mkWrite(0x01);
  uint8_t v = mkRead(); mkRead();
  return v;
}
// A D4 addressed read of n data bytes (n <= 15) into buf; strips the trailing ACK.
void d4ReadBlock(uint16_t addr, uint8_t *buf, uint8_t n) {
  makita.reset(); delayMicroseconds(400);
  mkWrite(0xCC); mkWrite(0xD4);
  mkWrite(addr & 0xFF); mkWrite((addr >> 8) & 0xFF); mkWrite(n);
  for (uint8_t i = 0; i < n; i++) buf[i] = mkRead();
  mkRead();   // ACK terminator (0x06)
}

// Round a percentage UP to a 5% step (ceil), so any nonzero event count reads >= 5%
// rather than 0 (e.g. an over-discharge count of 1 over 83 cycles reads 5%). This is the
// 5% quantizer used for the OD/OL wear percentages.
uint8_t round5up(uint32_t num, uint32_t den) {
  if (den == 0) return 0;
  uint32_t p = (num * 100) / den;
  if (p > 100) p = 100;
  return (uint8_t)(((p + 4) / 5) * 5);
}

// Read the latched-fault markers (D6 0x58D / 0x309), the assembly date (D4 0x000-0x002,
// YY MM DD binary) AND the extended D4 wear counters, all in one TESTMODE session.
//   0x150 -> SOC (charge level, u16 LE)     0x0BA -> over-discharge event count (u8)
//   0x08D -> over-load block (7B, bit-packed; over-load count = counterC + counterE)
// Family A (D4) is our LXT packs; famB/famC (D6) return 0. Derived %s are SECONDARY: the
// "% of cycles" framing is unproven (a raw count can exceed the charge count), so the raw
// counter is the primary figure everywhere and the % is shown only as a soft hint.
void readExtended() {
  bat.latchedFault = false;
  bat.extValid = false;
  bat.socRaw = 0; bat.odEventCount = 0; bat.olEventCount = 0;
  bat.odWearPct = 0; bat.olWearPct = 0; bat.faultMkA = 0; bat.faultMkB = 0;
  if (!bat.valid || strcmp(bat.commandVersion, "F0513") == 0) return;

  digitalWrite(ENABLE_PIN, HIGH); delay(400);
  makita.reset(); delayMicroseconds(400);
  mkWrite(0x33); mkWrite(0xD9); mkWrite(0x96); mkWrite(0xA5); delay(20);   // TESTMODE

  uint8_t a = d6ReadByte(bmsAddr->faultMkA), b = d6ReadByte(bmsAddr->faultMkB);
  bat.faultMkA = a; bat.faultMkB = b;
  bat.asmY = d4ReadByte(bmsAddr->asmDate);
  bat.asmM = d4ReadByte(bmsAddr->asmDate + 1);
  bat.asmD = d4ReadByte(bmsAddr->asmDate + 2);

  // SOC (charge level) and over-discharge event count.
  uint8_t soc[2]; d4ReadBlock(bmsAddr->soc, soc, 2);
  bat.socRaw = le16(soc, 0);
  bat.odEventCount = d4ReadByte(bmsAddr->odCount);

  // Over-load block: 7 bytes, two packed counters summed. Byte b2 (0x08F) is unused by
  // this field. This decode reads ~0-1 on all tested packs.
  uint8_t ol[7]; d4ReadBlock(bmsAddr->olBlock, ol, 7);
  uint16_t counterC = ((ol[4] & 0x03) << 8 | ol[3]) + ((ol[0] >> 6) | (ol[1] & 0x3F) << 2);
  uint16_t counterE = (ol[5] >> 4) | (ol[6] & 0x0F) << 4;
  bat.olEventCount = counterC + counterE;

  makita.reset(); delayMicroseconds(400);
  mkWrite(0xCC); mkWrite(0xD9); mkWrite(0xFF); mkWrite(0xFF);              // TESTMODE exit
  digitalWrite(ENABLE_PIN, LOW);

  // Some old packs answer the AA frame + F0513 cells but NOT the CC-addressed D4/D6 reads:
  // those read back 0xFF or unstable noise. A 0xFF over-discharge count (255) means the D4
  // path did not answer, so the whole extended block — OD/OL AND the D6 fault markers — is
  // untrustworthy (seen on one old pack: odCnt=FF, faultMk 0x309=FD -> a false 70 %% / latched).
  if (bat.odEventCount == 0xFF) {
    bat.socRaw = 0; bat.odEventCount = 0; bat.olEventCount = 0;
    bat.asmY = 0; bat.asmM = 0; bat.asmD = 0;
    bat.faultMkA = 0; bat.faultMkB = 0; bat.latchedFault = false;
    bat.extValid = false;
    return;
  }

  bat.latchedFault = ((a != 0 && a != 0xFF) || (b != 0 && b != 0xFF));
  // Secondary (unproven) wear percentages, denominator = charge count.
  bat.odWearPct = round5up(bat.odEventCount, bat.chargeCount);
  bat.olWearPct = round5up(bat.olEventCount, bat.chargeCount);
  bat.extValid = true;

#if COMM_DEBUG
  dumpDecoded();   // bench-validation readout (COMM_DEBUG only)
#endif
}

// key/value row (size 2, value right-aligned) using the shared _ry cursor.
// smooth-font helpers (labels/chrome). Classic font stays for data/values.
// NOTE: GFX custom fonts are scaled by textSize in Adafruit_GFX. Callers often
// leave textSize at 2 (for classic values), which would DOUBLE the smooth font.
// Always force textSize(1) so a GFX label renders at its true point size.
int gfxText(const GFXfont* f, int x, int baseY, const char* s, uint16_t col) {
  tft.setFont(f); tft.setTextSize(1); tft.setTextColor(col);
  int16_t x1, y1; uint16_t w, h; tft.getTextBounds(s, 0, baseY, &x1, &y1, &w, &h);
  tft.setCursor(x, baseY); tft.print(s); tft.setFont(NULL);
  return w;
}
void gfxCenter(const GFXfont* f, int cx, int baseY, const char* s, uint16_t col) {
  tft.setFont(f); tft.setTextSize(1); tft.setTextColor(col);
  int16_t x1, y1; uint16_t w, h; tft.getTextBounds(s, 0, baseY, &x1, &y1, &w, &h);
  tft.setCursor(cx - w / 2, baseY); tft.print(s); tft.setFont(NULL);
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
  int vw = strlen(val) * 12;
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
  tft.setFont(&FreeSansBold9pt7b); tft.setTextSize(1);
  int16_t bx, by; uint16_t bw, bh; tft.getTextBounds(t, 0, 228, &bx, &by, &bw, &bh);
  bool hasIcon = (v == V_HEALTHY || v == V_REPAIRABLE || v == V_SUSPECT || v == V_FAULT);
  int iconW = hasIcon ? 24 : 0, gap = hasIcon ? 10 : 0;
  int sx = (320 - (iconW + gap + (int)bw)) / 2;
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
  tft.setTextColor(COL_BG); tft.setCursor(sx + iconW + gap, 228); tft.print(t); tft.setFont(NULL);
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
      char ms[24];
      if (bat.valid) { snprintf(ms, sizeof(ms), "%.1fV %s", bat.packVoltage, verdictText(v));
                       tft.setTextColor(verdictColor(v)); }
      else           { strcpy(ms, "no pack"); tft.setTextColor(COL_MUTED); }
      tft.setTextSize(1);
      // centered in the gap between the icon's visible bottom (~y+48) and the label (~y+70)
      tft.setCursor(cx - (int)strlen(ms) * 3, y + 55); tft.print(ms);
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
  else if (bat.odEventCount == 0) strcpy(buf, "none");
  else snprintf(buf, sizeof(buf), "%u (%u%%)", bat.odEventCount, bat.odWearPct);
  kvRow(tr(S_OVERDISCHARGE), buf, !bat.extValid ? COL_MUTED : (bat.odEventCount ? COL_TEXT : COL_GREEN));
  if (!bat.extValid) strcpy(buf, "-");
  else if (bat.olEventCount == 0) strcpy(buf, "none");
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
  int vw = strlen(val) * 12;
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
  tft.setFont(&FreeSansBold9pt7b); tft.setTextSize(1); tft.setTextColor(COL_BG);
  int16_t pbx, pby; uint16_t pbw, pbh; tft.getTextBounds(bt, 0, 228, &pbx, &pby, &pbw, &pbh);
  tft.setCursor((320 - (int)pbw) / 2, 228); tft.print(bt); tft.setFont(NULL);
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
  int grp = 0; char action[40];
  HwFault hw = findHardwareFault(&grp, action, sizeof(action));

  // Checklist (the raw signals behind the verdict).
  uint8_t causes = !isF0513 ? lockCauses(bat.msg) : 0;
  char cb[24]; lockCausesText(causes, cb, sizeof(cb));
  _ry = y;
  rowLabel(6, _ry, tr(S_CHARGER_LOCK));
  { const char* s = causes ? cb : tr(S_NONE); tft.setTextSize(2); int vw = strlen(s) * 12;
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
  const char* title; uint16_t tcol; char line2[44];

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
    gfxText(&FreeSansBold9pt7b, 14, iy + 22, tr((StrId)(S_FLIP + i)), sel ? COL_HEAD : COL_TEXT);
    tft.setTextSize(2);
    if (i < 2) {                               // boolean toggles
      const char* on = vals[i] ? "ON" : "OFF";
      tft.setTextColor(vals[i] ? COL_GREEN : COL_MUTED);
      int vw = strlen(on) * 12; tft.setCursor(300 - vw, iy + 10); tft.print(on);
    } else {                                   // Language: current code (click cycles)
      const char* code = LANG_CODE[lang];
      tft.setTextColor(COL_ACCENT);
      int vw = strlen(code) * 12; tft.setCursor(300 - vw, iy + 10); tft.print(code);
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

// ---------- Buttons / navigation logic (V2) ----------
void handleClick() {
  switch (state) {
    case LAUNCHER:
      switch (launcherIndex) {
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
      switch (toolIndex) {
        case 0: bridgeActive = true; state = PC_BRIDGE; break;   // enter active by default
        case 1: ledsOn();  toast(tr(S_LEDS_ON_MSG), COL_GREEN); break;
        case 2: ledsOff(); toast(tr(S_LEDS_OFF_MSG), COL_MUTED); break;
        case 3: state = readAllData() ? CONFIRM_RESET : COMM_ERROR; break;
        case 4: state = readAllData() ? DEBUG_RAW : COMM_ERROR; break;
        case 5: settingsIndex = 0; state = SETTINGS; break;
      }
      break;
    case SETTINGS:
      if (settingsIndex == 0) { cfgFlip = !cfgFlip; tft.setRotation(cfgFlip ? 3 : 1);
                                prefs.putBool("flip", cfgFlip); lastRenderedState = -1; }
      else if (settingsIndex == 1) { cfgBridgeBoot = !cfgBridgeBoot; prefs.putBool("bridge", cfgBridgeBoot); }
      else                    { lang = (lang + 1) % LANG_COUNT; prefs.putInt("lang", lang);
                                lastRenderedState = -1; }   // full redraw so all text swaps
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

// ---------- PC bridge (ArduinoOBI-compatible USB <-> OneWire) ----------
// Reads one command frame [0x01, len, rsp_len, cmd, data...] from Serial,
// runs it (same OneWire transactions as standalone), and writes back the
// response [cmd, rsp_len, payload...]. Drop-in for the ArduinoOBI USB bridge,
// so the Open Battery Information PC app talks to PocketOBI directly.
// Only called in the PC_BRIDGE state; serial debug is suppressed there.

// Returns the byte read, or -1 on timeout (distinct from a real 0x00 data byte).
static int bridgeReadByte(uint16_t timeoutMs) {
  unsigned long t0 = millis();
  while (!Serial.available()) {
    if (millis() - t0 > timeoutMs) return -1;
  }
  return (uint8_t)Serial.read();
}

void serviceBridge() {
  if (Serial.available() < 1) return;
  if ((uint8_t)Serial.peek() != 0x01) { Serial.read(); return; } // resync on junk
  Serial.read();                                    // consume start byte 0x01

  int len    = bridgeReadByte(50);
  int rspLen = bridgeReadByte(50);
  int cmd    = bridgeReadByte(50);
  if (len < 0 || rspLen < 0 || cmd < 0) return;     // truncated header -> drop; the PC resends
  // Clamp to the local buffer capacities below (data[48]/c[52], payload[40]/rsp[48]) so a
  // malformed length from the USB side can never overrun the stack. Legit OBI frames
  // (len <= ~4, rsp_len <= 40) are never affected.
  if (len    > 48) len    = 48;
  if (rspLen > 40) rspLen = 40;

  uint8_t data[48];
  for (int i = 0; i < len; i++) {
    int b = bridgeReadByte(50);
    if (b < 0) return;                              // truncated body -> drop
    data[i] = (uint8_t)b;
  }

  uint8_t rsp[48];
  int outLen = rspLen;

  if (cmd == 0x01) {                                // interface version query
    rsp[0] = FW_VER_MAJOR; rsp[1] = FW_VER_MINOR; rsp[2] = FW_VER_PATCH;  // derived from FW_VERSION
  } else if (cmd == 0x02) {                         // compatibility-contract query
    rsp[0] = PROTOCOL_VERSION;                       // app checks this to warn on mismatch
    rsp[1] = gammeId;                                // family id -> companion-app decoder routing
    rsp[2] = cellCount;                              // active family cell count
  } else if (cmd == 0x31 || cmd == 0x32) {          // F0513 raw model/version
    uint8_t b1 = 0xFF, b2 = 0xFF;
    readF0513Raw(cmd, &b1, &b2);
    rsp[0] = b2; rsp[1] = b1;                        // ArduinoOBI byte order
  } else if (cmd == 0x33) {
    uint8_t c[52]; c[0] = 0x01; c[1] = len; c[2] = rspLen; c[3] = cmd;
    for (int i = 0; i < len; i++) c[4 + i] = data[i];
    uint8_t payload[40], rom[8];
    sendCommand(c, payload, rom);
    for (int i = 0; i < 8 && i < outLen; i++) rsp[i] = rom[i];
    for (int i = 0; i < outLen - 8; i++) rsp[8 + i] = payload[i];
  } else if (cmd == 0xCC) {
    uint8_t c[52]; c[0] = 0x01; c[1] = len; c[2] = rspLen; c[3] = cmd;
    for (int i = 0; i < len; i++) c[4 + i] = data[i];
    uint8_t payload[40];
    sendCommand(c, payload);
    for (int i = 0; i < outLen; i++) rsp[i] = payload[i];
  } else {
    outLen = 0;
  }

  Serial.write((uint8_t)cmd);
  Serial.write((uint8_t)rspLen);
  for (int i = 0; i < outLen; i++) Serial.write(rsp[i]);
}

// ---------- Setup / loop ----------
void setup() {
  Serial.begin(115200);

  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, LOW);

  pinMode(ENC_BTN, INPUT_PULLUP);
  pinMode(BACK_BTN, INPUT_PULLUP);

  // Encoder: the library enables the internal pull-ups. Read by polling
  // (tick() called in the loop): simple and free of interrupt concerns.
  // A/B swapped on purpose so the rotation direction matches the display.
  encoder = new RotaryEncoder(ENC_B, ENC_A, RotaryEncoder::LatchMode::FOUR3);

  // Hardware SPI on our pins (SCLK=GPIO0, MOSI=GPIO1). Must be called before
  // tft.init(): this "claims" the bus with the correct pins.
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  // Load persisted settings before configuring the display.
  prefs.begin("pocketobi", false);
  cfgFlip       = prefs.getBool("flip", false);
  cfgBridgeBoot = prefs.getBool("bridge", false);
  lang          = prefs.getInt("lang", LANG_EN);
  if (lang < 0 || lang >= LANG_COUNT) lang = LANG_EN;

  tft.init(240, 320);        // 2.4" resolution
  tft.invertDisplay(false);  // correct colors for this ST7789 panel
  tft.setRotation(cfgFlip ? 3 : 1);

  // Boot splash, then straight to the launcher (or the PC bridge if configured). We deliberately
  // do NOT auto-read the pack at boot: a full read is ~seconds (the ENABLE wake dominates), which
  // made startup feel slow. The read is on demand — launcher item 0 (Battery) / 1 (Repair) trigger
  // it. Insert pack, boot lands on the launcher instantly, click to read.
  drawSplash();
  delay(1500);
  state = cfgBridgeBoot ? PC_BRIDGE : LAUNCHER;
  render();
}

void loop() {
  // Encoder polling: to be called as often as possible.
  encoder->tick();

  // PC bridge mode: act as a USB<->OneWire bridge for the PC app (only while active).
  if (state == PC_BRIDGE && bridgeActive) serviceBridge();

#if ENC_DEBUG
  // Trace raw pin transitions + the library position over serial.
  static int lastRawA = -1, lastRawB = -1;
  int ra = digitalRead(ENC_A), rb = digitalRead(ENC_B);
  if (ra != lastRawA || rb != lastRawB) {
    lastRawA = ra; lastRawB = rb;
    Serial.printf("A=%d B=%d pos=%ld\n", ra, rb, encoder->getPosition());
  }
#endif

  // Encoder rotation: the position (in detents) is maintained by the library.
  // Apply the difference since the last read.
  long pos = encoder->getPosition();
  long diff = pos - lastEncPos;
  lastEncPos = pos;
  int steps = abs(diff);
  for (int i = 0; i < steps; i++) {
    handleRotate(diff > 0 ? 1 : -1);
  }

  // Encoder button (simple debounce, acts on press)
  bool pressed = (digitalRead(ENC_BTN) == LOW);
  if (pressed && !btnPressed && millis() - lastBtnTime > 250) {
    btnPressed = true;
    lastBtnTime = millis();
    handleClick();
  } else if (!pressed) {
    btnPressed = false;
  }

  // Back button: short press (on release) = back, long press = home.
  bool backNow = (digitalRead(BACK_BTN) == LOW);
  if (backNow && !backDown && millis() - backStart > 50) {
    backDown = true;
    backStart = millis();
    backLongFired = false;
  } else if (backNow && backDown && !backLongFired &&
             millis() - backStart >= BACK_LONG_MS) {
    backLongFired = true;  // long press reached -> home immediately, fire once
    goHome();
  } else if (!backNow && backDown) {
    backDown = false;
    if (!backLongFired) handleBack();  // released before the long threshold
  }

  delay(1);
}
