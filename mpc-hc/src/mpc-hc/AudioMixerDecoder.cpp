// SPDX-License-Identifier: GPL-3.0-or-later
// Kept separate from the player's older, statically linked FFmpeg headers.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dshow.h>
#include <limits>
#include <array>
#include <cwctype>
#include <cerrno>
#include "AudioMixer.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

namespace AudioMix {
namespace {
std::string Utf8(const std::wstring& s) {
    int n = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string out(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, out.data(), n, nullptr, nullptr);
    out.resize(n ? n - 1 : 0);
    return out;
}
std::wstring Wide(const char* s) {
    if (!s) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    std::wstring out(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s, -1, out.data(), n);
    out.resize(n ? n - 1 : 0);
    return out;
}

// Function pointers avoid collisions with MPC-HC's static FFmpeg library.
struct Api {
    std::vector<HMODULE> modules;
    std::wstring error;
#define MIX_API(name) decltype(&name) name = nullptr
    MIX_API(avformat_version); MIX_API(avcodec_version); MIX_API(avutil_version); MIX_API(swresample_version);
    MIX_API(avformat_open_input); MIX_API(avformat_find_stream_info); MIX_API(avformat_close_input);
    MIX_API(avformat_seek_file); MIX_API(av_read_frame);
    MIX_API(avcodec_find_decoder); MIX_API(avcodec_alloc_context3); MIX_API(avcodec_parameters_to_context);
    MIX_API(avcodec_open2); MIX_API(avcodec_free_context); MIX_API(avcodec_flush_buffers);
    MIX_API(avcodec_send_packet); MIX_API(avcodec_receive_frame);
    MIX_API(av_packet_alloc); MIX_API(av_packet_free); MIX_API(av_packet_unref);
    MIX_API(av_frame_alloc); MIX_API(av_frame_free); MIX_API(av_frame_unref);
    MIX_API(av_dict_get); MIX_API(av_rescale_q); MIX_API(av_rescale_rnd);
    MIX_API(av_channel_layout_default); MIX_API(av_channel_layout_uninit);
    MIX_API(swr_alloc_set_opts2); MIX_API(swr_init); MIX_API(swr_free); MIX_API(swr_convert); MIX_API(swr_get_delay);
#undef MIX_API
    Api() {
        wchar_t exe[32768] = {};
        if (!GetModuleFileNameW(nullptr, exe, _countof(exe))) { error = L"Executable path unavailable"; return; }
        std::wstring dir(exe);
        dir.resize(dir.find_last_of(L"\\/") + 1);
#ifdef _WIN64
        dir += L"LAVFilters64\\";
#else
        dir += L"LAVFilters\\";
#endif
        const std::wstring names[] = {
            L"avutil-lav-" + std::to_wstring(LIBAVUTIL_VERSION_MAJOR),
            L"swresample-lav-" + std::to_wstring(LIBSWRESAMPLE_VERSION_MAJOR),
            L"avcodec-lav-" + std::to_wstring(LIBAVCODEC_VERSION_MAJOR),
            L"avformat-lav-" + std::to_wstring(LIBAVFORMAT_VERSION_MAJOR)
        };
        for (const auto& name : names) {
            HMODULE module = LoadLibraryExW((dir + name + L".dll").c_str(), nullptr,
                                          LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
            if (!module) { error = L"Missing mixer dependency: " + name + L".dll"; return; }
            modules.push_back(module);
        }
        const auto get = [this](const char* name) -> FARPROC {
            for (auto module : modules) if (auto p = GetProcAddress(module, name)) return p;
            return nullptr;
        };
#define LOAD(name) name = reinterpret_cast<decltype(name)>(get(#name)); if (!name) { error = L"Incompatible LAV audio libraries"; return; }
        LOAD(avformat_version); LOAD(avcodec_version); LOAD(avutil_version); LOAD(swresample_version);
        LOAD(avformat_open_input); LOAD(avformat_find_stream_info); LOAD(avformat_close_input);
        LOAD(avformat_seek_file); LOAD(av_read_frame);
        LOAD(avcodec_find_decoder); LOAD(avcodec_alloc_context3); LOAD(avcodec_parameters_to_context);
        LOAD(avcodec_open2); LOAD(avcodec_free_context); LOAD(avcodec_flush_buffers);
        LOAD(avcodec_send_packet); LOAD(avcodec_receive_frame);
        LOAD(av_packet_alloc); LOAD(av_packet_free); LOAD(av_packet_unref);
        LOAD(av_frame_alloc); LOAD(av_frame_free); LOAD(av_frame_unref);
        LOAD(av_dict_get); LOAD(av_rescale_q); LOAD(av_rescale_rnd);
        LOAD(av_channel_layout_default); LOAD(av_channel_layout_uninit);
        LOAD(swr_alloc_set_opts2); LOAD(swr_init); LOAD(swr_free); LOAD(swr_convert); LOAD(swr_get_delay);
#undef LOAD
        if ((avformat_version() >> 16) != LIBAVFORMAT_VERSION_MAJOR ||
            (avcodec_version() >> 16) != LIBAVCODEC_VERSION_MAJOR ||
            (avutil_version() >> 16) != LIBAVUTIL_VERSION_MAJOR ||
            (swresample_version() >> 16) != LIBSWRESAMPLE_VERSION_MAJOR) error = L"LAV audio ABI mismatch";
    }
    // Deliberately retain modules until process exit; decoder objects may outlive graph teardown.
};
Api& Lib() { static Api api; return api; }

bool OpenFormat(const std::wstring& path, AVFormatContext** format, std::wstring& error) {
    auto& a = Lib();
    if (!a.error.empty()) { error = a.error; return false; }
    if (a.avformat_open_input(format, Utf8(path).c_str(), nullptr, nullptr) < 0 ||
        a.avformat_find_stream_info(*format, nullptr) < 0) {
        if (*format) a.avformat_close_input(format);
        error = L"Unable to read the local media file";
        return false;
    }
    return true;
}
}

std::shared_ptr<Session> Probe(const std::wstring& path, int selected, std::wstring& error) {
    const auto dot = path.find_last_of(L'.');
    std::wstring ext = dot == std::wstring::npos ? L"" : path.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), towlower);
    const DWORD attrs = GetFileAttributesW(path.c_str());
    if ((ext != L".mkv" && ext != L".mp4") || attrs == INVALID_FILE_ATTRIBUTES ||
        (attrs & FILE_ATTRIBUTE_DIRECTORY) || path.find(L"://") != std::wstring::npos || path.rfind(L"\\\\", 0) == 0) {
        error = L"The mixer supports local MKV and MP4 files"; return {};
    }
    AVFormatContext* format = nullptr;
    if (!OpenFormat(path, &format, error)) return {};
    auto& a = Lib();
    auto session = std::make_shared<Session>();
    session->path = path;
    session->duration = format->duration > 0 ? format->duration * 10 : 0;
    for (unsigned i = 0; i < format->nb_streams; ++i) {
        AVStream* stream = format->streams[i];
        if (stream->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) continue;
        Track track;
        track.streamIndex = int(i);
        auto name = a.av_dict_get(stream->metadata, "title", nullptr, 0);
        auto lang = a.av_dict_get(stream->metadata, "language", nullptr, 0);
        track.name = name ? Wide(name->value) : L"Track " + std::to_wstring(session->tracks.size() + 1);
        track.language = lang ? Wide(lang->value) : L"";
        if (!a.avcodec_find_decoder(stream->codecpar->codec_id)) track.error = L"Unsupported codec";
        session->tracks.push_back(std::move(track));
    }
    a.avformat_close_input(&format);
    if (session->tracks.empty() || session->duration <= 0) {
        error = L"No audio tracks or no usable duration"; return {};
    }
    if (selected < 0 || size_t(selected) >= session->tracks.size()) selected = 0;
    session->selected = selected;
    session->tracks[selected].volume = 1.0f;
    return session;
}

struct Decoder::Impl {
    std::wstring path;
    AVFormatContext* format = nullptr;
    AVCodecContext* codec = nullptr;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;
    SwrContext* swr = nullptr;
    int stream = -1;
    int sampleRate = 0;
    int sampleFormat = -1;
    int inputChannels = 0;
    bool draining = false;
    bool eof = false;
    int64_t origin = 0;
    int64_t pendingStart = 0;
    int64_t nextFrame = 0;
    bool haveTimeline = false;
    std::vector<float> pending;
    ~Impl() {
        auto& a = Lib();
        if (swr) a.swr_free(&swr);
        if (frame) a.av_frame_free(&frame);
        if (packet) a.av_packet_free(&packet);
        if (codec) a.avcodec_free_context(&codec);
        if (format) a.avformat_close_input(&format);
    }
    bool Next(std::wstring& error) {
        auto& a = Lib();
        pending.clear();
        while (!eof) {
            int r = a.avcodec_receive_frame(codec, frame);
            if (r == AVERROR_EOF) {
                eof = true;
                if (swr) {
                    const int count = int(a.swr_get_delay(swr, SampleRate));
                    if (count > 0) {
                        pending.resize(size_t(count) * Channels);
                        uint8_t* out = reinterpret_cast<uint8_t*>(pending.data());
                        r = a.swr_convert(swr, &out, count, nullptr, 0);
                        if (r < 0) { error = L"Audio resampling failed"; return false; }
                        pending.resize(size_t(r) * Channels);
                        pendingStart = nextFrame;
                        nextFrame += r;
                    }
                }
                return true;
            }
            if (r == AVERROR(EAGAIN)) {
                if (draining) { error = L"Audio decoder failed to drain"; return false; }
                do {
                    r = a.av_read_frame(format, packet);
                    if (r < 0) break;
                    if (packet->stream_index != stream) a.av_packet_unref(packet);
                    else break;
                } while (true);
                if (r < 0) {
                    if (r != AVERROR_EOF) { error = L"Unable to read audio packets"; return false; }
                    draining = true;
                    r = a.avcodec_send_packet(codec, nullptr);
                } else {
                    r = a.avcodec_send_packet(codec, packet);
                    a.av_packet_unref(packet);
                }
                if (r < 0 && r != AVERROR_EOF) { error = L"Audio decoding failed"; return false; }
                continue;
            }
            if (r < 0) { error = L"Audio decoding failed"; return false; }
            if (!swr || sampleRate != frame->sample_rate || sampleFormat != frame->format || inputChannels != frame->ch_layout.nb_channels) {
                a.swr_free(&swr);
                AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
                if (frame->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC) {
                    const int channels = frame->ch_layout.nb_channels;
                    a.av_channel_layout_uninit(&frame->ch_layout);
                    a.av_channel_layout_default(&frame->ch_layout, channels);
                }
                if (a.swr_alloc_set_opts2(&swr, &stereo, AV_SAMPLE_FMT_FLT, SampleRate,
                        &frame->ch_layout, AVSampleFormat(frame->format), frame->sample_rate, 0, nullptr) < 0 || a.swr_init(swr) < 0) {
                    error = L"Unable to convert this audio track to stereo"; return false;
                }
                sampleRate = frame->sample_rate;
                sampleFormat = frame->format;
                inputChannels = frame->ch_layout.nb_channels;
            }
            int64_t pts = frame->best_effort_timestamp;
            int64_t start = pts == AV_NOPTS_VALUE ? nextFrame :
                a.av_rescale_q(pts, format->streams[stream]->time_base, AVRational{1, SampleRate}) - origin;
            const int64_t delay = a.swr_get_delay(swr, SampleRate);
            pendingStart = start - delay;
            // Container timestamps (notably Matroska) can have millisecond
            // precision. Preserve sample continuity within one timestamp tick.
            const int64_t tick = std::max<int64_t>(1, a.av_rescale_q(1, format->streams[stream]->time_base, AVRational{1, SampleRate}));
            if (haveTimeline && std::abs(pendingStart - nextFrame) <= tick + 2) pendingStart = nextFrame;
            const int capacity = int(a.av_rescale_rnd(a.swr_get_delay(swr, sampleRate) + frame->nb_samples,
                                                    SampleRate, sampleRate, AV_ROUND_UP)) + 32;
            pending.resize(size_t(capacity) * Channels);
            uint8_t* out = reinterpret_cast<uint8_t*>(pending.data());
            r = a.swr_convert(swr, &out, capacity, const_cast<const uint8_t**>(frame->extended_data), frame->nb_samples);
            a.av_frame_unref(frame);
            if (r < 0) { error = L"Audio resampling failed"; return false; }
            pending.resize(size_t(r) * Channels);
            nextFrame = pendingStart + r;
            haveTimeline = true;
            if (r) return true;
        }
        return true;
    }
};

Decoder::Decoder() : m_impl(std::make_unique<Impl>()) {}
Decoder::~Decoder() = default;
bool Decoder::Open(const std::wstring& path, int stream, std::wstring& error) {
    m_impl = std::make_unique<Impl>();
    auto& p = *m_impl;
    auto& a = Lib();
    if (!OpenFormat(path, &p.format, error)) return false;
    if (stream < 0 || unsigned(stream) >= p.format->nb_streams) { error = L"Audio stream missing"; return false; }
    auto params = p.format->streams[stream]->codecpar;
    auto decoder = a.avcodec_find_decoder(params->codec_id);
    if (!decoder) { error = L"Unsupported codec"; return false; }
    p.codec = a.avcodec_alloc_context3(decoder);
    if (!p.codec || a.avcodec_parameters_to_context(p.codec, params) < 0) { error = L"Unable to initialize audio decoder"; return false; }
    p.codec->pkt_timebase = p.format->streams[stream]->time_base;
    if (a.avcodec_open2(p.codec, decoder, nullptr) < 0) { error = L"Unable to open audio decoder"; return false; }
    p.packet = a.av_packet_alloc();
    p.frame = a.av_frame_alloc();
    if (!p.packet || !p.frame) { error = L"Out of memory"; return false; }
    p.stream = stream;
    p.path = path;
    p.origin = p.format->start_time == AV_NOPTS_VALUE ? 0 : a.av_rescale_q(p.format->start_time, AVRational{1, AV_TIME_BASE}, AVRational{1, SampleRate});
    return true;
}

bool Decoder::Seek(int64_t frame, std::wstring& error) {
    auto& p = *m_impl;
    auto& a = Lib();
    if (!p.codec) { error = L"Audio decoder not open"; return false; }
    if (frame < 0) { error = L"Negative seek position"; return false; }
    if (frame <= SampleRate) {
        // Seeking the demuxer to timestamp zero can skip negative-time AAC
        // priming packets. Reopen from the beginning to retain that preroll.
        const std::wstring path = p.path;
        const int stream = p.stream;
        return Open(path, stream, error);
    }
    // Seek earlier to give codecs and the resampler preroll; discard by timestamp.
    const int64_t target = std::max<int64_t>(0, frame - SampleRate);
    const int64_t ts = a.av_rescale_q(target + p.origin, AVRational{1, SampleRate}, AVRational{1, AV_TIME_BASE});
    if (a.avformat_seek_file(p.format, -1, std::numeric_limits<int64_t>::min(), ts, ts, 0) < 0) {
        error = L"Unable to seek this audio track"; return false;
    }
    a.avcodec_flush_buffers(p.codec);
    a.av_packet_unref(p.packet);
    a.av_frame_unref(p.frame);
    a.swr_free(&p.swr);
    p.pending.clear();
    p.draining = p.eof = false;
    p.nextFrame = target;
    p.haveTimeline = false;
    return true;
}

bool Decoder::Read(int64_t startFrame, int frames, std::vector<float>& output, std::wstring& error) {
    if (!m_impl->codec || startFrame < 0 || frames < 0 || frames > SampleRate * 10) {
        error = L"Invalid audio read request"; return false;
    }
    output.assign(size_t(frames) * Channels, 0.0f);
    auto& p = *m_impl;
    while (true) {
        const int64_t end = p.pendingStart + int64_t(p.pending.size() / Channels);
        if (!p.pending.empty()) {
            CopyOverlap(p.pending, p.pendingStart, output, startFrame);
            if (end >= startFrame + frames || p.pendingStart >= startFrame + frames) return true;
            p.pending.clear();
        }
        if (p.eof) return true;
        if (!p.Next(error)) return false;
    }
}
}
