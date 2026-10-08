// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// hostsim shim: resistive touch controller. It alternates "touched" / "released" on each
// poll, with a firm press, so blocking touch flows (the calibration) run to the end;
// hostsimTouchHook, when set, is called on every getPoint() (the driver grabs a frame).
#pragma once
#include "SPI.h"

class TS_Point {
public:
  TS_Point() : x(0), y(0), z(0) {}
  int16_t x, y, z;
};

extern void (*hostsimTouchHook)();

class XPT2046_Touchscreen {
public:
  XPT2046_Touchscreen(uint8_t, uint8_t = 255) {}
  bool begin(SPIClass&) { return true; }
  bool begin() { return true; }
  void setRotation(uint8_t) {}
  bool touched() { down_ = !down_; return down_; }
  TS_Point getPoint() {
    if (hostsimTouchHook) hostsimTouchHook();
    TS_Point p; p.x = 2000; p.y = 2000; p.z = 1000; return p;
  }
private:
  bool down_ = false;
};
