// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// hostsim shim: I2C bus with a fake GT911 on it (touch_gt911.h). The status register alternates
// "point ready" / "nothing" on each read, with a firm point in the middle of the panel, so blocking
// touch flows (the calibration) run to the end; hostsimTouchHook, when set, is called on every
// point read (the driver grabs a frame), like XPT2046_Touchscreen's getPoint() does.
#pragma once
#include <stdint.h>
#include <stddef.h>

extern void (*hostsimTouchHook)();

class TwoWire {
public:
  bool begin(int = -1, int = -1, uint32_t = 100000) { return true; }
  void setClock(uint32_t) {}
  void beginTransmission(uint8_t) { n_ = 0; }
  size_t write(uint8_t b) { if (n_ < 2) reg_ = (uint16_t)((reg_ << 8) | b); n_++; return 1; }
  uint8_t endTransmission(bool = true) { return 0; }
  uint8_t requestFrom(uint8_t, uint8_t len) {
    len_ = len; pos_ = 0;
    if (reg_ == 0x814F && hostsimTouchHook) hostsimTouchHook();
    if (reg_ == 0x814E) ready_ = !ready_;
    return len;
  }
  int read() {
    uint8_t i = pos_++;
    if (reg_ == 0x814E) return ready_ ? 0x81 : 0x00;                 // ready, one point
    if (reg_ == 0x814F) { static const uint8_t p[8] = { 0, 0xC8, 0, 0xC8, 0, 30, 0, 0 }; return p[i & 7]; }
    if (reg_ == 0x8140) { static const uint8_t id[4] = { '9', '1', '1', 0 }; return id[i & 3]; }
    return 0;
  }
private:
  uint16_t reg_ = 0; uint8_t n_ = 0, len_ = 0, pos_ = 0; bool ready_ = false;
};
extern TwoWire Wire;
