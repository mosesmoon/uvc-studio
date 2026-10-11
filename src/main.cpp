// UVC Studio — native desktop edition (Linux / Windows).
#include <SDL.h>

#include <algorithm>
#include <cstdio>
#include <string>

#include "App.h"
#include "Platform.h"
#include "Ui.h"
#include "gl.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "stb_image.h"

#ifdef _WIN32
#include <windows.h>
#endif

static void setWindowIcon(SDL_Window* w) {
  std::string p = platform::joinPath(platform::assetsDir(), "images/uvc-studio-256.png");
  int iw, ih, n;
  unsigned char* px = stbi_load(p.c_str(), &iw, &ih, &n, 4);
  if (!px) return;
  SDL_Surface* s = SDL_CreateRGBSurfaceWithFormatFrom(px, iw, ih, 32, iw * 4, SDL_PIXELFORMAT_RGBA32);
  if (s) {
    SDL_SetWindowIcon(w, s);
    SDL_FreeSurface(s);
  }
  stbi_image_free(px);
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
#ifdef _WIN32
  {
    // No console on Windows: keep a log next to the settings for support.
    std::string log = platform::joinPath(platform::configDir(), "uvc-studio.log");
    int n = MultiByteToWideChar(CP_UTF8, 0, log.c_str(), -1, nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, log.c_str(), -1, w.data(), n);
    _wfreopen(w.c_str(), L"w", stdout);
    _wfreopen(w.c_str(), L"a", stderr);
    setvbuf(stdout, nullptr, _IONBF, 0);
  }
  SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
#endif
  SDL_SetHint(SDL_HINT_APP_NAME, "UVC Studio");
  SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
  SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
  SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "0");
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
    std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);

  // Initial window size follows the same automatic scale as the UI.
  float dpiScale = platform::displayScale(0);
  int ww = (int)(1280 * dpiScale), wh = (int)(800 * dpiScale);
  SDL_DisplayMode dm;
  if (SDL_GetDesktopDisplayMode(0, &dm) == 0) {
    ww = std::min(ww, dm.w - 80);
    wh = std::min(wh, dm.h - 120);
  }
  SDL_Window* win = SDL_CreateWindow("UVC Studio", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ww, wh,
                                     SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  if (!win) {
    std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    return 1;
  }
  SDL_SetWindowMinimumSize(win, (int)(640 * dpiScale), (int)(480 * dpiScale));
  SDL_GLContext gl = SDL_GL_CreateContext(win);
  if (!gl) {
    std::fprintf(stderr, "OpenGL 3.3 context failed: %s\n", SDL_GetError());
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "UVC Studio",
                             "OpenGL 3.3 is required. Please update your graphics driver.", win);
    return 1;
  }
  SDL_GL_MakeCurrent(win, gl);
  if (SDL_GL_SetSwapInterval(-1) != 0) SDL_GL_SetSwapInterval(1);
  if (!uvcLoadGL()) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "UVC Studio", "OpenGL 3.3 entry points missing.", win);
    return 1;
  }
  setWindowIcon(win);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.LogFilename = nullptr;
  io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange * 0;
  io.ConfigWindowsMoveFromTitleBarOnly = true;
  ImGui_ImplSDL2_InitForOpenGL(win, gl);
  ImGui_ImplOpenGL3_Init("#version 330 core");
  if (!ui::loadFonts(platform::assetsDir()))
    std::fprintf(stderr, "warning: CJK fonts not found under %s\n", platform::assetsDir().c_str());

  App app;
  app.init(win);

  while (!app.quitRequested()) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      ImGui_ImplSDL2_ProcessEvent(&e);
      app.onEvent(e);
    }
    if (SDL_GetWindowFlags(win) & SDL_WINDOW_MINIMIZED) {
      app.preRender();  // keep recording / draining frames
      SDL_Delay(10);
      continue;
    }
    app.preRender();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    app.frame();
    ImGui::Render();
    int fbw, fbh;
    SDL_GL_GetDrawableSize(win, &fbw, &fbh);
    glViewport(0, 0, fbw, fbh);
    glClearColor(0.039f, 0.039f, 0.039f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(win);
  }

  app.shutdown();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  SDL_GL_DeleteContext(gl);
  SDL_DestroyWindow(win);
  SDL_Quit();
  return 0;
}
