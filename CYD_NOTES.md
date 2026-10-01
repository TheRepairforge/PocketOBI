# PocketOBI on the CYD (alternative board)

The **CYD** (ESP32-2432S028R, dual-USB / ST7789 revision, XPT2046 resistive touch) is an
**alternative** board, supported since v2.2.0. The reference product is the ESP32-C3 +
ST7789 + EC11 encoder — see `HARDWARE.md`.

**Selecting the CYD build:** the board is chosen at build time. The pin map and the
board-select mechanism live in `board_config.h`; `platformio.ini` has one env per board
(`pio run -e cyd`). To build the CYD from the Arduino IDE, drop a gitignored
`board_local.h` next to the sketch containing `#define POCKETOBI_BOARD BOARD_CYD`. The
logic branches only on the `BOARD_HAS_ENCODER` / `BOARD_HAS_TOUCH` capability flags, never
on the board id.

**Status:** the CYD is **bench-validated end to end**: the touch UI and a real pack read
over the CN1 connector both work on hardware. The CN1 wiring (DATA = IO22, ENABLE = IO27,
4.7 kΩ pull-ups to 3V3, GND to B-, never B+) is in the README, section
"Alternative board: CYD".
