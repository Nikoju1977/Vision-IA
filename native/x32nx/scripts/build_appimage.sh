#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${ROOT}/../.." && pwd)"

cd "$ROOT"

if [[ ! -x build/vision_ia_node ]]; then
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build --parallel
fi

rm -rf AppDir

mkdir -p   AppDir/usr/bin   AppDir/usr/share/vision-ia

cp build/vision_ia_node   AppDir/usr/bin/

cp build/run_fec_tests   AppDir/usr/bin/

cp build/run_component_tests   AppDir/usr/bin/

cp python/cli_router.py   AppDir/usr/bin/

cp AppRun   AppDir/AppRun

cp vision-ia.desktop   AppDir/vision-ia.desktop

if [[ -f assets/vision-ia.png ]]; then
  cp assets/vision-ia.png     AppDir/vision-ia.png
elif [[ -f "${REPO_ROOT}/icon-512.png" ]]; then
  cp "${REPO_ROOT}/icon-512.png"     AppDir/vision-ia.png
else
  echo "Icône Vision-IA introuvable." >&2
  exit 2
fi

chmod +x   AppDir/AppRun   AppDir/usr/bin/cli_router.py

LINUXDEPLOY="./linuxdeploy-x86_64.AppImage"

if [[ ! -x "$LINUXDEPLOY" ]]; then
  curl -L     https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage     -o "$LINUXDEPLOY"

  chmod +x "$LINUXDEPLOY"
fi

"$LINUXDEPLOY"   --appdir AppDir   --executable AppDir/usr/bin/vision_ia_node   --desktop-file AppDir/vision-ia.desktop   --icon-file AppDir/vision-ia.png   --output appimage
