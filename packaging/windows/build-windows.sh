#!/bin/bash
# Cross-compiles UVCStudio.exe with mingw-w64 and packages the NSIS
# installer + portable zip. Static deps (SDL2, LGPL-only FFmpeg) in
# /opt/win-deps.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
source "$ROOT/packaging/version.sh"
TOOLCHAIN="${TOOLCHAIN:-$ROOT/packaging/windows/toolchain-mingw64.cmake}"
cmake -S "$ROOT" -B "$ROOT/build-win" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
  -DCMAKE_BUILD_TYPE=Release -DDEPS_PREFIX=/opt/win-deps >/dev/null
ninja -C "$ROOT/build-win"
STAGE="$ROOT/dist-win/UVCStudio"
rm -rf "$ROOT/dist-win" && mkdir -p "$STAGE"
cp "$ROOT/build-win/UVCStudio.exe" "$STAGE/"
x86_64-w64-mingw32-strip "$STAGE/UVCStudio.exe"
cp -r "$ROOT/assets" "$STAGE/assets"
cp "$ROOT/README.md" "$STAGE/README.txt"
cp "$ROOT/packaging/windows/installer.nsi" "$ROOT/packaging/windows/uvc-studio.ico" "$ROOT/dist-win/"
(cd "$ROOT/dist-win" && makensis -V2 -DSRCDIR=UVCStudio "-DVERSION=$VNAME" "-DVERFILE=$VFILE" "-DVERNUM=$VNUM" installer.nsi && rm installer.nsi uvc-studio.ico)
(cd "$ROOT/dist-win" && zip -qr "UVCStudio-Portable-$VFILE.zip" UVCStudio)
mkdir -p "$ROOT/dist"
cp "$ROOT/dist-win/UVCStudio-Setup-$VFILE.exe" "$ROOT/dist-win/UVCStudio-Portable-$VFILE.zip" "$ROOT/dist/"
ls -la "$ROOT/dist"
