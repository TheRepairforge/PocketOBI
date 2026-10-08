// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
#pragma once
#include "pocketobi.h"

bool readStaticInfo();
bool readF0513Cells();
bool readLiveData();
bool readAllData();
bool tempImplausible(float t);
bool thermistorFault();
bool thermistorSuspect();
HwFault findHardwareFault(int *group, char *action, size_t n);
Verdict computeVerdict();
void dumpDecoded();
const char* verdictText(Verdict v);
uint16_t verdictColor(Verdict v);
void formatSerial(char *out);
uint8_t round5up(uint32_t num, uint32_t den);
void readExtended();
