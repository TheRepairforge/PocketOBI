// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// hostsim shim: definitions behind Arduino.h / SPI.h, plus a OneWire that never answers
// (OneWire2.cpp itself is not compiled: it drives ESP32 GPIO registers directly).
// Guarded whole: PlatformIO builds every .cpp under src_dir = . (D14), this one included.
#ifdef POCKETOBI_HOSTSIM
#include "Arduino.h"
#include "SPI.h"
#include "../../../OneWire2.h"

HardwareSerial Serial;
SPIClass SPI;
void (*hostsimTouchHook)() = nullptr;

static unsigned long g_us = 0;          // simulated clock: advances only when code waits
unsigned long millis() { return g_us / 1000; }
unsigned long micros() { return g_us; }
void delay(unsigned long ms) { g_us += ms * 1000UL; }
void delayMicroseconds(unsigned int us) { g_us += us; }
void pinMode(uint8_t, uint8_t) {}
void digitalWrite(uint8_t, uint8_t) {}
int  digitalRead(uint8_t) { return HIGH; }   // buttons idle (active-low)
long random(long max) { return max > 0 ? rand() % max : 0; }
long random(long lo, long hi) { return hi > lo ? lo + rand() % (hi - lo) : lo; }

// No pack on the bus: reset() reports no presence, reads float high.
void OneWire::begin(uint8_t) {}
uint8_t OneWire::reset(void) { return 0; }
void OneWire::select(const uint8_t*) {}
void OneWire::skip(void) {}
void OneWire::write(uint8_t, uint8_t) {}
void OneWire::write_bytes(const uint8_t*, uint16_t, bool) {}
uint8_t OneWire::read(void) { return 0xFF; }
void OneWire::read_bytes(uint8_t* buf, uint16_t n) { memset(buf, 0xFF, n); }
void OneWire::write_bit(uint8_t) {}
uint8_t OneWire::read_bit(void) { return 1; }
void OneWire::depower(void) {}
#if ONEWIRE_SEARCH
void OneWire::reset_search() {}
void OneWire::target_search(uint8_t) {}
bool OneWire::search(uint8_t*, bool) { return false; }
#endif
#if ONEWIRE_CRC
uint8_t OneWire::crc8(const uint8_t*, uint8_t) { return 0; }
#if ONEWIRE_CRC16
bool OneWire::check_crc16(const uint8_t*, uint16_t, const uint8_t*, uint16_t) { return true; }
uint16_t OneWire::crc16(const uint8_t*, uint16_t, uint16_t) { return 0; }
#endif
#endif
#endif // POCKETOBI_HOSTSIM
