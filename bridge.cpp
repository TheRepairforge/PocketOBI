// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// PocketOBI — PC bridge (ArduinoOBI-compatible USB<->OneWire) (split from PocketOBI.ino, DECISIONS.md D21).
#include "pocketobi.h"
#include "protocol.h"
#include "unlock.h"
#include "decode.h"
#include "display.h"
#include "ui_nav.h"
#include "bridge.h"

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
