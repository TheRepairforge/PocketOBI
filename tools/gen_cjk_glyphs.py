#!/usr/bin/env python3
"""
gen_cjk_glyphs.py - build glyphs_<lang>.h from the strings in strings_<lang>.h.

--lang picks the language: ja (Japanese, the default) or ko (Korean) (DECISIONS.md D58).
Below, "ja" stands for whichever language is picked: strings_ko.h, STRTAB_KO, glyphs_ko.h...

The firmware tree is this repository by default; --tree points at another firmware tree
that holds the same strings_i18n.h / strings_ja.h pair. If the tree holds toolcat_ja.h
(an extra TOOLCAT_JA[] table of names that are not StrIds), its strings feed the same
glyph set. PocketOBI has none.

Reads every string literal of STRTAB_JA in strings_ja.h, collects the non-ASCII
code points they use, rasterises each one from a TrueType/OpenType font into
1-bpp bitmaps in two cells, 16x16 (next to the GFX fonts and the classic font at
size 2) and 12x12 (classic font at size 1), and writes glyphs_ja.h (data only,
compiled when ENABLE_CJK=1; see DECISIONS.md D19/D33). Only the characters
actually used are extracted, so the table stays small.

It also checks strings_ja.h and stops with an error when:
  - it does not have exactly one entry per StrId of strings_i18n.h, in enum order;
  - a string is longer than the firmware's text buffers (MAX_BYTES UTF-8 bytes).
--check does that and also tells whether glyphs_ja.h is current (same code points as the
strings, right Regenerate line), without fonts; it writes nothing.

Bitmap format matches Adafruit_GFX drawBitmap(): row-major, 2 bytes per row,
MSB = leftmost pixel. Set bits are drawn in the requested colour.

FONTS. The glyph SHAPES carry the source font's licence. The shipped glyphs come
from two fonts per language, all SIL Open Font License 1.1. Japanese:
  - 16 px cell: Noto Sans CJK JP Medium. Bold matched the FreeSans Bold labels better
    but merged the strokes of complex kanji on the panel; Medium keeps them apart
    (checked on the CYD, JP 2026-10-01).
  - 12 px cell: Fusion Pixel 12px monospaced, Japanese variant (TakWolf). An outline
    font rasterised to 12x12 at 1 bpp merges the strokes of complex kanji (識, 警, 難);
    a pixel font is drawn on that grid, one blank pixel between strokes. Fusion rather
    than Ark Pixel (same author): Ark lacks some common kanji (処 変 既 警).
Korean: the same two families, Korean variants:
  - 16 px cell: Noto Sans CJK KR Medium.
  - 12 px cell: Fusion Pixel 12px monospaced, Korean variant (its Hangul is Galmuri, OFL).
Download them once into tools/.fonts/, next to this script (gitignored):
    https://github.com/notofonts/noto-cjk/raw/main/Sans/SubsetOTF/JP/NotoSansJP-Medium.otf
    https://github.com/notofonts/noto-cjk/raw/main/Sans/SubsetOTF/KR/NotoSansKR-Medium.otf
    https://github.com/TakWolf/fusion-pixel-font/releases  ->  fusion-pixel-font-12px-
        monospaced-otf-v*.zip, keep fusion-pixel-12px-monospaced-ja.otf and -ko.otf
When they are missing the script falls back to a Windows font (MS Gothic / Yu Gothic /
Meiryo; Malgun Gothic for Korean). Those are NOT redistributable: such a glyphs_*.h is
for development only and must never be committed.

A character the font does not have would silently come out as its "missing glyph"
box; the script stops with an error instead.

Usage:
    python tools/gen_cjk_glyphs.py                 # shipped fonts from tools/.fonts, writes ../glyphs_ja.h
    python tools/gen_cjk_glyphs.py --lang ko       # Korean: strings_ko.h -> glyphs_ko.h
    python tools/gen_cjk_glyphs.py --font tools/.fonts/NotoSansJP-Medium.otf --font12 tools/.fonts/fusion-pixel-12px-monospaced-ja.otf
    python tools/gen_cjk_glyphs.py --threshold 110 --preview
    python tools/gen_cjk_glyphs.py --check         # validate strings_ja.h; is glyphs_ja.h current?
    python tools/gen_cjk_glyphs.py --tree ../<other tree>   # another firmware tree
"""
import argparse
import os
import re
import sys
import unicodedata

CELLS = (16, 12)    # glyph cells in px, see the module docstring
MAX_BYTES = 63      # longest string the firmware's 64-byte text buffers take (JA/KO = 3 B/char)
EM_BASELINE = 0.88  # baseline position in the em box of a CJK font

HERE = os.path.dirname(os.path.abspath(__file__))
FONTS_DIR = os.path.join(HERE, ".fonts")
# Per language: name, shipped fonts per cell (all OFL), licence line, DEV-ONLY fallbacks
# (first that exists wins). MS Gothic first for JA: it is the crispest full-width face at a
# 16px cell (its gothic bitmap heritage fills the grid where Yu Gothic renders thin).
LANGS = {
    "ja": {"name": "Japanese",
           "shipped": {16: "NotoSansJP-Medium.otf", 12: "fusion-pixel-12px-monospaced-ja.otf"},
           "licence": "SIL Open Font License 1.1 (Noto Sans CJK JP, Fusion Pixel; see THIRD-PARTY.md)",
           "dev": [r"C:\Windows\Fonts\msgothic.ttc", r"C:\Windows\Fonts\YuGothM.ttc",
                   r"C:\Windows\Fonts\meiryo.ttc"]},
    "ko": {"name": "Korean",
           "shipped": {16: "NotoSansKR-Medium.otf", 12: "fusion-pixel-12px-monospaced-ko.otf"},
           "licence": "SIL Open Font License 1.1 (Noto Sans CJK KR, Fusion Pixel; see THIRD-PARTY.md)",
           "dev": [r"C:\Windows\Fonts\malgun.ttf"]},
}
LANG = LANGS["ja"]  # set by main() from --lang

LIT = r'"((?:[^"\\]|\\.)*)"'


def strid_keys(i18n_src):
    m = re.search(r"enum\s+StrId\s*\{(.*?)\};", i18n_src, re.DOTALL)
    if not m:
        sys.exit("error: enum StrId not found in strings_i18n.h")
    body = re.sub(r"//[^\n]*", "", m.group(1))
    return [k for k in re.findall(r"S_[A-Z0-9_]+", body) if k != "S_COUNT"]


def parse_ja(ja_src, code="ja"):
    """Return [(StrId label, string or None)] in file order, from STRTAB_<CODE>."""
    m = re.search(r"STRTAB_%s\[\]\s*=\s*\{(.*?)\n\};" % code.upper(), ja_src, re.DOTALL)
    if not m:
        sys.exit(f"error: STRTAB_{code.upper()} initializer not found in strings_{code}.h")
    rows = []
    for line in m.group(1).splitlines():
        line = line.split("//", 1)[0]
        lab = re.search(r"/\*\s*(S_[A-Z0-9_]+)\s*\*/", line)
        if not lab:
            continue
        lits = re.findall(LIT, line)
        rows.append((lab.group(1), lits[0] if lits else None))
    return rows


def check_rows(rows, keys, code="ja"):
    labels = [r[0] for r in rows]
    if labels != keys:
        missing = [k for k in keys if k not in labels]
        extra = [k for k in labels if k not in keys]
        first = next((i for i, (a, b) in enumerate(zip(labels, keys)) if a != b), None)
        msg = f"error: strings_{code}.h does not match enum StrId"
        if missing: msg += f"\n  missing: {missing}"
        if extra:   msg += f"\n  unknown: {extra}"
        if first is not None and not missing and not extra:
            msg += f"\n  order differs at row {first}: {labels[first]} (expected {keys[first]})"
        sys.exit(msg)
    long = [(k, len(v.encode("utf-8"))) for k, v in rows if v and len(v.encode("utf-8")) > MAX_BYTES]
    if long:
        sys.exit(f"error: too long for the firmware buffers (max {MAX_BYTES} UTF-8 bytes, a "
                 "CJK character is 3): " + ", ".join(f"{k}={n}" for k, n in long))


def shipped_paths():
    return {c: os.path.join(FONTS_DIR, f) for c, f in LANG["shipped"].items()}


def pick_fonts(font, font12):
    """Return {cell: font path}."""
    if font:
        return {16: font, 12: font12 or font}
    shipped = shipped_paths()
    if all(os.path.exists(p) for p in shipped.values()):
        return shipped
    for path in LANG["dev"]:
        if os.path.exists(path):
            print("WARNING: the shipped fonts are not all in tools/.fonts - using a DEV-ONLY Windows font;"
                  " do not commit the result (see the module docstring)")
            return {16: path, 12: path}
    sys.exit(f"No {LANG['name']} font found: download the shipped fonts into tools/.fonts "
             "(see the module docstring)")


def parse_toolcat(src, code="ja"):
    """Return [("TOOLCAT", string or None)] from a toolcat_<code>.h TOOLCAT_<CODE>[] initializer."""
    m = re.search(r"TOOLCAT_%s\[\]\s*=\s*\{(.*?)\n\};" % code.upper(), src, re.DOTALL)
    if not m:
        sys.exit(f"error: TOOLCAT_{code.upper()} initializer not found in toolcat_{code}.h")
    rows = []
    for line in m.group(1).splitlines():
        lits = re.findall(LIT, line.split("//", 1)[0])
        if lits and len(lits[0].encode("utf-8")) > MAX_BYTES:
            sys.exit(f"error: toolcat_{code}.h string too long: " + lits[0])
        rows.append(("TOOLCAT", lits[0] if lits else None))
    return rows


def regen_command(tree, code="ja"):
    """The Regenerate line: the command that is true in the tree's own repository.
    Japanese is the default language, so its line carries no --lang."""
    tree = os.path.abspath(tree)
    opt = "" if code == "ja" else f" --lang {code}"
    if os.path.normcase(tree) == os.path.normcase(os.path.normpath(os.path.join(HERE, ".."))):
        return "python tools/gen_cjk_glyphs.py" + opt
    script = os.path.relpath(os.path.abspath(__file__), os.path.dirname(tree)).replace(os.sep, "/")
    return f"python {script} --tree {os.path.basename(tree)}{opt}"


def header_state(path):
    """(code points in the glyph tables, Regenerate command) of an existing glyphs_<lang>.h."""
    with open(path, encoding="utf-8") as f:
        src = f.read()
    cps = {int(h, 16) for h in re.findall(r"^\s*\{ 0x([0-9A-F]{4,6}),", src, re.MULTILINE)}
    m = re.search(r"^// Regenerate  : (.*)$", src, re.MULTILINE)
    return cps, (m.group(1).strip() if m else None)


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


def emit_header(tables, paths, threshold, out_path, regen, code="ja"):
    """tables = {cell: {cp: (ch, bytes)}}; every table has the same code points.
    paths = {cell: font file}. Symbols carry the language (CJK_GLYPHS16_JA, ...) so several
    headers can be compiled together; the structs are shared."""
    count = len(next(iter(tables.values())))
    total = sum(len(data) for t in tables.values() for _, data in t.values())
    shipped = set(LANG["shipped"].values())
    dev_only = any(os.path.basename(p) not in shipped for p in paths.values())
    names = ", ".join(f"{c}px {os.path.basename(p)}" for c, p in paths.items())
    sfx = code.upper()
    lines = [
        "#pragma once",
        f"// glyphs_{code}.h - AUTO-GENERATED by tools/gen_cjk_glyphs.py from strings_{code}.h.",
        f"// Do not edit by hand: edit strings_{code}.h, then regenerate.",
        "//",
        f"// 1-bpp {LANG['name']} glyphs for the ENABLE_CJK build (DECISIONS.md D19/D33/D58), two cells:",
        "// 16x16 (next to GFX fonts and classic size >= 2) and 12x12 (classic size 1).",
        "// Bitmap layout = Adafruit_GFX drawBitmap(): 2 bytes/row, MSB = leftmost pixel.",
        "// Tables are sorted by code point for the binary search in cjk_render.cpp.",
        "//",
        f"// Source font : {names}",
        ("// Licence     : DEV-ONLY, not redistributable - ship must use the OFL fonts of the docstring"
         if dev_only else "// Licence     : " + LANG["licence"]),
        f"// Threshold   : {threshold}",
        f"// Glyphs      : {count} code points x {len(tables)} sizes ({total} bytes)",
        f"// Regenerate  : {regen}",
        "",
        "#include <stdint.h>",
        "",
        f"#define CJK_GLYPH_COUNT_{sfx} {count}",
        "",
        "#ifndef CJK_GLYPH_STRUCTS",
        "#define CJK_GLYPH_STRUCTS",
    ]
    for cell in tables:
        nbytes = ((cell + 7) // 8) * cell
        lines += [f"#define CJK_GLYPH{cell}_W {cell}",
                  f"struct CjkGlyph{cell} {{ uint16_t cp; uint8_t bmp[{nbytes}]; }};"]
    lines += ["#endif",
              "",
              "// ESP32 maps flash .rodata, so a plain const array is directly addressable;",
              "// drawBitmap's pgm_read_byte resolves to a normal load on this target."]
    for cell, t in tables.items():
        lines.append(f"static const CjkGlyph{cell} CJK_GLYPHS{cell}_{sfx}[] = {{")
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
    ap = argparse.ArgumentParser(description="strings_<lang>.h -> glyphs_<lang>.h")
    ap.add_argument("--lang", choices=sorted(LANGS), default="ja",
                    help="language (default ja): strings_<lang>.h -> glyphs_<lang>.h")
    ap.add_argument("--tree", default=os.path.join(HERE, ".."),
                    help="firmware tree with strings_i18n.h + strings_ja.h (default: this repository)")
    ap.add_argument("--font", help="TTF/TTC/OTF path (default: a Windows JP font, dev-only)")
    ap.add_argument("--font12", help="different font for the 12 px cell (default: --font)")
    ap.add_argument("--index", type=int, default=None, help="face index in a .ttc/.otc")
    ap.add_argument("--threshold", type=int, default=128, help="1-bpp cutoff 0-255")
    ap.add_argument("--out", default=None, help="output header path (default <tree>/glyphs_<lang>.h)")
    ap.add_argument("--preview", action="store_true", help="print ASCII art per glyph")
    ap.add_argument("--check", action="store_true",
                    help="validate strings_<lang>.h and tell whether glyphs_<lang>.h is current; write nothing")
    args = ap.parse_args()
    global LANG
    code = args.lang
    LANG = LANGS[code]
    sname, gname = f"strings_{code}.h", f"glyphs_{code}.h"
    root = os.path.normpath(os.path.abspath(args.tree))
    if args.out is None:
        args.out = os.path.join(root, gname)
    regen = regen_command(root, code)

    with open(os.path.join(root, "strings_i18n.h"), encoding="utf-8") as f:
        keys = strid_keys(f.read())
    with open(os.path.join(root, sname), encoding="utf-8") as f:
        rows = parse_ja(f.read(), code)
    check_rows(rows, keys, code)
    done = sum(1 for _, s in rows if s)
    extra = os.path.join(root, f"toolcat_{code}.h")
    if os.path.exists(extra):
        with open(extra, encoding="utf-8") as f:
            rows += parse_toolcat(f.read(), code)
    cps = unique_codepoints(rows)
    print(f"{sname} : {done}/{len(keys)} strings translated, {len(cps)} distinct glyphs")
    if args.check:
        if not os.path.exists(args.out):
            sys.exit(f"error: {args.out} missing (regenerate: {regen})")
        have, cmd = header_state(args.out)
        if have != set(cps) or cmd != regen:
            why = []
            if have != set(cps):
                why.append(f"{len(set(cps) - have)} glyph(s) missing, {len(have - set(cps))} unused")
            if cmd != regen:
                why.append(f"Regenerate line is '{cmd}'")
            sys.exit(f"error: {gname} stale ({'; '.join(why)}) - regenerate: {regen}")
        print(f"{gname}  : current")
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

    total = emit_header(tables, paths, args.threshold, args.out, regen, code)
    print("Fonts  : " + ", ".join(f"{c}px {os.path.basename(p)}" for c, p in paths.items()))
    print(f"Glyphs : {len(cps)} code points x {len(CELLS)} sizes, {total} bytes")
    print(f"Wrote  : {args.out}")


if __name__ == "__main__":
    main()
