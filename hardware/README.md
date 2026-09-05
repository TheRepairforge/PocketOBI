# PocketOBI — hardware

Fabrication files for the PocketOBI carrier PCB: a small 2-layer board that carries the
ESP32-C3 SuperMini, the display + encoder module header, the Makita LXT battery connector
and the two pull-up resistors. **This is the board only — it does not restate the
wiring/pinout, which lives in [`../HARDWARE.md`](../HARDWARE.md) and the main
[README](../README.md).**

> ⚠️ Same safety rule as the firmware: **never connect battery B+ (18 V) to the ESP32.**

## Revision

First fabricated and **bench-validated** revision — Gerbers dated **2026-08-05**
(`gerber/PocketOBI-HW-gerbers-2026-08-05.zip`). This is the exact set that was ordered and
tested. The KiCad sources have since had minor top-copper cleanup only; pads, vias, drill
holes and the bottom-copper routing are unchanged.

## What's here

| File | What it is |
|---|---|
| `gerber/PocketOBI-HW-gerbers-2026-08-05.zip` | ready-to-order Gerbers + drill — upload this |
| `gerber/*.gbr`, `*.drl` | the same layers, extracted, for inspection |
| `PocketOBI-HW-schematic.pdf` | the schematic — to read and troubleshoot, not editable |
| `PocketOBI-HW-BOM.csv` | bill of materials (6 parts) |

The editable KiCad sources are not published — buildable, not forkable, by choice.

## Order the board

Upload `gerber/PocketOBI-HW-gerbers-2026-08-05.zip` to a fab house (JLCPCB, PCBWay, …).
Defaults are fine: **2 layers**, 1.6 mm, HASL, any colour — no controlled impedance and no
special stackup.

## Bill of materials

| Ref | Part | Note |
|---|---|---|
| U1 | ESP32-C3 SuperMini | any ESP32-C3 board with native USB |
| M1 | 1×12 pin socket, 2.54 mm | header for the 2-in-1 TFT + EC11 module |
| Battery_Interface1 | JST-PH 6-pin, vertical | to the Makita LXT adapter |
| Battery_GND1 | 2-pin screw terminal, 3.5 mm | battery B- |
| R1, R2 | 4.7 kΩ resistor, THT | DATA and ENABLE pull-ups (470 Ω also works) |

Full list: [`PocketOBI-HW-BOM.csv`](PocketOBI-HW-BOM.csv). Everything is through-hole and
hand-solderable. (R1/R2 export as a generic value `R` in the CSV — they are **4.7 kΩ**.)

## Assembly

Solder the through-hole parts, then seat the ESP32-C3 and the display + encoder module on
the headers. **Wiring, pinout and the DATA/ENABLE identification gotcha are in
[`../HARDWARE.md`](../HARDWARE.md)** — not repeated here so the two never drift.

## Enclosure

A 3D-printable enclosure is on Printables:
**<https://www.printables.com/model/1833871-pocketobi-makita-18v-lxt-battery-diagnostic-tool-c>**
— "PocketOBI – Makita 18V LXT Battery Diagnostic Tool", by The Repair Forge. The STL files
are not committed here; download them from the Printables page.

## License

The hardware design files in this folder (Gerbers, schematic, BOM) are licensed
**CC BY-NC-SA 4.0** — see [LICENSE](LICENSE). The firmware is licensed separately under the
PolyForm Noncommercial License 1.0.0 (PolyForm is a software licence, so the board files use
the Creative Commons equivalent).
