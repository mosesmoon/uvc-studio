// Minimal OpenGL 3.3 core function loader (via SDL_GL_GetProcAddress).
// On Android (Quest) the app uses OpenGL ES 3.x directly.
#pragma once
#include <SDL.h>
#include <SDL_opengl.h>

#define UVC_GL_FUNCS(X)                                                   \
  X(PFNGLCREATESHADERPROC, glCreateShader)                               \
  X(PFNGLSHADERSOURCEPROC, glShaderSource)                               \
  X(PFNGLCOMPILESHADERPROC, glCompileShader)                             \
  X(PFNGLGETSHADERIVPROC, glGetShaderiv)                                 \
  X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog)                       \
  X(PFNGLDELETESHADERPROC, glDeleteShader)                               \
  X(PFNGLCREATEPROGRAMPROC, glCreateProgram)                             \
  X(PFNGLATTACHSHADERPROC, glAttachShader)                               \
  X(PFNGLLINKPROGRAMPROC, glLinkProgram)                                 \
  X(PFNGLGETPROGRAMIVPROC, glGetProgramiv)                               \
  X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog)                     \
  X(PFNGLDELETEPROGRAMPROC, glDeleteProgram)                             \
  X(PFNGLUSEPROGRAMPROC, glUseProgram)                                   \
  X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation)                   \
  X(PFNGLUNIFORM1IPROC, glUniform1i)                                     \
  X(PFNGLUNIFORM1FPROC, glUniform1f)                                     \
  X(PFNGLUNIFORM2FPROC, glUniform2f)                                     \
  X(PFNGLUNIFORM2IPROC, glUniform2i)                                     \
  X(PFNGLUNIFORM3FPROC, glUniform3f)                                     \
  X(PFNGLUNIFORM4FPROC, glUniform4f)                                     \
  X(PFNGLUNIFORMMATRIX3FVPROC, glUniformMatrix3fv)                       \
  X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays)                         \
  X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray)                         \
  X(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays)                   \
  X(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers)                         \
  X(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer)                         \
  X(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D)               \
  X(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus)           \
  X(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers)                   \
  X(PFNGLGENBUFFERSPROC, glGenBuffers)                                   \
  X(PFNGLBINDBUFFERPROC, glBindBuffer)                                   \
  X(PFNGLBUFFERDATAPROC, glBufferData)                                   \
  X(PFNGLDELETEBUFFERSPROC, glDeleteBuffers)                             \
  X(PFNGLMAPBUFFERRANGEPROC, glMapBufferRange)                           \
  X(PFNGLUNMAPBUFFERPROC, glUnmapBuffer)                                 \
  X(PFNGLACTIVETEXTUREPROC, glActiveTexture)                             \
  X(PFNGLFENCESYNCPROC, glFenceSync)                                     \
  X(PFNGLCLIENTWAITSYNCPROC, glClientWaitSync)                           \
  X(PFNGLDELETESYNCPROC, glDeleteSync)                                   \
  X(PFNGLGENQUERIESPROC, glGenQueries)                                   \
  X(PFNGLDELETEQUERIESPROC, glDeleteQueries)                             \
  X(PFNGLBEGINQUERYPROC, glBeginQuery)                                   \
  X(PFNGLENDQUERYPROC, glEndQuery)                                       \
  X(PFNGLGETQUERYOBJECTUIVPROC, glGetQueryObjectuiv)

#define UVC_GL_DECL(type, name) extern type uvc_##name;
UVC_GL_FUNCS(UVC_GL_DECL)
#undef UVC_GL_DECL

#ifndef UVC_GL_NO_MACROS
#define glCreateShader uvc_glCreateShader
#define glShaderSource uvc_glShaderSource
#define glCompileShader uvc_glCompileShader
#define glGetShaderiv uvc_glGetShaderiv
#define glGetShaderInfoLog uvc_glGetShaderInfoLog
#define glDeleteShader uvc_glDeleteShader
#define glCreateProgram uvc_glCreateProgram
#define glAttachShader uvc_glAttachShader
#define glLinkProgram uvc_glLinkProgram
#define glGetProgramiv uvc_glGetProgramiv
#define glGetProgramInfoLog uvc_glGetProgramInfoLog
#define glDeleteProgram uvc_glDeleteProgram
#define glUseProgram uvc_glUseProgram
#define glGetUniformLocation uvc_glGetUniformLocation
#define glUniform1i uvc_glUniform1i
#define glUniform1f uvc_glUniform1f
#define glUniform2f uvc_glUniform2f
#define glUniform2i uvc_glUniform2i
#define glUniform3f uvc_glUniform3f
#define glUniform4f uvc_glUniform4f
#define glUniformMatrix3fv uvc_glUniformMatrix3fv
#define glGenVertexArrays uvc_glGenVertexArrays
#define glBindVertexArray uvc_glBindVertexArray
#define glDeleteVertexArrays uvc_glDeleteVertexArrays
#define glGenFramebuffers uvc_glGenFramebuffers
#define glBindFramebuffer uvc_glBindFramebuffer
#define glFramebufferTexture2D uvc_glFramebufferTexture2D
#define glCheckFramebufferStatus uvc_glCheckFramebufferStatus
#define glDeleteFramebuffers uvc_glDeleteFramebuffers
#define glGenBuffers uvc_glGenBuffers
#define glBindBuffer uvc_glBindBuffer
#define glBufferData uvc_glBufferData
#define glDeleteBuffers uvc_glDeleteBuffers
#define glMapBufferRange uvc_glMapBufferRange
#define glUnmapBuffer uvc_glUnmapBuffer
#define glActiveTexture uvc_glActiveTexture
#define glFenceSync uvc_glFenceSync
#define glClientWaitSync uvc_glClientWaitSync
#define glDeleteSync uvc_glDeleteSync
#define glGenQueries uvc_glGenQueries
#define glDeleteQueries uvc_glDeleteQueries
#define glBeginQuery uvc_glBeginQuery
#define glEndQuery uvc_glEndQuery
#define glGetQueryObjectuiv uvc_glGetQueryObjectuiv
#endif

// Returns false if a required entry point is missing.
bool uvcLoadGL();
