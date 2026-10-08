# Third-party notices

PocketOBI itself is licensed under the PolyForm Noncommercial License 1.0.0 (see
[LICENSE](LICENSE)). The following third-party components are bundled in or reused
by PocketOBI and remain under their own licenses. PolyForm Noncommercial does **not**
apply to them.

--------------------------------------------------------------------------------

## OneWire2 (bundled: `OneWire2.h`, `OneWire2.cpp`, `util/`)

A modified version of the OneWire library, taken from the Open Battery Information
project. Licensed under the MIT license. Original copyright and permission notices
are retained in the source files.

  Copyright (c) 2007 Jim Studt and contributors
  Maintained since 2010 by Paul Stoffregen (paul@pjrc.com)
  Bit-level timing modifications from the Open Battery Information project

## Open Battery Information (base project / protocol implementation)

PocketOBI builds on the Open Battery Information project by Martin Jansson, from
which the OneWire2 library and the reference protocol implementation come.
Licensed under the MIT license.

  https://github.com/mnh-jansson/open-battery-information
  Copyright (c) 2024 Martin Jansson

--------------------------------------------------------------------------------

## MIT License

The MIT license below applies to the OneWire2 library and to the Open Battery
Information project listed above (each with its own copyright holder as noted).

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

--------------------------------------------------------------------------------

## Noto Sans CJK JP and Fusion Pixel (glyph bitmaps in `glyphs_ja.h`, Japanese build only)

The Japanese characters of the `ENABLE_CJK` build are 1-bit bitmaps rasterised by
`tools/gen_cjk_glyphs.py`: the 16 px cell from Noto Sans JP Medium (Noto Sans CJK,
version 2.004), the 12 px cell from Fusion Pixel 12px monospaced, Japanese variant
(release 2026.09.25). They are a modified form of the fonts and stay under the SIL Open
Font License 1.1, reproduced below. The default (non-Japanese) build contains none of them.

  https://github.com/notofonts/noto-cjk
  Copyright 2014-2021 Adobe (http://www.adobe.com/). Noto is a trademark of Google Inc.

  https://github.com/TakWolf/fusion-pixel-font
  Copyright (c) 2022, TakWolf (https://takwolf.com).
  Fusion Pixel merges glyphs from Ark Pixel Font (Copyright (c) 2021, TakWolf), Cubic 11
  (Copyright (c) 2005 M+ FONTS PROJECT, (C) 2002-2004 COZ) and Galmuri (Copyright (c)
  2019-2025 Lee Minseo), all under the SIL Open Font License 1.1.

--------------------------------------------------------------------------------

## Acknowledgements (facts / documentation only — no code reused)

The Makita LXT protocol facts that PocketOBI reimplements independently are
documented by several community projects. These contributed knowledge, not code,
and impose no licensing obligation; they are credited with thanks in the README
(e.g. rosvall/makita-lxt-protocol, drakosha/makita-battery-tools). Where any such
project ships without a license, none of its code is used here.

--------------------------------------------------------------------------------

## SIL Open Font License 1.1 (applies to the Noto Sans CJK JP and Fusion Pixel glyphs above)

This Font Software is licensed under the SIL Open Font License,
Version 1.1.

This license is copied below, and is also available with a FAQ at:
http://scripts.sil.org/OFL

-----------------------------------------------------------
SIL OPEN FONT LICENSE Version 1.1 - 26 February 2007
-----------------------------------------------------------

PREAMBLE
The goals of the Open Font License (OFL) are to stimulate worldwide
development of collaborative font projects, to support the font
creation efforts of academic and linguistic communities, and to
provide a free and open framework in which fonts may be shared and
improved in partnership with others.

The OFL allows the licensed fonts to be used, studied, modified and
redistributed freely as long as they are not sold by themselves. The
fonts, including any derivative works, can be bundled, embedded,
redistributed and/or sold with any software provided that any reserved
names are not used by derivative works. The fonts and derivatives,
however, cannot be released under any other type of license. The
requirement for fonts to remain under this license does not apply to
any document created using the fonts or their derivatives.

DEFINITIONS
"Font Software" refers to the set of files released by the Copyright
Holder(s) under this license and clearly marked as such. This may
include source files, build scripts and documentation.

"Reserved Font Name" refers to any names specified as such after the
copyright statement(s).

"Original Version" refers to the collection of Font Software
components as distributed by the Copyright Holder(s).

"Modified Version" refers to any derivative made by adding to,
deleting, or substituting -- in part or in whole -- any of the
components of the Original Version, by changing formats or by porting
the Font Software to a new environment.

"Author" refers to any designer, engineer, programmer, technical
writer or other person who contributed to the Font Software.

PERMISSION & CONDITIONS
Permission is hereby granted, free of charge, to any person obtaining
a copy of the Font Software, to use, study, copy, merge, embed,
modify, redistribute, and sell modified and unmodified copies of the
Font Software, subject to the following conditions:

1) Neither the Font Software nor any of its individual components, in
Original or Modified Versions, may be sold by itself.

2) Original or Modified Versions of the Font Software may be bundled,
redistributed and/or sold with any software, provided that each copy
contains the above copyright notice and this license. These can be
included either as stand-alone text files, human-readable headers or
in the appropriate machine-readable metadata fields within text or
binary files as long as those fields can be easily viewed by the user.

3) No Modified Version of the Font Software may use the Reserved Font
Name(s) unless explicit written permission is granted by the
corresponding Copyright Holder. This restriction only applies to the
primary font name as presented to the users.

4) The name(s) of the Copyright Holder(s) or the Author(s) of the Font
Software shall not be used to promote, endorse or advertise any
Modified Version, except to acknowledge the contribution(s) of the
Copyright Holder(s) and the Author(s) or with their explicit written
permission.

5) The Font Software, modified or unmodified, in part or in whole,
must be distributed entirely under this license, and must not be
distributed under any other license. The requirement for fonts to
remain under this license does not apply to any document created using
the Font Software.

TERMINATION
This license becomes null and void if any of the above conditions are
not met.

DISCLAIMER
THE FONT SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
OF COPYRIGHT, PATENT, TRADEMARK, OR OTHER RIGHT. IN NO EVENT SHALL THE
COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
INCLUDING ANY GENERAL, SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL
DAMAGES, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF THE USE OR INABILITY TO USE THE FONT SOFTWARE OR FROM
OTHER DEALINGS IN THE FONT SOFTWARE.
