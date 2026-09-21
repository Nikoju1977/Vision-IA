#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

FPS="${VISIONIA_FPS:-30}"
OUTPUT_DIR="${VISIONIA_OUTPUT_DIR:-output_frames}"
AUDIO_FILE="${VISIONIA_AUDIO_FILE:-assets/instrumental_ebm_industrial.wav}"
OUTPUT_FILE="${VISIONIA_MASTER_FILE:-master_thelovestronautes.mp4}"

if [[ ! -x build/vision_ia_node ]]; then
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build --parallel
fi

timeout 10s ./build/vision_ia_node --port 8001 || true

mkdir -p "$OUTPUT_DIR"

blender   --background   --python python/setup_cinematic_compositor.py   --render-output "//${OUTPUT_DIR}/frame_"   --render-format PNG   -a

if ! compgen -G "${OUTPUT_DIR}/frame_*.png" >/dev/null; then
  echo "Aucune frame Blender rendue dans ${OUTPUT_DIR}." >&2
  exit 3
fi

FFMPEG_ARGS=(
  -y
  -framerate "$FPS"
  -i "${OUTPUT_DIR}/frame_%04d.png"
)

if [[ -f "$AUDIO_FILE" ]]; then
  FFMPEG_ARGS+=(
    -i "$AUDIO_FILE"
  )
fi

FFMPEG_ARGS+=(
  -c:v libx265
  -crf 18
  -preset slow
  -pix_fmt yuv420p10le
)

if [[ -f "$AUDIO_FILE" ]]; then
  FFMPEG_ARGS+=(
    -c:a aac
    -b:a 320k
    -shortest
  )
fi

FFMPEG_ARGS+=(
  "$OUTPUT_FILE"
)

ffmpeg "${FFMPEG_ARGS[@]}"

echo "Master créé : $OUTPUT_FILE"
