#include "FrameDecoder.h"

#include <algorithm>
#include <chrono>
#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}


double nowSeconds() {
  using namespace std::chrono;
  return duration<double>(steady_clock::now().time_since_epoch()).count();
}

Frame::~Frame() { releaseAv(); }

void Frame::releaseAv() {
  if (av) {
    av_frame_free(&av);
    av = nullptr;
  }
}

struct FrameDecoder::Impl {
  AVCodecContext* ctx = nullptr;
  AVPacket* pkt = nullptr;
  AVFrame* frame = nullptr;
  SwsContext* sws = nullptr;
  int swsW = 0, swsH = 0, swsFmt = -1;
};

FrameDecoder::~FrameDecoder() { close(); }

bool FrameDecoder::open(Compressed codec, int width, int height, std::string& err) {
  close();
  AVCodecID id = AV_CODEC_ID_MJPEG;
  if (codec == Compressed::H264) id = AV_CODEC_ID_H264;
  if (codec == Compressed::HEVC) id = AV_CODEC_ID_HEVC;
  const AVCodec* dec = avcodec_find_decoder(id);
  if (!dec) {
    err = "decoder not available";
    return false;
  }
  impl_ = new Impl();
  impl_->ctx = avcodec_alloc_context3(dec);
  impl_->ctx->width = width;
  impl_->ctx->height = height;
  // Latency first: no frame threading (it would add a frame of delay per
  // thread). MJPEG at 4K still decodes comfortably in real time on a single
  // modern core.
  impl_->ctx->thread_count = 1;
  impl_->ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;
  impl_->ctx->flags2 |= AV_CODEC_FLAG2_FAST;
  if (avcodec_open2(impl_->ctx, dec, nullptr) < 0) {
    err = "could not open decoder";
    close();
    return false;
  }
  impl_->pkt = av_packet_alloc();
  impl_->frame = av_frame_alloc();
  return true;
}

void FrameDecoder::close() {
  if (!impl_) return;
  if (impl_->sws) sws_freeContext(impl_->sws);
  if (impl_->frame) av_frame_free(&impl_->frame);
  if (impl_->pkt) av_packet_free(&impl_->pkt);
  if (impl_->ctx) avcodec_free_context(&impl_->ctx);
  delete impl_;
  impl_ = nullptr;
}

static bool mapAvFormat(int f, PixFmt& out, bool& full) {
  full = false;
  switch (f) {
    case AV_PIX_FMT_YUVJ420P: full = true; out = PixFmt::I420; return true;
    case AV_PIX_FMT_YUV420P: out = PixFmt::I420; return true;
    case AV_PIX_FMT_YUVJ422P: full = true; out = PixFmt::YUV422P; return true;
    case AV_PIX_FMT_YUV422P: out = PixFmt::YUV422P; return true;
    case AV_PIX_FMT_YUVJ444P: full = true; out = PixFmt::YUV444P; return true;
    case AV_PIX_FMT_YUV444P: out = PixFmt::YUV444P; return true;
    case AV_PIX_FMT_NV12: out = PixFmt::NV12; return true;
    case AV_PIX_FMT_GRAY8: full = true; out = PixFmt::GRAY8; return true;
    default: return false;
  }
}

bool FrameDecoder::decode(const uint8_t* data, size_t size, Frame& out) {
  if (!impl_ || !data || size == 0) return false;
  AVPacket* pkt = impl_->pkt;
  pkt->data = const_cast<uint8_t*>(data);
  pkt->size = (int)size;
  if (avcodec_send_packet(impl_->ctx, pkt) < 0) {
    return false;
  }
  bool got = false;
  while (true) {
    int r = avcodec_receive_frame(impl_->ctx, impl_->frame);
    if (r < 0) break;
    got = true;
    out.releaseAv();
    PixFmt pf;
    bool full;
    AVFrame* f = impl_->frame;
    if (mapAvFormat(f->format, pf, full)) {
      out.av = av_frame_clone(f);
      out.fmt = pf;
      out.fullRange = full || f->color_range == AVCOL_RANGE_JPEG;
      for (int i = 0; i < 3; ++i) {
        out.plane[i] = out.av->data[i];
        out.stride[i] = out.av->linesize[i];
      }
    } else {
      // Anything exotic: convert to I420 with swscale.
      if (!impl_->sws || impl_->swsW != f->width || impl_->swsH != f->height || impl_->swsFmt != f->format) {
        if (impl_->sws) sws_freeContext(impl_->sws);
        impl_->sws = sws_getContext(f->width, f->height, (AVPixelFormat)f->format, f->width, f->height,
                                    AV_PIX_FMT_YUV420P, SWS_POINT, nullptr, nullptr, nullptr);
        impl_->swsW = f->width;
        impl_->swsH = f->height;
        impl_->swsFmt = f->format;
      }
      AVFrame* c = av_frame_alloc();
      c->format = AV_PIX_FMT_YUV420P;
      c->width = f->width;
      c->height = f->height;
      av_frame_get_buffer(c, 32);
      sws_scale(impl_->sws, f->data, f->linesize, 0, f->height, c->data, c->linesize);
      out.av = c;
      out.fmt = PixFmt::I420;
      out.fullRange = false;
      for (int i = 0; i < 3; ++i) {
        out.plane[i] = c->data[i];
        out.stride[i] = c->linesize[i];
      }
    }
    out.width = f->width;
    out.height = f->height;
    // MJPEG is BT.601 by definition; for H.264/HEVC trust the bitstream.
    if (impl_->ctx->codec_id == AV_CODEC_ID_MJPEG)
      out.bt709 = false;
    else
      out.bt709 = f->colorspace == AVCOL_SPC_BT709 || (f->colorspace == AVCOL_SPC_UNSPECIFIED && f->height >= 720);
    av_frame_unref(impl_->frame);
  }
  return got;
}

bool fillRawFrame(PixFmt fmt, int w, int h, int bpl, const uint8_t* data, size_t size, Frame& out) {
  out.releaseAv();
  out.fmt = fmt;
  out.width = w;
  out.height = h;
  size_t need = 0;
  int bpp = 0;
  switch (fmt) {
    case PixFmt::YUYV:
    case PixFmt::UYVY: bpp = 2; break;
    case PixFmt::RGB24:
    case PixFmt::BGR24: bpp = 3; break;
    case PixFmt::RGBA:
    case PixFmt::BGRA: bpp = 4; break;
    case PixFmt::GRAY8:
    case PixFmt::NV12:
    case PixFmt::NV21:
    case PixFmt::I420:
    case PixFmt::YUV422P:
    case PixFmt::YUV444P: bpp = 1; break;
    default: return false;
  }
  if (bpl <= 0) bpl = w * bpp;
  size_t ysz = (size_t)bpl * h;
  switch (fmt) {
    case PixFmt::NV12:
    case PixFmt::NV21: need = ysz + (size_t)bpl * ((h + 1) / 2); break;
    case PixFmt::I420: need = ysz + 2 * (size_t)(bpl / 2) * ((h + 1) / 2); break;
    case PixFmt::YUV422P: need = ysz + 2 * (size_t)(bpl / 2) * h; break;
    case PixFmt::YUV444P: need = ysz * 3; break;
    default: need = ysz; break;
  }
  if (size < need) return false;
  out.storage.resize(need);
  std::memcpy(out.storage.data(), data, need);
  const uint8_t* p = out.storage.data();
  out.plane[0] = p;
  out.stride[0] = bpl;
  out.plane[1] = out.plane[2] = nullptr;
  out.stride[1] = out.stride[2] = 0;
  switch (fmt) {
    case PixFmt::NV12:
    case PixFmt::NV21:
      out.plane[1] = p + ysz;
      out.stride[1] = bpl;
      break;
    case PixFmt::I420:
      out.plane[1] = p + ysz;
      out.stride[1] = bpl / 2;
      out.plane[2] = p + ysz + (size_t)(bpl / 2) * ((h + 1) / 2);
      out.stride[2] = bpl / 2;
      break;
    case PixFmt::YUV422P:
      out.plane[1] = p + ysz;
      out.stride[1] = bpl / 2;
      out.plane[2] = p + ysz + (size_t)(bpl / 2) * h;
      out.stride[2] = bpl / 2;
      break;
    case PixFmt::YUV444P:
      out.plane[1] = p + ysz;
      out.stride[1] = bpl;
      out.plane[2] = p + 2 * ysz;
      out.stride[2] = bpl;
      break;
    default: break;
  }
  return true;
}

// ---------------------------------------------------------------------------
DecodePool::~DecodePool() { close(); }

bool DecodePool::open(Compressed codec, int w, int h, Acquire acquire, Publish publish, std::string& err) {
  close();
  stop_ = false;
  acquire_ = std::move(acquire);
  publish_ = std::move(publish);
  ordered_ = codec != Compressed::MJPEG;
  int n = 1;
  if (!ordered_) {
    unsigned hc = std::thread::hardware_concurrency();
    long rate = (long)w * h;
    n = rate > 1920L * 1080 ? 4 : 2;
    if (hc && (unsigned)n > std::max(1u, hc / 2)) n = (int)std::max(1u, hc / 2);
  }
  for (int i = 0; i < n; ++i) {
    auto wk = std::make_unique<Worker>();
    if (!wk->dec.open(codec, w, h, err)) {
      close();
      return false;
    }
    workers_.push_back(std::move(wk));
  }
  for (auto& wk : workers_) {
    Worker* p = wk.get();
    p->th = std::thread([this, p] { run(p); });
  }
  return true;
}

void DecodePool::submit(const uint8_t* data, size_t size) {
  if (workers_.empty()) return;
  uint64_t seq = ++seq_;
  Worker* target = nullptr;
  if (ordered_) {
    target = workers_[0].get();
  } else {
    // Prefer an idle worker; otherwise replace the stale payload waiting on
    // the next worker in round-robin order.
    for (size_t k = 0; k < workers_.size(); ++k) {
      Worker* w = workers_[(rr_ + k) % workers_.size()].get();
      std::lock_guard<std::mutex> lk(w->mu);
      if (!w->busy && w->q.empty()) {
        target = w;
        rr_ = (rr_ + k + 1) % workers_.size();
        break;
      }
    }
    if (!target) {
      target = workers_[rr_].get();
      rr_ = (rr_ + 1) % workers_.size();
    }
  }
  {
    std::lock_guard<std::mutex> lk(target->mu);
    if (!ordered_) target->q.clear();
    if (ordered_ && target->q.size() > 30) target->q.pop_front();
    target->q.emplace_back(seq, std::vector<uint8_t>(data, data + size));
  }
  target->cv.notify_one();
}

void DecodePool::run(Worker* w) {
  while (true) {
    std::pair<uint64_t, std::vector<uint8_t>> job;
    {
      std::unique_lock<std::mutex> lk(w->mu);
      w->cv.wait(lk, [&] { return stop_.load() || !w->q.empty(); });
      if (stop_) return;
      job = std::move(w->q.front());
      w->q.pop_front();
      w->busy = true;
    }
    FramePtr f = acquire_();
    bool ok = w->dec.decode(job.second.data(), job.second.size(), *f);
    if (ok) {
      std::lock_guard<std::mutex> lk(pubMu_);
      if (job.first > lastPublished_) {
        lastPublished_ = job.first;
        publish_(f);
      }
    }
    std::lock_guard<std::mutex> lk(w->mu);
    w->busy = false;
  }
}

void DecodePool::close() {
  stop_ = true;
  for (auto& w : workers_) {
    w->cv.notify_all();
    if (w->th.joinable()) w->th.join();
  }
  workers_.clear();
  lastPublished_ = 0;
  seq_ = 0;
  rr_ = 0;
}
