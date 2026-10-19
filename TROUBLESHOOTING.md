# Troubleshooting

Two common situations, each with its own section below: a build that **won't read a
pack**, and **bridge mode not connecting** to a PC.

## Won't read a pack — "Comm error" / NO RESPONSE

**Read this if PocketOBI powers up and shows the interface, but won't read a pack** —
opening Battery or Repair gives "Comm error" (NO RESPONSE), or the reads are all-`0xFF`.

That symptom means the ESP32 is getting no answer from the battery's BMS. It is almost
always **wiring, a pull-up resistor, or a contact — not the firmware**, so re-flashing the
same code will not change it.

This guide finds the fault with a **multimeter** in about 10 minutes. Do the steps **in
order** — each one clears a single suspect. For every pin identity, resistor value and the
connection map, this guide points you at **[HARDWARE.md](HARDWARE.md#netlist-connection-map)**
instead of repeating them; keep it open alongside so nothing here can drift out of date.

## Before you start

- Power PocketOBI from **USB**, never from the pack.
- You need two multimeter modes: **continuity / beep** (which doubles as ohms) and
  **DC volts**.
- ⚠️ One battery contact is **B+, at about 18 V** — it must never reach the ESP32. The
  connection map in [HARDWARE.md](HARDWARE.md#netlist-connection-map) shows which pins are
  used (DATA, ENABLE, GND) and which stay unconnected; Step 0 confirms B+ on your own pack
  so you can keep clear of it.

## Step 0 — Find B+ so you can stay away from it (DC volts)

On the bare pack, black probe on the main **B−** terminal, red probe on each contact in turn:

- One reads **~18–20 V** → that is **B+**. Note where it is; it stays disconnected.
- The signal contacts read near 0 V.

## Step 1 — Is the board actually powered? (DC volts)

Board on USB. Measure the **3.3 V rail** (black on a GND pin, red on the 3V3 pin) → expect
**~3.3 V**. No 3.3 V → a USB or board power fault; fix that first, nothing downstream can
work without it.

## Step 2 — Pull-ups present and correct (ohms, power OFF)

The DATA and ENABLE lines each need a pull-up to 3.3 V. With the board off, measure
resistance **from each line to the 3V3 rail** → you should read the pull-up resistor. The
**exact value, and which resistor is which, are in
[HARDWARE.md](HARDWARE.md#netlist-connection-map)** — check yours matches.

- **Open / no reading** → the pull-up isn't connected, or there's a cold solder joint.
- **Reads much higher than the specified value, and you're on a breadboard** → that is the
  marginal case HARDWARE.md warns about. Fit the specified value. **A weak pull-up on DATA
  is the single most common cause of "won't read".**

## Step 3 — DATA line idles high (DC volts, board on USB, NO battery)

Black on GND, red on the **DATA line** → expect **~3.3 V** at rest, held there by its
pull-up. ~0 V or a wandering value → DATA is shorted to GND or its pull-up is dead, and it
will always report "Comm error".

*(Don't voltage-test ENABLE this way — the ESP32 drives it, so it sits low at rest; its
pull-up was already checked in Step 2.)*

## Step 4 — Continuity ESP32 → adapter, and the #1 gotcha (beep, power OFF)

Confirm each signal actually reaches the pack. Using the connection map in
[HARDWARE.md](HARDWARE.md#netlist-connection-map), beep out:

- ESP32 **DATA pin** ↔ the adapter's **DATA** contact
- ESP32 **ENABLE pin** ↔ the adapter's **ENABLE** contact
- ESP32 **GND** ↔ pack **B−** (a common ground is mandatory)

**The most common reason a perfect-looking build gets "Comm error" is DATA and
ENABLE swapped.** If the continuity above lands on the wrong contacts, **swap the two leads
and re-test.** The adapter's connector can be numbered in a way that makes this easy to get
wrong — always identify the ESP32 side by its **silkscreen label**, never by physical
position (see [HARDWARE.md](HARDWARE.md#module-pin-reference)).

## Step 5 — Ground quality with the pack clipped in (ohms)

Clip the adapter onto the pack and measure **ESP32 GND ↔ pack B−** → expect **under ~1 Ω**.
A few ohms of flaky ground is the classic "reads once, then nothing". Ground should come
from the **main B− terminal**, not a thin signal pin (see
[HARDWARE.md](HARDWARE.md#netlist-connection-map)).

## Step 6 — Everything passed and it still won't read

A basic multimeter can't see the fast one-wire pulses, so the static checks end here.
What's left:

- **Marginal pull-up** — if you haven't already, fit the value HARDWARE.md specifies for the
  DATA line.
- **The serial log tells the rest.** Set **`COMM_DEBUG` to `1`** in `pocketobi.h`,
  reflash, open the serial monitor at **115200**, and read a pack. It prints the presence
  flag and the raw bytes:
  - **no presence** → back to wiring / pull-up (Steps 2–5);
  - **presence but garbage / all-`0xFF`** → a marginal contact, or a pack whose BMS genuinely
    isn't responding (dead, or not an OBI-compatible pack).

## Symptom → most likely cause

| Symptom | Look at |
|---|---|
| No 3.3 V on the board | Power / USB (Step 1) |
| Pull-up open or wrong value | Step 2 |
| DATA not ~3.3 V at rest | Short to GND or dead pull-up (Step 3) |
| Continuity lands on the wrong contacts | **DATA / ENABLE swapped** (Step 4) |
| Reads once, then "Comm error" | Flaky ground (Step 5) |
| All checks pass, still nothing | Pull-up value, or read the serial log (Step 6) |

## Bridge mode won't connect (PC app / PackScope)

You put PocketOBI in **PC bridge** mode, the PC sees the COM port, you click Connect — and it
just hangs ("opening… / waiting for connect"). Work through these in order:

1. **Did you flash with `USB CDC On Boot: Enabled`?** This is the #1 cause when you built it
   yourself. Without it the ESP32-C3 can't carry the serial link over USB, so the app waits
   forever even though a COM port shows up. Re-flash with it turned on — it is set in the
   Arduino IDE before uploading (see the Build & flash steps in the
   [README](README.md#build--flash)).
2. **Is the device on the PC bridge screen, showing ACTIVE in green?** PocketOBI only answers
   the PC while that screen is up. Turning the encoder there toggles it (on the CYD: tap the
   screen); make sure it is not showing OFF.
3. **Does opening the port reboot the board out of bridge mode?** Most PC apps pulse the USB
   reset line when they open the port, which reboots the ESP32-C3 back to the menu, so the
   handshake never lands. Two fixes:
   - On the device, **Tools → Settings → "PC bridge at boot" = ON**, then reconnect — after the
     reset it boots straight back into bridge mode and answers.
   - Or use **[PackScope](https://github.com/TheRepairforge/PackScope)**, our companion app,
     which opens the port **without** resetting the board.
4. **Is another program holding the COM port?** The Arduino IDE **Serial Monitor** will keep the
   port and block the app — close it. And use a **data** USB cable, not charge-only.

---

Still stuck? Open an issue with a **photo of your wiring** and, if you can, the serial log:
<https://github.com/TheRepairforge/PocketOBI/issues/new/choose>
