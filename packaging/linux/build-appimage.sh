#!/bin/bash
# Builds UVCStudio-<version>-x86_64.AppImage. Run inside the Ubuntu 22.04 build root
# (glibc 2.35 baseline) after the static deps in /opt/deps are built.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
source "$ROOT/packaging/version.sh"
BUILD="$ROOT/build-linux"
OUT="${OUT:-$ROOT/dist}"
cmake -S "$ROOT" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release -DDEPS_PREFIX=/opt/deps >/dev/null
ninja -C "$BUILD"
APPDIR="$BUILD/AppDir"
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/share/uvc-studio" "$APPDIR/usr/lib/fallback" \
         "$APPDIR/usr/share/applications" "$APPDIR/usr/share/icons/hicolor/256x256/apps"
install -m755 "$BUILD/uvc-studio" "$APPDIR/usr/bin/"
strip "$APPDIR/usr/bin/uvc-studio"
cp -r "$ROOT/assets/." "$APPDIR/usr/share/uvc-studio/"
for l in libva.so.2 libva-drm.so.2 libdrm.so.2; do
  cp -L "/usr/lib/x86_64-linux-gnu/$l" "$APPDIR/usr/lib/fallback/"
done
install -m755 "$ROOT/packaging/linux/AppRun" "$APPDIR/AppRun"
sed "s/^X-AppImage-Version=.*/X-AppImage-Version=$VNAME/" "$ROOT/packaging/linux/uvc-studio.desktop" > "$APPDIR/uvc-studio.desktop"
cp "$APPDIR/uvc-studio.desktop" "$APPDIR/usr/share/applications/"
cp "$ROOT/assets/images/uvc-studio-256.png" "$APPDIR/uvc-studio.png"
cp "$ROOT/assets/images/uvc-studio-256.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/uvc-studio.png"
ln -sf uvc-studio.png "$APPDIR/.DirIcon"
mkdir -p "$OUT"
ARCH=x86_64 /opt/appimagetool/AppRun --no-appstream --runtime-file /opt/runtime-x86_64 \
  "$APPDIR" "$OUT/UVCStudio-$VFILE-x86_64.AppImage"
ls -la "$OUT"
