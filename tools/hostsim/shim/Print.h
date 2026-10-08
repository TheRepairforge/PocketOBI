// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Copyright (c) 2026 TheRepairForge
// hostsim shim: Arduino's Print, enough for Adafruit_GFX text output and the firmware's
// print()/printf() calls. Formatting follows the Arduino core (2 decimals for floats).
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define DEC 10
#define HEX 16

class Print {
public:
  virtual ~Print() {}
  virtual size_t write(uint8_t c) = 0;
  virtual size_t write(const uint8_t* buf, size_t n) {
    size_t k = 0; while (n--) k += write(*buf++); return k;
  }
  size_t write(const char* s) { return s ? write((const uint8_t*)s, strlen(s)) : 0; }

  size_t print(const char* s) { return write(s); }
  size_t print(char c) { return write((uint8_t)c); }
  size_t print(int v, int base = DEC)           { return fmt(base == HEX ? "%X" : "%d", v); }
  size_t print(unsigned v, int base = DEC)      { return fmt(base == HEX ? "%X" : "%u", v); }
  size_t print(long v, int base = DEC)          { return fmt(base == HEX ? "%lX" : "%ld", v); }
  size_t print(unsigned long v, int base = DEC) { return fmt(base == HEX ? "%lX" : "%lu", v); }
  size_t print(double v, int digits = 2)        { return fmt("%.*f", digits, v); }
  size_t println() { return write("\r\n"); }
  template <typename T> size_t println(T v) { size_t n = print(v); return n + println(); }

  size_t printf(const char* f, ...) __attribute__((format(printf, 2, 3))) {
    char b[256]; va_list a; va_start(a, f); vsnprintf(b, sizeof(b), f, a); va_end(a);
    return write(b);
  }

private:
  size_t fmt(const char* f, ...) {
    char b[64]; va_list a; va_start(a, f); vsnprintf(b, sizeof(b), f, a); va_end(a);
    return write(b);
  }
};
