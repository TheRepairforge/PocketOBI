#pragma once
// PROBE_PULSE (test build only, never in a release env): serial-driven step-by-step probe.
// Sends one group of pack commands per key typed in the serial monitor, leaves ENABLE low
// and logs a timestamp, so a scope on the pack output shows which group leaves the pack
// in a given state, and which remedy clears it.
#ifndef PROBE_PULSE
#define PROBE_PULSE 0
#endif

#if PROBE_PULSE
void probeBegin();
void probeService();
#endif
