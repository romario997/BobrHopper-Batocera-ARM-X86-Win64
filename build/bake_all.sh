#!/bin/sh
# Regenerates data/ from the pristine upstream tarball. Run from Git Bash.
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
SRC=upstream/expo-crossy-road/assets
if [ ! -d "$SRC" ]; then
  mkdir -p upstream && tar -xzf upstream/expo-crossy-road-6f2e84e5-noaudio.tar.gz -C upstream
fi
rm -rf data
python tools/bake_models.py --all "$SRC" data
python tools/bake_textures.py --all "$SRC" data
python tools/bake_audio.py --all "$SRC" data
python tools/bake_font.py --all "$SRC" data
# big screens (Batocera PC): the same sizes baked for UI scales 1.5 / 2.25 / 3 (720p, 1080p, 1440p) -
# TextRenderer::pixelScale draws size N from retro_<round(N * scale)>; 480-line screens never load these
python tools/bake_font.py --all "$SRC" data --sizes 21,24,27,36,41,42,54,72,96,108,144
python tools/bake_music.py --all Music data
python tools/write_manifest.py data/manifest.txt
echo "bake_all: $(find data -type f | wc -l) files, $(du -sh data | cut -f1)"
