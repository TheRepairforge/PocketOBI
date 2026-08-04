# PocketOBI — PlatformIO build

A conventional [PlatformIO](https://platformio.org/) project (VS Code friendly),
as an alternative to the Arduino IDE build at the repository root.

## Build & upload

From this folder:

```bash
pio run              # build
pio run -t upload    # build + flash
pio device monitor   # serial monitor (115200)
```

Board defaults to `esp32-c3-devkitm-1` (works for the ESP32-C3 SuperMini). Change
`board` in `platformio.ini` if you use a different ESP32-C3 module.

## ⚠️ This folder is a MIRROR — keep it in sync

The **canonical source of truth is `../PocketOBI.ino`** (the Arduino sketch at the
repo root). The files here are copies:

```
platformio/
  src/PocketOBI.ino        <- copy of ../PocketOBI.ino
  lib/OneWire2/…           <- copy of ../OneWire2.* and ../util/
```

After editing the root sketch (or OneWire2), regenerate the mirror so both builds
stay identical:

```bash
# Windows
platformio\sync.ps1
# macOS / Linux
sh platformio/sync.sh
```

Do **not** edit the files under `platformio/src` or `platformio/lib` directly —
they are overwritten by the sync script.
