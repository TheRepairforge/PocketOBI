// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
#pragma once
// FT6336G capacitive touch driver for the ES3C28P (DECISIONS.md D48). Header-only, no external
// library. It offers the same two calls the sketch makes on the XPT2046 (touched(), getPoint()),
// so the navigation code does not know which controller is fitted.
//
// Unlike the XPT2046 (raw 0..4095 per axis), the FT6336 reports PIXELS in the panel's native
// portrait frame: x 0..239, y 0..319. The screen mapping (landscape, axes transposed) is done once,
// in mapTouch() (ui_nav.cpp), through the per-board TCAL_DEF_* defaults in board_config.h.
#include <Arduino.h>
#include <Wire.h>
#include "board_config.h"

#if BOARD_TOUCH_DRIVER != TOUCH_FT6336
#error "touch_ft6336.h is only included for BOARD_TOUCH_DRIVER == TOUCH_FT6336"
#endif

// Same fields as XPT2046_Touchscreen's TS_Point (the sketch reads .x, .y and .z only). Defined
// here because the FT6336 build does not include the XPT2046 library.
class TS_Point {
public:
  TS_Point() : x(0), y(0), z(0) {}
  TS_Point(int16_t px, int16_t py, int16_t pz) : x(px), y(py), z(pz) {}
  int16_t x, y, z;
};

class FT6336_Touch {
public:
  enum : uint8_t {
    ADDR          = 0x38,   // FT6336G fixed I2C address
    REG_TD_STATUS = 0x02,   // low nibble = number of touch points (0..2)
    REG_P1_XH     = 0x03,   // first touch point: XH, XL, YH, YL (4 bytes)
  };
  // The FT6336 has no pressure channel. This stand-in sits above TOUCH_Z_MIN (pocketobi.h, 300)
  // so the pressure filter lets every real touch through.
  enum : int16_t { PRESSED_Z = 1000 };

  // Starts the I2C bus, pulses the reset pin, and checks that the controller answers at 0x38.
  // Returns false when nothing answers (the caller reports it; the sketch keeps running).
  bool begin() {
    Wire.begin(TOUCH_SDA, TOUCH_SCL, 400000);
    pinMode(TOUCH_RST, OUTPUT);
    digitalWrite(TOUCH_RST, LOW);
    delay(10);
    digitalWrite(TOUCH_RST, HIGH);
    delay(300);                      // the controller needs ~300 ms after reset before it answers
    Wire.beginTransmission(ADDR);
    return Wire.endTransmission() == 0;
  }

  // True while one or two fingers are down.
  bool touched() {
    uint8_t st = 0;
    if (!readRegs(REG_TD_STATUS, &st, 1)) return false;
    const uint8_t n = st & 0x0F;
    return n >= 1 && n <= 2;
  }

  // First touch point, in panel pixels (portrait frame). Returns z = 0 when the point is not
  // trusted, so the pressure filter drops it instead of making a tap at the wrong place; the
  // caller does not latch on z = 0 and simply asks again on its next poll.
  // The first report after a press-down can still carry the previous touch's coordinates on
  // these controllers, so a lone press-down sample is never used. Up to SAMPLES reads SETTLE_MS
  // apart; the point is trusted as soon as two consecutive finger-down samples agree within
  // SETTLE_PX, or when a contact sample (event 2, i.e. after the press-down) is the last one
  // before the finger lifts: a quick tap (test2 rejected those, "not very sensitive").
  TS_Point getPoint() {
    int16_t px = 0, py = 0, x, y;
    bool have = false;
    uint8_t prevEv = 3;
    for (uint8_t n = 0; n < SAMPLES; n++) {
      if (n) delay(SETTLE_MS);
      if (!sample(&x, &y)) {
        if (have && prevEv == 2) { lastEvent = prevEv; return TS_Point(px, py, PRESSED_Z); }
        return TS_Point(px, py, 0);
      }
      if (have && abs(x - px) <= SETTLE_PX && abs(y - py) <= SETTLE_PX) return TS_Point(x, y, PRESSED_Z);
      px = x; py = y; prevEv = lastEvent; have = true;
    }
    return TS_Point(px, py, 0);
  }

  // Event flag of the last sample (P1_XH bits 7:6): 0 press-down, 1 lift-up, 2 contact,
  // 3 no event. Kept for the touch test screen, which prints it.
  uint8_t lastEvent = 3;

  // No-op, kept for interface compatibility with XPT2046_Touchscreen. The orientation is applied
  // on the screen side (mapTouch), not in the controller.
  void setRotation(uint8_t) {}

private:
  enum : int16_t { SAMPLES = 4, SETTLE_MS = 8, SETTLE_PX = 8, PANEL_W = 240, PANEL_H = 320 };

  // One read of the first touch point. False on a bus error, a lift-up / no-event flag, or
  // coordinates outside the panel.
  bool sample(int16_t *x, int16_t *y) {
    uint8_t r[4] = { 0, 0, 0, 0 };   // XH, XL, YH, YL
    if (!readRegs(REG_P1_XH, r, 4)) return false;
    lastEvent = r[0] >> 6;
    if (lastEvent == 1 || lastEvent == 3) return false;
    *x = (int16_t)(((r[0] & 0x0F) << 8) | r[1]);
    *y = (int16_t)(((r[2] & 0x0F) << 8) | r[3]);
    return *x < PANEL_W && *y < PANEL_H;
  }

  // Reads `len` consecutive registers starting at `reg`. Returns false on any bus error.
  bool readRegs(uint8_t reg, uint8_t *buf, uint8_t len) {
    Wire.beginTransmission(ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;   // repeated start, no stop
    if (Wire.requestFrom((uint8_t)ADDR, len) != len) return false;
    for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
    return true;
  }
};
