# PocketOBI — Hardware / PCB notes

This document describes the interconnect ("carrier") PCB for PocketOBI so it can
be recreated in KiCad. **Status: PCB not manufactured yet, but the wiring below
is VALIDATED** — it reads a real BL1860B correctly (2026-07).

Validated battery wiring (adapter's orange 7-position connector): **GPIO3 = DATA <-
orange plot 6** (470 ohm pull-up = R1), **GPIO4 = ENABLE <- orange plot 2** (470 ohm
pull-up = R2), **GND from the main B- terminal** (orange plot 1 unused). Identify
the ESP32 pins by their silkscreen label ("3"/"4"), NOT by position. This adapter
numbers its orange connector from the opposite end vs the canonical Makita pins.
On 3.3 V logic the DATA read line needed a lower pull-up (470 ohm worked); 4.7 k is
the reference value and may also work with the PCB's short/clean traces — keep the
DATA pull-up (R1) low if reads are flaky.

The two active parts (ESP32-C3 SuperMini and the ST7789 + EC11 module) are
ready-made boards. The PCB is just a **carrier**: it sockets both modules,
carries the two pull-up resistors, and breaks out the battery connector.

> ⚠️ **Safety:** the battery's B+ (up to ~21 V) is NEVER routed on this board.
> Only DATA, ENABLE and GND (from the B- terminal) come from the pack.

## Components (BOM)

| Ref | Part | Qty | Notes |
|-----|------|-----|-------|
| U1 | ESP32-C3 SuperMini | 1 | socketed on female headers |
| M1 | 2.4" ST7789 TFT + EC11 encoder module | 1 | the 12-pin 2-in-1 module |
| R1, R2 | 470 Ω resistor | 2 | single value for both pull-ups (see note) |
| J1 | female header, matches U1 | 1–2 | 2.54 mm pitch |
| J2 | female header, matches M1 | 1 | 2.54 mm pitch, 12 pins |
| J3 | JST-PH 6-pin, board header (B6B-PH-K, top entry) | 1 | battery signal plug; only 2 pins used |
| J4 | screw terminal 2-pin, 3.5 mm | 1 | battery GND wire from the B- terminal |
| PCB | 2-layer | 1 | ~JLCPCB/PCBWay |

## Module pin reference

**M1 (display + encoder module)** header, 12 pins:
`GND, VCC, SCL, SDA, RES, DC, CS, BLK, KO, PUSH, A, B`
- SCL = SPI clock, SDA = SPI MOSI.
- KO = secondary button, PUSH = encoder button, A/B = encoder phases.
- The buttons and encoder common are referenced to the module's GND internally,
  so only the signal pins go to the ESP32 (firmware uses internal pull-ups).

**U1 (ESP32-C3 SuperMini)** — connect by GPIO name; the physical pin order
depends on your board, so **check the silkscreen** of your exact SuperMini.

## Netlist (connection map)

Each line is one electrical net = everything that must be joined together.

```
NET 3V3      : U1.3V3, M1.VCC, M1.BLK, R1.p1, R2.p1
NET GND      : U1.GND, M1.GND, J4.p1           ; GND wire from the battery B- terminal
NET SPI_SCLK : U1.GPIO0, M1.SCL
NET SPI_MOSI : U1.GPIO1, M1.SDA
NET TFT_RES  : U1.GPIO10, M1.RES
NET TFT_DC   : U1.GPIO20, M1.DC
NET TFT_CS   : U1.GPIO21, M1.CS
NET ENC_A    : U1.GPIO5, M1.A
NET ENC_B    : U1.GPIO6, M1.B
NET ENC_PUSH : U1.GPIO7, M1.PUSH
NET BACK_KO  : U1.GPIO2, M1.KO
NET DATA     : U1.GPIO3, R1.p2, J3.p1          ; OneWire data (JST pin 1), pull-up via R1
NET ENABLE   : U1.GPIO4, R2.p2, J3.p5          ; enable (JST pin 5), pull-up via R2
No-Connect   : J3.p2, J3.p3, J3.p4, J3.p6      ; unused JST pins (incl. battery B-/signal GND)
```

Battery JST-PH pin map (adapter omits B+, so JST pin N = battery pin N+1):
- J3.p1 = battery pin 2 = **DATA** (used)
- J3.p5 = battery pin 6 = **ENABLE** (used)
- J3.p2/p3/p4/p6 = battery pins 3/4/5/7 = unused here (p4 = signal GND, p6 = B-)

Notes:
- Ground is brought in separately on **J4** from the main battery **B-** terminal
  (user's choice: sturdier than the small signal pins). Common ground between the
  ESP32 and the pack is **mandatory** for communication.
- R1 pulls DATA up to 3V3, R2 pulls ENABLE up to 3V3. **Both 470 Ω** (single BOM
  value). DATA *needs* the stronger 470 Ω (the pack loads the line near the 3.3 V
  input threshold); ENABLE's pull-up value is non-critical (ENABLE is a driven
  output), so it uses 470 Ω too for uniformity. Both resistors required.
- **No B+ (18 V) anywhere on this board** — the adapter does not route it into the
  JST, and we do not wire it. Put No-Connect flags on the unused JST pins.
- M1.BLK tied to 3V3 = backlight always on.

## Suggested footprints

| Ref | Footprint | Note |
|-----|-----------|------|
| U1 | 2× pin header 2.54 mm (or a community "ESP32-C3 SuperMini" footprint) | verify pin count/row spacing on your board |
| M1 | 1×12 pin header 2.54 mm | verify count/pitch/order on your module |
| R1, R2 | R_0805 (SMD) or R_Axial_DIN0207 (through-hole) | beginner: through-hole is easier to hand-solder |
| J3 | `Connector_JST:JST_PH_B6B-PH-K_1x06_P2.00mm_Vertical` (top entry; or `S6B-PH-K ..._Horizontal` for side entry) | matches the battery signal plug |
| J4 | `TerminalBlock_Phoenix:TerminalBlock_Phoenix_MC-1,5-2-3.5_1x02_P3.50mm` (or any 2-pin 3.5 mm you have) | GND wire from B-; wire horizontal / screw on top |

## Placement / routing guidelines

- Put J3 (battery) at one board edge, away from the ESP32; silkscreen-label
  DATA / ENABLE / GND, and add a "DO NOT CONNECT B+" note.
- Place R1/R2 next to the ESP32 GPIO3/GPIO4 pins.
- 2-layer board: pour a GND copper fill on the bottom layer, tied to NET GND.
- SPI speed is modest; trace lengths are not critical, just keep them tidy.
- Socket both modules on female headers so they stay removable/repairable.

## KiCad quick-start (absolute beginner)

1. Install **KiCad 9** from kicad.org. Open it, **File → New Project**.
2. **Schematic Editor**: place the parts (`A` to add a symbol):
   - Generic headers for U1, M1, J3 (e.g. `Conn_01x12` for M1). It's fine to use
     plain connector symbols for a carrier board.
   - Two resistors `R` for R1, R2.
3. **Wire it with net labels** (easiest method): instead of drawing wires
   everywhere, put a **net label** (shortcut `L`) on each pin using the names
   from the netlist above. KiCad connects pins that share a label. This turns
   the netlist above directly into your schematic.
4. **Assign footprints**: Tools → *Assign Footprints*, pick from the table above.
5. **PCB Editor**: *Update PCB from Schematic* (`F8`), place the parts, draw the
   board outline (Edge.Cuts), then **route** the tracks (the thin "ratsnest"
   lines show what to connect). Add a GND copper pour.
6. Run **DRC** (Design Rule Check) and fix any errors.
7. **Plot / Fabrication Outputs** → Gerbers + drill files → zip → send to a
   fab house (JLCPCB / PCBWay).

## Fabrication / JLCPCB export (KiCad 10)

Exact workflow for this 2-layer, hand-assembled board (no stencil).

**`File → Plot` — enable exactly these layers:**
`F.Cu`, `B.Cu`, `F.Silkscreen`, `B.Silkscreen`, `F.Mask`, `B.Mask`, `Edge.Cuts`.
Skip `F.Paste`/`B.Paste` (only needed for a stencil) and `F.Fab`/`B.Fab`/`User.*`.

**Plot options:** format `Gerber`; Output directory `gerbers/`; Drill marks `None`;
Scaling `1:1`; Plot mode `Filled`; keep **Plot reference designators** on; coordinate
format `4.6 mm`; leave **Use extended X2 format** at the KiCad default. Click **Plot**.

**Drill files — `Generate Drill Files…`:** format `Excellon`; tick **Merge PTH and
NPTH into one file**; Units `Millimeters`; Drill origin `Absolute`; Map file `None`.
Click **Generate Drill File**.

**Files produced in `gerbers/` (zip them together, files at the zip root):**

```
PocketOBI-F_Cu.gbr          front copper
PocketOBI-B_Cu.gbr          back copper
PocketOBI-F_Silkscreen.gbr  front silkscreen
PocketOBI-B_Silkscreen.gbr  back silkscreen
PocketOBI-F_Mask.gbr        front soldermask
PocketOBI-B_Mask.gbr        back soldermask
PocketOBI-Edge_Cuts.gbr     board outline
PocketOBI.drl               drills (PTH + NPTH merged)
```

**Upload:** zip → jlcpcb.com → *Add gerber file*; check the preview (outline, 2 layers
detected, silkscreen legible). Order settings: **Layers = 2**, thickness **1.6 mm**,
rest default.

**Before upload:** open the `gerbers/` folder in **GerbView** (KiCad's Gerber viewer)
for a final visual sanity check — what you see there is what the fab receives.

## Recommended path

The battery communication is validated on real hardware; still, it is wise to
freeze the wiring on a **perfboard** first, then commit to this PCB.
