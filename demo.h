// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
#pragma once
// Screen demo (review build only, UI_DEMO=1: envs esp32-c3-demo / cyd-demo). Shows every screen of the real
// display code with fictional packs (healthy, locked, fault, suspect, F0513, no pack), so a UI review on the
// panel does not need a pack in each state. Never reads or writes a pack, never starts the bridge.
#ifndef UI_DEMO
#define UI_DEMO 0
#endif

#include "ui_nav.h"

#if UI_DEMO
void demoBegin();
void demoRotate(int dir);   // encoder turn / tap left-right: previous / next screen
void demoClick();           // encoder click / tap the title bar: next pack
void demoBack();            // Back short: previous pack
void demoNextLang();        // Back long / tap the footer: next language
#if BOARD_HAS_TOUCH
void demoServiceTouch();
#endif
#endif

// Input -> action, chosen at compile time: the demo steps through packs and screens, every other build
// calls exactly the normal navigation (PocketOBI.ino calls these, never the two sets directly).
#if UI_DEMO
inline void navRotate(int d) { demoRotate(d); }
inline void navClick()       { demoClick(); }
inline void navBack()        { demoBack(); }
inline void navHome()        { demoNextLang(); }
#if BOARD_HAS_TOUCH
inline void navTouch()       { demoServiceTouch(); }
#endif
#else
inline void navRotate(int d) { handleRotate(d); }
inline void navClick()       { handleClick(); }
inline void navBack()        { handleBack(); }
inline void navHome()        { goHome(); }
#if BOARD_HAS_TOUCH
inline void navTouch()       { serviceTouch(); }
#endif
#endif
