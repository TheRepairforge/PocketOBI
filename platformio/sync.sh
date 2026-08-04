#!/bin/sh
# Regenerate the PlatformIO mirror from the canonical Arduino sources at the repo
# root. Run after editing ../PocketOBI.ino or ../OneWire2.* so the PlatformIO
# build stays identical. (../PocketOBI.ino is the single source of truth.)
set -e
cd "$(dirname "$0")"
mkdir -p src lib/OneWire2/util
cp ../PocketOBI.ino src/PocketOBI.ino
cp ../OneWire2.h ../OneWire2.cpp lib/OneWire2/
cp ../util/* lib/OneWire2/util/
echo "PlatformIO mirror synced from the root sources."
