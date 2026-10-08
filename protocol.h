// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
#pragma once
#include "pocketobi.h"

void mkWrite(uint8_t b);
uint8_t mkRead();
uint8_t nibbleSwap(uint8_t b);
uint16_t le16(const uint8_t *buf, int idx);
bool sendCommand(const uint8_t *c, uint8_t *outPayload, uint8_t *romIdOut = nullptr);
bool isPrintableAscii(const uint8_t *b, int n);
bool sendCommandRetry(const uint8_t *c, uint8_t *outPayload, uint8_t *romIdOut, uint8_t tries);
bool readF0513Raw(uint8_t cmdByte, uint8_t *byte1, uint8_t *byte2);
void busPowerCycle();
void resetErrors();
void ledsSet(bool on);
void ledsOn();
void ledsOff();
uint8_t d6ReadByte(uint16_t addr);
uint8_t d4ReadByte(uint16_t addr);
void d4ReadBlock(uint16_t addr, uint8_t *buf, uint8_t n);
