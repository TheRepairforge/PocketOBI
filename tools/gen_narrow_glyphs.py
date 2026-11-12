#!/usr/bin/env python3
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
# Copyright (c) 2026 TheRepairForge
"""Generate glyphs_narrow.h: the accented-Latin and Vietnamese glyph data of the narrow text path.

    python tools/gen_narrow_glyphs.py            # regenerate glyphs_narrow.h
    python tools/gen_narrow_glyphs.py --check    # exit 1 if glyphs_narrow.h is stale or a string is refused

How the narrow path draws a letter (docs/NARROW_GLYPHS.md): an accented letter is the CURRENT font's own
ASCII glyph (classic 5x7 or a FreeSans GFX font, drawn unaltered at run time) plus one to four small mark
bitmaps (acute, circumflex, horn, dot below ...) placed from that glyph's ink box. So one table serves every
font and size, the advance is the base letter's advance, and no font glyph is copied or redrawn here.
A letter that is no ASCII letter plus marks (only the German sharp s today) is a FULL glyph: the classic
5x7 one from the library's classic font, the GFX ones rasterised from Noto Sans JP Medium (OFL, the font
the CJK path already uses), one per GFX font the UI uses, matched on that font's cap height.

What it covers: every letter of Latin-1 Supplement, Latin Extended-A and Vietnamese that composes from an
ASCII letter + supported marks, plus the specials below. It does NOT need the strings to pick the set; it
reads the narrow string columns only to refuse a character the path cannot draw and a string over 63 bytes
(64-byte buffers, as gen_cjk_glyphs.py). A new language in the Latin script therefore needs no regeneration
unless it uses a letter listed as refused in the generated header.

Cyrillic (#81) is a later table of FULL glyphs (one per GFX font + classic), same structure as the sharp s.

Inputs: the tree's Adafruit GFX library (classic font + the GFX font headers, from .pio/libdeps after one
`pio run`) and tools/.fonts/NotoSansJP-Medium.otf (gitignored, see gen_cjk_glyphs.py). The committed
glyphs_narrow.h is what every build uses: a contributor never needs either.
"""
import argparse
import glob
import os
import re
import sys
import unicodedata

HERE = os.path.dirname(os.path.abspath(__file__))
TREE = os.path.dirname(HERE)
OUT = "glyphs_narrow.h"
MAX_BYTES = 63

# GFX fonts the UI draws text with (ui_theme.h FONT_LEVEL*, pocketobi.h). A full glyph is generated for each.
GFX_FONTS = ["FreeSans9pt7b", "FreeSansBold9pt7b", "FreeSans12pt7b", "FreeSansBold18pt7b", "FreeSansBold24pt7b"]

# ---- marks -------------------------------------------------------------------------------------------
# Placement: ABOVE stacks upward from the ink top; BELOW hangs under the ink bottom; OGONEK at the bottom
# right; HORN at the top right; STROKE across the middle (centre or left stem); BAR across the ascender
# (d) or the middle of the left stem (D).
PLACES = ["P_ABOVE", "P_BELOW", "P_OGONEK", "P_HORN", "P_STROKE", "P_STROKE_L", "P_BAR_HI", "P_BAR_MID"]

# name: (placement, art for the classic font (scaled by text size), art for the GFX fonts (scaled by
# x-height / 12)). 'X' = ink.
MARKS = {
    "ACUTE":     ("P_ABOVE",  ["..X", ".X."],              ["...X", "..X.", ".X.."]),
    "GRAVE":     ("P_ABOVE",  ["X..", ".X."],              ["X...", ".X..", "..X."]),
    "CIRCUMFLEX":("P_ABOVE",  [".X.", "X.X"],              ["..X..", ".X.X.", "X...X"]),
    "CARON":     ("P_ABOVE",  ["X.X", ".X."],              ["X...X", ".X.X.", "..X.."]),
    "TILDE":     ("P_ABOVE",  [".X.X", "X.X."],            [".XX..X", "X..XX."]),
    "DIAERESIS": ("P_ABOVE",  ["X.X"],                     ["XX..XX"]),
    "RING":      ("P_ABOVE",  [".X.", "X.X", ".X."],       [".XX.", "X..X", ".XX."]),
    "DOT":       ("P_ABOVE",  ["X"],                       ["XX"]),
    "MACRON":    ("P_ABOVE",  ["XXX"],                     ["XXXXX"]),
    "BREVE":     ("P_ABOVE",  ["X..X", ".XX."],            ["X...X", ".XXX."]),
    "DBLACUTE":  ("P_ABOVE",  [".X.X", "X.X."],            ["..X..X", ".X..X.", "X..X.."]),
    "HOOK":      ("P_ABOVE",  ["XX", ".X"],                [".XX.", "...X", "..X."]),
    "HORN":      ("P_HORN",   [".X", "X."],                ["..X", "..X", "XX."]),
    "DOTBELOW":  ("P_BELOW",  ["X"],                       ["XX"]),
    "CEDILLA":   ("P_BELOW",  [".X", "XX"],                [".X.", "..X", "XX."]),
    "COMMABELOW":("P_BELOW",  ["X", "X"],                  [".X", ".X", "X."]),
    "OGONEK":    ("P_OGONEK", ["X.", ".X"],                ["X..", "X..", ".XX"]),
    "STROKE":    ("P_STROKE", ["..X", ".X.", "X.."],       ["...X", "..X.", ".X..", "X..."]),
    "STROKE_L":  ("P_STROKE_L", ["..X", ".X.", "X.."],     ["...X", "..X.", ".X..", "X..."]),
    "BAR_HI":    ("P_BAR_HI", ["XXX"],                     ["XXXXX"]),
    "BAR_MID":   ("P_BAR_MID", ["XXX"],                    ["XXXXXXX"]),
}
MARK_IDS = ["NONE"] + list(MARKS)

COMBINING = {
    0x0301: "ACUTE", 0x0300: "GRAVE", 0x0302: "CIRCUMFLEX", 0x030C: "CARON", 0x0303: "TILDE",
    0x0308: "DIAERESIS", 0x030A: "RING", 0x0307: "DOT", 0x0304: "MACRON", 0x0306: "BREVE",
    0x030B: "DBLACUTE", 0x0309: "HOOK", 0x031B: "HORN", 0x0323: "DOTBELOW", 0x0327: "CEDILLA",
    0x0326: "COMMABELOW", 0x0328: "OGONEK",
}
# Letters with no canonical decomposition that are still an ASCII letter + a mark.
SPECIAL = {0x0111: ("d", ["BAR_HI"]), 0x0110: ("D", ["BAR_MID"]), 0x00D0: ("D", ["BAR_MID"]),
           0x0142: ("l", ["STROKE"]), 0x0141: ("L", ["STROKE_L"]), 0x0131: ("i", [])}
# Full glyphs: code point -> (classic font index in the library's CP437-ordered table, ASCII stand-in
# whose advance and bounds it takes, fallback text when a font has no glyph).
FULL = {0x00DF: (0xE1, "B", "ss"),
        0x00A1: (0xAD, "!", "!"),     # inverted exclamation mark (Spanish)
        0x00BF: (0xA8, "?", "?")}     # inverted question mark (Spanish)
DOTLESS = {"i": 1, "j": 2}          # base codes 1/2: i/j drawn without their dot (a mark goes above)
FULL_BASE = 16                      # base code FULL_BASE + k = FULL glyph k

COVER = list(range(0x00C0, 0x0100)) + list(range(0x0100, 0x0180)) + [0x01A0, 0x01A1, 0x01AF, 0x01B0] + \
    list(range(0x1EA0, 0x1EFA)) + [0x0218, 0x0219, 0x021A, 0x021B]


def decompose(cp):
    """(base char, [mark names]) or None when the narrow path cannot draw cp."""
    if cp in SPECIAL:
        b, m = SPECIAL[cp]
        return b, list(m)
    d = unicodedata.normalize("NFD", chr(cp))
    if len(d) < 2 or not ("a" <= d[0] <= "z" or "A" <= d[0] <= "Z"):
        return None
    marks = []
    for ch in d[1:]:
        if ord(ch) not in COMBINING:
            return None
        marks.append(COMBINING[ord(ch)])
    above = [m for m in marks if MARKS[m][0] == "P_ABOVE"]
    if len(above) > 2 or len(marks) > 4:
        return None
    return d[0], marks


# ---- font inputs -----------------------------------------------------------------------------------
def classic_font(lib):
    t = open(os.path.join(lib, "glcdfont.c"), encoding="latin-1").read()
    body = t[t.index("font[]"):]
    return [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", body)]


def classic_cols(font, idx):
    return font[idx * 5: idx * 5 + 5]


def ink_box(cols):
    """(top, left, right, bottom) rows/cols of a 5-column classic glyph (bit 0 = top row)."""
    xs = [i for i, c in enumerate(cols) if c]
    ys = [r for r in range(8) if any(c >> r & 1 for c in cols)]
    return min(ys), min(xs), max(xs), max(ys)


def load_gfx(lib, name):
    t = open(os.path.join(lib, "Fonts", name + ".h"), encoding="latin-1").read()
    bmp = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", t[t.index("Bitmaps[]"):t.index("Glyphs[]")])]
    gl = [tuple(map(int, g)) for g in
          re.findall(r"\{\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(-?\d+),\s*(-?\d+)\s*\}", t[t.index("Glyphs[]"):])]
    ya = int(re.search(r"0x7E,\s*(\d+)\s*\}", t).group(1))
    return bmp, gl, ya


def fingerprint(gl, ya):
    """What narrow_render.cpp computes from a GFXfont at run time: each translation unit holds its own copy
    of a font header, so a font is recognised by its content, never by its address."""
    return (ya << 16) | gl[ord("W") - 0x20][0]


def raster_full(cp, gl, ya, font_path, bold, stand_in):
    """Rasterise cp from the OFL font so its cap height matches the GFX font's stand-in letter."""
    from PIL import ImageFont
    sb = gl[ord(stand_in) - 0x20]
    target_h = sb[2]
    size = 10
    for _ in range(60):
        f = ImageFont.truetype(font_path, size)
        bb = f.getbbox(stand_in)
        h = bb[3] - bb[1]
        if h >= target_h:
            break
        size += 1
    f = ImageFont.truetype(font_path, size)
    ascent = f.getmetrics()[0]
    mask = f.getmask(chr(cp), mode="L")
    w, h = mask.size
    bb = f.getbbox(chr(cp))                       # relative to the text origin (top of the ascent)
    thr = 90 if bold else 128
    rows = [[1 if mask.getpixel((x, y)) >= thr else 0 for x in range(w)] for y in range(h)]
    if bold:                                      # Noto Medium is lighter than FreeSans Bold: widen by 1 px
        rows = [[1 if r[x] or (x and r[x - 1]) else 0 for x in range(w + 1)] for r in [rr + [0] for rr in rows]]
        w += 1
    ys = [y for y in range(h) if any(rows[y])]
    xs = [x for x in range(w) if any(rows[y][x] for y in range(h))]
    rows = [r[min(xs):max(xs) + 1] for r in rows[min(ys):max(ys) + 1]]
    gw, gh = len(rows[0]), len(rows)
    y_off = bb[1] + min(ys) - ascent              # top of the ink relative to the baseline
    x_off = sb[4]
    bits, acc, n = [], 0, 0
    for r in rows:
        for v in r:
            acc = (acc << 1) | v
            n += 1
            if n == 8:
                bits.append(acc); acc = 0; n = 0
    if n:
        bits.append(acc << (8 - n))
    return dict(w=gw, h=gh, adv=sb[3], xo=x_off, yo=y_off, bits=bits)


# ---- strings -----------------------------------------------------------------------------------------
LIT = re.compile(r'"((?:[^"\\]|\\.)*)"')


def narrow_strings(tree):
    """(where, text) of every string the narrow path draws: STRTAB FR/DE/ES + strings_vi.h + strings_pl.h."""
    out = []
    t = open(os.path.join(tree, "strings_i18n.h"), encoding="utf-8").read()
    body = t[t.index("STRTAB[S_COUNT][LANG_COUNT]"):]
    for line in body.splitlines():
        m = re.match(r"\s*/\*(S_\w+)\*/", line)
        if not m:
            continue
        lits = LIT.findall(line)
        for col, s in zip(("EN", "FR", "DE", "ES"), lits):
            out.append(("%s %s" % (m.group(1), col), s))
    for fn, tag in (("strings_vi.h", "VI"), ("strings_pl.h", "PL")):
        p = os.path.join(tree, fn)
        if not os.path.exists(p):
            continue
        for line in open(p, encoding="utf-8").read().splitlines():
            m = re.match(r"\s*/\*(S_\w+)\*/\s*\"((?:[^\"\\]|\\.)*)\"", line)
            if m:
                out.append(("%s %s" % (m.group(1), tag), m.group(2)))
    # Trees with a tool-category table (lockstep copies of this engine): every non-ASCII literal it draws.
    for fn in ("toolcat.cpp", "toolcat_vi.h"):
        p = os.path.join(tree, fn)
        if not os.path.exists(p):
            continue
        for n, line in enumerate(open(p, encoding="utf-8").read().splitlines(), 1):
            for s in LIT.findall(line):
                if any(ord(c) > 127 for c in s):
                    out.append(("%s:%d" % (fn, n), s))
    return out


# ---- emit --------------------------------------------------------------------------------------------
def art_bytes(art):
    out = []
    w = len(art[0])
    for row in art:
        acc = 0
        for x in range(w):
            if row[x] == "X":
                acc |= 0x80 >> (x & 7)
            if x & 7 == 7 or x == w - 1:
                out.append(acc); acc = 0
    return out


def generate(lib, fonts_dir):
    cfont = classic_font(lib)
    comps, refused = [], []
    for cp in COVER:
        if cp in FULL:
            continue
        d = decompose(cp)
        if d is None:
            refused.append(cp)
            continue
        base, marks = d
        above = [m for m in marks if MARKS[m][0] == "P_ABOVE"]
        bcode = ord(base)
        if base in DOTLESS and above:
            bcode = DOTLESS[base]
        comps.append((cp, bcode, base, marks))
    full_list = sorted(FULL)
    for k, cp in enumerate(full_list):
        comps.append((cp, FULL_BASE + k, FULL[cp][1], []))
    comps.sort()
    have = {c[0] for c in comps}

    def upper_of(cp):
        u = chr(cp).upper()
        if len(u) == 1 and ord(u) != cp and ord(u) in have and len(u.encode()) == len(chr(cp).encode()):
            return ord(u)
        return 0

    L = []
    w = L.append
    w("// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0")
    w("// Copyright (c) 2026 TheRepairForge")
    w("// GENERATED by tools/gen_narrow_glyphs.py -- do not edit. Regenerate: python tools/gen_narrow_glyphs.py")
    w("// Narrow text path data (docs/NARROW_GLYPHS.md): accented Latin + Vietnamese as base letter + marks,")
    w("// full glyphs where composition does not apply. Classic-font full glyphs come from the Adafruit GFX")
    w("// classic font (BSD); GFX-font full glyphs are rasterised from Noto Sans JP Medium (SIL OFL 1.1).")
    w("#pragma once")
    w("#include <stdint.h>")
    w("")
    w("enum NarrowPlace : uint8_t { %s };" % ", ".join(PLACES))
    w("enum NarrowMarkId : uint8_t { %s, NM_COUNT };" % ", ".join("NM_" + m for m in MARK_IDS))
    w("struct NarrowArt { uint8_t place, w, h; uint16_t off; };   // off = byte offset in the set's bitmap")
    for set_i, set_name in ((1, "S"), (2, "M")):
        bits, arts = [], ["  { 0, 0, 0, 0 },"]
        for m in MARK_IDS[1:]:
            art = MARKS[m][set_i]
            arts.append("  { %s, %d, %d, %d },   // %s" % (MARKS[m][0], len(art[0]), len(art), len(bits), m))
            bits += art_bytes(art)
        w("// Mark set %s: %s." % (set_name, "classic font, scaled by the text size" if set_name == "S"
                                 else "GFX fonts, scaled by x-height / 12"))
        w("static const NarrowArt NARROW_ART_%s[NM_COUNT] = {" % set_name)
        L.extend(arts)
        w("};")
        w("static const uint8_t NARROW_ART_%s_BITS[] = { %s };" % (set_name, ", ".join("0x%02X" % b for b in bits)))
    w("")
    w("// base: an ASCII letter; %d/%d = dotless i/j (a mark sits where the dot was); %d..31 = FULL glyph"
      % (DOTLESS["i"], DOTLESS["j"], FULL_BASE))
    w("// (base - %d). cbox = classic-font ink box of the base: top | left << 3 | right << 6 | bottom << 9."
      % FULL_BASE)
    w("// upper = the capital (same UTF-8 length) for upperLabel(), 0 if none.")
    w("struct NarrowComp { uint16_t cp, upper, cbox; uint8_t base; uint8_t mark[4]; };")
    w("static const NarrowComp NARROW_COMP[] = {")
    custom = {1: [0x00, 0x44, 0x7C, 0x40, 0x00], 2: [0x20, 0x40, 0x44, 0x3C, 0x00]}   # dotless i, j
    for cp, bcode, base, marks in comps:
        if FULL_BASE <= bcode < 32:
            cols = classic_cols(cfont, FULL[cp][0])
        elif bcode in custom:
            cols = custom[bcode]
        else:
            cols = classic_cols(cfont, ord(base))
        t, l, r, b = ink_box(cols)
        mk = ["NM_" + m for m in marks] + ["NM_NONE"] * (4 - len(marks))
        bs = ("%d" % bcode) if bcode < 32 else "'%s'" % base
        w("  { 0x%04X, 0x%04X, 0x%04X, %s, { %s } },   // %s" % (cp, upper_of(cp), t | l << 3 | r << 6 | b << 9, bs,
                                                         ", ".join(mk), chr(cp)))
    w("};")
    w("static const int NARROW_COMP_N = sizeof(NARROW_COMP) / sizeof(NARROW_COMP[0]);")
    w("")
    w("// Classic-font columns (bit 0 = top row) of the bases the classic font lacks: dotless i, dotless j,")
    w("// then the FULL glyphs in order.")
    rows = [custom[1], custom[2]] + [classic_cols(cfont, FULL[cp][0]) for cp in full_list]
    w("static const uint8_t NARROW_CLASSIC[][5] = {")
    for r in rows:
        w("  { %s }," % ", ".join("0x%02X" % c for c in r))
    w("};")
    w("")
    w("// FULL glyphs per GFX font, recognised by fingerprint (yAdvance << 16 | bitmapOffset of 'W').")
    w("// A font with no entry draws the fallback text instead.")
    w("struct NarrowFull { uint32_t fp; uint8_t full; uint8_t w, h, adv; int8_t xo, yo; uint16_t off; };")
    font_path = os.path.join(fonts_dir, "NotoSansJP-Medium.otf")
    if not os.path.exists(font_path):
        sys.exit("missing %s (the OFL fonts of gen_cjk_glyphs.py)" % font_path)
    fbits, fents = [], []
    for name in GFX_FONTS:
        bmp, gl, ya = load_gfx(lib, name)
        fp = fingerprint(gl, ya)
        for k, cp in enumerate(full_list):
            g = raster_full(cp, gl, ya, font_path, "Bold" in name, FULL[cp][1])
            fents.append("  { 0x%08X, %d, %d, %d, %d, %d, %d, %d },   // %s %s"
                         % (fp, k, g["w"], g["h"], g["adv"], g["xo"], g["yo"], len(fbits), name, chr(cp)))
            fbits += g["bits"]
    w("static const NarrowFull NARROW_FULL[] = {")
    L.extend(fents)
    w("};")
    w("static const int NARROW_FULL_N = sizeof(NARROW_FULL) / sizeof(NARROW_FULL[0]);")
    w("static const uint8_t NARROW_FULL_BITS[] = { %s };" % ", ".join("0x%02X" % b for b in fbits))
    w("static const char* const NARROW_FULL_FALLBACK[] = { %s };" % ", ".join('"%s"' % FULL[cp][2] for cp in full_list))
    w("static const char NARROW_FULL_STANDIN[] = { %s };" % ", ".join("'%s'" % FULL[cp][1] for cp in full_list))
    w("")
    w("// Refused (no ASCII letter + supported marks, no full glyph yet): %s"
      % " ".join("U+%04X" % c for c in refused))
    return "\n".join(L) + "\n", have


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--tree", default=TREE)
    ap.add_argument("--gfx", help="Adafruit GFX Library folder (default: <tree>/.pio/libdeps/esp32-c3/...)")
    ap.add_argument("--fonts", default=os.path.join(HERE, ".fonts"))
    ap.add_argument("--check", action="store_true")
    a = ap.parse_args()
    lib = a.gfx or os.path.join(a.tree, ".pio", "libdeps", "esp32-c3", "Adafruit GFX Library")
    if not os.path.isdir(lib):
        hits = glob.glob(os.path.join(a.tree, ".pio", "libdeps", "*", "Adafruit GFX Library"))
        if not hits:
            sys.exit("no Adafruit GFX Library under %s/.pio/libdeps: run `pio run -e esp32-c3` once, or pass --gfx"
                     % a.tree)
        lib = hits[0]
    text, have = generate(lib, a.fonts)

    bad = []
    for where, s in narrow_strings(a.tree):
        if len(s.encode("utf-8")) > MAX_BYTES:
            bad.append("%s: %d bytes > %d" % (where, len(s.encode("utf-8")), MAX_BYTES))
        for ch in s:
            if ord(ch) >= 0x80 and ord(ch) not in have:
                bad.append("%s: U+%04X %r has no narrow glyph" % (where, ord(ch), ch))
    if bad:
        print("\n".join(bad))
        sys.exit(1)

    out = os.path.join(a.tree, OUT)
    if a.check:
        cur = open(out, encoding="utf-8").read() if os.path.exists(out) else ""
        if cur != text:
            print("%s is stale: python tools/gen_narrow_glyphs.py" % OUT)
            sys.exit(1)
        print("%s up to date" % OUT)
        return
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print("wrote %s (%d letters)" % (out, len(have)))


if __name__ == "__main__":
    main()
