// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// PocketOBI — clean-room frame repair / charger-unlock (split from PocketOBI.ino, DECISIONS.md D21).
#include "pocketobi.h"
#include "protocol.h"
#include "unlock.h"
#include "decode.h"
#include "display.h"
#include "ui_nav.h"
#include "bridge.h"

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
