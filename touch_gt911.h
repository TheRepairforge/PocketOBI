// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// Contributed by yukonx (PocketOBI issue #43), included under PolyForm Noncommercial 1.0.0 with his agreement.
#pragma once
// GT911 capacitive touch driver. Header-only, no external library. It offers the same calls
// the sketch makes on the XPT2046 / FT6336 (touched(), getPoint(), lastEvent, setRotation()),
// so the navigation code does not know which controller is fitted.
//
// Like the FT6336, the GT911 reports PIXELS in the panel's native portrait frame, so the
// screen mapping (landscape, axes transposed) is done in mapTouch() (ui_nav.cpp) through the
// per-board TCAL_DEF_* defaults in board_config.h.
//
// Differences from the FT6336 the driver has to handle:
//  - 16-bit register addresses (high byte first).
//  - The I2C address (0x5D or 0x14) is chosen by the level of INT while RST rises.
//  - After every read the "buffer ready" flag (reg 0x814E) must be written back to 0, and the
//    controller only sets it again on its next scan (~100 Hz). touched() therefore keeps the
//    last state for HOLD_MS so a tight "while (touch.touched())" loop does not see a false
//    release between two scans.
#include <Arduino.h>
#include <Wire.h>
#include "board_config.h"

#if BOARD_TOUCH_DRIVER != TOUCH_GT911
#error "touch_gt911.h is only included for BOARD_TOUCH_DRIVER == TOUCH_GT911"
#endif

// Same fields as XPT2046_Touchscreen's TS_Point (the sketch reads .x, .y and .z only).
class TS_Point {
public:
  TS_Point() : x(0), y(0), z(0) {}
  TS_Point(int16_t px, int16_t py, int16_t pz) : x(px), y(py), z(pz) {}
  int16_t x, y, z;
};

class GT911_Touch {
public:
  enum : uint16_t {
    REG_PRODUCT_ID = 0x8140,   // 4 ASCII bytes: "911\0"
    REG_X_MAX      = 0x8048,   // configured X / Y range, 2 bytes each (little endian)
    REG_STATUS     = 0x814E,   // bit 7 = buffer ready, bits 3:0 = number of touch points
    REG_POINT1     = 0x814F,   // point 1: track id, XL, XH, YL, YH, sizeL, sizeH, reserved (0x814E is the status)
  };
  // No pressure channel: this stand-in sits above TOUCH_Z_MIN (pocketobi.h, 300).
  enum : int16_t { PRESSED_Z = 1000 };

  // Starts the I2C bus, runs the reset sequence that selects address 0x5D, and checks that the
  // controller answers. Returns false when nothing answers (the caller reports it; the sketch
  // keeps running). RST may be shared with the display's reset: call this BEFORE tft.init().
  bool begin() {
    Wire.begin(TOUCH_SDA, TOUCH_SCL, 400000);
    pinMode(TOUCH_INT, OUTPUT);
    pinMode(TOUCH_RST, OUTPUT);
    digitalWrite(TOUCH_INT, LOW);    // INT low while RST rises -> address 0x5D
    digitalWrite(TOUCH_RST, LOW);
    delay(10);
    digitalWrite(TOUCH_RST, HIGH);
    delay(6);
    pinMode(TOUCH_INT, INPUT);       // release INT: the controller drives it from now on
    delay(60);                       // configuration load after reset

    addr_ = 0;
    static const uint8_t cand[2] = { 0x5D, 0x14 };
    for (uint8_t i = 0; i < 2; i++) {
      Wire.beginTransmission(cand[i]);
      if (Wire.endTransmission() == 0) { addr_ = cand[i]; break; }
    }
    if (!addr_) {
      Serial.println("GT911: no answer at 0x5D / 0x14; I2C devices found:");
      bool any = false;
      for (uint8_t a = 1; a < 127; a++) {
        Wire.beginTransmission(a);
        if (Wire.endTransmission() == 0) { Serial.printf("  0x%02X\n", a); any = true; }
      }
      if (!any) Serial.println("  none (check SDA/SCL wiring, 3V3 and pull-ups)");
      return false;
    }
    uint8_t id[4] = { 0, 0, 0, 0 }, res[4] = { 0, 0, 0, 0 };
    const bool okId  = readRegs(REG_PRODUCT_ID, id, 4);
    const bool okRes = readRegs(REG_X_MAX, res, 4);
    if (okId && okRes) {
#if defined(GT911_DEBUG) && GT911_DEBUG
      Serial.printf("GT911 @0x%02X id=%c%c%c  configured range x 0..%u, y 0..%u\n", addr_,
                    id[0] ? id[0] : '?', id[1] ? id[1] : '?', id[2] ? id[2] : '?',
                    (unsigned)(res[0] | (res[1] << 8)), (unsigned)(res[2] | (res[3] << 8)));
#endif
    } else {
      Serial.printf("GT911 @0x%02X answers but register reads fail (bus problem)\n", addr_);
    }
    writeReg(REG_STATUS, 0);         // drop any stale "buffer ready" flag
    return true;
  }

  // True while at least one finger is down.
  bool touched() {
    refresh();
    return down_;
  }

  // The last captured point, in panel pixels (portrait frame). Call it after touched() returned
  // true; it does not read the bus again, so a quick tap is not lost if the finger lifts between
  // the two calls. z = 0 when no sample has been captured yet.
  TS_Point getPoint() {
#if defined(TOUCH_SWAP_XY) && TOUCH_SWAP_XY
    return TS_Point(y_, x_, haveSample_ ? PRESSED_Z : 0);   // the controller's X/Y are the other way round
#else
    return TS_Point(x_, y_, haveSample_ ? PRESSED_Z : 0);
#endif
  }

  // Kept for the touch test screen: 2 = contact, 3 = no event (FT6336-style values).
  uint8_t lastEvent = 3;

  // No-op, kept for interface compatibility with XPT2046_Touchscreen.
  void setRotation(uint8_t) {}

private:
  enum : uint32_t { HOLD_MS = 50 };

  uint8_t  addr_ = 0;
  bool     down_ = false;
  bool     haveSample_ = false;
  int16_t  x_ = 0, y_ = 0;
  uint32_t seenMs_ = 0;

  void refresh() {
    if (!addr_) return;
    uint8_t st = 0;
    if (!readRegs(REG_STATUS, &st, 1)) return;           // bus error: keep the previous state
    if (st & 0x80) {
      const uint8_t n = st & 0x0F;
      if (n >= 1 && n <= 5) {
        uint8_t r[8];
        if (readRegs(REG_POINT1, r, 8)) {
          x_ = (int16_t)(r[1] | (r[2] << 8));
          y_ = (int16_t)(r[3] | (r[4] << 8));
#if defined(GT911_DEBUG) && GT911_DEBUG
          Serial.printf("gt911 n=%u raw x=%d y=%d\n", n, (int)(r[1] | (r[2] << 8)), (int)(r[3] | (r[4] << 8)));
#endif
          down_ = true;
          haveSample_ = true;
          seenMs_ = millis();
          lastEvent = 2;
        }
      } else {
        down_ = false;
        lastEvent = 3;
      }
      writeReg(REG_STATUS, 0);                            // ask for the next scan
    } else if (down_ && millis() - seenMs_ > HOLD_MS) {
      down_ = false;                                      // no report for a while: finger lifted
      lastEvent = 3;
    }
  }

  bool readRegs(uint16_t reg, uint8_t *buf, uint8_t len) {
    Wire.beginTransmission(addr_);
    Wire.write((uint8_t)(reg >> 8));
    Wire.write((uint8_t)(reg & 0xFF));
    if (Wire.endTransmission(false) != 0) return false;   // repeated start, no stop
    if (Wire.requestFrom((uint8_t)addr_, len) != len) return false;
    for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
    return true;
  }

  bool writeReg(uint16_t reg, uint8_t v) {
    Wire.beginTransmission(addr_);
    Wire.write((uint8_t)(reg >> 8));
    Wire.write((uint8_t)(reg & 0xFF));
    Wire.write(v);
    return Wire.endTransmission() == 0;
  }
};
