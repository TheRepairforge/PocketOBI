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
 *  GPIO3  -> Battery pin 2 (DATA, OneWire)   + 4.7k pull-up to 3.3V
 *  GPIO4  -> Battery pin 6 (ENABLE)          + 4.7k pull-up to 3.3V
 *  GND    -> Battery pin 5 (B-)
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
 */

#include "OneWire2.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <RotaryEncoder.h>
#include <SPI.h>
#include <string.h>

// ---------- Pins ----------
#define ONEWIRE_PIN 3
#define ENABLE_PIN  4

#define ENC_A    5
#define ENC_B    6
#define ENC_BTN  7
#define BACK_BTN 2   // module "KO" secondary button: short = back, long = home

// ST7789 TFT display pins (full config kept in this sketch)
#define TFT_CS   21
#define TFT_DC   20
#define TFT_RST  10
#define TFT_MOSI 1   // SDA
#define TFT_SCLK 0   // SCL

OneWire makita(ONEWIRE_PIN);
// Hardware-SPI constructor (much faster than software SPI): (&SPI, cs, dc, rst).
// SCLK/MOSI pins are assigned via SPI.begin() in setup().
Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, TFT_CS, TFT_DC, TFT_RST);

// Firmware version (see CHANGELOG.md).
#define FW_VERSION "0.6.0"

// ---------- Color palette (dark dashboard theme) ----------
// Compile-time RGB888 -> RGB565 conversion.
#define RGB565(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

#define COL_BG      RGB565(0x0C, 0x15, 0x24)  // page background (dark navy)
#define COL_ACCENT  RGB565(0x0E, 0x7C, 0x86)  // header bars, highlight (teal)
#define COL_PANEL   RGB565(0x1A, 0x28, 0x3A)  // bar tracks, chips
#define COL_TEXT    RGB565(0xE6, 0xED, 0xF5)  // primary text
#define COL_MUTED   RGB565(0x82, 0x98, 0xB0)  // secondary text
#define COL_HEAD    RGB565(0xEA, 0xFB, 0xFC)  // text on accent header
#define COL_GREEN   RGB565(0x22, 0xC5, 0x5E)  // normal cell / OK
#define COL_YELLOW  RGB565(0xEA, 0xB3, 0x08)  // warning
#define COL_RED     RGB565(0xEF, 0x44, 0x44)  // critical / locked
#define COL_ORANGE  RGB565(0xF2, 0x66, 0x22)  // RepairForge brand spark

#define HEADER_H 28  // height of the colored title bar

// ---------- Protocol commands (from makita_lxt.py) ----------
const uint8_t MODEL_CMD[]       = {0x01, 0x02, 0x10, 0xCC, 0xDC, 0x0C};
const uint8_t READ_DATA_CMD[]   = {0x01, 0x04, 0x1D, 0xCC, 0xD7, 0x00, 0x00, 0xFF};
const uint8_t TESTMODE_CMD[]    = {0x01, 0x03, 0x09, 0x33, 0xD9, 0x96, 0xA5};
const uint8_t LEDS_ON_CMD[]     = {0x01, 0x02, 0x09, 0x33, 0xDA, 0x31};
const uint8_t LEDS_OFF_CMD[]    = {0x01, 0x02, 0x09, 0x33, 0xDA, 0x34};
const uint8_t RESET_ERROR_CMD[] = {0x01, 0x02, 0x09, 0x33, 0xDA, 0x04};
const uint8_t READ_MSG_CMD[]    = {0x01, 0x02, 0x28, 0x33, 0xAA, 0x00};
const uint8_t CLEAR_CMD[]       = {0x01, 0x02, 0x00, 0xCC, 0xF0, 0x00};

// Commands specific to older F0513 batteries
const uint8_t F0513_VCELL1_CMD[] = {0x01, 0x01, 0x02, 0xCC, 0x31};
const uint8_t F0513_VCELL2_CMD[] = {0x01, 0x01, 0x02, 0xCC, 0x32};
const uint8_t F0513_VCELL3_CMD[] = {0x01, 0x01, 0x02, 0xCC, 0x33};
const uint8_t F0513_VCELL4_CMD[] = {0x01, 0x01, 0x02, 0xCC, 0x34};
const uint8_t F0513_VCELL5_CMD[] = {0x01, 0x01, 0x02, 0xCC, 0x35};
const uint8_t F0513_TEMP_CMD[]   = {0x01, 0x01, 0x02, 0xCC, 0x52};

// ---------- UI state ----------
enum UiState { HOME, MENU, DETAILS, CONFIRM_RESET, RESET_RESULT, DEBUG_RAW, COMM_ERROR, ABOUT };
UiState state = HOME;
int lastRenderedState = -1; // so the screen is only cleared when the screen changes

const char* menuItems[] = {
  "Read battery info",
  "View details",
  "Reset error",
  "Pack LEDs on",
  "Pack LEDs off",
  "Debug / raw",
  "Version / info"
};
// Bullet color per menu entry (small accent dot, drawn with a primitive).
const uint16_t menuIcons[] = {
  COL_GREEN, COL_ACCENT, COL_RED, COL_YELLOW, COL_MUTED, COL_MUTED, COL_ACCENT
};
const int menuCount = 7;
int menuIndex = 0;

// Kept for the reset visual feedback (error code before -> after).
uint8_t resetErrBefore = 0;
uint8_t resetErrAfter = 0;
bool resetLockedAfter = false;

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
#define COMM_DEBUG 1

// ---------- Battery data ----------
struct BatteryData {
  bool valid = false;
  char model[10];
  char commandVersion[8]; // "" = standard, "F0513" = older generation
  uint8_t romId[8];
  uint8_t msg[32];        // raw "battery message" frame (after ROM ID)
  uint16_t chargeCount;
  bool locked;
  uint8_t errorCode;
  uint8_t mfgDay, mfgMonth;
  uint16_t mfgYear;
  float capacityAh;
  uint8_t batteryType;

  float packVoltage;
  float cell[5];
  float cellDiff;
  float tempCell;
  float tempMosfet; // -1 if unavailable (F0513 case)
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
    payloadLen = rspLen - 8; // rspLen includes the 8 ROM ID bytes
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
  bool gotModel = sendCommand(MODEL_CMD, modelPayload);

  if (gotModel && isPrintableAscii(modelPayload, 7)) {
    strcpy(bat.commandVersion, "");
    memcpy(bat.model, modelPayload, 7);
    bat.model[7] = 0;

    uint8_t payload[32];
    uint8_t romId[8];
    if (!sendCommand(READ_MSG_CMD, payload, romId)) return false;

    memcpy(bat.romId, romId, 8);
    memcpy(bat.msg, payload, 32);

    // chargeCount: nibble-swap of payload[26] (MSB) and payload[27] (LSB),
    // big-endian order matching makita_lxt.py (bytearray[::-1] + int.from_bytes 'big').
    uint16_t swapped = (nibbleSwap(payload[26]) << 8) | nibbleSwap(payload[27]);
    bat.chargeCount = swapped & 0x0FFF;
    bat.locked = (payload[20] & 0x0F) > 0;
    bat.errorCode = payload[19];
    bat.capacityAh = nibbleSwap(payload[16]) / 10.0;
    bat.batteryType = nibbleSwap(payload[11]);
    bat.mfgYear = 2000 + romId[0];
    bat.mfgMonth = romId[1];
    bat.mfgDay = romId[2];

  } else {
    // No valid ASCII response -> try the older F0513 generation
    uint8_t b1, b2;
    if (!readF0513Raw(0x31, &b1, &b2)) return false;

    strcpy(bat.commandVersion, "F0513");
    snprintf(bat.model, sizeof(bat.model), "BL%X%X", b2, b1);

    uint8_t tmp[8];
    sendCommand(CLEAR_CMD, tmp); // reset state, like get_f0513_model()

    bat.chargeCount = 0;
    bat.locked = false;
    bat.errorCode = 0;
    bat.capacityAh = 0;
    bat.batteryType = 0;
    bat.mfgYear = 0;
    bat.mfgMonth = 0;
    bat.mfgDay = 0;
  }

  bat.valid = true;
  return true;
}

// Read live data (voltages, temperatures) -> on_read_data_click().
// Switches between the standard path and the F0513 path depending on what
// readStaticInfo() detected.
bool readLiveData() {
  if (strcmp(bat.commandVersion, "F0513") == 0) {
    uint8_t tmp[8];
    sendCommand(CLEAR_CMD, tmp);
    sendCommand(CLEAR_CMD, tmp);

    uint8_t c1[2], c2[2], c3[2], c4[2], c5[2], t[2];
    if (!sendCommand(F0513_VCELL1_CMD, c1)) return false;
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

    float sum = 0, mn = 99, mx = 0;
    for (int i = 0; i < 5; i++) {
      sum += bat.cell[i];
      if (bat.cell[i] < mn) mn = bat.cell[i];
      if (bat.cell[i] > mx) mx = bat.cell[i];
    }
    bat.packVoltage = sum;
    bat.cellDiff = mx - mn;
    bat.tempCell = le16(t, 0) / 100.0;
    bat.tempMosfet = -1; // not available on F0513
    return true;
  }

  // Standard path
  uint8_t payload[29];
  if (!sendCommand(READ_DATA_CMD, payload)) return false;

  bat.packVoltage = le16(payload, 0) / 1000.0;
  bat.cell[0] = le16(payload, 2) / 1000.0;
  bat.cell[1] = le16(payload, 4) / 1000.0;
  bat.cell[2] = le16(payload, 6) / 1000.0;
  bat.cell[3] = le16(payload, 8) / 1000.0;
  bat.cell[4] = le16(payload, 10) / 1000.0;

  float mn = 99, mx = 0;
  for (int i = 0; i < 5; i++) {
    if (bat.cell[i] < mn) mn = bat.cell[i];
    if (bat.cell[i] > mx) mx = bat.cell[i];
  }
  bat.cellDiff = mx - mn;
  bat.tempCell = le16(payload, 14) / 100.0;
  bat.tempMosfet = le16(payload, 16) / 100.0;
  return true;
}

bool readAllData() {
  bool ok1 = readStaticInfo();
  bool ok2 = readLiveData();
  return ok1 && ok2;
}

// Error reset -> on_reset_errors_click(): TESTMODE then RESET_ERROR
void resetErrors() {
  uint8_t tmp[8];
  sendCommand(TESTMODE_CMD, tmp);
  delay(20);
  sendCommand(RESET_ERROR_CMD, tmp);
}

void ledsOn() {
  uint8_t tmp[8];
  sendCommand(TESTMODE_CMD, tmp);
  delay(20);
  sendCommand(LEDS_ON_CMD, tmp);
}

void ledsOff() {
  uint8_t tmp[8];
  sendCommand(TESTMODE_CMD, tmp);
  delay(20);
  sendCommand(LEDS_OFF_CMD, tmp);
}

// ---------- Display ----------
// Cell diagnostic thresholds (Makita 18V Li-ion):
//  - bar scale : 2.5 V (empty) to 4.2 V (full)
//  - red    : critical cell (< 3.0 V), or the lowest cell of a badly
//             imbalanced pack (spread > 0.30 V)
//  - yellow : lowest cell of a moderately imbalanced pack (spread > 0.15 V)
//  - green  : normal
#define CELL_V_MIN   2.5f
#define CELL_V_MAX   4.2f
#define CELL_V_CRIT  3.0f
#define DIFF_WARN    0.15f
#define DIFF_BAD     0.30f

// Cell color based on its voltage and its position within the pack.
uint16_t cellColor(float v, float minV, float diff) {
  if (v < CELL_V_CRIT) return COL_RED;
  bool isLowest = (v <= minV + 0.001f);
  if (isLowest && diff > DIFF_BAD)  return COL_RED;
  if (isLowest && diff > DIFF_WARN) return COL_YELLOW;
  return COL_GREEN;
}

// Colored title bar at the top of every screen.
void drawHeader(const char* title) {
  tft.fillRect(0, 0, tft.width(), HEADER_H, COL_ACCENT);
  tft.setTextSize(2);
  tft.setTextColor(COL_HEAD, COL_ACCENT);
  tft.setCursor(6, 6);
  tft.print(title);
}

// Rounded "chip" with a label (footer status pills).
void drawChip(int x, int y, int w, const char* txt, uint16_t fg) {
  tft.fillRoundRect(x, y, w, 26, 5, COL_PANEL);
  tft.setTextSize(2);
  tft.setTextColor(fg, COL_PANEL);
  tft.setCursor(x + 8, y + 5);
  tft.print(txt);
}

void drawHome() {
  bool isF0513 = strcmp(bat.commandVersion, "F0513") == 0;

  // Header: title + model (right-aligned)
  drawHeader("MAKITA LXT");
  if (bat.valid) {
    int w = strlen(bat.model) * 12;
    tft.setTextColor(COL_HEAD, COL_ACCENT);
    tft.setCursor(tft.width() - w - 6, 6);
    tft.print(bat.model);
  }

  if (!bat.valid) {
    tft.setTextSize(2);
    tft.setTextColor(COL_TEXT, COL_BG);
    tft.setCursor(10, 90);
    tft.print("No battery found");
    tft.setTextColor(COL_MUTED, COL_BG);
    tft.setCursor(10, 124);
    tft.print("Connect a pack, then");
    tft.setCursor(10, 146);
    tft.print("Menu > Read battery");
    return;
  }

  // Pack voltage, large
  tft.setTextSize(3);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setCursor(6, 34);
  tft.printf("%.2f", bat.packVoltage);
  tft.setTextSize(2);
  tft.setTextColor(COL_MUTED, COL_BG);
  tft.setCursor(6 + 5 * 18 + 8, 40);
  tft.print("V");

  // Lowest cell (for imbalance coloring)
  float minV = 99;
  for (int i = 0; i < 5; i++) if (bat.cell[i] < minV) minV = bat.cell[i];

  // Per-cell voltage bars
  const int top = 64, rowH = 22, barX = 34, barH = 14, valX = 268;
  const int barW = valX - barX - 6;
  for (int i = 0; i < 5; i++) {
    int y = top + i * rowH;
    uint16_t col = cellColor(bat.cell[i], minV, bat.cellDiff);

    tft.setTextSize(2);
    tft.setTextColor(COL_MUTED, COL_BG);
    tft.setCursor(2, y);
    tft.printf("C%d", i + 1);

    // Track + proportional colored fill
    tft.fillRoundRect(barX, y, barW, barH, 3, COL_PANEL);
    int fill = (int)((bat.cell[i] - CELL_V_MIN) / (CELL_V_MAX - CELL_V_MIN) * barW);
    if (fill < 0) fill = 0;
    if (fill > barW) fill = barW;
    if (fill >= 6) tft.fillRoundRect(barX, y, fill, barH, 3, col);
    else if (fill > 0) tft.fillRect(barX, y, fill, barH, col);

    tft.setTextColor(col, COL_BG);
    tft.setCursor(valX, y);
    tft.printf("%.2f", bat.cell[i]);
  }

  // Footer chips: temperature, spread, error/state
  int fy = top + 5 * rowH + 6;
  char buf[16];
  snprintf(buf, sizeof(buf), "T %.0fC", bat.tempCell);
  drawChip(6, fy, 96, buf, COL_TEXT);
  snprintf(buf, sizeof(buf), "dV%.2f", bat.cellDiff);
  drawChip(108, fy, 96, buf, COL_TEXT);
  if (isF0513) {
    drawChip(210, fy, 104, "F0513", COL_YELLOW);
  } else {
    drawChip(210, fy, 104, bat.locked ? "LOCKED" : "OK", bat.locked ? COL_RED : COL_GREEN);
  }
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
void iconInfo(int cx, int cy, uint16_t c) {
  tft.drawCircle(cx, cy, 7, c);
  tft.fillRect(cx - 1, cy - 4, 2, 2, c);   // dot
  tft.fillRect(cx - 1, cy - 1, 2, 5, c);   // stem
}

void drawMenuIcon(int i, int cx, int cy, uint16_t c) {
  switch (i) {
    case 0: iconBattery(cx, cy, c); break;
    case 1: iconList(cx, cy, c);    break;
    case 2: iconRefresh(cx, cy, c); break;
    case 3: iconSun(cx, cy, c);     break;
    case 4: iconSunOff(cx, cy, c);  break;
    case 5: iconCode(cx, cy, c);    break;
    case 6: iconInfo(cx, cy, c);    break;
  }
}

void drawMenu() {
  drawHeader("MENU");
  const int top = HEADER_H;
  const int rowH = (tft.height() - top) / menuCount;
  tft.setTextSize(2);
  for (int i = 0; i < menuCount; i++) {
    bool sel = (i == menuIndex);
    uint16_t bg = sel ? COL_ACCENT : COL_BG;
    uint16_t fg = sel ? COL_HEAD : COL_TEXT;
    int y = top + i * rowH;
    // Full-width row fill: moves the highlight without clearing the screen.
    tft.fillRect(0, y, tft.width(), rowH, bg);
    // On the selected (accent) row, force a light icon so it stays visible.
    drawMenuIcon(i, 16, y + rowH / 2, sel ? COL_HEAD : menuIcons[i]);
    tft.setTextColor(fg, bg);
    tft.setCursor(32, y + (rowH - 16) / 2);
    tft.print(menuItems[i]);
  }
}

void drawDetails() {
  drawHeader("DETAILS");
  int y = HEADER_H + 8;
  tft.setTextSize(2);
  tft.setTextColor(COL_TEXT, COL_BG);

  if (!bat.valid) {
    tft.setCursor(6, y);
    tft.print("No data");
    return;
  }

  bool isF0513 = strcmp(bat.commandVersion, "F0513") == 0;
  const int lh = 24;
  if (isF0513) {
    tft.setCursor(6, y);          tft.print("Generation: F0513");
    tft.setTextColor(COL_MUTED, COL_BG);
    tft.setCursor(6, y + lh);     tft.print("(older, limited");
    tft.setCursor(6, y + 2 * lh); tft.print(" diagnostics only)");
  } else {
    tft.setCursor(6, y);          tft.printf("Charges: %d", bat.chargeCount);
    tft.setCursor(6, y + lh);     tft.printf("Mfg: %02d/%02d/%d", bat.mfgDay, bat.mfgMonth, bat.mfgYear);
    tft.setCursor(6, y + 2 * lh); tft.printf("Capacity: %.1f Ah", bat.capacityAh);
    tft.setCursor(6, y + 3 * lh); tft.printf("Type: %d", bat.batteryType);
    tft.setCursor(6, y + 4 * lh); tft.printf("ErrCode: 0x%02X", bat.errorCode);
  }
  tft.setTextColor(COL_MUTED, COL_BG);
  tft.setCursor(6, 220);
  tft.print("(click = back)");
}

void drawConfirmReset() {
  drawHeader("RESET ERROR?");
  int y = HEADER_H + 12;
  tft.setTextSize(2);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setCursor(6, y);          tft.print("Command sent:");
  tft.setTextColor(COL_MUTED, COL_BG);
  tft.setCursor(6, y + 24);     tft.print("TESTMODE + RESET");
  tft.setTextColor(COL_GREEN, COL_BG);
  tft.setCursor(6, y + 66);     tft.print("Click = confirm");
  tft.setTextColor(COL_RED, COL_BG);
  tft.setCursor(6, y + 92);     tft.print("Turn  = cancel");
}

// Visual feedback after a reset: error code before -> after, plus a verdict.
void drawResetResult() {
  drawHeader("RESET DONE");
  int y = HEADER_H + 10;
  tft.setTextSize(2);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setCursor(6, y);          tft.printf("Before: 0x%02X", resetErrBefore);
  tft.setCursor(6, y + 24);     tft.printf("After : 0x%02X", resetErrAfter);

  bool cleared = (resetErrBefore != 0 && resetErrAfter == 0);
  tft.setCursor(6, y + 60);
  if (cleared && !resetLockedAfter) {
    tft.setTextColor(COL_GREEN, COL_BG);
    tft.print("-> Error cleared!");
  } else if (resetErrAfter == resetErrBefore) {
    tft.setTextColor(COL_YELLOW, COL_BG);
    tft.print("-> Unchanged");
  } else {
    tft.setTextColor(COL_YELLOW, COL_BG);
    tft.printf("-> State: %s", resetLockedAfter ? "LOCKED" : "OK");
  }

  tft.setTextColor(COL_MUTED, COL_BG);
  tft.setCursor(6, 220);
  tft.print("(click = back)");
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
}

void drawCommError() {
  drawHeader("COMM ERROR");
  int y = HEADER_H + 12;
  tft.setTextSize(2);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setCursor(6, y);          tft.print("No response on bus.");
  tft.setTextColor(COL_MUTED, COL_BG);
  tft.setCursor(6, y + 30);     tft.print("Check DATA wiring,");
  tft.setCursor(6, y + 52);     tft.print("pull-ups and GND.");
  tft.setCursor(6, 220);        tft.print("(click = back)");
}

void drawAbout() {
  drawHeader("INFO");

  // Tool name, prominent: Pocket (teal) + OBI (orange)
  tft.setTextSize(3);                       // 18 px per char
  tft.setTextColor(COL_ACCENT, COL_BG);
  tft.setCursor(6, 36);   tft.print("Pocket");   // 6 * 18 = 108 px
  tft.setTextColor(COL_ORANGE, COL_BG);
  tft.setCursor(114, 36); tft.print("OBI");

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED, COL_BG);
  tft.setCursor(6, 66);   tft.printf("Firmware v%s  -  %s", FW_VERSION, __DATE__);
  tft.setCursor(6, 80);   tft.print("ESP32-C3 + ST7789  standalone OBI client");

  // Creator
  tft.setTextSize(2);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setCursor(6, 104);  tft.print("Created by");
  tft.setTextColor(COL_ORANGE, COL_BG);
  tft.setCursor(6, 128);  tft.print("The Repair Forge");

  // Credit to the base project
  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED, COL_BG);
  tft.setCursor(6, 158);  tft.print("Based on Open Battery Information");
  tft.setCursor(6, 172);  tft.print("by Martin Jansson (MIT)");
  tft.setCursor(6, 186);  tft.print("github.com/mnh-jansson");

  tft.setCursor(6, 220);
  tft.print("(click = back)");
}

// Boot splash shown while the first auto-read runs.
// Name in big two-tone letters: "Pocket" (teal) + "OBI" (orange).
void drawSplash() {
  tft.fillScreen(COL_BG);

  tft.setTextSize(4);                    // 24 px per char
  tft.setTextColor(COL_ACCENT, COL_BG);
  tft.setCursor(52, 74);
  tft.print("Pocket");                   // 6 chars * 24 = 144 px
  tft.setTextColor(COL_ORANGE, COL_BG);
  tft.setCursor(196, 74);                // right after "Pocket"
  tft.print("OBI");

  tft.setTextSize(2);
  tft.setTextColor(COL_MUTED, COL_BG);
  tft.setCursor(34, 128);
  tft.print("standalone OBI client");

  tft.setTextSize(1);
  tft.setCursor(73, 180);
  tft.printf("v%s  -  reading battery...", FW_VERSION);
}

void render() {
  // Only clear the screen when the screen changes -> no flicker on each detent.
  if ((int)state != lastRenderedState) {
    tft.fillScreen(COL_BG);
    lastRenderedState = (int)state;
  }
  switch (state) {
    case HOME:          drawHome();         break;
    case MENU:          drawMenu();         break;
    case DETAILS:       drawDetails();      break;
    case CONFIRM_RESET: drawConfirmReset(); break;
    case RESET_RESULT:  drawResetResult();  break;
    case DEBUG_RAW:     drawDebugRaw();      break;
    case COMM_ERROR:    drawCommError();     break;
    case ABOUT:         drawAbout();         break;
  }
}

// ---------- Buttons / navigation logic ----------
void handleClick() {
  switch (state) {
    case HOME:
      state = MENU;
      break;
    case MENU:
      switch (menuIndex) {
        case 0: // Read battery info
          state = readAllData() ? HOME : COMM_ERROR;
          break;
        case 1: // Details
          state = DETAILS;
          break;
        case 2: // Reset error
          state = CONFIRM_RESET;
          break;
        case 3: // Pack LEDs on
          ledsOn();
          break;
        case 4: // Pack LEDs off
          ledsOff();
          break;
        case 5: // Debug
          state = DEBUG_RAW;
          break;
        case 6: // Version / info
          state = ABOUT;
          break;
      }
      break;
    case DETAILS:
    case DEBUG_RAW:
    case COMM_ERROR:
    case RESET_RESULT:
    case ABOUT:
      state = MENU;
      break;
    case CONFIRM_RESET:
      resetErrBefore = bat.errorCode;      // remember the state before
      resetErrors();
      readAllData();                       // refresh the state after reset
      resetErrAfter = bat.errorCode;
      resetLockedAfter = bat.locked;
      state = RESET_RESULT;                // before -> after verdict screen
      break;
  }
  render();
}

void handleRotate(int dir) {
  if (state == HOME) {
    // The home screen says "Turn = menu": honor that gesture.
    state = MENU;
    render();
  } else if (state == MENU) {
    menuIndex = (menuIndex + dir + menuCount) % menuCount;
    render();
  } else if (state == CONFIRM_RESET) {
    // Turn = cancel the reset, back to the menu.
    state = MENU;
    render();
  }
}

// Back button, short press: go one screen back.
void handleBack() {
  switch (state) {
    case HOME:
      return;              // already at the top, nothing to do
    case MENU:
      state = HOME;
      break;
    default:              // any sub-screen (details, reset, debug, about...) -> menu
      state = MENU;
      break;
  }
  render();
}

// Back button, long press: jump straight to the home screen.
void goHome() {
  if (state != HOME) {
    state = HOME;
    render();
  }
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

  tft.init(240, 320);        // 2.4" resolution
  tft.invertDisplay(false);  // correct colors for this ST7789 panel
  tft.setRotation(1);

  // Auto-read at boot so the home screen reflects reality instead of claiming
  // "no battery" before ever checking. The splash covers the read latency.
  drawSplash();
  readAllData();
  state = HOME;
  render();
}

void loop() {
  // Encoder polling: to be called as often as possible.
  encoder->tick();

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
