#pragma once
// Japanese / CJK build capability (DECISIONS.md D19). ONE flag, ENABLE_CJK:
//   0 (default) = the shipping EN/FR/DE/ES build. No CJK code, strings or glyphs
//                 are compiled in; the binary matches a build without this feature.
//   1           = the "JA" build: EN/FR/DE/ES + Japanese, all switchable at runtime
//                 from Settings > Language. Adds the glyph table (glyphs_ja.h), the JA
//                 string column (strings_ja.h) and the UTF-8 renderer (cjk_render.cpp).
//
// This lives in a real header, included before strings_i18n.h, for the same reason as
// board_config.h: PlatformIO's .ino->.cpp conversion relocates #define directives, a
// header is included verbatim by both toolchains. A feature is not a board, so it is
// kept out of board_config.h.
//
// How to turn it on:
//   - PlatformIO / CI : the "-ja" envs (esp32-c3-ja, cyd-ja) pass -D ENABLE_CJK=1.
//   - Arduino IDE     : add "#define ENABLE_CJK 1" to the gitignored local override
//                       board_local.h (the same file that selects the board). Never
//                       edit this tracked file to flip it.
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
