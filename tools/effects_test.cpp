// Offline check of the GPU effects on a still image (desktop only):
//   effects_test <in.png> <out-dir> [part ...]
// Parts: depth (depth.png, enhanced.png), stereo (left.png, right.png,
// anaglyph.png, sbs.png = cross-eye side by side, tab.png), retro (crt.png,
// vhs.png, arcade.png, and the same over FSR as *-fsr.png), picture (one
// image per picture preset), upscale (fsr, anime4k, lanczos_cas, nis,
// bilinear), timing. Without parts: everything except timing.
// UVC_FX_AMOUNT sets the stereo shift (largest per-eye shift as a fraction
// of the width; default 0.02, the Quest immersive maximum).
#include <SDL.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "Depth3D.h"
#include "Frame.h"
#include "FrameDecoder.h"
#include "VideoRenderer.h"
#include "gl.h"
#include "stb_image.h"
#include "stb_image_write.h"

static void save(const std::string& path, const GpuImage& img) {
  std::vector<uint8_t> px;
  readImage(img, px);
  stbi_write_png(path.c_str(), img.w, img.h, 4, px.data(), img.w * 4);
  std::printf("wrote %s (%dx%d)\n", path.c_str(), img.w, img.h);
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: %s in.png out-dir\n", argv[0]);
    return 2;
  }
  std::string out = argv[2];
  std::vector<std::string> parts;
  for (int i = 3; i < argc; ++i) parts.push_back(argv[i]);
  auto want = [&](const char* p) {
    if (parts.empty()) return std::strcmp(p, "timing") != 0;
    for (auto& q : parts)
      if (q == p) return true;
    return false;
  };
  float amount = 0.02f;
  if (const char* a = std::getenv("UVC_FX_AMOUNT")) amount = (float)std::atof(a);
  SDL_Init(SDL_INIT_VIDEO);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_Window* w = SDL_CreateWindow("fx", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  SDL_GLContext ctx = SDL_GL_CreateContext(w);
  if (!ctx || !uvcLoadGL()) {
    std::fprintf(stderr, "no GL: %s\n", SDL_GetError());
    return 1;
  }
  if (!GpuPrograms::get().init() || !DepthPrograms::get().init()) return 1;

  int W, H, n;
  unsigned char* px = stbi_load(argv[1], &W, &H, &n, 3);
  if (!px) return 1;
  Frame f;
  fillRawFrame(PixFmt::RGB24, W, H, W * 3, px, (size_t)W * H * 3, f);
  VideoPipeline pipe;
  pipe.upload(f);

  // Depth map (a few updates so the temporal filter settles).
  DepthEngine depth;
  for (int i = 0; i < 20; ++i) depth.update(pipe.src());
  if (want("depth")) {
    GpuImage vis;
    vis.ensure(depth.depth().w, depth.depth().h);
    glBindFramebuffer(GL_FRAMEBUFFER, vis.fbo);
    glViewport(0, 0, vis.w, vis.h);
    auto& P = GpuPrograms::get();
    glUseProgram(P.blit);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depth.depth().tex);
    glUniform1i(glGetUniformLocation(P.blit, "u_tex"), 0);
    P.quad(P.blit, vis.w, vis.h, 0, 0, (float)vis.w, (float)vis.h);
    save(out + "/depth.png", vis);
    // Full-precision copy for analysis: int32 width, int32 height, then the
    // depth values (float32, row 0 = top).
    {
      const GpuImage& dm = depth.depth();
      std::vector<float> f((size_t)dm.w * dm.h * 4), r((size_t)dm.w * dm.h);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, dm.fbo);
      glReadPixels(0, 0, dm.w, dm.h, GL_RGBA, GL_FLOAT, f.data());
      for (size_t i = 0; i < r.size(); ++i) r[i] = f[i * 4];
      FILE* fp = std::fopen((out + "/depth.f32").c_str(), "wb");
      if (fp) {
        int32_t wh[2] = {dm.w, dm.h};
        std::fwrite(wh, sizeof wh, 1, fp);
        std::fwrite(r.data(), sizeof(float), r.size(), fp);
        std::fclose(fp);
      }
    }
  }
  // 2D enhancer at full strength.
  if (want("depth")) {
    GpuImage e;
    depth.enhance(pipe.src(), e, 1.f);
    save(out + "/enhanced.png", e);
  }
  // Stereo pair.
  if (want("stereo")) {
    GpuImage L, R;
    for (int eye : {-1, 1}) {
      GpuImage& t = eye < 0 ? L : R;
      t.ensure(W, H);
      glBindFramebuffer(GL_FRAMEBUFFER, t.fbo);
      depth.drawStereo(pipe.src(), eye, amount, W, H, false);
    }
    save(out + "/left.png", L);
    save(out + "/right.png", R);
    {
      std::vector<uint8_t> l, r, a((size_t)W * H * 4), sbs((size_t)W * H * 4);
      readImage(L, l);
      readImage(R, r);
      for (size_t i = 0; i < (size_t)W * H; ++i) {
        a[i * 4 + 0] = l[i * 4 + 0];
        a[i * 4 + 1] = r[i * 4 + 1];
        a[i * 4 + 2] = r[i * 4 + 2];
        a[i * 4 + 3] = 255;
      }
      stbi_write_png((out + "/anaglyph.png").c_str(), W, H, 4, a.data(), W * 4);
      // Cross-eye side by side at half size: right view on the left.
      for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
          const std::vector<uint8_t>& s = x < W / 2 ? r : l;
          int sx = (x % (W / 2)) * 2;
          std::memcpy(&sbs[((size_t)y * W + x) * 4], &s[((size_t)y * W + sx) * 4], 4);
        }
      stbi_write_png((out + "/sbs.png").c_str(), W, H, 4, sbs.data(), W * 4);
      std::printf("wrote anaglyph.png, sbs.png\n");
    }
    // Top-and-bottom layout (left eye on top).
    {
      GpuImage tab;
      tab.ensure(W, H);
      glBindFramebuffer(GL_FRAMEBUFFER, tab.fbo);
      depth.drawStereo(pipe.src(), -1, amount, W, H / 2, false, 0, 0);
      depth.drawStereo(pipe.src(), 1, amount, W, H - H / 2, false, 0, H / 2);
      save(out + "/tab.png", tab);
    }
  }
  // TV styles, on the picture as it is and after FSR (shown at 1920 wide).
  if (want("retro")) {
    const char* names[] = {"", "crt", "vhs", "arcade"};
    pipe.setDisplaySize(1920, 1080);
    pipe.setTime(12.3);
    for (int r = RetroCrt; r <= RetroArcade; ++r) {
      pipe.process(Effect::None, ColorAdjust{}, r);
      save(out + "/" + names[r] + ".png", pipe.view());
      pipe.process(Effect::Upscale, ColorAdjust{}, r);
      save(out + "/" + names[r] + "-fsr.png", pipe.view());
    }
    // VHS over a few seconds: the tracking band and noise move.
    for (int i = 0; i < 4; ++i) {
      pipe.setTime(3.0 + i * 2.2);
      pipe.process(Effect::None, ColorAdjust{}, RetroVhs);
      save(out + "/vhs-t" + std::to_string(i) + ".png", pipe.view());
    }
  }
  // Picture looks: brightness, contrast, saturation as gains, gamma.
  if (want("picture")) {
    struct Look {
      const char* name;
      ColorAdjust c;
    };
    const char* env = std::getenv("UVC_FX_LOOKS");  // name:b:c:s:g;...
    std::vector<Look> looks;
    if (env) {
      std::string all = env;
      size_t p = 0;
      while (p < all.size()) {
        size_t e = all.find(';', p);
        std::string item = all.substr(p, e == std::string::npos ? std::string::npos : e - p);
        char nm[64];
        float b, c, sa, g;
        if (std::sscanf(item.c_str(), "%63[^:]:%f:%f:%f:%f", nm, &b, &c, &sa, &g) == 5) {
          Look l{strdup(nm), {}};
          l.c.brightness = 1 + b / 100, l.c.contrast = 1 + c / 100, l.c.saturation = 1 + sa / 100, l.c.gamma = g;
          looks.push_back(l);
        }
        if (e == std::string::npos) break;
        p = e + 1;
      }
    }
    for (auto& l : looks) {
      pipe.process(Effect::None, l.c);
      save(out + "/look-" + l.name + ".png", pipe.view());
    }
  }
  if (want("upscale")) {
    pipe.process(Effect::Upscale, ColorAdjust{});
    save(out + "/fsr.png", pipe.view());
    {
      int UW = pipe.view().w, UH = pipe.view().h;
      GpuImage bl;
      bl.ensure(UW, UH);
      glBindFramebuffer(GL_FRAMEBUFFER, bl.fbo);
      glViewport(0, 0, UW, UH);
      auto& P = GpuPrograms::get();
      glUseProgram(P.blit);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, pipe.src().tex);
      glUniform1i(glGetUniformLocation(P.blit, "u_tex"), 0);
      P.quad(P.blit, UW, UH, 0, 0, (float)UW, (float)UH);
      save(out + "/bilinear.png", bl);
    }
    pipe.process(Effect::Anime4K, ColorAdjust{});
    save(out + "/anime4k.png", pipe.view());
    pipe.process(Effect::LanczosCas, ColorAdjust{});
    save(out + "/lanczos_cas.png", pipe.view());
    pipe.process(Effect::Nis, ColorAdjust{});
    save(out + "/nis.png", pipe.view());
  }
  // Timing (GPU, finished per iteration).
  if (want("timing")) {
    const char* names[] = {"", "fsr", "anime4k", "lanczos_cas", "nis"};
    for (Effect e : {Effect::Upscale, Effect::Anime4K, Effect::LanczosCas, Effect::Nis}) {
      pipe.process(e, ColorAdjust{});
      glFinish();
      Uint64 t0 = SDL_GetPerformanceCounter();
      for (int i = 0; i < 20; ++i) pipe.process(e, ColorAdjust{});
      glFinish();
      double ms = (SDL_GetPerformanceCounter() - t0) * 1000.0 / SDL_GetPerformanceFrequency() / 20;
      std::printf("%s: %.2f ms/frame\n", names[(int)e], ms);
    }
  }
  stbi_image_free(px);
  return 0;
}
