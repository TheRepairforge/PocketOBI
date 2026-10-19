# Translating PocketOBI

PocketOBI's screens exist in English, French, German, Spanish, Japanese, Korean and Vietnamese.
English and French are written by the maintainer, Vietnamese by a Vietnamese user; **German,
Spanish, Japanese and Korean are first drafts that no native speaker has checked yet.** If you
speak one of them, your corrections are very welcome.

| Language | File | Notes |
|---|---|---|
| English, French, German, Spanish | `strings_i18n.h` (`STRTAB`) | ASCII only: the device fonts have no accents (FR/ES accents dropped, German umlauts written ae/oe/ue/ss) |
| Japanese | `strings_ja.h` (`STRTAB_JA`) | Only in the Asia build (`ENABLE_CJK=1`), drawn with 16x16 bitmap glyphs |
| Korean | `strings_ko.h` (`STRTAB_KO`) | Only in the Asia build (`ENABLE_CJK=1`), drawn with 16x16 bitmap glyphs |
| Vietnamese | `strings_vi.h` (`STRTAB_VI`) | Only in the Asia build (`ENABLE_VI=1`). ASCII only, like FR/DE/ES: no accents |

The standard build offers EN / FR / DE / ES, the Asia build EN / JA / KO / VI.

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

## Japanese and Korean: help wanted

The Japanese and Korean texts are **best-effort drafts written without a native speaker**.
They are published that way on purpose, so they can be corrected by the people who use them.
If you read one of them, and especially if you repair or use power-tool batteries, a review is
the most useful contribution you can make. What follows says `strings_ja.h`; for Korean read
`strings_ko.h`, it works the same way.

A string left as `nullptr` shows the English text. A few technical screens (Debug, the
touch calibration) and the credits stay in English in every language.

### See your text on the screens (no board needed)

```
pip install pillow ziglang
python tools/hostsim/hostsim.py --asia
```

This compiles the real firmware for your PC and saves every screen as an image, in
`tools/hostsim/out/c3-asia/` (`sheet_JA.png` and `sheet_KO.png` show them all on one page). Details:
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
- **Short.** The screen is 320 px wide. A Japanese or Korean character is 13 to 17 px wide
  depending on the text size, so a line fits 18 to 24 characters; labels next to a value have
  much less room. Check with hostsim. The firmware also limits a string to 63 bytes (a
  Japanese or Korean character takes 3), and `tools/gen_cjk_glyphs.py` refuses a longer one.
- Labels are short noun phrases. Full sentences (hints, repair advice) can use です・ます in
  Japanese, the polite -요 / -ㅂ니다 endings in Korean. Korean keeps its spaces between words:
  a long line breaks at a space.
- Japanese or Korean characters, ASCII letters and digits are fine. Avoid half-width katakana,
  old Hangul jamo and emoji: there is no glyph for them.

The PackScope desktop app has its own Japanese catalog and a glossary of the battery terms
it uses; keeping both consistent helps users who use the two together.

## For maintainers: after editing `strings_ja.h` or `strings_ko.h`

```
python tools/gen_cjk_glyphs.py            # check the table + regenerate glyphs_ja.h
python tools/gen_cjk_glyphs.py --lang ko  # same for strings_ko.h -> glyphs_ko.h
pio run -e esp32-c3-asia                  # or cyd-asia
```

The script stops with an error if the strings file no longer has one line per message, in
the order of `enum StrId`. A character missing from the glyph table draws as a hollow box on
the device, so a forgotten regeneration is visible. In the Arduino IDE, put
`#define ENABLE_CJK 1` and `#define ENABLE_EU 0` in the local, untracked `board_local.h`
instead of using `-D`.

**Fonts.** The glyphs are rasterised from fonts under the SIL Open Font License: **Noto Sans
JP / KR Medium** for the 16 px cell, **Fusion Pixel 12px** (Japanese / Korean variant), a pixel
font drawn for that grid, for the 12 px cell. Download the four `.otf` files once into
`tools/.fonts/` (links at the top of `tools/gen_cjk_glyphs.py`). Without them the script
falls back to a Windows font, which is not redistributable: never commit that output.

## Credit

Tell us in the issue how you want to be thanked in the release notes, or leave it empty to
stay anonymous.
