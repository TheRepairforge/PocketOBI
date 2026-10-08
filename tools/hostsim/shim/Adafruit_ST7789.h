// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// hostsim shim: the ST7789 display replaced by Adafruit_GFX's own RAM canvas. Every
// drawing call the firmware makes runs the real Adafruit_GFX code into a 16-bit
// framebuffer that the simulator saves as an image.
#pragma once
#include <Adafruit_GFX.h>
#include <SPI.h>

class Adafruit_ST7789 : public GFXcanvas16 {
public:
  Adafruit_ST7789(SPIClass*, int8_t, int8_t, int8_t) : GFXcanvas16(240, 320) {}
  void init(uint16_t, uint16_t, uint8_t = 0) {}
  void invertDisplay(bool) {}
};
