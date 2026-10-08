// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// hostsim shim: rotary encoder (never turns).
#pragma once
#include "Arduino.h"

class RotaryEncoder {
public:
  enum class LatchMode { FOUR3 = 1, FOUR0 = 2, TWO03 = 3 };
  RotaryEncoder(int, int, LatchMode = LatchMode::FOUR0) {}
  void tick() {}
  long getPosition() { return 0; }
  void setPosition(long) {}
};
