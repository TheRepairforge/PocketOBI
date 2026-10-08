// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
#pragma once
#include "pocketobi.h"

void activate(UiState s, int idx);
void handleClick();
void handleRotate(int dir);
void handleBack();
void goHome();
void mapTouch(int rawX, int rawY, int *sx, int *sy);
void touchFeedback(int x, int y);
void calibrateTouch();
int touchHitIndex(UiState s, int sx, int sy);
void serviceTouch();
