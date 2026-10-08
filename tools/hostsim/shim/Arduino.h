// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// hostsim shim: the subset of the Arduino-ESP32 core the firmware uses, for a PC build.
// Nothing here talks to hardware: GPIO is a no-op, time is a counter, Serial is silent.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef ARDUINO
#define ARDUINO 10813
#endif
#ifndef ARDUINO_ARCH_ESP32
#define ARDUINO_ARCH_ESP32 1     // selects the 32-bit register type in OneWire2's headers
#endif

typedef uint8_t byte;
typedef bool boolean;

#define PROGMEM
#define PGM_P const char*
#define pgm_read_byte(a)    (*(const uint8_t*)(a))
#define pgm_read_word(a)    (*(const uint16_t*)(a))
#define pgm_read_dword(a)   (*(const uint32_t*)(a))
#define pgm_read_pointer(a) (*(void* const*)(a))
#define F(s) (s)

#define HIGH 1
#define LOW  0
#define INPUT        0x01
#define OUTPUT       0x03
#define INPUT_PULLUP 0x05
#define HSPI 2
#define VSPI 3

#ifndef radians
#define radians(deg) ((deg) * 0.017453292519943295)
#define degrees(rad) ((rad) * 57.29577951308232)
#endif
#ifndef constrain
#define constrain(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))
#endif

inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return in_max == in_min ? out_min : (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

unsigned long millis();
unsigned long micros();
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int  digitalRead(uint8_t pin);
inline void noInterrupts() {}
inline void interrupts() {}
long random(long max);
long random(long min, long max);
inline void yield() {}

// Only what Adafruit_GFX's String / F() overloads need; the firmware itself uses neither.
class __FlashStringHelper;
class String {
public:
  String(const char* s = "") : s_(s ? s : "") {}
  const char* c_str() const { return s_; }
  unsigned length() const { return (unsigned)strlen(s_); }
private:
  const char* s_;
};

#include "Print.h"

class HardwareSerial : public Print {
public:
  void begin(unsigned long) {}
  int available() { return 0; }
  int read() { return -1; }
  int peek() { return -1; }
  void flush() {}
  size_t write(uint8_t) override { return 1; }   // silent: the sim prints its own log
  using Print::write;
  operator bool() const { return true; }
};
extern HardwareSerial Serial;
