# hostsim: PocketOBI screens on your PC

Renders every screen of the firmware, in every language, as PNG images, **without a
board**. It compiles the real UI code (`display.cpp`, `cjk_render.cpp`, the string
tables, ...) for the PC against the real Adafruit_GFX library. The images are what the
device draws, pixel for pixel: same fonts, same coordinates, same clipping.

Use it to:
- check that a translation fits (Japanese especially, see `docs/TRANSLATING.md`);
- preview a UI change before flashing;
- make screenshots for the README or a video.

## Run

```
pip install pillow ziglang                 # once: image library + a C++ compiler
python tools/hostsim/hostsim.py            # EN / FR / DE / ES
python tools/hostsim/hostsim.py --ja       # same + Japanese (ENABLE_CJK=1 build)
python tools/hostsim/hostsim.py --ja --board cyd --scale 2
```

Output, regenerated on each run and not committed:

```
tools/hostsim/out/<board>[-ja]/sheet_<LANG>.png        all screens on one page
tools/hostsim/out/<board>[-ja]/<LANG>/<nn>_<screen>.png one image per screen
```

You also need the **Adafruit GFX Library**. If you have built PocketOBI with the Arduino
IDE or PlatformIO it is already installed and found automatically; otherwise pass
`--gfx <folder>`. Any C++17 compiler works in place of zig: `--cxx g++`.

## How it works

| File | Role |
|---|---|
| `shim/` | Fakes of the hardware-facing libraries: `Adafruit_ST7789` is a `GFXcanvas16` RAM framebuffer; GPIO, `Serial`, preferences, encoder, touch and the battery bus do nothing; time only advances when the code waits. |
| `hostsim_main.cpp` | Loads a fixture pack (illustrative values, not a real capture), sets the navigation state of each screen, calls the firmware's `render()`, saves the frame. |
| `hostsim.py` | Finds the compiler and Adafruit_GFX, builds, runs, converts to PNG, makes the contact sheets. |

To add a screen or a scenario (for example a locked pack), add a line to `SCREENS` in
`hostsim_main.cpp`.

**Every `.cpp` here is wrapped whole in `#ifdef POCKETOBI_HOSTSIM`.** PlatformIO compiles
every source file under the repository root (`src_dir = .`), this folder included; the
guard makes these files empty in a firmware build. Keep it on any new `.cpp`, and never
copy a `shim/` header to the repository root, where it would hide the real library.
