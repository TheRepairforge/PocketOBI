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


#include "pocketobi.h"
#include "protocol.h"
#include "unlock.h"
#include "decode.h"
#include "display.h"
#include "ui_nav.h"
#include "bridge.h"
#include "strings_ja.h"   // STRTAB_JA, only when ENABLE_CJK=1


OneWire makita(ONEWIRE_PIN);
// Hardware-SPI constructor (much faster than software SPI): (&SPI, cs, dc, rst).
// SCLK/MOSI pins are assigned via SPI.begin() in setup().
PocketTFT tft(&SPI, TFT_CS, TFT_DC, TFT_RST);   // = Adafruit_ST7789, UTF-8 aware in the JA build

#if BOARD_HAS_TOUCH
// The display drives the default SPI object (remapped to its HSPI pins in setup()),
// so the XPT2046 touch controller gets its OWN bus on a second SPIClass.
#include <XPT2046_Touchscreen.h>
SPIClass touchSPI(HSPI);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

// Resistive-touch calibration: raw XPT2046 -> screen pixels. On this CYD the axes
// are crossed (screen X from the raw Y axis inverted, screen Y from the raw X axis);
// map() handles the inverted ranges. The four corner values live in NVS and are
// editable from Settings > Calibrate; these #defines are the factory fallback,
// measured on the bench. Re-run Calibrate after a panel swap.
int tcXL = TCAL_DEF_X_LEFT, tcXR = TCAL_DEF_X_RIGHT,
    tcYT = TCAL_DEF_Y_TOP,  tcYB = TCAL_DEF_Y_BOTTOM;
// Set to 1 to calibrate manually: each tap draws a dot where it maps + prints
// raw/mapped coords and does NOT navigate. Set back to 0 when tuned.
// Edge-detect state: act once per touch, on touch-down (not while held).
bool touchDown = false;
unsigned long lastTouchMs = 0;
#endif






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

// ---------- i18n: string table lives in strings_i18n.h (data only). The tr()
// accessor and the mutable `lang` selector are logic/state and stay here. ----------
int lang = LANG_EN;
const char* tr(StrId id) {   // prototype in pocketobi.h
#if ENABLE_CJK
  if (lang == LANG_JA) return STRTAB_JA[id] ? STRTAB_JA[id] : STRTAB[id][LANG_EN];
#endif
  return STRTAB[id][lang];
}

const int   toolCount = 6;
#if BOARD_HAS_TOUCH
const int   settingsCount = 4;   // Flip screen, PC bridge at boot, Language, Calibrate touch
#else
const int   settingsCount = 3;   // Flip screen, PC bridge at boot, Language
#endif

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
bool backDown = false;
bool backLongFired = false;
unsigned long backStart = 0;







BatteryData bat;

// ---------- Makita OneWire low level ----------
// Inter-byte timing taken from the official ArduinoOBI firmware (main.cpp):
// 90 us between each byte written or read, 400 us after each reset.
// The intra-bit timing (slots) is handled by OneWire2 itself.
// ENABLE_PIN is driven HIGH + 400 ms wait before every transaction.















































// PC bridge mode: the tool acts as a USB<->OneWire bridge for the Open Battery
// Information PC app (drop-in ArduinoOBI replacement). Serial debug is suppressed
// while in this state (it would corrupt the binary protocol).
// PC bridge running state; the encoder toggles it live on the PC bridge screen.
bool bridgeActive = true;



// ================= V2 UI ==================
// (enum Verdict is defined near the top, with the other enums.)

int lastBatteryPage = -1;  // force a screen clear when the Battery page changes







































// ---------- Setup / loop ----------
void setup() {
  Serial.begin(115200);

  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, LOW);

#if BOARD_HAS_ENCODER
  pinMode(ENC_BTN, INPUT_PULLUP);
  pinMode(BACK_BTN, INPUT_PULLUP);

  // Encoder: the library enables the internal pull-ups. Read by polling
  // (tick() called in the loop): simple and free of interrupt concerns.
  // A/B swapped on purpose so the rotation direction matches the display.
  encoder = new RotaryEncoder(ENC_B, ENC_A, RotaryEncoder::LatchMode::FOUR3);
#endif

#if BOARD_HAS_TOUCH
  // XPT2046 on its own SPI bus; we read raw points and map them ourselves.
  touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touch.begin(touchSPI);
  touch.setRotation(0);
#endif

#ifdef TFT_BL
  // CYD: the backlight is GPIO-controlled — without this the panel stays black.
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
#endif

  // Hardware SPI on the board's pins (see BOARD HEADER). Must be called before
  // tft.init(): this "claims" the bus with the correct pins.
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  // Load persisted settings before configuring the display.
  prefs.begin("pocketobi", false);
  cfgFlip       = prefs.getBool("flip", false);
  cfgBridgeBoot = prefs.getBool("bridge", false);
  lang          = prefs.getInt("lang", LANG_EN);
  if (lang < 0 || lang >= LANG_NUM) lang = LANG_EN;   // e.g. JA saved, then a non-JA build flashed
#if BOARD_HAS_TOUCH
  tcXL = prefs.getInt("tcXL", TCAL_DEF_X_LEFT);
  tcXR = prefs.getInt("tcXR", TCAL_DEF_X_RIGHT);
  tcYT = prefs.getInt("tcYT", TCAL_DEF_Y_TOP);
  tcYB = prefs.getInt("tcYB", TCAL_DEF_Y_BOTTOM);
#endif

  tft.init(240, 320);        // 240x320 ST7789 panel (both boards)
  tft.invertDisplay(false);  // correct colors on both boards' ST7789 panels
  applyRotation();

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
#if BOARD_HAS_ENCODER
  // Encoder polling: to be called as often as possible.
  encoder->tick();
#endif

  // PC bridge mode: act as a USB<->OneWire bridge for the PC app (only while active).
  if (state == PC_BRIDGE && bridgeActive) serviceBridge();

#if BOARD_HAS_ENCODER
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
#endif // BOARD_HAS_ENCODER

#if BOARD_HAS_TOUCH
  serviceTouch();   // read the XPT2046, hit-test the screen, drive the nav core
#endif

  delay(1);
}

