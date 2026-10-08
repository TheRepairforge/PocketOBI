// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// PocketOBI — family profile, frame decode, verdict logic (split from PocketOBI.ino, DECISIONS.md D21).
#include "pocketobi.h"
#include "protocol.h"
#include "unlock.h"
#include "decode.h"
#include "display.h"
#include "ui_nav.h"
#include "bridge.h"

uint8_t cellCount = 5;   // active family cell count (LXT)
uint8_t gammeId = GAMME_LXT;   // active family (XGT seam)
const BmsAddrMap LXT_ADDR = {
  /*asmDate */ 0x000,
  /*soc     */ 0x150,
  /*odCount */ 0x0BA,
  /*olBlock */ 0x08D,
  /*faultMkA*/ 0x58D,
  /*faultMkB*/ 0x309,
};
const BmsAddrMap *bmsAddr = &LXT_ADDR;   // active family address map (LXT)
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
  // Soft / empirical signals -> "possible" HINT, never a firm fault. The D6 "latched"
  // marker used to feed a SUSPECT here; it was retired (D24) as non-discriminating -
  // healthy BL1850B packs carry the same 0x0B/0x4x constant as the one genuinely-locked
  // pack it was ever "confirmed" on, so it flagged every BL1850B. A genuinely latched pack
  // is still caught by its charger lock below (nibble=3 -> V_REPAIRABLE). The sensor-spread
  // remains the empirical hint, surfaced as an orange V_SUSPECT rather than a red verdict.
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

  // Latched-fault detection DISABLED (D24). The D6 markers (0x58D/0x309) are NOT a per-pack
  // fault record: two healthy BL1850B units (2022) read an identical 0x0B/0x49, and the one
  // pack this marker was ever "confirmed" on (0x0B/0x48, 2021) reads the same constant - so
  // 0x0B/0x4x is the model's resting value, not a latch. That pack was in fact identifiable
  // by its charger lock, not by this marker. No validated true positive exists, so we do not
  // derive a verdict from it. Raw a/b stay in faultMkA/faultMkB for the Debug screen.
  bat.latchedFault = false;
  // Secondary (unproven) wear percentages, denominator = charge count.
  bat.odWearPct = round5up(bat.odEventCount, bat.chargeCount);
  bat.olWearPct = round5up(bat.olEventCount, bat.chargeCount);
  bat.extValid = true;

#if COMM_DEBUG
  dumpDecoded();   // bench-validation readout (COMM_DEBUG only)
#endif
}
