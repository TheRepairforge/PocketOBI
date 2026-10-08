// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// hostsim shim: NVS preferences. Nothing is stored; every read returns its default.
#pragma once
#include "Arduino.h"

class Preferences {
public:
  bool begin(const char*, bool = false) { return true; }
  void end() {}
  bool clear() { return true; }
  bool getBool(const char*, bool d = false) { return d; }
  int32_t getInt(const char*, int32_t d = 0) { return d; }
  size_t putBool(const char*, bool) { return 1; }
  size_t putInt(const char*, int32_t) { return 4; }
};
