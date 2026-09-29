#!/bin/bash
# BobrHopper launcher for Batocera PC (x86_64). Installed as /userdata/roms/ports/BobrHopper.sh next to
# /userdata/roms/ports/bobrhopper/. Batocera's configgen runs it as `/bin/bash <script>` as root, with DISPLAY=:0 and
# the pad mappings already exported (SDL_GAMECONTROLLERCONFIG, /tmp/gamecontrollerdb.txt).
#
# Video: Batocera PC (v42 on the author's machine: AMD GPU, 1920x1080 on DP-1) runs EmulationStation on an X11 server;
# newer builds may bring a Wayland compositor, and a bare console would take KMSDRM. The launcher does not guess: it
# tries the drivers in order and moves on when the game exits with 4 (no window / GL context) or 5 (renderer / assets).
# The driver that worked is remembered in conf/video_mode and tried first next time. The game always goes fullscreen
# at the desktop resolution (SDL_WINDOW_FULLSCREEN_DESKTOP) and scales its 640x480 UI to it (2.25x at 1080p).
# Input: the game reads the pad itself - no gptokeyb, no evmapy .keys file (either would double every press).
# Log: every step goes to bobrhopper/bobrhopper-launcher.log followed by sync, so it survives a hard power-off.

progdir="$(cd "$(dirname "$0")" && pwd)"
GAMEDIR="$progdir/bobrhopper"
CONFDIR="$GAMEDIR/conf"
BIN="$GAMEDIR/bobrhopper.x86_64"
LOG="$GAMEDIR/bobrhopper-launcher.log"

log() { echo "[$(date '+%H:%M:%S' 2>/dev/null)] $*" >> "$LOG"; sync 2>/dev/null; }

mkdir -p "$CONFDIR"
: > "$LOG"; sync
log "STEP0  launcher start progdir=$progdir arch=$(uname -m 2>/dev/null) kernel=$(uname -r 2>/dev/null) user=$(id -un 2>/dev/null)"
log "STEP1  system: $(cat /usr/share/batocera/batocera.version 2>/dev/null | head -1) | $(ldd --version 2>/dev/null | head -1)"
log "STEP1a binary=$BIN libSDL2: $(ls /usr/lib/libSDL2-2.0.so.0* /usr/lib64/libSDL2-2.0.so.0* /usr/lib/x86_64-linux-gnu/libSDL2-2.0.so.0* 2>/dev/null | tr '\n' ' ') dri: $(ls /dev/dri 2>/dev/null | tr '\n' ' ')"
log "STEP1b mount: $(grep " $(df -P "$GAMEDIR" 2>/dev/null | awk 'NR==2{print $6}') " /proc/mounts 2>/dev/null | head -1)"

if [ "$(uname -m 2>/dev/null)" != "x86_64" ]; then
  log "FATAL  this package is for Batocera PC (x86_64), this machine is $(uname -m) - use the ARM package"
  exit 1
fi

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
# X11: do not let SDL turn the desktop compositor off / minimise the window when it loses focus to a notification
export SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS=0
log "STEP2  WAYLAND_DISPLAY=${WAYLAND_DISPLAY:-none} DISPLAY=${DISPLAY:-none} XDG_RUNTIME_DIR=$XDG_RUNTIME_DIR pad_mapping=$([ -n "$SDL_GAMECONTROLLERCONFIG" ] && echo string || echo no) pad_mapping_file=${SDL_GAMECONTROLLERCONFIG_FILE:-none}"

cd "$GAMEDIR" || { log "FATAL  no game directory $GAMEDIR"; exit 1; }
if [ ! -f "$BIN" ]; then
  log "FATAL  binary missing: $BIN"
  exit 1
fi
chmod +x "$BIN" 2>/dev/null

# The SHARE partition may be exFAT/NTFS (the author's is exFAT, label ROMS) - normally mounted with exec rights, but a
# noexec mount would refuse the binary. Then it runs from a copy in /tmp, in a folder whose data/ and conf/ point back
# here (the game finds both next to its own executable), so settings and the best score still land in bobrhopper/conf.
RUNBIN="$BIN"
MNT=$(df -P "$GAMEDIR" 2>/dev/null | awk 'NR==2{print $6}')
MNTOPTS=$(awk -v m="$MNT" '$2 == m {print $4}' /proc/mounts 2>/dev/null | tail -1)
if [ ! -x "$BIN" ] || case ",$MNTOPTS," in *,noexec,*) true ;; *) false ;; esac; then
  RUNDIR=/tmp/bobrhopper-run
  rm -rf "$RUNDIR"; mkdir -p "$RUNDIR"
  cp "$BIN" "$RUNDIR/bobrhopper.x86_64" && chmod +x "$RUNDIR/bobrhopper.x86_64"
  ln -s "$GAMEDIR/data" "$RUNDIR/data"
  ln -s "$CONFDIR" "$RUNDIR/conf"
  RUNBIN="$RUNDIR/bobrhopper.x86_64"
  log "STEP2a binary not executable where it is (noexec mount?) - running a copy: $RUNBIN"
fi

# --- choosing the video driver -------------------------------------------------------------------------------
video_failed() { [ "$1" -eq 4 ] || [ "$1" -eq 5 ]; }

candidates=""
add() { case " $candidates " in *" $1 "*) ;; *) candidates="$candidates $1" ;; esac; }
remembered=$(cat "$CONFDIR/video_mode" 2>/dev/null)
[ -n "$remembered" ] && add "$remembered"
[ -n "$WAYLAND_DISPLAY" ] && add wayland
[ -n "$DISPLAY" ] && add x11
add kmsdrm
add default     # no SDL_VIDEODRIVER at all: SDL picks
log "STEP3  video drivers to try:$candidates"

rc=1
for drv in $candidates; do
  if [ "$drv" = default ]; then unset SDL_VIDEODRIVER; else export SDL_VIDEODRIVER="$drv"; fi
  log "STEP4  run with SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-<unset>}: $RUNBIN $*"
  echo "----- game output ($drv) -----" >> "$LOG"; sync
  "$RUNBIN" "$@" >> "$LOG" 2>&1
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
