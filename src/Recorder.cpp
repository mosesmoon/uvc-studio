#include "Recorder.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "AudioEngine.h"
#include "Frame.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

struct Recorder::Impl {
  AVFormatContext* fmt = nullptr;
  AVCodecContext* venc = nullptr;
  AVCodecContext* aenc = nullptr;
  AVStream* vst = nullptr;
  AVStream* ast = nullptr;
  AVBufferRef* hwdev = nullptr;
  AVBufferRef* hwframes = nullptr;
  SwsContext* sws = nullptr;
  int swsW = 0, swsH = 0;
  AVFrame* vframe = nullptr;  // software frame in encoder pixel format
  AVFrame* aframe = nullptr;
  AVPacket* pkt = nullptr;
  AVPixelFormat swFmt = AV_PIX_FMT_YUV420P;
  int64_t lastVpts = -1;
  int64_t aSamples = 0;
  std::vector<float> aAccum;  // interleaved stereo waiting for a full AAC frame
  bool hw = false;
};

static std::string averr(int e) {
  char buf[256];
  av_strerror(e, buf, sizeof buf);
  return buf;
}

Recorder::Recorder() {}
bool Recorder::acceptsTexture() const { return false; }
void Recorder::pushTexture(unsigned, int, int, double) {}
Recorder::~Recorder() {
  stop();
  if (thread_.joinable()) thread_.join();
}

std::string Recorder::encoderName() const {
  std::lock_guard<std::mutex> lk(mu_);
  return encName_;
}

static bool openVideoEncoder(Recorder::Options& o, Recorder* self, void* implv, std::string& name, std::string& err) {
  (void)self;
  auto* I = static_cast<Recorder::Impl*>(implv);
  struct Cand {
    const char* name;
    AVPixelFormat pix;
    bool vaapi;
    bool soft = false;  // h264_mf: Windows' own built-in (software) H.264 encoder
  };
  std::vector<Cand> cands = {
      {"h264_nvenc", AV_PIX_FMT_NV12, false},
#ifdef _WIN32
      // Windows: no bundled software encoder (no x264). NVIDIA / AMD / Intel
      // hardware first, then the H.264 encoder that ships with Windows.
      {"h264_amf", AV_PIX_FMT_NV12, false},
      {"h264_qsv", AV_PIX_FMT_NV12, false},
      {"h264_mf", AV_PIX_FMT_NV12, false},
      {"h264_mf", AV_PIX_FMT_NV12, false, true},
#else
      // Linux: hardware only (no bundled software encoder).
      {"h264_vaapi", AV_PIX_FMT_NV12, true},
      {"h264_amf", AV_PIX_FMT_NV12, false},
#endif
  };
  if (const char* forced = getenv("UVC_STUDIO_ENCODER")) {
    std::vector<Cand> f;
    for (auto& c : cands)
      if (!strcmp(c.name, forced)) f.push_back(c);
    if (!f.empty()) cands = f;
  }
  // Shown when no encoder opens (the per-encoder details go to the log).
  const std::string noEncoder = "No video encoder: recording needs NVIDIA, AMD or Intel graphics";
  std::string lastErr = noEncoder;
  for (auto& c : cands) {
    const AVCodec* codec = avcodec_find_encoder_by_name(c.name);
    if (!codec) continue;
    AVCodecContext* ctx = avcodec_alloc_context3(codec);
    ctx->width = o.width;
    ctx->height = o.height;
    ctx->time_base = AVRational{1, o.fps};
    ctx->framerate = AVRational{o.fps, 1};
    ctx->gop_size = o.fps * 2;
    ctx->max_b_frames = 0;
    ctx->bit_rate = o.videoBitrate;
    ctx->rc_max_rate = o.videoBitrate * 5 / 4;
    ctx->rc_buffer_size = (int)(o.videoBitrate * 2);
    ctx->color_range = AVCOL_RANGE_MPEG;
    ctx->colorspace = AVCOL_SPC_BT709;
    ctx->color_primaries = AVCOL_PRI_BT709;
    ctx->color_trc = AVCOL_TRC_BT709;
    if (I->fmt->oformat->flags & AVFMT_GLOBALHEADER) ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    AVDictionary* opts = nullptr;
    AVBufferRef* hwdev = nullptr;
    AVBufferRef* hwframes = nullptr;
    if (c.vaapi) {
      if (av_hwdevice_ctx_create(&hwdev, AV_HWDEVICE_TYPE_VAAPI, nullptr, nullptr, 0) < 0) {
        avcodec_free_context(&ctx);
        continue;
      }
      hwframes = av_hwframe_ctx_alloc(hwdev);
      auto* fc = (AVHWFramesContext*)hwframes->data;
      fc->format = AV_PIX_FMT_VAAPI;
      fc->sw_format = AV_PIX_FMT_NV12;
      fc->width = o.width;
      fc->height = o.height;
      fc->initial_pool_size = 16;
      if (av_hwframe_ctx_init(hwframes) < 0) {
        av_buffer_unref(&hwframes);
        av_buffer_unref(&hwdev);
        avcodec_free_context(&ctx);
        continue;
      }
      ctx->pix_fmt = AV_PIX_FMT_VAAPI;
      ctx->hw_frames_ctx = av_buffer_ref(hwframes);
      av_dict_set(&opts, "rc_mode", "VBR", 0);
    } else {
      ctx->pix_fmt = c.pix;
    }
    std::string n = c.name;
    if (n == "h264_nvenc") {
      av_dict_set(&opts, "preset", "p4", 0);
      av_dict_set(&opts, "rc", "vbr", 0);
      av_dict_set(&opts, "profile", "high", 0);
    } else if (n == "h264_amf") {
      av_dict_set(&opts, "usage", "transcoding", 0);
      av_dict_set(&opts, "quality", "balanced", 0);
      av_dict_set(&opts, "rc", "vbr_peak", 0);
    } else if (n == "h264_qsv") {
      av_dict_set(&opts, "preset", "medium", 0);
    } else if (n == "h264_mf") {
      av_dict_set(&opts, "rate_control", "u_vbr", 0);
      if (!c.soft) av_dict_set(&opts, "hw_encoding", "1", 0);
    }
    int r = avcodec_open2(ctx, codec, &opts);
    av_dict_free(&opts);
    if (r < 0) {
      lastErr = std::string(c.name) + ": " + averr(r);
      avcodec_free_context(&ctx);
      av_buffer_unref(&hwframes);
      av_buffer_unref(&hwdev);
      continue;
    }
    I->venc = ctx;
    I->hwdev = hwdev;
    I->hwframes = hwframes;
    I->hw = c.vaapi;
    I->swFmt = c.vaapi ? AV_PIX_FMT_NV12 : c.pix;
    name = c.soft ? "h264_mf (Windows built-in)" : c.name;
    return true;
  }
  if (lastErr != noEncoder) std::fprintf(stderr, "recorder: %s\n", lastErr.c_str());
  err = noEncoder;
  return false;
}

bool Recorder::start(const Options& o, AudioEngine* audio, std::string& err) {
  if (thread_.joinable()) {
    stopReq_ = true;
    cv_.notify_all();
    thread_.join();
  }
  opts_ = o;
  opts_.width &= ~1;
  opts_.height &= ~1;
  if (opts_.fps <= 0) opts_.fps = 60;
  audio_ = audio;
  opts_mic_ = o.mic;
  opts_game_ = o.gameAudio;
  delete impl_;
  impl_ = new Impl();
  Impl& I = *impl_;
  int r = avformat_alloc_output_context2(&I.fmt, nullptr, "mp4", opts_.path.c_str());
  if (r < 0 || !I.fmt) {
    err = "mp4 muxer: " + averr(r);
    return false;
  }
  std::string ename;
  if (!openVideoEncoder(opts_, this, &I, ename, err)) {
    avformat_free_context(I.fmt);
    I.fmt = nullptr;
    return false;
  }
  I.vst = avformat_new_stream(I.fmt, nullptr);
  avcodec_parameters_from_context(I.vst->codecpar, I.venc);
  I.vst->time_base = I.venc->time_base;
  I.vst->avg_frame_rate = AVRational{opts_.fps, 1};

  const AVCodec* ac = avcodec_find_encoder_by_name("aac");
  I.aenc = avcodec_alloc_context3(ac);
  I.aenc->sample_fmt = AV_SAMPLE_FMT_FLTP;
  I.aenc->sample_rate = AudioEngine::kRate;
  AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
  av_channel_layout_copy(&I.aenc->ch_layout, &stereo);
  I.aenc->bit_rate = 192000;
  I.aenc->time_base = AVRational{1, AudioEngine::kRate};
  if (I.fmt->oformat->flags & AVFMT_GLOBALHEADER) I.aenc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  if ((r = avcodec_open2(I.aenc, ac, nullptr)) < 0) {
    err = "aac: " + averr(r);
    return false;
  }
  I.ast = avformat_new_stream(I.fmt, nullptr);
  avcodec_parameters_from_context(I.ast->codecpar, I.aenc);
  I.ast->time_base = I.aenc->time_base;

  if ((r = avio_open(&I.fmt->pb, opts_.path.c_str(), AVIO_FLAG_WRITE)) < 0) {
    err = "cannot write " + opts_.path + ": " + averr(r);
    return false;
  }
  AVDictionary* mopts = nullptr;
  av_dict_set(&mopts, "movflags", "+faststart", 0);
  if ((r = avformat_write_header(I.fmt, &mopts)) < 0) {
    av_dict_free(&mopts);
    err = "mp4 header: " + averr(r);
    return false;
  }
  av_dict_free(&mopts);

  I.vframe = av_frame_alloc();
  I.vframe->format = I.swFmt;
  I.vframe->width = opts_.width;
  I.vframe->height = opts_.height;
  av_frame_get_buffer(I.vframe, 32);
  I.aframe = av_frame_alloc();
  I.aframe->format = AV_SAMPLE_FMT_FLTP;
  I.aframe->nb_samples = I.aenc->frame_size ? I.aenc->frame_size : 1024;
  av_channel_layout_copy(&I.aframe->ch_layout, &stereo);
  I.aframe->sample_rate = AudioEngine::kRate;
  av_frame_get_buffer(I.aframe, 0);
  I.pkt = av_packet_alloc();

  {
    std::lock_guard<std::mutex> lk(mu_);
    encName_ = ename;
    queue_.clear();
    finished_ = false;
  }
  if (audio_) audio_->setTap(true);
  startT_ = nowSeconds();
  stopReq_ = false;
  running_ = true;
  thread_ = std::thread([this] { run(); });
  return true;
}

void Recorder::pushVideo(std::vector<uint8_t>&& rgba, int w, int h, double t) {
  if (!running_) return;
  std::lock_guard<std::mutex> lk(mu_);
  if (queue_.size() >= 4) queue_.pop_front();  // encoder can't keep up: drop oldest
  queue_.push_back(VFrame{std::move(rgba), w, h, t});
  cv_.notify_one();
}

void Recorder::stop() {
  if (!running_) return;
  stopReq_ = true;
  cv_.notify_all();
}

bool Recorder::pollFinished(std::string& path, bool& ok, std::string& err) {
  {
    std::lock_guard<std::mutex> lk(mu_);
    if (!finished_) return false;
    finished_ = false;
    ok = finOk_;
    err = finErr_;
    path = opts_.path;
  }
  if (thread_.joinable()) thread_.join();
  return true;
}

static void drainPackets(AVCodecContext* enc, AVStream* st, AVFormatContext* fmt, AVPacket* pkt) {
  while (avcodec_receive_packet(enc, pkt) == 0) {
    av_packet_rescale_ts(pkt, enc->time_base, st->time_base);
    pkt->stream_index = st->index;
    av_interleaved_write_frame(fmt, pkt);
  }
}

void Recorder::run() {
  Impl& I = *impl_;
  std::string failure;
  const int rate = AudioEngine::kRate;
  const int fsz = I.aframe->nb_samples;
  std::vector<float> game, mic;
  const double kAudioDelay = 0.12;  // seconds of slack so device buffers can fill

  auto encodeAudioUpTo = [&](double elapsed, bool flushAll) {
    int64_t target = (int64_t)std::floor((elapsed - (flushAll ? 0.0 : kAudioDelay)) * rate);
    int64_t pendingFrames = I.aSamples + (int64_t)I.aAccum.size() / 2;
    int64_t need = target - pendingFrames;
    if (need > 0) {
      size_t n = (size_t)need * 2;
      game.assign(n, 0.f);
      mic.assign(n, 0.f);
      if (audio_) {
        auto& gr = audio_->gameRing();
        auto& mr = audio_->micRing();
        // Clock drift guard: never let a ring run more than 250 ms ahead.
        size_t maxLead = n + (size_t)(0.25 * rate) * 2;
        if (gr.available() > maxLead) gr.discard(gr.available() - maxLead);
        if (mr.available() > maxLead) mr.discard(mr.available() - maxLead);
        size_t g = gr.read(game.data(), n);
        size_t m = mr.read(mic.data(), n);
        if (!opts_game_) std::fill(game.begin(), game.begin() + g, 0.f);
        if (!opts_mic_) std::fill(mic.begin(), mic.begin() + m, 0.f);
      }
      size_t base = I.aAccum.size();
      I.aAccum.resize(base + n);
      for (size_t i = 0; i < n; ++i) {
        float s = game[i] + mic[i];
        I.aAccum[base + i] = s > 1.f ? 1.f : (s < -1.f ? -1.f : s);
      }
    }
    while ((int)I.aAccum.size() / 2 >= fsz) {
      av_frame_make_writable(I.aframe);
      float* L = (float*)I.aframe->data[0];
      float* R = (float*)I.aframe->data[1];
      for (int i = 0; i < fsz; ++i) {
        L[i] = I.aAccum[2 * i];
        R[i] = I.aAccum[2 * i + 1];
      }
      I.aAccum.erase(I.aAccum.begin(), I.aAccum.begin() + (size_t)fsz * 2);
      I.aframe->pts = I.aSamples;
      I.aSamples += fsz;
      if (avcodec_send_frame(I.aenc, I.aframe) >= 0) drainPackets(I.aenc, I.ast, I.fmt, I.pkt);
    }
  };

  auto encodeVideo = [&](VFrame& vf) {
    double elapsed = vf.t - startT_;
    if (elapsed < 0) elapsed = 0;
    int64_t pts = (int64_t)std::llround(elapsed * opts_.fps);
    if (pts <= I.lastVpts) return;
    if (!I.sws || I.swsW != vf.w || I.swsH != vf.h) {
      if (I.sws) sws_freeContext(I.sws);
      I.sws = sws_getContext(vf.w, vf.h, AV_PIX_FMT_RGBA, opts_.width, opts_.height, I.swFmt, SWS_BILINEAR, nullptr,
                             nullptr, nullptr);
      const int* coefDst = sws_getCoefficients(SWS_CS_ITU709);
      const int* coefSrc = sws_getCoefficients(SWS_CS_DEFAULT);
      sws_setColorspaceDetails(I.sws, coefSrc, 1, coefDst, 0, 0, 1 << 16, 1 << 16);
      I.swsW = vf.w;
      I.swsH = vf.h;
    }
    av_frame_make_writable(I.vframe);
    const uint8_t* src[1] = {vf.data.data()};
    int srcStride[1] = {vf.w * 4};
    sws_scale(I.sws, src, srcStride, 0, vf.h, I.vframe->data, I.vframe->linesize);
    AVFrame* toSend = I.vframe;
    AVFrame* hwf = nullptr;
    if (I.hw) {
      hwf = av_frame_alloc();
      if (av_hwframe_get_buffer(I.hwframes, hwf, 0) < 0 || av_hwframe_transfer_data(hwf, I.vframe, 0) < 0) {
        av_frame_free(&hwf);
        return;
      }
      toSend = hwf;
    }
    toSend->pts = pts;
    I.lastVpts = pts;
    if (avcodec_send_frame(I.venc, toSend) >= 0) drainPackets(I.venc, I.vst, I.fmt, I.pkt);
    if (hwf) av_frame_free(&hwf);
  };

  while (true) {
    VFrame vf;
    bool have = false;
    {
      std::unique_lock<std::mutex> lk(mu_);
      cv_.wait_for(lk, std::chrono::milliseconds(10), [&] { return !queue_.empty() || stopReq_.load(); });
      if (!queue_.empty()) {
        vf = std::move(queue_.front());
        queue_.pop_front();
        have = true;
      }
    }
    if (have) encodeVideo(vf);
    encodeAudioUpTo(nowSeconds() - startT_, false);
    if (stopReq_) {
      std::unique_lock<std::mutex> lk(mu_);
      if (queue_.empty()) break;
    }
  }
  if (audio_) audio_->setTap(false);
  // Flush: audio up to the end, then both encoders.
  double end = nowSeconds() - startT_;
  encodeAudioUpTo(end, true);
  if (!I.aAccum.empty()) {
    I.aAccum.resize((size_t)fsz * 2, 0.f);
    encodeAudioUpTo(end, true);
  }
  avcodec_send_frame(I.venc, nullptr);
  drainPackets(I.venc, I.vst, I.fmt, I.pkt);
  avcodec_send_frame(I.aenc, nullptr);
  drainPackets(I.aenc, I.ast, I.fmt, I.pkt);
  int r = av_write_trailer(I.fmt);
  if (r < 0) failure = "finalising mp4: " + averr(r);
  avio_closep(&I.fmt->pb);

  if (I.sws) sws_freeContext(I.sws);
  av_frame_free(&I.vframe);
  av_frame_free(&I.aframe);
  av_packet_free(&I.pkt);
  avcodec_free_context(&I.venc);
  avcodec_free_context(&I.aenc);
  av_buffer_unref(&I.hwframes);
  av_buffer_unref(&I.hwdev);
  avformat_free_context(I.fmt);
  I.fmt = nullptr;
  running_ = false;
  std::lock_guard<std::mutex> lk(mu_);
  finished_ = true;
  finOk_ = failure.empty();
  finErr_ = failure;
}
