#pragma once
// Language build capabilities (DECISIONS.md D19, D58). TWO independent flags:
//   ENABLE_CJK  0 (default) = no CJK code, strings or glyphs compiled in; the binary
//                 matches a build without this feature.
//               1 = Japanese + Korean: the glyph tables (glyphs_ja.h, glyphs_ko.h), the
//                 string columns (strings_ja.h, strings_ko.h) and the UTF-8 renderer
//                 (cjk_render.cpp).
//   ENABLE_EU   1 (default) = FR/DE/ES selectable. 0 = not offered in Settings (their
//                 strings stay compiled: STRTAB is always four-wide).
//   ENABLE_VI   0 (default) = no Vietnamese. 1 = the strings_vi.h column (D50, D60): plain
//                 ASCII like FR/DE/ES, so no glyphs and no renderer are involved.
// Builds: standard = CJK 0 / EU 1 / VI 0 (EN FR DE ES); "Asia" = CJK 1 / EU 0 / VI 1
// (EN JA KO VI); demo = 1 / 1 / 1 (every language, for review). EN is in every build.
//
// This lives in a real header, included before strings_i18n.h, for the same reason as
// board_config.h: PlatformIO's .ino->.cpp conversion relocates #define directives, a
// header is included verbatim by both toolchains. A feature is not a board, so it is
// kept out of board_config.h.
//
// How to turn it on:
//   - PlatformIO / CI : the "-asia" envs (esp32-c3-asia, cyd-asia) pass
//                       -D ENABLE_CJK=1 -D ENABLE_EU=0 -D ENABLE_VI=1.
//   - Arduino IDE     : add "#define ENABLE_CJK 1", "#define ENABLE_EU 0" and
//                       "#define ENABLE_VI 1" to the
//                       gitignored local override board_local.h (the same file that
//                       selects the board). Never edit this tracked file to flip them.
// An explicit build flag always wins. The local file is only consulted when neither
// ENABLE_CJK nor POCKETOBI_BOARD came from the build, so a per-env PlatformIO build is
// deterministic even on a machine that carries a board_local.h.
#if !defined(ENABLE_CJK) && !defined(POCKETOBI_BOARD) && defined(__has_include)
#  if __has_include("board_local.h")
#    include "board_local.h"
#  endif
#endif

#ifndef ENABLE_CJK
#  define ENABLE_CJK 0
#endif

#ifndef ENABLE_EU
#  define ENABLE_EU 1
#endif

#ifndef ENABLE_VI
#  define ENABLE_VI 0
#endif
