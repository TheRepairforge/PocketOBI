// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// PocketOBI — OneWire low level, command frames, D4/D6 register reads (split from PocketOBI.ino, DECISIONS.md D21).
#include "pocketobi.h"
#include "protocol.h"
#include "unlock.h"
#include "decode.h"
#include "display.h"
#include "ui_nav.h"
#include "bridge.h"

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
bool sendCommand(const uint8_t *c, uint8_t *outPayload, uint8_t *romIdOut) {  // default arg in protocol.h
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
