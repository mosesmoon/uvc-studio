#!/bin/bash
# Static deps for the Linux build: SDL2 and an LGPL-only FFmpeg (no x264 —
# recording uses VAAPI / NVENC / AMF hardware encoders).
set -e
P=/opt/deps
mkdir -p $P
export PKG_CONFIG_PATH=$P/lib/pkgconfig
J=$(nproc)
cd /src/nv-codec-headers && make PREFIX=$P install >/dev/null
mkdir -p $P/include/AMF && cp -r /src/amf/amf/public/include/* $P/include/AMF/
cd /src/SDL2 && mkdir -p build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$P -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_STATIC_PIC=ON -DSDL_TEST=OFF >/dev/null
ninja >/dev/null && ninja install >/dev/null
echo "sdl done"
cd /src/ffmpeg && mkdir -p build && cd build
../configure --prefix=$P --pkg-config-flags=--static --extra-cflags="-I$P/include" --extra-ldflags="-L$P/lib" \
 --disable-everything --disable-programs --disable-doc --disable-debug --enable-static --disable-shared --enable-pic \
 --enable-vaapi --enable-ffnvcodec --enable-nvenc --enable-amf \
 --disable-xlib --disable-libxcb --disable-sdl2 --disable-iconv --disable-zlib --disable-bzlib --disable-lzma --disable-vdpau --disable-vulkan \
 --enable-decoder=mjpeg,h264,hevc,rawvideo \
 --enable-encoder=aac,h264_vaapi,h264_nvenc,h264_amf \
 --enable-muxer=mp4,mov --enable-protocol=file --enable-parser=mjpeg,h264,hevc,aac \
 --enable-bsf=extract_extradata,h264_mp4toannexb,aac_adtstoasc \
 --enable-swscale --enable-swresample --enable-hwaccel=h264_vaapi > /tmp/ffconf.log
make -j$J >/dev/null 2>/tmp/ffmake.err && make install >/dev/null
echo "ffmpeg done"
