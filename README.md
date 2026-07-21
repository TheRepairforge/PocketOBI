# PocketOBI

A standalone, screen-based reader and diagnostic tool for Makita LXT (18V)
batteries, running on an ESP32-C3 — no PC required. It reads cell voltages,
temperatures, charge count and error/lock state, and can reset false BMS
lockouts to rescue packs that are still good.

PocketOBI is a standalone **OBI client**: it speaks the same Makita OneWire
protocol documented by the [Open Battery Information](https://github.com/mnh-jansson/open-battery-information)
project, but as a self-contained handheld device with a TFT screen and a
rotary encoder instead of a computer.

Created by **The Repair Forge**.

> ⚠️ **Safety first.** These packs contain lithium cells and up to ~21 V on
> B+. **Never connect B+ to the ESP32.** Resetting a BMS error only helps a
> pack whose cells are actually healthy (a false lockout). Do **not** force a
> pack with a genuinely bad/low cell back into service — it is a fire risk.
> Use this tool at your own risk.

## Features

- Auto-read on boot: plug in a pack, power up, see its state.
- Per-cell voltage bars with color-coded health (green / yellow / red) and
  imbalance detection.
- Pack voltage, cell and MOSFET temperatures, cell spread.
- Model, charge count, manufacturing date, capacity, error code, lock state.
- Automatic detection of standard vs older **F0513** BMS generations.
- Error reset with before → after feedback.
- Pack LED test (on/off).
- Raw debug view (ROM ID + message frame).
- Secondary "back" button (short = back, long = home).

## Hardware

| Component | Notes |
|---|---|
| ESP32-C3 SuperMini | any ESP32-C3 board with native USB |
| 2.4" SPI TFT, ST7789 (240×320) | + integrated EC11 rotary encoder module |
| Makita BL1830 LXT adapter | clips onto the 18V pack |
| 2 × 4.7 kΩ resistors | pull-ups for the DATA and ENABLE lines |
| USB-C power (power bank/charger) | powers the tool — NOT the battery |

### Pinout

| Function | ESP32-C3 |
|---|---|
| Battery DATA (pin 2) | GPIO3 + 4.7 kΩ pull-up to 3.3 V |
| Battery ENABLE (pin 6) | GPIO4 + 4.7 kΩ pull-up to 3.3 V |
| Battery GND (pin 5) | GND |
| Battery B+ (18 V) | **NEVER CONNECT** |
| TFT SCLK / MOSI / RST / DC / CS | GPIO0 / 1 / 10 / 20 / 21 |
| TFT VCC / BLK | 3.3 V |
| Encoder A / B / PUSH | GPIO5 / 6 / 7 |
| Module KO (back button) | GPIO2 |

The Makita signal connector pins are numbered from the B+ side: pin 2 = Data,
pin 5 = GND, pin 6 = Enable. Both DATA and ENABLE need their own 4.7 kΩ pull-up.

## Build & flash (Arduino IDE)

1. Install the **ESP32 board package** (Espressif) via the Boards Manager.
2. Install these libraries via the Library Manager:
   - Adafruit GFX Library
   - Adafruit ST7735 and ST7789 Library
   - RotaryEncoder (by Matthias Hertel)
   - (OneWire2 is bundled in this repo — nothing to install.)
3. Open `PocketOBI/PocketOBI.ino`.
4. Board: **ESP32C3 Dev Module**, USB CDC On Boot: **Enabled**.
5. Upload.

> Note: Arduino requires the sketch to live in a folder named `PocketOBI`.
> If you downloaded a ZIP (GitHub adds a `-main` suffix), rename the inner
> sketch folder back to `PocketOBI` before opening it.

## Usage

Power the tool over USB, connect DATA / ENABLE / GND to the pack (never B+),
and it reads automatically. Turn the encoder to navigate, click to select.
If the home screen shows "No battery found", check wiring and use
Menu → Read battery. "Comm error" / all-`0xFF` means the pack's BMS is not
responding (dead, or not an OBI-compatible pack).

## Versioning

See [CHANGELOG.md](CHANGELOG.md). The current version is shown on the
Version / info screen and defined as `FW_VERSION` in the sketch.

## Credits

- **Open Battery Information** by **Martin Jansson** — the original project that
  documents the Makita protocol and provides the OneWire2 library.
  https://github.com/mnh-jansson/open-battery-information (MIT)
- ESP32-C3 wiring reference: the `obi-esp32` port by appositeit.

## License

MIT — see [LICENSE](LICENSE). The bundled OneWire2 retains its own MIT license
and copyright notices.
