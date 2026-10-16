// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
#pragma once
#include "pocketobi.h"
#include "ui_components.h"   // drawing vocabulary, so every module that includes display.h keeps its helpers

// Screens (composition layer). The drawing vocabulary is ui_components.h; sizes are the generated
// tokens (ui_theme.h).
uint16_t cellColor(float v, float minV, float diff);
void applyRotation();
#if PANEL_ILI9341_INIT
void ili9341PanelInit();   // ILI9341 power / VCOM / gamma after tft.init() (board_config.h)
#endif
void lockCausesText(uint8_t causes, char *out, size_t n);
void fmtPackDate(char *out, size_t n, uint16_t year, uint8_t m, uint8_t d);
void drawLauncher();
void drawBatteryPage();
void drawBatteryOverview();
void drawBatteryHealth();
void drawBatteryIdentity();
void drawWizardDiag();
void drawConfirmUnlock();
void drawUnlockWorking();
void drawUnlockProgress(int step, int total);   // UnlockProgress hook (unlock.h)
void drawUnlockResult();
void drawConfirmReset();
void drawResetResult();
void drawTools();
void drawSettings();
void drawDebugRaw();
void drawPcBridge();
void drawAbout();
void drawGuruCrash();
void drawWrapCentered(const char* s, int y, uint16_t col, uint8_t size);
void drawCommError();
void drawSplash();
void render();
// Cursor moved on a list screen (Launcher / Tools / Settings): repaint the two items involved.
void redrawSelection(int prev, int cur);
// Touch hit-testing for the cursor screens, from the same geometry that draws them.
int touchHit(UiState s, int sx, int sy);
#define TOUCH_NEXT_PAGE (-2)   // touchHit(TOOLS): tap under the rows = the other page
int toolAt(int pos);    // Tools: position on screen -> tool id (activate() order)
