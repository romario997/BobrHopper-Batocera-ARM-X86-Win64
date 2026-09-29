#!/bin/sh
# Windows RELEASE build (Git Bash): the game as players get it - out/windows/BobrHopper.exe + SDL2.dll.
# Same compile as build_pc.sh bobrhopper, plus: the GUI subsystem (no console window; the log goes to bobrhopper.log
# next to the settings - see windowsUserDir in apps/bobrhopper.cpp) and the resources of port/windows/bobrhopper.rc
# (icon, version). tools/package_windows.ps1 runs this and zips the result.
# Usage: sh build/build_windows.sh
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd -W)
TOOLS=${CROSSY_TOOLS:-$(cygpath -m "$LOCALAPPDATA")/BobrHopper/tools}
ZIG="$TOOLS/zig-x86_64-windows-0.14.1/zig.exe"
SDL="$TOOLS/SDL2-2.30.12/x86_64-w64-mingw32"
OUT="$ROOT/out/windows"
[ -x "$ZIG" ] || { echo "missing zig in $TOOLS: run sh build/setup_tools.sh"; exit 1; }
mkdir -p "$OUT"

. "$ROOT/build/sources.sh"
SOURCES=$(sources_for bobrhopper)
SRC_ABS=""
for s in $SOURCES; do SRC_ABS="$SRC_ABS $ROOT/$s"; done

[ -f "$ROOT/port/windows/bobrhopper.ico" ] || python "$ROOT/tools/make_windows_icon.py"

"$ZIG" c++ -target x86_64-windows-gnu -std=c++17 -O2 -g0 -Wall -ffp-contract=off \
  -DSDL_MAIN_HANDLED -I"$SDL/include/SDL2" -I"$ROOT/src" \
  $SRC_ABS "$ROOT/port/windows/bobrhopper.rc" "$SDL/lib/libSDL2.dll.a" \
  -Wl,--subsystem,windows -o "$OUT/BobrHopper.exe"
rm -f "$OUT/BobrHopper.pdb"
cp -f "$SDL/bin/SDL2.dll" "$OUT/"
echo "built $OUT/BobrHopper.exe"
