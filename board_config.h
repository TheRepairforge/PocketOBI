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
#define BOARD_ES3C28P 3  // LCDWiki ES3C28P: ESP32-S3 + ILI9341V + FT6336G capacitive touch (D48, community-tested)

// Touch driver, one selector per board (D48): never two booleans. The sketch includes and
// instantiates the driver this names; the logic only calls touch.touched() / touch.getPoint().
#define TOUCH_NONE    0   // no touch input
#define TOUCH_XPT2046 1   // resistive, on its own SPI bus (CYD)
#define TOUCH_FT6336  2   // capacitive, on I2C, reports pixels (ES3C28P, touch_ft6336.h)

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
#define BOARD_HAS_TOUCH   (BOARD_TOUCH_DRIVER != TOUCH_NONE)

// ================= BOARD HEADER =================
// Every GPIO assignment lives here, nowhere else. A new board adds one branch.
// Three sub-blocks per board: battery bus / input device / display. Each branch also
// sets the per-board capability selectors: BOARD_TOUCH_DRIVER (a TOUCH_* value),
// BOARD_TOUCH_NEEDS_CAL (1 = Settings > Calibrate is offered), PANEL_ILI9341
// (1 = write the ILI9341 MADCTL orientation in applyRotation()) and PANEL_INVERT
// (1 = the panel needs display inversion ON for correct colours, setup()) and
// PANEL_ILI9341_INIT (1 = send the ILI9341 power / VCOM / gamma registers after tft.init(), setup()).
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
// --- Capabilities: no touch, no calibration, ST7789 orientation ---
#define BOARD_TOUCH_DRIVER    TOUCH_NONE
#define BOARD_TOUCH_NEEDS_CAL 0
#define PANEL_ILI9341         0
#define PANEL_INVERT          0
#define PANEL_ILI9341_INIT    0

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
// Some dual-USB CYDs ship with an ILI9341 panel instead of the ST7789. The ST7789
// init drives it fine, but its MADCTL reads differently: mirrored image, red/blue
// swapped. Set CYD_ILI9341 1 (board_local.h, or -D from the cyd-ili9341 env) for
// those boards; the firmware then writes the ILI9341 orientation (applyRotation()).
#ifndef CYD_ILI9341
#define CYD_ILI9341 0
#endif
// --- Capabilities ---
#define BOARD_TOUCH_DRIVER    TOUCH_XPT2046
#define BOARD_TOUCH_NEEDS_CAL 1
#define PANEL_ILI9341         CYD_ILI9341
#define PANEL_INVERT          0
#define PANEL_ILI9341_INIT    0
// Factory touch calibration (raw XPT2046 -> screen pixels, see ui_nav.cpp). Measured on the
// bench; Settings > Calibrate overwrites them in NVS.
#define TCAL_DEF_X_LEFT   3765   // raw Y at screen x=0   (left edge)
#define TCAL_DEF_X_RIGHT   235   // raw Y at screen x=319 (right edge)
#define TCAL_DEF_Y_TOP     415   // raw X at screen y=0   (top edge)
#define TCAL_DEF_Y_BOTTOM 3751   // raw X at screen y=239 (bottom edge)

#elif POCKETOBI_BOARD == BOARD_ES3C28P
// --- LCDWiki ES3C28P: ESP32-S3 (16 MB flash, PSRAM unused), ILI9341V 240x320, FT6336G touch ---
// Native USB on the Type-C. Pins as wired by the community tester (D48); no hardware built
// or bench-tested by us. GPIO3 is an S3 strapping pin (JTAG source select) and is pulled up
// by the 4.7k resistor at boot: if a pack ever fails to read right after power-on, check it.
// --- Battery bus (expansion header) ---
#define ONEWIRE_PIN 2    // DATA, + 4.7k pull-up to 3V3
#define ENABLE_PIN  3    // ENABLE, + 4.7k pull-up to 3V3
// --- Input: capacitive touch (FT6336G on I2C, no encoder, no back button) ---
#define TOUCH_SDA  16
#define TOUCH_SCL  15
#define TOUCH_RST  18    // active low: pulsed once in begin()
#define TOUCH_INT  17    // active low while touched; optional, not read (the sketch polls over I2C)
// --- Display: ILI9341V (SPI, hardware) ---
#define TFT_CS   10
#define TFT_DC   46      // strapping pin on the S3, driven by the display after boot
#define TFT_RST  -1      // tied to the chip reset on this carrier
#define TFT_MOSI 11
#define TFT_SCLK 12      // MISO not wired to the sketch (no read-back)
#define TFT_BL   45      // backlight: driven HIGH in setup(), else the panel stays black
// --- Capabilities ---
#define BOARD_TOUCH_DRIVER    TOUCH_FT6336
#define BOARD_TOUCH_NEEDS_CAL 1   // a capacitive panel reports pixels; the factory defaults below are a guess
#define PANEL_ILI9341         1
// The ES3C28P's ILI9341V shows every colour inverted with inversion OFF (tester photos, #22:
// near-black background drawn cream, teal header salmon, green bars magenta).
#define PANEL_INVERT          1
// tft.init() is the ST7789 sequence: on this ILI9341V it leaves power, VCOM and gamma at their reset
// values (tester: near-black drawn grey-blue). The generic Adafruit ILI9341 values were tried in test5
// and broke the picture on this module (noise lines, then horizontal stripes on every redraw), so they
// stay OFF here until the board vendor's own init values are known.
#define PANEL_ILI9341_INIT    0
// Factory touch calibration: UNVERIFIED (no board in our hands). The FT6336 reports pixels in the
// panel's native portrait frame (x 0..239, y 0..319). With the ILI9341 landscape orientation
// (MADCTL 0x28) the screen is that frame transposed without a mirror, so screen X follows raw Y
// and screen Y follows raw X, and the identity map below is the expected one. If the touch
// frame is mirrored or swapped on this module, the tester runs Settings > Calibrate.
#define TCAL_DEF_X_LEFT   0      // raw Y at screen x=0   (left edge)
#define TCAL_DEF_X_RIGHT  319    // raw Y at screen x=319 (right edge)
#define TCAL_DEF_Y_TOP    0      // raw X at screen y=0   (top edge)
#define TCAL_DEF_Y_BOTTOM 239    // raw X at screen y=239 (bottom edge)

#else
#  error "POCKETOBI_BOARD must be BOARD_LXT_C3, BOARD_CYD or BOARD_ES3C28P"
#endif
