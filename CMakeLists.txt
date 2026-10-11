cmake_minimum_required(VERSION 3.16)
project(UVCStudio LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Where the static SDL2 / FFmpeg builds live (see packaging/*/build-deps.sh).
set(DEPS_PREFIX "/opt/deps" CACHE PATH "Prefix of third-party static libraries")
set(ENV{PKG_CONFIG_PATH} "${DEPS_PREFIX}/lib/pkgconfig")
if(CMAKE_CROSSCOMPILING)
  set(ENV{PKG_CONFIG_LIBDIR} "${DEPS_PREFIX}/lib/pkgconfig")
endif()
find_package(PkgConfig REQUIRED)
set(PKG_CONFIG_EXECUTABLE ${PKG_CONFIG_EXECUTABLE})
list(APPEND PKG_CONFIG_EXECUTABLE "--static")
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libswscale libswresample libavutil)

include(cmake/Version.cmake)
uvc_read_version(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/gen)

set(IMGUI third_party/imgui)
add_executable(uvc-studio WIN32
  src/main.cpp
  src/App.cpp
  src/AppUi.cpp
  src/AppActions.cpp
  src/AudioEngine.cpp
  src/FrameDecoder.cpp
  src/I18n.cpp
  src/Icons.cpp
  src/Platform.cpp
  src/Recorder.cpp
  src/Ui.cpp
  src/VideoCapture_common.cpp
  src/VideoRenderer.cpp
  src/Depth3D.cpp
  src/gl.cpp
  src/stb_impl.cpp
  ${IMGUI}/imgui.cpp
  ${IMGUI}/imgui_draw.cpp
  ${IMGUI}/imgui_tables.cpp
  ${IMGUI}/imgui_widgets.cpp
  ${IMGUI}/backends/imgui_impl_sdl2.cpp
  ${IMGUI}/backends/imgui_impl_opengl3.cpp
)
if(WIN32)
  target_sources(uvc-studio PRIVATE src/VideoCapture_mf.cpp packaging/windows/uvc-studio.rc)
else()
  target_sources(uvc-studio PRIVATE src/VideoCapture_v4l2.cpp)
endif()

target_include_directories(uvc-studio PRIVATE src third_party ${IMGUI} ${IMGUI}/backends ${CMAKE_CURRENT_BINARY_DIR}/gen)
target_compile_definitions(uvc-studio PRIVATE IMGUI_DEFINE_MATH_OPERATORS)
target_link_libraries(uvc-studio PRIVATE PkgConfig::SDL2 PkgConfig::FFMPEG)

if(WIN32)
  target_link_libraries(uvc-studio PRIVATE opengl32 mfplat mfreadwrite mf mfuuid strmiids ole32 oleaut32 shlwapi
    shell32 uuid bcrypt secur32 ws2_32 winmm setupapi imm32 version cfgmgr32 dxgi d3d11)
  target_link_options(uvc-studio PRIVATE -static -static-libgcc -static-libstdc++)
  set_target_properties(uvc-studio PROPERTIES OUTPUT_NAME UVCStudio)
  target_compile_definitions(uvc-studio PRIVATE _WIN32_WINNT=0x0A00 WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
else()
  target_link_libraries(uvc-studio PRIVATE GL pthread dl m)
  target_link_options(uvc-studio PRIVATE -static-libstdc++ -static-libgcc -Wl,--as-needed)
  set_target_properties(uvc-studio PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
endif()

install(TARGETS uvc-studio RUNTIME DESTINATION bin)
install(DIRECTORY assets/ DESTINATION share/uvc-studio)

# Offline effects check (tools/effects_test.cpp): cmake --build <dir> --target effects-test
if(NOT WIN32)
  add_executable(effects-test EXCLUDE_FROM_ALL
    tools/effects_test.cpp src/VideoRenderer.cpp src/Depth3D.cpp src/FrameDecoder.cpp src/gl.cpp src/stb_impl.cpp)
  target_include_directories(effects-test PRIVATE src third_party ${CMAKE_CURRENT_BINARY_DIR}/gen)
  target_link_libraries(effects-test PRIVATE PkgConfig::SDL2 PkgConfig::FFMPEG GL pthread dl m)
endif()
