# Changelog

All notable changes to the PocketOBI firmware are documented here.
Versioning follows [Semantic Versioning](https://semver.org/): MAJOR.MINOR.PATCH.

The version is defined in `PocketOBI.ino` as `FW_VERSION` and shown
on the on-device "Version / info" screen.

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
- RepairForge branding: stylized logo (anvil + wrench + orange spark) drawn with
  primitives, shown on the boot splash and the About screen, plus the REPAIR
  FORGE wordmark.
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
