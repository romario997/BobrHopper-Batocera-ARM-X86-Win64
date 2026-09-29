#!/bin/sh
# Batocera x86_64 (PC) build (Git Bash on Windows): zig c++ -> x86_64-linux-gnu with glibc 2.28.
# SDL2 2.0.9 headers + libSDL2.so were copied out of a Debian buster amd64 sysroot in WSL (/opt/crossy-sysroot-amd64)
# into $CROSSY_TOOLS/amd64-sdl2 - the same pattern as r36s-sdl2 (see build/setup_tools.sh). Batocera ships its own
# (newer) libSDL2-2.0.so.0; linking against 2.0.9 keeps the binary on the oldest API it could meet.
# WSL is only used afterwards for readelf (check_elf.sh with the x86_64 loader allowed).
# Usage: sh build/build_batocera_x86.sh [target]   -> out/batocera-x86/<target>.x86_64 (default: bobrhopper)

REPO_DIR=$(cd "$(dirname "$0")/.." && pwd)
REPO_WIN=$(cd "$REPO_DIR" && pwd -W 2>/dev/null) || REPO_WIN=$REPO_DIR
REPO_DRIVE_UPPER=$(printf '%s' "$REPO_WIN" | cut -c1)
REPO_DRIVE=$(printf '%s' "$REPO_DRIVE_UPPER" | tr 'A-Z' 'a-z')
REPO_WSL="/mnt/$REPO_DRIVE$(printf '%s' "$REPO_WIN" | cut -c3- | tr '\\' '/')"

set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd -W)
TOOLS=${CROSSY_TOOLS:-$(cygpath -m "$LOCALAPPDATA")/BobrHopper/tools}
ZIG="$TOOLS/zig-x86_64-windows-0.14.1/zig.exe"
SDL="$TOOLS/amd64-sdl2"
OUT="$ROOT/out/batocera-x86"
TARGET=${1:-bobrhopper}
[ -f "$SDL/lib/libSDL2.so" ] || { echo "missing $SDL (SDL2 headers + libSDL2.so from the buster amd64 sysroot)"; exit 1; }
mkdir -p "$OUT"

. "$ROOT/build/sources.sh"
SOURCES=$(sources_for "$TARGET") || { echo "unknown target $TARGET"; exit 1; }

SRC_ABS=""
for s in $SOURCES; do SRC_ABS="$SRC_ABS $ROOT/$s"; done

BIN="$OUT/$TARGET.x86_64"
"$ZIG" c++ -target x86_64-linux-gnu.2.28 -std=c++17 -O2 -g0 -Wall -ffp-contract=off \
  -D_REENTRANT -I"$SDL/include/SDL2" -I"$ROOT/src" \
  $SRC_ABS -L"$SDL/lib" -lSDL2 -Wl,--allow-shlib-undefined -o "$BIN"
echo "built $BIN"

MSYS_NO_PATHCONV=1 wsl.exe -d Ubuntu-22.04 -e sh -c \
  "ls '$REPO_WSL' >/dev/null 2>&1 || sudo -n mount -t drvfs ${REPO_DRIVE_UPPER}: /mnt/${REPO_DRIVE}; sh $REPO_WSL/build/check_elf.sh $REPO_WSL/out/batocera-x86/$TARGET.x86_64"
