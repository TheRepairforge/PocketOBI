// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// hostsim shim: SPI bus (no-op).
#pragma once
#include "Arduino.h"

class SPIClass {
public:
  SPIClass(int = 0) {}
  void begin(int8_t = -1, int8_t = -1, int8_t = -1, int8_t = -1) {}
};
extern SPIClass SPI;
