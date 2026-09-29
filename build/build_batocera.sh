#!/bin/sh
# Batocera ARM build (RG35XX H / Allwinner H700, Cortex-A53) - the same zig cross-compile as build_r36s.sh, only
# tuned for the A53 and written to out/batocera. TWO binaries: <target>.aarch64 and <target>.armhf. Official Batocera
# v43+ on the H700 is 64-bit, but the community Batocera v40 for the RG35XX H (stock Anbernic 4.9 kernel, Mali fbdev
# blob) has a 32-bit armhf userland - the launcher picks the binary by the dynamic loader the system has. The binary contract is unchanged (glibc <= 2.28, NEEDED only
# libSDL2 + libc family, GL through SDL_GL_GetProcAddress): Batocera v43+ has glibc 2.40 and SDL2 2.32, Knulli the
# same glibc, so one narrow binary runs on both.
# Usage: sh build/build_batocera.sh [target]   (default: bobrhopper)

REPO_DIR=$(cd "$(dirname "$0")/.." && pwd)
REPO_WIN=$(cd "$REPO_DIR" && pwd -W 2>/dev/null) || REPO_WIN=$REPO_DIR
REPO_DRIVE_UPPER=$(printf '%s' "$REPO_WIN" | cut -c1)
REPO_DRIVE=$(printf '%s' "$REPO_DRIVE_UPPER" | tr 'A-Z' 'a-z')
REPO_WSL="/mnt/$REPO_DRIVE$(printf '%s' "$REPO_WIN" | cut -c3- | tr '\\' '/')"

set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd -W)
TOOLS=${CROSSY_TOOLS:-$(cygpath -m "$LOCALAPPDATA")/BobrHopper/tools}
ZIG="$TOOLS/zig-x86_64-windows-0.14.1/zig.exe"
SDL="$TOOLS/r36s-sdl2"
SDL_ARMHF="$TOOLS/armhf-sdl2"   # SDL2 2.0.9 headers + libSDL2.so from a Debian buster armhf sysroot
OUT="$ROOT/out/batocera"
TARGET=${1:-bobrhopper}
[ -f "$SDL/lib/libSDL2.so" ] || { echo "missing $SDL: run sh build/setup_tools.sh"; exit 1; }
[ -f "$SDL_ARMHF/lib/libSDL2.so" ] || { echo "missing $SDL_ARMHF (armhf SDL2, see CLAUDE.md)"; exit 1; }
mkdir -p "$OUT"

. "$ROOT/build/sources.sh"
SOURCES=$(sources_for "$TARGET") || { echo "unknown target $TARGET"; exit 1; }

SRC_ABS=""
for s in $SOURCES; do SRC_ABS="$SRC_ABS $ROOT/$s"; done

BIN="$OUT/$TARGET.aarch64"
"$ZIG" c++ -target aarch64-linux-gnu.2.28 -mcpu=cortex_a53 -std=c++17 -O2 -g0 -Wall -ffp-contract=off   -D_REENTRANT -I"$SDL/include/SDL2" -I"$ROOT/src"   $SRC_ABS -L"$SDL/lib" -lSDL2 -Wl,--allow-shlib-undefined -o "$BIN"

BIN32="$OUT/$TARGET.armhf"
"$ZIG" c++ -target arm-linux-gnueabihf.2.28 -mcpu=cortex_a53 -mfpu=neon-fp-armv8 -std=c++17 -O2 -g0 -Wall -ffp-contract=off   -D_REENTRANT -I"$SDL_ARMHF/include/SDL2" -I"$ROOT/src"   $SRC_ABS -L"$SDL_ARMHF/lib" -lSDL2 -Wl,--allow-shlib-undefined -o "$BIN32"

MSYS_NO_PATHCONV=1 wsl.exe -d Ubuntu-22.04 -e sh -c   "ls '$REPO_WSL' >/dev/null 2>&1 || sudo -n mount -t drvfs ${REPO_DRIVE_UPPER}: /mnt/${REPO_DRIVE}; sh $REPO_WSL/build/check_elf.sh $REPO_WSL/out/batocera/$TARGET.aarch64 && sh $REPO_WSL/build/check_elf.sh $REPO_WSL/out/batocera/$TARGET.armhf"
