#!/usr/bin/env python3
"""
gen_cjk_glyphs.py - build glyphs_ja.h from the Japanese strings in strings_ja.h.

Reads every string literal of STRTAB_JA in strings_ja.h, collects the non-ASCII
code points they use, rasterises each one from a TrueType/OpenType font into
1-bpp bitmaps in two cells, 16x16 (next to the GFX fonts and the classic font at
size 2) and 12x12 (classic font at size 1), and writes glyphs_ja.h (data only,
compiled when ENABLE_CJK=1; see DECISIONS.md D19/D33). Only the characters
actually used are extracted, so the table stays small.

It also checks strings_ja.h and stops with an error when:
  - it does not have exactly one entry per StrId of strings_i18n.h, in enum order;
  - a string is longer than the firmware's text buffers (MAX_BYTES UTF-8 bytes).

Bitmap format matches Adafruit_GFX drawBitmap(): row-major, 2 bytes per row,
MSB = leftmost pixel. Set bits are drawn in the requested colour.

FONTS. The glyph SHAPES carry the source font's licence. The shipped glyphs come
from two fonts, both SIL Open Font License 1.1:
  - 16 px cell: Noto Sans CJK JP Medium. Bold matched the FreeSans Bold labels better
    but merged the strokes of complex kanji on the panel; Medium keeps them apart
    (checked on the CYD, JP 2026-10-01).
  - 12 px cell: Fusion Pixel 12px monospaced, Japanese variant (TakWolf). An outline
    font rasterised to 12x12 at 1 bpp merges the strokes of complex kanji (識, 警, 難);
    a pixel font is drawn on that grid, one blank pixel between strokes. Fusion rather
    than Ark Pixel (same author): Ark lacks some common kanji (処 変 既 警).
Download them once into tools/.fonts/ (gitignored):
    https://github.com/notofonts/noto-cjk/raw/main/Sans/SubsetOTF/JP/NotoSansJP-Medium.otf
    https://github.com/TakWolf/fusion-pixel-font/releases  ->  fusion-pixel-font-12px-
        monospaced-otf-v*.zip, keep fusion-pixel-12px-monospaced-ja.otf
When they are missing the script falls back to a Windows font (MS Gothic / Yu Gothic /
Meiryo). Those are NOT redistributable: such a glyphs_ja.h is for development only
and must never be committed.

A character the font does not have would silently come out as its "missing glyph"
box; the script stops with an error instead.

Usage:
    python tools/gen_cjk_glyphs.py                 # shipped fonts from tools/.fonts, writes ../glyphs_ja.h
    python tools/gen_cjk_glyphs.py --font tools/.fonts/NotoSansJP-Medium.otf --font12 tools/.fonts/fusion-pixel-12px-monospaced-ja.otf
    python tools/gen_cjk_glyphs.py --threshold 110 --preview
    python tools/gen_cjk_glyphs.py --check         # only validate strings_ja.h
"""
import argparse
import os
import re
import sys
import unicodedata

CELLS = (16, 12)    # glyph cells in px, see the module docstring
MAX_BYTES = 63      # longest string the firmware's 64-byte text buffers take (JA = 3 B/char)
EM_BASELINE = 0.88  # baseline position in the em box of a CJK font

HERE = os.path.dirname(os.path.abspath(__file__))
SHIPPED = {16: os.path.join(HERE, ".fonts", "NotoSansJP-Medium.otf"),               # OFL
           12: os.path.join(HERE, ".fonts", "fusion-pixel-12px-monospaced-ja.otf")}  # OFL
SHIPPED_LICENCE = "SIL Open Font License 1.1 (Noto Sans CJK JP, Fusion Pixel; see THIRD-PARTY.md)"
DEV_FONTS = [       # fallback when Noto is absent; first that exists wins; DEV-ONLY
    # MS Gothic first: it is the crispest full-width face at a 16px cell (its
    # gothic bitmap heritage fills the grid where Yu Gothic renders thin).
    (r"C:\Windows\Fonts\msgothic.ttc", 0),
    (r"C:\Windows\Fonts\YuGothM.ttc", 0),
    (r"C:\Windows\Fonts\meiryo.ttc", 0),
]

LIT = r'"((?:[^"\\]|\\.)*)"'


def strid_keys(i18n_src):
    m = re.search(r"enum\s+StrId\s*\{(.*?)\};", i18n_src, re.DOTALL)
    if not m:
        sys.exit("error: enum StrId not found in strings_i18n.h")
    body = re.sub(r"//[^\n]*", "", m.group(1))
    return [k for k in re.findall(r"S_[A-Z0-9_]+", body) if k != "S_COUNT"]


def parse_ja(ja_src):
    """Return [(StrId label, JA string or None)] in file order."""
    m = re.search(r"STRTAB_JA\[\]\s*=\s*\{(.*?)\n\};", ja_src, re.DOTALL)
    if not m:
        sys.exit("error: STRTAB_JA initializer not found in strings_ja.h")
    rows = []
    for line in m.group(1).splitlines():
        line = line.split("//", 1)[0]
        lab = re.search(r"/\*\s*(S_[A-Z0-9_]+)\s*\*/", line)
        if not lab:
            continue
        lits = re.findall(LIT, line)
        rows.append((lab.group(1), lits[0] if lits else None))
    return rows


def check_rows(rows, keys):
    labels = [r[0] for r in rows]
    if labels != keys:
        missing = [k for k in keys if k not in labels]
        extra = [k for k in labels if k not in keys]
        first = next((i for i, (a, b) in enumerate(zip(labels, keys)) if a != b), None)
        msg = "error: strings_ja.h does not match enum StrId"
        if missing: msg += f"\n  missing: {missing}"
        if extra:   msg += f"\n  unknown: {extra}"
        if first is not None and not missing and not extra:
            msg += f"\n  order differs at row {first}: {labels[first]} (expected {keys[first]})"
        sys.exit(msg)
    long = [(k, len(v.encode("utf-8"))) for k, v in rows if v and len(v.encode("utf-8")) > MAX_BYTES]
    if long:
        sys.exit(f"error: too long for the firmware buffers (max {MAX_BYTES} UTF-8 bytes, a "
                 "Japanese character is 3): " + ", ".join(f"{k}={n}" for k, n in long))


def pick_fonts(font, font12):
    """Return {cell: font path}."""
    if font:
        return {16: font, 12: font12 or font}
    if all(os.path.exists(p) for p in SHIPPED.values()):
        return dict(SHIPPED)
    for path, _ in DEV_FONTS:
        if os.path.exists(path):
            print("WARNING: the shipped fonts are not all in tools/.fonts - using a DEV-ONLY Windows font;"
                  " do not commit the result (see the module docstring)")
            return {16: path, 12: path}
    sys.exit("No JP font found: download the shipped fonts into tools/.fonts (see the module docstring)")


def unique_codepoints(rows):
    seen = {}
    for _, s in rows:
        for ch in s or "":
            if ord(ch) >= 0x80:          # ASCII stays on the font path, never in the table
                seen.setdefault(ord(ch), ch)
    return dict(sorted(seen.items()))


def rasterise(font, cp, threshold, cell):
    from PIL import Image, ImageDraw
    ch = chr(cp)
    img = Image.new("L", (cell, cell), 0)
    d = ImageDraw.Draw(img)
    # Place the character on the font's em box, NOT centred on its ink: the em box keeps
    # small kana (ッ), punctuation (。、) and the long-vowel mark where the font puts them.
    # CJK fonts put the baseline at 0.88 em (ideographic em box: 880/-120 per 1000). A pixel
    # font drawn for this cell (ascent + descent == cell) puts it at its ascent instead.
    ascent, descent = font.getmetrics()
    base = ascent if ascent + descent == cell else round(cell * EM_BASELINE)
    adv = font.getlength(ch)
    d.text(((cell - adv) / 2, base), ch, fill=255, font=font, anchor="ls")
    px = img.load()
    row_bytes = (cell + 7) // 8
    data = bytearray()
    for y in range(cell):
        for xb in range(row_bytes):
            b = 0
            for bit in range(8):
                x = xb * 8 + bit
                if x < cell and px[x, y] >= threshold:
                    b |= 1 << (7 - bit)
            data.append(b)
    return bytes(data), img


def mask_sig(font, ch):
    m = font.getmask(ch)
    return m.size, bytes(m.getpixel((x, y)) for y in range(m.size[1]) for x in range(m.size[0]))


def ascii_preview(img):
    px = img.load()
    return "\n".join("".join("#" if px[x, y] >= 128 else "." for x in range(img.width))
                     for y in range(img.height))


def emit_header(tables, paths, threshold, out_path):
    """tables = {cell: {cp: (ch, bytes)}}; every table has the same code points.
    paths = {cell: font file}."""
    count = len(next(iter(tables.values())))
    total = sum(len(data) for t in tables.values() for _, data in t.values())
    shipped = {os.path.basename(p) for p in SHIPPED.values()}
    dev_only = any(os.path.basename(p) not in shipped for p in paths.values())
    names = ", ".join(f"{c}px {os.path.basename(p)}" for c, p in paths.items())
    lines = [
        "#pragma once",
        "// glyphs_ja.h - AUTO-GENERATED by tools/gen_cjk_glyphs.py from strings_ja.h.",
        "// Do not edit by hand: edit strings_ja.h, then regenerate.",
        "//",
        "// 1-bpp Japanese glyphs for the ENABLE_CJK build (DECISIONS.md D19/D33), two cells:",
        "// 16x16 (next to GFX fonts and classic size >= 2) and 12x12 (classic size 1).",
        "// Bitmap layout = Adafruit_GFX drawBitmap(): 2 bytes/row, MSB = leftmost pixel.",
        "// Tables are sorted by code point for the binary search in cjk_render.cpp.",
        "//",
        f"// Source font : {names}",
        ("// Licence     : DEV-ONLY, not redistributable - ship must use Noto Sans CJK JP (OFL)"
         if dev_only else "// Licence     : " + SHIPPED_LICENCE),
        f"// Threshold   : {threshold}",
        f"// Glyphs      : {count} code points x {len(tables)} sizes ({total} bytes)",
        "// Regenerate  : python tools/gen_cjk_glyphs.py",
        "",
        "#include <stdint.h>",
        "",
        f"#define CJK_GLYPH_COUNT {count}",
    ]
    for cell in tables:
        nbytes = ((cell + 7) // 8) * cell
        lines += [f"#define CJK_GLYPH{cell}_W {cell}",
                  f"struct CjkGlyph{cell} {{ uint16_t cp; uint8_t bmp[{nbytes}]; }};"]
    lines += ["",
              "// ESP32 maps flash .rodata, so a plain const array is directly addressable;",
              "// drawBitmap's pgm_read_byte resolves to a normal load on this target."]
    for cell, t in tables.items():
        lines.append(f"static const CjkGlyph{cell} CJK_GLYPHS{cell}[] = {{")
        for cp, (ch, data) in t.items():
            body = ", ".join(f"0x{b:02X}" for b in data)
            lines.append(f"  {{ 0x{cp:04X}, {{ {body} }} }},  // {ch}  {unicodedata.name(ch, '?')}")
        lines += ["};", ""]
    with open(out_path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))
    return total


def main():
    try:
        sys.stdout.reconfigure(encoding="utf-8")  # console may default to cp1252 on Windows
    except Exception:
        pass
    root = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
    ap = argparse.ArgumentParser(description="strings_ja.h -> glyphs_ja.h")
    ap.add_argument("--font", help="TTF/TTC/OTF path (default: a Windows JP font, dev-only)")
    ap.add_argument("--font12", help="different font for the 12 px cell (default: --font)")
    ap.add_argument("--index", type=int, default=None, help="face index in a .ttc/.otc")
    ap.add_argument("--threshold", type=int, default=128, help="1-bpp cutoff 0-255")
    ap.add_argument("--out", default=os.path.join(root, "glyphs_ja.h"), help="output header path")
    ap.add_argument("--preview", action="store_true", help="print ASCII art per glyph")
    ap.add_argument("--check", action="store_true", help="validate strings_ja.h only, write nothing")
    args = ap.parse_args()

    with open(os.path.join(root, "strings_i18n.h"), encoding="utf-8") as f:
        keys = strid_keys(f.read())
    with open(os.path.join(root, "strings_ja.h"), encoding="utf-8") as f:
        rows = parse_ja(f.read())
    check_rows(rows, keys)
    done = sum(1 for _, s in rows if s)
    cps = unique_codepoints(rows)
    print(f"strings_ja.h : {done}/{len(keys)} strings translated, {len(cps)} distinct glyphs")
    if args.check:
        return

    from PIL import ImageFont
    paths = pick_fonts(args.font, args.font12)
    tables = {}
    for cell in CELLS:
        font = ImageFont.truetype(paths[cell], size=cell, index=args.index or 0)
        tables[cell] = {}
        notdef = mask_sig(font, "􏿽")      # what the font draws for a missing character
        missing = [ch for ch in cps.values() if mask_sig(font, ch) == notdef]
        if missing:
            sys.exit(f"error: {os.path.basename(paths[cell])} has no glyph for: {''.join(missing)}")
        for cp, ch in cps.items():
            data, img = rasterise(font, cp, args.threshold, cell)
            tables[cell][cp] = (ch, data)
            if args.preview:
                print(f"\n{cell}px U+{cp:04X} {ch} {unicodedata.name(ch, '?')}\n{ascii_preview(img)}")

    total = emit_header(tables, paths, args.threshold, args.out)
    print("Fonts  : " + ", ".join(f"{c}px {os.path.basename(p)}" for c, p in paths.items()))
    print(f"Glyphs : {len(cps)} code points x {len(CELLS)} sizes, {total} bytes")
    print(f"Wrote  : {args.out}")


if __name__ == "__main__":
    main()
