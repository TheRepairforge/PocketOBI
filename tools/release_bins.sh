#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# release_bins.sh — build the PocketOBI release images.
#
# Usage: tools/release_bins.sh <git-ref> <out-dir> [<version>]
#   e.g. tools/release_bins.sh release/2.6.0 /tmp/bins          (version read from FW_VERSION)
#
# The same script runs in CI (.github/workflows/build.yml, job release-bins) and locally.
# Needs `pio` on PATH and an esptool (v4 or v5): `esptool` on PATH, else the copy PlatformIO
# bundles (tool-esptoolpy); override with ESPTOOL="...". On Windows it builds from a short-path
# worktree (C:/pob, long-path limit) and uses PLATFORMIO_CORE_DIR=C:/pio when those exist;
# elsewhere a temporary worktree and PlatformIO's default core.
#
# Output per env: PocketOBI-v<version>-<env>.bin (merged, flash at 0x0) and
# PocketOBI-v<version>-<env>-parts.zip (separate images + OFFSETS.txt), then SHA256SUMS.txt.
# ENVS below is the one list of release envs; every entry must exist in platformio.ini.
# ---------------------------------------------------------------------------
set -e
REF=$1; OUT=$2; VER=$3
[ -n "$OUT" ] || { echo "usage: $0 <git-ref> <out-dir> [<version>]"; exit 2; }
ENVS="esp32-c3 cyd esp32-c3-asia cyd-asia cyd-ili9341"   # -ja up to 2.3.1, -asia from 2.5.0
REPO=$(cd "$(dirname "$0")/.." && pwd)
if [ -d /c ]; then WT=/c/pob; else WT=$(mktemp -d)/pob; fi
if [ -d /c/pio ]; then export PLATFORMIO_CORE_DIR=C:/pio; fi
CORE=${PLATFORMIO_CORE_DIR:-$HOME/.platformio}; CORE=${CORE/C:/\/c}

mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
rm -rf "$WT"; git -C "$REPO" worktree prune
git -C "$REPO" worktree add -f --detach "$WT" "$REF"
trap 'git -C "$REPO" worktree remove --force "$WT"' EXIT

FW=$(sed -n 's/^#define FW_VERSION "\([^"]*\)".*/\1/p' "$WT/pocketobi.h")
[ -n "$FW" ] || { echo "no FW_VERSION in $REF"; exit 1; }
[ -z "$VER" ] || [ "$VER" = "$FW" ] || { echo "FW_VERSION in $REF is $FW, not $VER"; exit 1; }
VER=$FW
NAME=${GITHUB_REF_NAME:-$REF}                        # CI builds HEAD of the pushed branch
case $NAME in release/*) [ "${NAME#release/}" = "$VER" ] || { echo "$NAME carries FW_VERSION $VER"; exit 1; } ;; esac
grep -Eq '^#define COMM_DEBUG 0\b' "$WT/pocketobi.h" || { echo "COMM_DEBUG is not 0 in $REF"; exit 1; }
for env in $ENVS; do
    grep -q "^\[env:$env\]" "$WT/platformio.ini" || { echo "release env $env is not in platformio.ini"; exit 1; }
done

if [ -z "$ESPTOOL" ]; then
    if command -v esptool >/dev/null; then ESPTOOL=esptool
    else PY=$CORE/penv/Scripts/python; [ -x "$PY.exe" ] || [ -x "$PY" ] || PY=python
         ESPTOOL="$PY $CORE/packages/tool-esptoolpy/esptool.py"; fi
fi

echo "Building PocketOBI $VER from $REF: $ENVS"
# Build and package one env at a time: in a fresh worktree a pio run can wipe the previous
# env's .pio/build output (seen 2026-10-08), so nothing may rely on an earlier env still being there.
for env in $ENVS; do
    (cd "$WT" && pio run -e "$env")
    BA=$(find "$CORE/packages/framework-arduinoespressif32" -name boot_app0.bin | head -1)
    case $env in esp32-c3*) chip=esp32c3; bl=0x0 ;; es3c28p*) chip=esp32s3; bl=0x0 ;; *) chip=esp32; bl=0x1000 ;; esac
    b=$WT/.pio/build/$env; n=PocketOBI-v$VER-$env
    # merge_bin / --flash_mode: the spelling both esptool v4 and v5 accept
    $ESPTOOL --chip $chip merge_bin -o "$OUT/$n.bin" --flash_mode keep --flash_size keep \
        $bl "$b/bootloader.bin" 0x8000 "$b/partitions.bin" 0xe000 "$BA" 0x10000 "$b/firmware.bin"
    d=$(mktemp -d)
    cp "$b/bootloader.bin" "$b/partitions.bin" "$b/firmware.bin" "$d/"; cp "$BA" "$d/boot_app0.bin"
    printf "Flash offsets (chip %s):\n%s bootloader.bin\n0x8000 partitions.bin\n0xe000 boot_app0.bin\n0x10000 firmware.bin\n" \
        $chip $bl > "$d/OFFSETS.txt"
    (cd "$d" && python -c "import zipfile,sys,os;z=zipfile.ZipFile(sys.argv[1],'w',zipfile.ZIP_DEFLATED);[z.write(f) for f in sorted(os.listdir('.'))]" "$OUT/$n-parts.zip")
    rm -rf "$d"
done
(cd "$OUT" && sha256sum PocketOBI-v$VER-*.bin PocketOBI-v$VER-*-parts.zip > SHA256SUMS.txt && cat SHA256SUMS.txt)
