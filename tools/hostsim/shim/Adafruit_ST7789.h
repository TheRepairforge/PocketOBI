// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// hostsim shim: the ST7789 display replaced by Adafruit_GFX's own RAM canvas. Every
// drawing call the firmware makes runs the real Adafruit_GFX code into a 16-bit
// framebuffer that the simulator saves as an image.
#pragma once
#include <Adafruit_GFX.h>
#include <SPI.h>
#include "../../../board_config.h"   // PANEL_W / PANEL_H of a board with a bigger panel (BOARD_S3_ST7796U)

#define ST77XX_MADCTL 0x36

class Adafruit_ST7789 : public GFXcanvas16 {
public:
#ifdef PANEL_W
  Adafruit_ST7789(SPIClass*, int8_t, int8_t, int8_t) : GFXcanvas16(PANEL_H, PANEL_W) {}   // native portrait
#else
  Adafruit_ST7789(SPIClass*, int8_t, int8_t, int8_t) : GFXcanvas16(240, 320) {}
#endif
  void init(uint16_t, uint16_t, uint8_t = 0) {}
  void invertDisplay(bool) {}
  void sendCommand(uint8_t, const uint8_t* = nullptr, uint8_t = 0) {}
};
