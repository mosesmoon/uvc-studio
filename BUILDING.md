# Building UVC Studio (Linux and Windows)

UVC Studio is C++17 with CMake. Third-party code is in `third_party/` (Dear
ImGui, miniaudio, stb, NanoSVG); SDL2 and FFmpeg (LGPL-only build) are linked
statically. The version comes from `VERSION`.

## Linux (AppImage)

Built inside an Ubuntu 22.04 root (glibc 2.35 baseline):

1. Build the static dependencies (SDL2 and a minimal LGPL-only FFmpeg 7.1)
   into `/opt/deps`: `packaging/linux/build-deps.sh`.
2. `packaging/linux/build-appimage.sh` — output
   `dist/UVCStudio-v<version>-x86_64.AppImage`.

For development, a plain CMake build works too:

```
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build-linux
./build-linux/uvc-studio
```

Recording uses the graphics card's hardware H.264 encoder (VA-API, NVENC or
AMF); there is no software encoder.

## Windows

Cross-compiled with mingw-w64 against static dependencies in
`/opt/win-deps`; NSIS builds the installer:
`packaging/windows/build-windows.sh` — output
`dist/UVCStudio-Setup-v<version>.exe` and
`dist/UVCStudio-Portable-v<version>.zip`.

Recording uses NVENC, AMF or Intel Quick Sync through Media Foundation, else
Windows' built-in H.264 encoder.

## Strings, shaders, tests

- Interface strings live in `tools/gen_i18n.py` (12 languages, `area.name`
  keys); run it to regenerate `src/I18nData.inc`.
- `tools/gen_anime4k.py` regenerates `src/Anime4KData.inc` from the Anime4K
  mpv shaders; `tools/gen_nis_coef.py` extracts the NIS filter banks into
  `src/NISCoef.inc`.
- `cmake --build <dir> --target effects-test` builds an offline check that
  renders the depth map, the stereo pair, the retro looks, the picture looks
  and every upscaler for a still image.

## Licence

MIT — see `LICENSE`. Third-party components keep their own licences
(`assets/licenses/`, and Settings → About → Open-source licenses in the app).
