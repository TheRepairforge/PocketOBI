# Translating PocketOBI

PocketOBI's screens exist in English, French, German, Spanish and Japanese. English and French
are written by the maintainer; **German, Spanish and Japanese are first drafts that no native
speaker has checked yet.** If you speak one of them, your corrections are very welcome.

| Language | File | Notes |
|---|---|---|
| English, French, German, Spanish | `strings_i18n.h` (`STRTAB`) | ASCII only: the device fonts have no accents (FR/ES accents dropped, German umlauts written ae/oe/ue/ss) |
| Japanese | `strings_ja.h` (`STRTAB_JA`) | Only in the Japanese build (`ENABLE_CJK=1`), drawn with 16x16 bitmap glyphs |

English is the reference. The language is chosen on the device in **Settings > Language**.

## German and Spanish: the easy way

Open the review page for your language: https://therepairforge.github.io/translate/

It shows every device screen in your language and lists every string. Next to each one,
**Suggest a fix** opens a GitHub issue already filled in (language, key, English, current
text): you only type your version.

For a full pass, download the spreadsheet from the same page, fill the `your_suggestion`
column, and attach the file to a [translation issue](https://github.com/TheRepairforge/PocketOBI/issues/new?template=translation.yml).

### Two limits of the device screen

- **Plain ASCII only.** The screen fonts have no accented letters: write `Reparer`, `Bateria`,
  and German umlauts as `ae`, `oe`, `ue`, `ss`.
- **Limited width.** Keep a fix close to the longest version shown today (the review page
  gives the number of characters). Every accepted fix is rendered on the real screen before it
  ships, so nothing ends up cut off.

All these strings are in [`strings_i18n.h`](../strings_i18n.h), one row per string, one column
per language. Pull requests that edit that table directly are welcome too, but an issue is
enough.

## Japanese: help wanted

The Japanese text is a **best-effort draft written without a native speaker**. It is
published that way on purpose, so it can be corrected by the people who use it. If you
read Japanese, and especially if you repair or use power-tool batteries, a review is the
most useful contribution you can make.

A string left as `nullptr` shows the English text. A few technical screens (Debug, the
touch calibration) and the credits stay in English in every language.

### See your text on the screens (no board needed)

```
pip install pillow ziglang
python tools/hostsim/hostsim.py --ja
```

This compiles the real firmware for your PC and saves every screen as an image, in
`tools/hostsim/out/c3-ja/` (`sheet_JA.png` shows them all on one page). Details:
[`tools/hostsim/README.md`](../tools/hostsim/README.md).

### Fixing or improving a translation (no code needed)

1. Open `strings_ja.h`. Each line is one message, with the English source as a comment:

   ```c
   /*S_TEMP*/     "温度",    // EN: "Temp"
   /*S_CONDITION*/ nullptr,  // EN: "Condition"
   ```

2. Replace the text between the quotes, or replace `nullptr` with `"your text"`.
   Never change the `/*S_...*/` label or the order of the lines.
3. Open a pull request, or paste your corrections in a
   [translation issue](https://github.com/TheRepairforge/PocketOBI/issues/new?template=translation.yml) (key + better text).
   You do not need to build anything; the maintainer regenerates the glyphs.

### Rules that keep the device from breaking

- **Keep the placeholders exactly** (`%d`, `%s`, `%.1f`, ...), in the same order as English.
- **Short.** The screen is 320 px wide. A Japanese character is 13 to 17 px wide depending
  on the text size, so a line fits 18 to 24 characters; labels next to a value have much
  less room. Check with hostsim. The firmware also limits a string to 63 bytes (a Japanese
  character takes 3), and `tools/gen_cjk_glyphs.py` refuses a longer one.
- Labels are short noun phrases. Full sentences (hints, repair advice) can use です・ます.
- Japanese characters, ASCII letters and digits are fine. Avoid half-width katakana and
  emoji: there is no glyph for them.

The PackScope desktop app has its own Japanese catalog and a glossary of the battery terms
it uses; keeping both consistent helps users who use the two together.

## For maintainers: after editing `strings_ja.h`

```
python tools/gen_cjk_glyphs.py            # check the table + regenerate glyphs_ja.h
pio run -e esp32-c3-ja                    # or cyd-ja
```

The script stops with an error if `strings_ja.h` no longer has one line per message, in
the order of `enum StrId`. A character missing from `glyphs_ja.h` draws as a hollow box on
the device, so a forgotten regeneration is visible. In the Arduino IDE, put
`#define ENABLE_CJK 1` in the local, untracked `board_local.h` instead of using `-D`.

**Fonts.** The glyphs are rasterised from two fonts under the SIL Open Font License:
**Noto Sans JP Medium** for the 16 px cell, **Fusion Pixel 12px** (Japanese variant), a pixel
font drawn for that grid, for the 12 px cell. Download the two `.otf` files once into
`tools/.fonts/` (links at the top of `tools/gen_cjk_glyphs.py`). Without them the script
falls back to a Windows font, which is not redistributable: never commit that output.

## Credit

Tell us in the issue how you want to be thanked in the release notes, or leave it empty to
stay anonymous.
