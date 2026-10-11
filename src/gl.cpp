#define UVC_GL_NO_MACROS
#include "gl.h"

#include <cstdio>

#define UVC_GL_DEF(type, name) type uvc_##name = nullptr;
UVC_GL_FUNCS(UVC_GL_DEF)
#undef UVC_GL_DEF

bool uvcLoadGL() {
  bool ok = true;
#define UVC_GL_LOAD(type, name)                                      \
  uvc_##name = (type)SDL_GL_GetProcAddress(#name);                   \
  if (!uvc_##name) {                                                 \
    std::fprintf(stderr, "OpenGL: missing entry point %s\n", #name); \
    ok = false;                                                     \
  }
  UVC_GL_FUNCS(UVC_GL_LOAD)
#undef UVC_GL_LOAD
  return ok;
}
