#!/bin/bash
# BobrHopper launcher for Batocera ARM (RG35XX H / H700) - also Knulli. Installed as /userdata/roms/ports/BobrHopper.sh
# next to /userdata/roms/ports/bobrhopper/. Batocera's configgen runs it as `/bin/bash <script>` as root, with the pad
# mappings already exported (SDL_GAMECONTROLLERCONFIG, /tmp/gamecontrollerdb.txt).
#
# Video: Batocera v43+ on the H700 runs a Wayland compositor (labwc, Panfrost GLES), Knulli draws through the Mali
# fbdev blob (SDL "mali" driver), and a plain console would take KMSDRM. The launcher does not guess: it tries the
# drivers in order and moves on when the game exits with 4 (no window / GL context) or 5 (renderer / assets). The
# driver that worked is remembered in conf/video_mode and tried first next time.
# Input: the game reads the pad itself - no gptokeyb, no evmapy .keys file (either would double every press).
# Log: every step goes to bobrhopper/bobrhopper-launcher.log followed by sync, so it survives a hard power-off.

progdir="$(cd "$(dirname "$0")" && pwd)"
GAMEDIR="$progdir/bobrhopper"
CONFDIR="$GAMEDIR/conf"
# Two binaries: official Batocera v43+ on the H700 is 64-bit, the community Batocera v40 for the RG35XX H is 32-bit
# armhf on a 64-bit kernel - so `uname -m` says aarch64 on both and only the dynamic loader tells them apart.
if [ -e /lib/ld-linux-aarch64.so.1 ] || [ -e /lib64/ld-linux-aarch64.so.1 ]; then
  BIN="$GAMEDIR/bobrhopper.aarch64"
else
  BIN="$GAMEDIR/bobrhopper.armhf"
fi
LOG="$GAMEDIR/bobrhopper-launcher.log"

log() { echo "[$(date '+%H:%M:%S' 2>/dev/null)] $*" >> "$LOG"; sync 2>/dev/null; }

mkdir -p "$CONFDIR"
: > "$LOG"; sync
log "STEP0  launcher start progdir=$progdir arch=$(uname -m 2>/dev/null) kernel=$(uname -r 2>/dev/null) user=$(id -un 2>/dev/null)"
log "STEP1  system: $(cat /usr/share/batocera/batocera.version 2>/dev/null | head -1) | $(ldd --version 2>/dev/null | head -1)"
log "STEP1a binary=$BIN loaders: $(ls /lib/ld-linux* /lib64/ld-linux* 2>/dev/null | tr '\n' ' ')"
log "STEP1b libSDL2: $(ls /usr/lib/libSDL2-2.0.so.0* /usr/lib/*-linux-gnu*/libSDL2-2.0.so.0* 2>/dev/null | tr '\n' ' ') dri: $(ls /dev/dri 2>/dev/null | tr '\n' ' ') mali: $(ls /dev/mali* 2>/dev/null | tr '\n' ' ')"

# --- environment ---------------------------------------------------------------------------------------------
: "${XDG_RUNTIME_DIR:=/var/run}"
export XDG_RUNTIME_DIR
if [ -z "$WAYLAND_DISPLAY" ] && [ -S "$XDG_RUNTIME_DIR/wayland-0" ]; then
  export WAYLAND_DISPLAY=wayland-0
fi
if [ -z "$DISPLAY" ] && [ -S /tmp/.X11-unix/X0 ]; then
  export DISPLAY=:0.0
fi
# pad mappings: Batocera exports the string; the file is the fallback (the game loads SDL_GAMECONTROLLERCONFIG_FILE
# itself, see src/engine/input_sdl.cpp)
if [ -z "$SDL_GAMECONTROLLERCONFIG" ] && [ -z "$SDL_GAMECONTROLLERCONFIG_FILE" ] && [ -f /tmp/gamecontrollerdb.txt ]; then
  export SDL_GAMECONTROLLERCONFIG_FILE=/tmp/gamecontrollerdb.txt
fi
# Batocera maps pads by POSITION (SDL "a" = the bottom button), but the handhelds' own buttons are labelled the
# Nintendo way: A is the RIGHT one - so pressing A arrived as "b" (seen on the RG35XX H, community v40:
# 'Deeplay-keys' b:b3,a:b4). Swap a/b for the built-in pad only; an external Xbox-style pad keeps its mapping.
swap_ab() { sed -E '/,(Deeplay-keys|H700 Gamepad),/{s/,a:/,@A@:/;s/,b:/,a:/;s/,@A@:/,b:/}'; }
if [ -n "$SDL_GAMECONTROLLERCONFIG" ]; then
  SDL_GAMECONTROLLERCONFIG=$(printf '%s\n' "$SDL_GAMECONTROLLERCONFIG" | swap_ab)
  export SDL_GAMECONTROLLERCONFIG
elif [ -n "$SDL_GAMECONTROLLERCONFIG_FILE" ] && [ -f "$SDL_GAMECONTROLLERCONFIG_FILE" ]; then
  swap_ab < "$SDL_GAMECONTROLLERCONFIG_FILE" > /tmp/bobrhopper-gamecontrollerdb.txt && \
    export SDL_GAMECONTROLLERCONFIG_FILE=/tmp/bobrhopper-gamecontrollerdb.txt
fi
log "STEP2  WAYLAND_DISPLAY=${WAYLAND_DISPLAY:-none} DISPLAY=${DISPLAY:-none} XDG_RUNTIME_DIR=$XDG_RUNTIME_DIR pad_mapping=$([ -n "$SDL_GAMECONTROLLERCONFIG" ] && echo string || echo no) pad_mapping_file=${SDL_GAMECONTROLLERCONFIG_FILE:-none}"

cd "$GAMEDIR" || { log "FATAL  no game directory $GAMEDIR"; exit 1; }
chmod +x "$BIN" 2>/dev/null
if [ ! -x "$BIN" ]; then
  log "FATAL  binary missing or not executable: $BIN"
  exit 1
fi

# --- choosing the video driver -------------------------------------------------------------------------------
video_failed() { [ "$1" -eq 4 ] || [ "$1" -eq 5 ]; }

candidates=""
add() { case " $candidates " in *" $1 "*) ;; *) candidates="$candidates $1" ;; esac; }
remembered=$(cat "$CONFDIR/video_mode" 2>/dev/null)
[ -n "$remembered" ] && add "$remembered"
[ -n "$WAYLAND_DISPLAY" ] && add wayland
[ -n "$DISPLAY" ] && add x11
[ -e /dev/mali0 ] || [ -e /dev/mali ] && add mali
add kmsdrm
add default     # no SDL_VIDEODRIVER at all: SDL picks
log "STEP3  video drivers to try:$candidates"

rc=1
for drv in $candidates; do
  if [ "$drv" = default ]; then unset SDL_VIDEODRIVER; else export SDL_VIDEODRIVER="$drv"; fi
  log "STEP4  run with SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-<unset>}: $BIN $*"
  echo "----- game output ($drv) -----" >> "$LOG"; sync
  "$BIN" "$@" >> "$LOG" 2>&1
  rc=$?
  log "STEP5  exit rc=$rc ($drv)"
  if ! video_failed $rc; then
    echo "$drv" > "$CONFDIR/video_mode"; sync
    break
  fi
done
video_failed $rc && log "FATAL  no video driver worked - bring bobrhopper/bobrhopper-launcher.log to the PC"
log "STEP9  done"
sync
exit 0
