#!/usr/bin/env python3
"""
hostsim.py - render every PocketOBI screen on the PC, in every language, without a board.

Compiles the REAL firmware UI code (the repo-root .cpp files + PocketOBI.ino) for the PC,
against the real Adafruit_GFX library, with the small fakes in shim/ (the display becomes
a GFXcanvas16 framebuffer; GPIO, Serial and the battery bus do nothing). hostsim_main.cpp
loads a fixture pack and draws each screen; this script turns the frames into PNGs.

Output (gitignored):  tools/hostsim/out/<build>/<LANG>/<nn>_<screen>.png
                      tools/hostsim/out/<build>/sheet_<LANG>.png   (all screens on one page)

Requirements: Python 3 + Pillow, a C++ compiler (default: zig, `pip install ziglang`), and
the Adafruit GFX Library as installed by the Arduino IDE or PlatformIO (found automatically,
or pass --gfx).

Usage:
    python tools/hostsim/hostsim.py            # default build (EN/FR/DE/ES)
    python tools/hostsim/hostsim.py --ja       # Japanese build (adds JA; ENABLE_CJK=1)
    python tools/hostsim/hostsim.py --ja --board cyd
"""
import argparse
import glob
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))   # firmware repo root
BOARDS = {"c3": "BOARD_LXT_C3", "cyd": "BOARD_CYD"}


def find_gfx(explicit):
    cands = [explicit] if explicit else []
    cands += glob.glob(os.path.join(ROOT, ".pio", "libdeps", "*", "Adafruit GFX Library"))
    home = os.path.expanduser("~")
    for docs in (os.path.join(home, "Documents"), os.path.join(home, "OneDrive", "Documents")):
        cands += sorted(glob.glob(os.path.join(docs, "Arduino", "libraries", "Adafruit*GFX*")), reverse=True)
    for c in cands:
        if c and os.path.isfile(os.path.join(c, "Adafruit_GFX.cpp")):
            return c
    sys.exit("Adafruit GFX Library not found: install it (Arduino IDE / pio) or pass --gfx <dir>")


def compiler(explicit):
    if explicit:
        return explicit.split()
    if shutil.which("g++"):
        return ["g++"]
    try:
        import ziglang  # noqa: F401
        return [sys.executable, "-m", "ziglang", "c++"]
    except ImportError:
        sys.exit("No C++ compiler: `pip install ziglang`, or pass --cxx")


def build(args, out):
    gfx = find_gfx(args.gfx)
    fw = sorted(f for f in glob.glob(os.path.join(ROOT, "*.cpp"))
                if os.path.basename(f) != "OneWire2.cpp")   # drives GPIO registers; stubbed in shim
    exe = os.path.join(out, "hostsim.exe" if os.name == "nt" else "hostsim")
    cmd = compiler(args.cxx) + [
        "-std=gnu++17", "-O1", "-w",
        "-DPOCKETOBI_HOSTSIM", "-DARDUINO=10813", "-DARDUINO_ARCH_ESP32=1",
        f"-DPOCKETOBI_BOARD={BOARDS[args.board]}",
        f"-DENABLE_CJK={1 if args.ja else 0}",
        "-I", os.path.join(HERE, "shim"), "-I", gfx,
        "-x", "c++", os.path.join(ROOT, "PocketOBI.ino"), "-x", "none",
        *fw,
        os.path.join(gfx, "Adafruit_GFX.cpp"),
        os.path.join(HERE, "shim", "shim_core.cpp"),
        os.path.join(HERE, "hostsim_main.cpp"),
        "-o", exe,
    ]
    print("compiling", "(JA)" if args.ja else "", "for", args.board, "...")
    r = subprocess.run(cmd)
    if r.returncode:
        sys.exit("build failed")
    return exe


def to_png(raw, out, scale):
    """Convert the raw frames in `raw` (a temp dir) into PNGs + contact sheets in `out`."""
    from PIL import Image
    for lang in sorted(os.listdir(raw)):
        d = os.path.join(raw, lang)
        if not os.path.isdir(d):
            continue
        os.makedirs(os.path.join(out, lang), exist_ok=True)
        frames = []
        for ppm in sorted(glob.glob(os.path.join(d, "*.ppm"))):
            with Image.open(ppm) as src:
                im = src.convert("RGB")
            if scale != 1:
                im = im.resize((im.width * scale, im.height * scale), Image.NEAREST)
            name = os.path.basename(ppm)[:-4]
            im.save(os.path.join(out, lang, name + ".png"))
            frames.append((name, im))
        if frames:
            contact_sheet(frames).save(os.path.join(out, f"sheet_{lang}.png"))


def contact_sheet(frames, cols=4, pad=8, label_h=16):
    from PIL import Image, ImageDraw
    w, h = frames[0][1].size
    rows = (len(frames) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * (w + pad) + pad, rows * (h + label_h + pad) + pad), (40, 40, 40))
    dr = ImageDraw.Draw(sheet)
    for i, (name, im) in enumerate(frames):
        x = pad + (i % cols) * (w + pad)
        y = pad + (i // cols) * (h + label_h + pad)
        dr.text((x, y), name, fill=(220, 220, 220))
        sheet.paste(im, (x, y + label_h))
    return sheet


def main():
    ap = argparse.ArgumentParser(description="Render PocketOBI screens on the PC")
    ap.add_argument("--ja", action="store_true", help="Japanese build (ENABLE_CJK=1)")
    ap.add_argument("--board", choices=sorted(BOARDS), default="c3")
    ap.add_argument("--scale", type=int, default=1, help="integer zoom of the PNGs")
    ap.add_argument("--gfx", help="Adafruit GFX Library folder")
    ap.add_argument("--cxx", help='C++ compiler command, e.g. "g++" or "clang++"')
    ap.add_argument("--out", help="output folder (default tools/hostsim/out/<board>[-ja])")
    args = ap.parse_args()

    out = args.out or os.path.join(HERE, "out", args.board + ("-ja" if args.ja else ""))
    shutil.rmtree(out, ignore_errors=True)       # best effort: a synced folder may hold a lock
    os.makedirs(out, exist_ok=True)
    # Build and raw frames go to a temp dir: a cloud-synced repo (Dropbox, OneDrive) locks
    # files while uploading them, which breaks the write-then-delete of the raw frames.
    with tempfile.TemporaryDirectory(prefix="hostsim-") as work:
        exe = build(args, work)
        raw = os.path.join(work, "raw")
        r = subprocess.run([exe, raw])
        if r.returncode:
            sys.exit("simulator failed")
        to_png(raw, out, args.scale)
    print("PNGs in", out)


if __name__ == "__main__":
    main()
