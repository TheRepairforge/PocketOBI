# PocketOBI

A standalone, screen-based reader and diagnostic tool for Makita LXT (18V)
batteries, running on an ESP32-C3 — no PC required. It reads cell voltages,
temperatures, charge count and error/lock state, and can reset false BMS
lockouts to rescue packs that are still good.

PocketOBI is a standalone **OBI client**: it speaks the same Makita OneWire
protocol documented by the [Open Battery Information](https://github.com/mnh-jansson/open-battery-information)
project, but as a self-contained handheld device with a TFT screen and a
rotary encoder instead of a computer.

Created by **The Repair Forge** — follow the build on YouTube:
https://www.youtube.com/channel/UCQL_-pcIEkrDPyljl3QPzcw

## 📺 Watch it in action

[![PocketOBI v2 on YouTube](https://img.youtube.com/vi/8Qe2dTXrJ8g/hqdefault.jpg)](https://youtu.be/8Qe2dTXrJ8g)

The **v2** walkthrough — the new interface, the traffic-light verdict and the staged
repair wizard in action:
**https://youtu.be/8Qe2dTXrJ8g**

Original build & protocol deep-dive — how it works, the reverse-engineering, and a live demo:
**https://youtu.be/57KsQQ7-Qd0**

## Project status — v2.1.0

The **v2.1.0** release: the V2 interface (a 2×2 launcher, a paged Battery view, a
traffic-light verdict and a staged Repair wizard), multilingual EN/FR/DE/ES, on a
single-source build. Validated on real BL18xx packs. Feedback and test reports
(especially serial logs from real packs) are very welcome.

> ⚠️ **Safety first.** These packs contain lithium cells and up to ~21 V on
> B+. **Never connect B+ to the ESP32.** Resetting a BMS error only helps a
> pack whose cells are actually healthy (a false lockout). Do **not** force a
> pack with a genuinely bad/low cell back into service — it is a fire risk.
> Use this tool at your own risk.

## Features

- **Standalone V2 interface** — a 2×2 launcher (Battery / Repair / Tools / About)
  driven by the rotary encoder (+ a secondary back button: short = back, long = home);
  it jumps straight to the Battery view when a pack is connected.
- **Battery view, paged** (turn the encoder): Overview / Health / Identity.
  - *Overview*: per-cell voltage bars (green / yellow / red), pack voltage, spread.
  - *Health*: our own cycle-based Condition estimate, over-discharge / over-load wear
    counters, cell min/max + spread, both temperature sensors + spread.
  - *Identity*: model, charge count, manufacturing date, capacity, and the pack
    **serial number** (ROM ID in Makita 16-char hex format).
- **Traffic-light verdict** — HEALTHY / REPAIRABLE / REAL FAULT (plus an orange
  "possible HW fix" tier). One verdict, computed from a single source and shown
  identically on the Battery tile, the screens and the Repair wizard.
- **Repair wizard** — a staged, feasibility-first check: comms → hardware faults
  (broken sense wire / weak group / imbalance / thermistor, each naming the group or
  sensor) → lock + prognosis (false lockout "should hold" vs a memorised fault
  "unlikely to hold"). A hardware fault blocks the unlock; the prognosis reads as
  guidance, never a guarantee.
- **Unlock / repair** — rewrites the frame to lift a *false* charger lockout on a
  pack whose cells are healthy: clears the charger-lock nybble, recomputes the
  charger-validated checksums, writes the frame back. Never forces a genuinely bad
  pack back into service. See below.
- **Error reset** with before → after feedback (full test-mode + power-cycle sequence).
- Automatic detection of standard vs older **F0513** BMS generations.
- **Multilingual UI** — English, French, German, Spanish.
- **PC bridge mode** — acts as a USB↔OneWire adapter (drop-in ArduinoOBI), so a desktop
  app works through PocketOBI: the original *Open Battery Information* app, or our own
  companion **[PackScope](https://github.com/TheRepairforge/PackScope)** — saved history,
  a health estimate and a guided repair workflow. Dual use: standalone tester **and** PC adapter.
- Tools: pack LED test, error reset, raw debug view (ROM ID + message frame).

## Architecture

[<img src="docs/architecture.png" alt="PocketOBI architecture diagram" width="480">](docs/architecture.png)

*(click for full size — full-resolution file in [`docs/architecture.png`](docs/architecture.png))*

From the battery up: the Makita pack speaks over a single data wire; the bundled
**OneWire2** driver (reused from OBI) handles the custom Makita bit timings; the
ESP32-C3 firmware is layered — protocol (frames + ENABLE, pack-type detection),
data decode (bytes → cells / temperature / errors / lock), display (Adafruit GFX
+ ST7789) and a small UI state machine — driving the TFT and, optionally, a USB
PC bridge.

## Hardware

| Component | Notes |
|---|---|
| ESP32-C3 SuperMini | any ESP32-C3 board with native USB |
| 2.4" SPI TFT, ST7789 (240×320) | + integrated EC11 rotary encoder module |
| Makita BL1830 LXT adapter | clips onto the 18V pack |
| 2 × 4.7 kΩ resistors | pull-ups for DATA and ENABLE (470 Ω also works) |
| USB-C power (power bank/charger) | powers the tool — NOT the battery |

### Wiring

[<img src="docs/wiring.png" alt="PocketOBI wiring diagram" width="560">](docs/wiring.png)

*(click for full size — full-resolution file in [`docs/wiring.png`](docs/wiring.png))*

**Display + encoder module (2-in-1 TFT + EC11):**

| ESP32-C3 | Module pin | Role |
|---|---|---|
| GPIO0  | SCL  | SPI clock |
| GPIO1  | SDA  | SPI data (MOSI) |
| GPIO10 | RES  | reset |
| GPIO20 | DC   | data / command select |
| GPIO21 | CS   | chip select (active low) |
| 3.3 V  | VCC  | logic power |
| 3.3 V  | BLK  | backlight (or leave unconnected = always on) |
| GND    | GND  | ground |
| GPIO5  | A    | encoder phase A |
| GPIO6  | B    | encoder phase B |
| GPIO7  | PUSH | encoder push button |
| GPIO2  | KO   | secondary button (short = back, long = home) |

**Battery adapter (Makita LXT connector):**

| ESP32-C3 | Battery pin | Role |
|---|---|---|
| GPIO3 + 4.7 kΩ pull-up to 3.3 V | Pin 2 — **DATA** | OneWire data |
| GPIO4 + 4.7 kΩ pull-up to 3.3 V | Pin 6 — **ENABLE** | enable (active high) |
| GND | main **B-** terminal | ground (sturdier than signal pin 5; same ground) |
| — | Pin 1 — **B+ (18 V)** | **NEVER CONNECT** |

Notes:
- **Pull-ups:** 4.7 kΩ is the reference value; **470 Ω** was used successfully on
  a breadboard (on 3.3 V logic the pack loads the DATA line near the input
  threshold, so a stronger pull-up can help with long/messy wiring).
- ⚠️ **Identify DATA/ENABLE by the ESP32 silkscreen labels ("3" / "4"), not by
  the adapter's connector position.** Some AliExpress adapters number their orange
  connector from the opposite end, so DATA can land on what looks like "plot 6"
  and ENABLE on "plot 2". Wrong plots = no comms (present = 0, all-`FF`/`00`).
- Power the tool from **USB-C**, never from the Makita pack (B+ is 18 V, and the
  BMS can cut its own output on error).

A carrier PCB has been **fabricated and bench-validated**. The fabrication files
(Gerbers, schematic PDF, BOM) and ordering notes are in [`hardware/`](hardware/); the
wiring and pinout reference stays in [HARDWARE.md](HARDWARE.md).

## Build & flash

### Arduino IDE

1. Install the **ESP32 board package** (Espressif) via the Boards Manager.

   > **Use a stable core (3.x).** PocketOBI is built and tested against the
   > **stable ESP32 core 3.x** (2.0.x also works). Avoid the **alpha/dev
   > releases** (e.g. `4.0.0-alpha1`): they build fine but flood the console
   > with harmless `-Wdeprecated-declarations` / `-Wmissing-field-initializers`
   > warnings coming from *the core itself* (`esp32-hal-ledc.c`, `Esp.cpp`, …),
   > not from PocketOBI. If you see those warnings, it is the alpha core, not a
   > bug here — select a 3.x version in the Boards Manager.

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

### PlatformIO (VS Code)

The `platformio.ini` at the repo root builds the **same sources** the Arduino IDE
uses, straight from the sketch folder — no separate project, no copy to keep in sync:

```bash
pio run             # build
pio run -t upload   # build + flash
pio device monitor  # serial monitor (115200)
```

Board defaults to `esp32-c3-devkitm-1` (works for the ESP32-C3 SuperMini); change
`board` in `platformio.ini` for a different ESP32-C3 module.

## Usage

Power the tool over USB, connect DATA / ENABLE / GND to the pack (never B+),
and it reads automatically. Turn the encoder to navigate, click to select.
If the home screen shows "No battery found", check wiring and use
Menu → Read battery. "Comm error" / all-`0xFF` means the pack's BMS is not
responding (dead, or not an OBI-compatible pack).

## Unlock / repair

Some packs refuse to charge even though their cells are healthy and balanced:
the BMS stores a frame that trips the charger's lock. The Makita charger only
validates **three** fields of the 32-byte frame:

- **nybble 34** (byte 17, low) — the charger lock, must be `0`;
- **CS0** (nybble 41) — `sum(nybbles 0–15) & 0x0F`;
- **CS2** (nybble 43) — `sum(nybbles 32–40) & 0x0F`.

The status byte (byte 19, e.g. `0xA5`) and the reported temperatures are **not**
part of that check. The battery's own internal lock additionally checks **CS1**
(nybbles 16–31, per the rosvall protocol docs), so the repair recomputes all
three checksums. The Unlock / repair menu entry clears nybble 34, recomputes
CS0/CS1/CS2, writes the frame back (arm → write → store) and clears the internal
error register. The failure code (nybble 40, e.g. `0xF` = dead) is **never**
cleared — a genuinely dead pack is not forced back into service. Manufacturing/status bytes are never touched, and if the frame
is already valid no write is performed.

> ⚠️ This **writes to the BMS flash** and is gated behind a confirmation screen.
> It only clears a *false* charger lockout on an otherwise-healthy pack; it never
> overrides the BMS's own fault protection. Never use it to force a pack with a
> bad or low cell back into service.

## Note on temperature units

Temperatures are decoded as **1/10 K** (`T_C = raw / 10 - 273.15`). This is now
corroborated by four independent sources: the
[rosvall protocol docs](https://codeberg.org/rosvall/makita-lxt-protocol), the
obi-esp32 encoding, and both sides of the
[m5din-makita](https://github.com/no-body-in-particular/m5din-makita) fork — its
reader (`(raw / 10) - 273.15`) *and* its BMS emulator (`(T_C + 273.15) * 10`).
The original Open Battery Information app decodes the same field as
**Celsius x100**; that appears to be the outlier. The unit is still not stated in
any official Makita document, so treat the absolute value as approximate, but the
1/10 K interpretation is the well-supported one. The dependable signal is *relative*: the
two sensors are shown side by side (e.g. `28/31`, and in red when a value is
implausible), so a reading that is far off or that disagrees strongly with the
other flags a likely faulty thermistor.

The two sensors are reported by the BMS over the data line (there is **no
separate thermistor pin** on the connector). The original protocol simply labels
them **"Sensor 1"** and **"Sensor 2"** — **which reading is the cell sensor vs the
MOSFET sensor is not documented**, and their physical placement on the BMS board
is unknown. PocketOBI just shows both values; do not assume which is which.

## Versioning

See [CHANGELOG.md](CHANGELOG.md). The current version is shown on the
Version / info screen and defined as `FW_VERSION` in the sketch.

## Credits

- **Open Battery Information** by **Martin Jansson** — the original project that
  documents the Makita protocol and provides the OneWire2 library.
  https://github.com/mnh-jansson/open-battery-information (MIT)
- ESP32-C3 wiring reference: the `obi-esp32` port by appositeit.
- Root protocol reverse-engineering: the
  [rosvall/makita-lxt-protocol](https://codeberg.org/rosvall/makita-lxt-protocol)
  documentation (frame byte/nybble map, the three checksum ranges, per-type
  command sets) — the origin much of the LXT decoding traces back to.
- Unlock / frame-repair research: the
  [synrais/Makita-LXT-Battery-Monitor-Unlocker](https://github.com/synrais/Makita-LXT-Battery-Monitor-Unlocker)
  project, which documented the frame byte map, the CS0/CS2 checksums, the
  charger-lock nybble and the arm/write/store opcodes. That repository ships with
  **no license** (all rights reserved), so **none of its code is used here**;
  PocketOBI's unlock feature is a clean-room reimplementation from those
  (unprotectable) protocol facts only, cross-checked against real battery dumps.

## License

PocketOBI is licensed under the **PolyForm Noncommercial License 1.0.0** — free to
use, modify, and share for any **noncommercial** purpose (personal, hobby, repair,
education, research). Commercial use requires a separate license. See [LICENSE](LICENSE).

Bundled and reused third-party components keep their own licenses — see
[THIRD-PARTY.md](THIRD-PARTY.md). The bundled OneWire2 library and the upstream
Open Battery Information project remain under the MIT license.

---

```
  ___         _       _    ___  ___ ___ 
 | _ \___  __| |_____| |_ / _ \| _ )_ _|
 |  _/ _ \/ _| / / -_)  _| (_) | _ \| | 
 |_| \___/\__|_\_\___|\__|\___/|___/___|
=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
       . No Guru Meditation required .
=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
```
