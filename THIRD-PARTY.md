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

## Acknowledgements (facts / documentation only — no code reused)

The Makita LXT protocol facts that PocketOBI reimplements independently are
documented by several community projects. These contributed knowledge, not code,
and impose no licensing obligation; they are credited with thanks in the README
(e.g. rosvall/makita-lxt-protocol, drakosha/makita-battery-tools). Where any such
project ships without a license, none of its code is used here.
