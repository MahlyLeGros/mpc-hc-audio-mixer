// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include <deque>
#include <cwctype>

namespace AudioMix {
constexpr int SampleRate = 48000;
constexpr int Channels = 2;
constexpr int BlockFrames = 480;

inline bool SupportsContainer(const std::wstring& path) {
    const auto dot = path.find_last_of(L'.');
    if (dot == std::wstring::npos) return false;
    std::wstring ext = path.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](wchar_t c) { return wchar_t(std::towlower(c)); });
    return ext == L".mkv" || ext == L".mp4" || ext == L".mov" || ext == L".avi" || ext == L".webm";
}

struct Track {
    int streamIndex = -1;
    std::wstring name;
    std::wstring language;
    std::wstring error;
    float volume = 0.0f;
    bool mute = false;
    bool solo = false;
    float peak = 0.0f;
};

// Decoding runs ahead of playback. Keep source levels on the media timeline.
class MeterTimeline {
    struct Block { int64_t begin, end; std::vector<float> peaks; };
    std::deque<Block> m_blocks;
public:
    void Clear() { m_blocks.clear(); }
    void Push(int64_t begin, int frames, const std::vector<Track>& tracks) {
        Block block{begin, begin + frames, {}};
        for (const auto& track : tracks) block.peaks.push_back(track.peak);
        m_blocks.push_back(std::move(block));
        while (m_blocks.size() > 1000) m_blocks.pop_front();
    }
    std::vector<float> Levels(int64_t position, size_t count) const {
        std::vector<float> levels(count, 0.0f);
        if (position < 0) return levels;
        // Cover one UI refresh interval, so short transients are not skipped.
        const int64_t window = SampleRate / 20;
        for (const auto& block : m_blocks) {
            if (block.begin > position) break;
            if (block.end <= position-window) continue;
            for (size_t i=0; i<std::min(count,block.peaks.size()); ++i)
                levels[i] = std::max(levels[i],block.peaks[i]);
        }
        return levels;
    }
};

// Used only by the delivery thread. Controls are copied at block boundaries.
class Mixer {
    std::vector<float> m_gains;
    float m_master = 1.0f;
    double m_limiter = 1.0;
public:
    void Reset(const std::vector<Track>& tracks, float master) {
        m_gains.resize(tracks.size());
        const bool solo = std::any_of(tracks.begin(), tracks.end(), [](const Track& t) { return t.solo; });
        for (size_t i = 0; i < tracks.size(); ++i) {
            m_gains[i] = tracks[i].mute || !tracks[i].error.empty() || (solo && !tracks[i].solo)
                ? 0.0f : std::clamp(tracks[i].volume, 0.0f, 1.0f);
        }
        m_master = std::clamp(master, 0.0f, 1.0f);
        m_limiter = 1.0f;
    }

    void Process(const std::vector<std::vector<float>>& input, std::vector<Track>& tracks,
                 float master, float* output, int frames) {
        if (m_gains.size() != tracks.size()) Reset(tracks, master);
        const bool solo = std::any_of(tracks.begin(), tracks.end(), [](const Track& t) { return t.solo; });
        std::vector<float> targets(tracks.size());
        for (size_t i = 0; i < tracks.size(); ++i) {
            targets[i] = tracks[i].mute || !tracks[i].error.empty() || (solo && !tracks[i].solo)
                ? 0.0f : std::clamp(tracks[i].volume, 0.0f, 1.0f);
            tracks[i].peak = 0.0f;
        }
        // 5 ms gain ramp, instant limiter attack, 50 ms release.
        constexpr float step = 1.0f / (SampleRate * 0.005f);
        constexpr float release = 1.0f - 0.99958342f;
        const auto approach = [=](float a, float b) { return a + std::clamp(b - a, -step, step); };
        for (int f = 0; f < frames; ++f) {
            double sum[2] = {};
            for (size_t i = 0; i < tracks.size(); ++i) {
                m_gains[i] = approach(m_gains[i], targets[i]);
                for (int c = 0; c < Channels; ++c) {
                    const size_t s = size_t(f) * Channels + c;
                    float v = i < input.size() && s < input[i].size() ? input[i][s] : 0.0f;
                    if (!std::isfinite(v)) v = 0.0f;
                    // Meter the decoded signal before volume, mute and solo gains.
                    tracks[i].peak = std::max(tracks[i].peak, std::abs(v));
                    v *= m_gains[i];
                    sum[c] += v;
                }
            }
            m_master = approach(m_master, std::clamp(master, 0.0f, 1.0f));
            const double peak = std::max(std::abs(sum[0]), std::abs(sum[1])) * m_master;
            const double limit = peak > 0.98 ? 0.98 / peak : 1.0;
            m_limiter = limit < m_limiter ? limit : std::min(limit, m_limiter + release * (1.0 - m_limiter));
            for (int c = 0; c < Channels; ++c) output[f * Channels + c] = float(sum[c] * m_master * m_limiter);
        }
    }
};

// Copies a timestamped decoded block onto the requested output interval.
inline void CopyOverlap(const std::vector<float>& decoded, int64_t decodedStart,
                        std::vector<float>& output, int64_t outputStart) {
    const int64_t begin = std::max(decodedStart, outputStart);
    const int64_t end = std::min(decodedStart + int64_t(decoded.size() / Channels),
                                 outputStart + int64_t(output.size() / Channels));
    if (begin < end) {
        std::copy_n(decoded.data() + (begin - decodedStart) * Channels, (end - begin) * Channels,
                    output.data() + (begin - outputStart) * Channels);
    }
}
}
