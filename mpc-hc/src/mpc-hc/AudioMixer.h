// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "AudioMixerCore.h"
#include <memory>
#include <mutex>
#include <chrono>

namespace AudioMix {
struct Session {
    std::mutex mutex;
    std::wstring path;
    std::vector<Track> tracks;
    int64_t duration = 0; // 100 ns units
    float master = 1.0f;
    bool enabled = false;
    bool active = true;
    bool initialized = false;
    int selected = 0;
    MeterTimeline meters;
    std::wstring error;
};

// Opens only local files. DLLs must come from this build's LAVFilters folder.
std::shared_ptr<Session> Probe(const std::wstring& path, int selected, std::wstring& error);

class Decoder {
    struct Impl;
    std::unique_ptr<Impl> m_impl;
public:
    Decoder();
    ~Decoder();
    bool Open(const std::wstring& path, int stream, std::wstring& error);
    bool Seek(int64_t frame, std::wstring& error);
    bool Read(int64_t startFrame, int frames, std::vector<float>& output, std::wstring& error);
};

// Creates a source owned by the DirectShow graph, with an IMediaSeeking output pin.
HRESULT AddSource(IGraphBuilder* graph, const std::shared_ptr<Session>& session);
}
