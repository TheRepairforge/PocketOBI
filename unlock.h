// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
#pragma once
#include "pocketobi.h"

uint8_t nybGet(const uint8_t *d, uint8_t n);
void nybSet(uint8_t *d, uint8_t n, uint8_t v);
uint8_t csCalc(const uint8_t *d, uint8_t s, uint8_t e);
uint8_t lockCauses(const uint8_t *frame);
void buildRepairedFrame(const uint8_t *in, uint8_t *out);
void ow33(const uint8_t *data, uint8_t dataLen, uint8_t *rsp, uint8_t rspLen);
void writeFrame(const uint8_t *frame);
uint8_t unlockRepair();
