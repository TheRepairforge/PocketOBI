#pragma once
// Board selection + per-board pin map. This lives in a real header ON PURPOSE:
// PlatformIO's .ino->.cpp converter relocates #define directives, which breaks the
// exclusivity of an #if/#elif pin-selection chain when it sits inside the .ino (both
// board blocks end up compiled, last-wins -> wrong pinout). A .h is included verbatim
// by both PlatformIO and the Arduino IDE, so the chain stays exclusive. Keep the
// board pin map here; the sketch only consumes the resulting macros.

// ================= BOARD SELECTION =================
// PocketOBI runs on more than one carrier board. Exactly one is selected here,
// as a plain #define so "open PocketOBI.ino in the Arduino IDE and flash" works
// with no external build config; a PlatformIO/CI build can override it with
// -DPOCKETOBI_BOARD=BOARD_xxx. A board differs only in the BOARD HEADER below
// (pins) and in which input device it carries (capability flags) — never in the
// logic. This is the XGT seam of REPO_MAP.md, generalised to a second board.
#define BOARD_LXT_C3 1   // ESP32-C3 SuperMini + ST7789 + EC11 encoder (shipping product)
#define BOARD_CYD    2   // "Cheap Yellow Display" ESP32-2432S028R: ST7789 + XPT2046 touch (dev/alt)

// Optional LOCAL board override (gitignored, never committed): drop a file
// "board_local.h" next to this header containing e.g.
//     #define POCKETOBI_BOARD BOARD_CYD
// to build for a non-default board WITHOUT editing this tracked file.
// An explicit build flag (-DPOCKETOBI_BOARD=... from a PlatformIO env or CI)
// takes precedence: board_local.h is only consulted when nothing defined it
// yet, so a per-env build is deterministic even on a machine that carries a
// board_local.h. The Arduino IDE (no build flag) still honours the file.
#if !defined(POCKETOBI_BOARD) && defined(__has_include)
#  if __has_include("board_local.h")
#    include "board_local.h"
#  endif
#endif

#if !defined(POCKETOBI_BOARD)
#  define POCKETOBI_BOARD BOARD_LXT_C3   // default = the shipping product; override via board_local.h
#endif

// Capability flags — test the CAPABILITY, not the board id, in the logic below,
// so a future third board reuses the same #if without editing every call site.
#define BOARD_HAS_ENCODER (POCKETOBI_BOARD == BOARD_LXT_C3)
#define BOARD_HAS_TOUCH   (POCKETOBI_BOARD == BOARD_CYD)

// ================= BOARD HEADER =================
// Every GPIO assignment lives here, nowhere else. A new board adds one branch.
// Three sub-blocks per board: battery bus / input device / display.
#if POCKETOBI_BOARD == BOARD_LXT_C3
// --- PocketOBI-LXT (ESP32-C3 SuperMini) — the shipping product ---
// --- Battery bus ---
#define ONEWIRE_PIN 3
#define ENABLE_PIN  4
// --- Input: rotary encoder (EC11) + secondary "back" button ---
#define ENC_A    5
#define ENC_B    6
#define ENC_BTN  7
#define BACK_BTN 2   // module "KO" secondary button: short = back, long = home
// --- Display: ST7789 TFT (SPI) ---
#define TFT_CS   21
#define TFT_DC   20
#define TFT_RST  10
#define TFT_MOSI 1   // SDA
#define TFT_SCLK 0   // SCL

#elif POCKETOBI_BOARD == BOARD_CYD
// --- CYD ESP32-2432S028R, dual-USB (USB-C + micro) revision (dev/alt target) ---
// Confirmed from this board's silkscreen: CN1 = {GND, IO22, IO27, 3V3}, and IO27
// being broken out means the backlight is on GPIO21 (not 27) -> both 22 and 27 are
// free. CN1 is therefore the whole pack connector on one 4-pin header:
//   GND | DATA(22) | ENABLE(27) | 3V3 (for the two 4.7k pull-ups)
// --- Battery bus (CN1) ---
#define ONEWIRE_PIN 22   // CN1 IO22, + 4.7k pull-up to 3V3
#define ENABLE_PIN  27   // CN1 IO27, + 4.7k pull-up to 3V3
// --- Input: resistive touch (XPT2046, its OWN SPI bus). No encoder, no back button. ---
#define TOUCH_CLK  25
#define TOUCH_MOSI 32
#define TOUCH_MISO 39
#define TOUCH_CS   33
#define TOUCH_IRQ  36
// --- Display: ST7789 TFT (HSPI) — SAME driver as the C3 board, different pins ---
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1   // tied to the board reset on this carrier
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_BL   21   // backlight: driven HIGH in setup(), else the panel stays black

#else
#  error "POCKETOBI_BOARD must be BOARD_LXT_C3 or BOARD_CYD"
#endif
