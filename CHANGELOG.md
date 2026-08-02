# Changelog

All notable changes to the PocketOBI firmware are documented here.
Versioning follows [Semantic Versioning](https://semver.org/): MAJOR.MINOR.PATCH.

The version is defined in `PocketOBI.ino` as `FW_VERSION` and shown
on the on-device "Version / info" screen.

## [0.9.6] - 2026-08-02

### Added
- **PlatformIO build** (`platformio/`): a conventional PlatformIO project
  (src/ + lib/) for VS Code users, alongside the Arduino IDE build. It mirrors
  the root Arduino sources via `platformio/sync.ps1` / `sync.sh`; the root
  `PocketOBI.ino` remains the single source of truth. (Addresses issue #1.)

## [0.9.5] - 2026-08-02

### Changed
- The home temperature chip now shows **both sensors** (cell / MOSFET, e.g.
  `28/31`) instead of only the cell reading, so a disagreement between the two is
  visible at a glance. It turns red when either reading is outside the plausible
  window (likely faulty thermistor). F0513 (single sensor) is unchanged.
- README/HARDWARE wording tidied for the public release; the unlock/repair is
  described by its scope (clears a false charger lockout on an otherwise-healthy
  pack; never overrides the BMS's own fault protection).

## [0.9.4] - 2026-07-30

### Fixed / hardened (pre-release audit against all protocol sources)
- **PC bridge** now reports the real firmware version in the interface-version
  query (was hardcoded to 0.8.x).
- **F0513**: clear `romId`/`msg` on the F0513 path so Debug/raw no longer shows
  stale data left over from a previously-read standard pack.
- **10-cell / 36V packs (BL36xx)**: detected (battery type >= 30) and shown as
  "not supported (18V only)" instead of a misleading 5-cell dashboard.
- **Robustness**: guard against a `uint8_t` underflow if a `0x33` command were
  ever defined with `rsp_len < 8`.
- Added a note that the **F0513 temperature unit is unverified** (kept at /100;
  the standard path uses 1/10 K).

## [0.9.3] - 2026-07-30

### Added
- Implausible-temperature flag: when the cell temperature falls outside a
  plausible window (`TEMP_MIN_PLAUS`..`TEMP_MAX_PLAUS`, -20..80 C), the home
  temperature chip turns red and shows a trailing `?` (e.g. `T -30?`) — a
  likely faulty thermistor. It is a suspicion, not a confirmed diagnosis.

## [0.9.2] - 2026-07-30

### Fixed
- **Temperature decoding**: the protocol reports temperature in **1/10 K**
  (`raw = (T_C + 273.15) * 10`), not Celsius x100. Cell and MOSFET temperatures
  are now decoded as `raw / 10 - 273.15`. Previous values were a few degrees off
  on a healthy pack, and — more importantly — a **faulty thermistor** (which
  reports an absurd value the charger rejects as a "temperature" fault) now shows
  its true reading (e.g. ~ -30 C) instead of a misleading normal-looking number.

## [0.9.1] - 2026-07-29

### Added
- Serial trace of the raw frame write/store (`ow33`) under `COMM_DEBUG`, so the
  otherwise-invisible unlock writes show up in the logs.

### Fixed
- Home + Details **LOCKED/UNLOCKED** now reflects the real **charger lock**
  (nybble 34 / CS0 / CS2 mismatch, i.e. what actually makes the charger refuse a
  pack), not only the failure code (nybble 40). Details also shows a `ChgLock:`
  line listing the causes (`N34`/`CS0`/`CS1`/`CS2`). Previously a frame whose
  charger-lock nybble was set could still read UNLOCKED on screen.

## [0.9.0] - 2026-07-26

### Added
- **Unlock / repair** menu entry: rewrites a battery frame to make the charger
  accept a pack whose cells are healthy but whose stored frame trips the charger
  lock. Clears the charger-lock nybble (nybble 34) and recomputes all three
  frame checksums (CS0, CS1, CS2 - CS0/CS2 gate the charger, CS1 the battery's
  internal lock), then writes the frame back
  (arm `CC F0 00` -> write `33 0F 00` + 32 bytes -> store `33 55 A5`), power-
  cycles the bus and clears the internal error register. Guarded by a
  confirmation screen; if the frame is already valid, no write is performed.
  Shows the lock causes before -> after. Manufacturing/status bytes (incl.
  byte 19 / `0xA5`) and the failure code (nybble 40) are never modified.
- Result screen reporting the remaining lock causes (`CS0`/`CS1`/`CS2`/`N34`) and
  an Unlocked / Still-locked verdict.

### Changed
- **Error reset** now runs the full sequence: enter test mode -> `DA 04` ->
  exit test mode (`CC D9 FF FF`) -> bus power-cycle, so the BMS actually commits
  the internal error-register clear (previously it stopped after `DA 04`).

### Notes / credit
- The unlock/frame-repair capability is a **clean-room** reimplementation from
  publicly documented protocol facts (frame byte/nybble map, checksum ranges +
  formula, nybble-34 charger lock, arm/write/store opcodes). Sources:
  - **rosvall/makita-lxt-protocol** (Codeberg) — root reverse-engineering of the
    frame layout and the three checksum ranges (0-15, 16-31, 32-40).
  - **synrais/Makita-LXT-Battery-Monitor-Unlocker** — frame-repair method and the
    empirical charger-validation results (only nybble 34, CS0, CS2 gate the
    charger). This repo ships with **no license** (all rights reserved), so none
    of its source code is used — only the unprotectable protocol facts, cross-
    checked against real battery dumps.
  - Base protocol credit remains Open Battery Information (Martin Jansson, MIT).

## [0.8.1] - 2026-07-21

### Changed
- Clearer lock status: home footer shows **UNLOCKED** (green) / **LOCKED** (red)
  instead of "OK". Added an explicit colored "State: UNLOCKED/LOCKED" line to the
  Details screen.

## [0.8.0] - 2026-07-21

### Added
- **PC bridge mode** (menu entry "PC bridge"): the device acts as a USB <->
  OneWire bridge, a drop-in replacement for the ArduinoOBI, so the desktop
  **Open Battery Information** app talks to PocketOBI directly (select the
  Arduino OBI interface + PocketOBI's COM port). Handles the version query and
  the 0xCC / 0x33 / F0513 (0x31,0x32) commands via the existing sendCommand().
  Back button exits. Dual use: standalone tester AND PC adapter.

### Changed
- COMM_DEBUG serial tracing defaults to OFF (comms validated). It MUST stay off
  in PC bridge mode or it would corrupt the binary protocol.

## [0.7.0] - 2026-07-21

### Added
- Estimated state of charge (SoC ~%) on the home screen, derived from the
  average cell voltage (piecewise-linear Li-ion OCV curve; approximate, shown
  with a "~").

### Changed
- Boot now goes: splash (~1.5 s) -> menu directly. No auto-read and no
  "no battery" home screen; the user picks what to do from the menu.

## [0.6.2] - 2026-07-21

### Changed
- Removed the auto-read at boot. The boot splash now shows for ~1.5 s as a
  simple logo screen, then the home screen appears; the user starts a read from
  the menu. (First real-hardware validation: reads a BL1860B correctly.)

## [0.6.1] - 2026-07-21

### Changed
- Ground is now taken from the main B- terminal (simpler and more reliable than
  signal pin 5; same ground). Wiring docs updated accordingly.
- "beta" tag shown on the splash and About screens; README gains a "Step 1
  (beta)" status section and a link to The Repair Forge YouTube channel.

## [0.6.0] - 2026-07-21

### Changed
- Tool named **PocketOBI** (standalone OBI client). Boot splash now shows the
  name in big two-tone letters ("Pocket" teal + "OBI" orange) instead of the
  logo. About screen features PocketOBI, credits "The Repair Forge" as creator,
  and keeps the base-project credit (Open Battery Information, Martin Jansson).

### Removed
- Primitive-drawn logo mark (anvil/wrench/spark) dropped in favor of the name.

## [0.5.0] - 2026-07-21

### Added
- Credit to the base project on the About screen: Open Battery Information by
  Martin Jansson (MIT), github.com/mnh-jansson.

## [0.4.0] - 2026-07-21

### Added
- Boot splash screen (title + version + "Reading battery...").
- Auto-read on startup: the tool attempts a battery read at boot, so the home
  screen shows real data immediately when a pack is present instead of always
  claiming "no battery" before ever checking.

### Changed
- Home "no battery" screen now only appears after an actual read attempt, with
  a clearer prompt ("Connect a pack, then Menu > Read battery").

## [0.3.2] - 2026-07-21

### Fixed
- Battery communication never started: the code aborted every transaction on
  `makita.reset()` returning 0. The Makita BMS does not assert a standard
  OneWire presence pulse, so the official ArduinoOBI ignores that return value.
  We now reset and talk regardless, and detect "no battery" from an all-0xFF
  response instead.

### Added
- COMM_DEBUG serial trace of every OneWire transaction (presence flag, ROM ID,
  received bytes) to diagnose battery communication on real hardware.

## [0.3.1] - 2026-07-21

### Fixed
- Encoder rotation direction was reversed; swapped A/B in the RotaryEncoder
  constructor so turning right moves down the menu.

## [0.3.0] - 2026-07-21

### Added
- Secondary "back" button support on the module's KO button (wire KO to GPIO2):
  short press = go one screen back (sub-screen -> menu, menu -> home),
  long press (>= 600 ms) = jump straight to home. Uses the internal pull-up,
  so it stays inert until KO is actually wired.

## [0.2.0] - 2026-07-20

### Changed
- Menu bullets replaced with real icons drawn from primitives (battery, list,
  refresh, sun, sun-off, code, info). On the highlighted row the icon is forced
  to the light header color so it stays visible on the accent background.

## [0.1.0] - 2026-07-20

First versioned release. The firmware is feature-complete on the UI side but
the battery OneWire2 communication has not yet been validated against real
hardware (waiting for the battery connector).

### Added
- Standalone Makita LXT reader on ESP32-C3 + ST7789 2.4" TFT + EC11 encoder.
- OneWire2 protocol layer (bundled), commands ported from `makita_lxt.py`.
- Automatic standard / F0513 battery detection.
- Home screen with per-cell voltage bars, color-coded anomalies
  (green / yellow / red) and status chips (temperature, spread, lock/error).
- Menu: read info, details, reset error, pack LEDs on/off, debug/raw,
  version/info.
- Error reset with before -> after visual feedback.
- Dark dashboard theme: accent header bars, rounded bars, colored menu bullets.
- "Version / info" screen showing firmware version and build date.

### Hardware / libraries
- Display driven with Adafruit GFX + Adafruit ST7789 over hardware SPI.
- Encoder decoded with the RotaryEncoder library (software, ESP32-C3 compatible).

### Known / untested
- OneWire2 battery communication never tested on real hardware.
- Display orientation is landscape (`setRotation(1)`); portrait not done yet.
