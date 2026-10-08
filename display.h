// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
#pragma once
#include "pocketobi.h"

uint16_t cellColor(float v, float minV, float diff);
void drawHeader(const char* title);
void iconBattery(int cx, int cy, uint16_t c);
void iconList(int cx, int cy, uint16_t c);
void iconRefresh(int cx, int cy, uint16_t c);
void iconSun(int cx, int cy, uint16_t c);
void iconSunOff(int cx, int cy, uint16_t c);
void iconCode(int cx, int cy, uint16_t c);
void iconKey(int cx, int cy, uint16_t c);
void iconInfo(int cx, int cy, uint16_t c);
void iconBridge(int cx, int cy, uint16_t c);
void drawConfirmReset();
void drawResetResult();
void lockCausesText(uint8_t causes, char *out, size_t n);
void drawConfirmUnlock();
void drawUnlockResult();
void drawDebugRaw();
void drawCommError();
void drawWrapCentered(const char* s, int y, uint16_t col, uint8_t size);
void drawGuruCrash();
void drawAbout();
void drawPcBridge();
void drawSplash();
void toast(const char* msg, uint16_t col);
int gfxText(const GFXfont* f, int x, int baseY, const char* s, uint16_t col);
void gfxCenter(const GFXfont* f, int cx, int baseY, const char* s, uint16_t col);
int gfxWidth(const GFXfont* f, const char* s);
int rowLabel(int x, int topY, const char* s);
void kvRow(const char* k, const char* val, uint16_t col);
void degC(uint16_t col);
void drawPageDots(int active, int count);
void drawVerdictBanner(Verdict v);
void drawTile(int x, int y, int w, int h, bool sel);
void drawLauncher();
void drawBatteryHealth();
void fmtPackDate(char *out, size_t n, uint16_t year, uint8_t m, uint8_t d);
void drawBatteryIdentity();
void drawBatteryOverview();
void drawBatteryPage();
void drawDiagRowSt(const char* k, const char* val, int st);
void drawDiagRow(const char* k, const char* val, bool ok);
void drawPrognosisBanner(uint16_t bc, const char* bt, bool hint);
void drawWizardDiag();
void drawTools();
void drawSettings();
void render();
