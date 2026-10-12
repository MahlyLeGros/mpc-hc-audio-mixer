// SPDX-License-Identifier: GPL-3.0-or-later
#include "../../src/mpc-hc/stdafx.h"
#include "../../src/mpc-hc/AudioMixer.h"
#include <atomic>
#include <iostream>
#include <stdexcept>

CWinApp testApp;
namespace {
void Check(bool b, const char* message) { if (!b) throw std::runtime_error(message); }
const CLSID TestSinkId = {0x6e36b2db, 0x2577, 0x4ae2, {0x84, 0x26, 0xa1, 0x54, 0xbb, 0x1a, 0x3d, 0x78}};
class Sink : public CBaseRenderer {
    REFERENCE_TIME m_expected = 0;
public:
    std::atomic<int> count{0}, discontinuities{0};
    std::atomic<bool> valid{true};
    Sink(HRESULT* hr) : CBaseRenderer(TestSinkId, NAME("Mixer test sink"), nullptr, hr) {}
    HRESULT CheckMediaType(const CMediaType* mt) override {
        return mt->majortype == MEDIATYPE_Audio && mt->subtype == MEDIASUBTYPE_IEEE_FLOAT ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
    }
    HRESULT DoRenderSample(IMediaSample* sample) override {
        REFERENCE_TIME begin, end;
        if (FAILED(sample->GetTime(&begin, &end))) valid = false;
        else {
            if (sample->IsDiscontinuity() == S_OK) { m_expected = 0; ++discontinuities; }
            if (begin != m_expected || end <= begin) valid = false;
            m_expected = end;
        }
        BYTE* data = nullptr;
        if (FAILED(sample->GetPointer(&data))) valid = false;
        else for (int i = 0; i < sample->GetActualDataLength() / int(sizeof(float)); ++i)
            if (!std::isfinite(reinterpret_cast<float*>(data)[i]) || std::abs(reinterpret_cast<float*>(data)[i]) > 0.98001f) valid = false;
        ++count;
        Sleep(1); // Allow the test to pause before the short fixture reaches EOS.
        return S_OK;
    }
};
void WaitSamples(Sink* sink, int target) {
    const ULONGLONG deadline = GetTickCount64() + 10000;
    while (sink->count < target && GetTickCount64() < deadline) Sleep(5);
    Check(sink->count >= target, "audio delivery timed out");
}
}
int wmain(int argc, wchar_t** argv) {
    if (!AfxWinInit(GetModuleHandle(nullptr), nullptr, GetCommandLine(), 0)) return 1;
    HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(init)) return 1;
    int result = 0;
    try {
        Check(argc == 2 || argc == 3, "usage: graph_tests media-directory [filename]");
        std::wstring error;
        auto session = AudioMix::Probe(std::wstring(argv[1]) + L"\\" + (argc == 3 ? argv[2] : L"three-tracks.mkv"), 0, error);
        Check(bool(session), "probe graph media");
        session->enabled = true;
        session->tracks[1].volume = 0.5f;
        CComPtr<IGraphBuilder> graph;
        Check(SUCCEEDED(graph.CoCreateInstance(CLSID_FilterGraph)), "create graph");
        HRESULT hr = S_OK;
        Sink* sink = new Sink(&hr);
        CComPtr<IBaseFilter> sinkFilter = sink;
        Check(SUCCEEDED(hr) && SUCCEEDED(graph->AddFilter(sinkFilter, L"Test PCM sink")), "add test sink");
        Check(SUCCEEDED(AudioMix::AddSource(graph, session)), "connect mixer source");
        CComQIPtr<IMediaControl> control = graph;
        CComQIPtr<IMediaSeeking> seeking = graph;
        CComQIPtr<IMediaFilter> mediaFilter = graph;
        mediaFilter->SetSyncSource(nullptr); // Offline test of delivery and graph state transitions.
        Check(SUCCEEDED(control->Run()), "run graph");
        WaitSamples(sink, 50);
        Check(SUCCEEDED(control->Pause()), "pause graph");
        OAFilterState state;
        Check(SUCCEEDED(control->GetState(5000, &state)) && state == State_Paused, "graph paused");
        const int paused = sink->count;
        Sleep(100);
        Check(sink->count == paused, "no delivery while paused");
        Check(SUCCEEDED(control->Run()), "resume graph");
        WaitSamples(sink, paused + 30);
        for (int second : {2, 6, 1, 7, 0}) {
            Check(SUCCEEDED(control->Pause()), "pause before seek");
            control->GetState(5000, &state);
            REFERENCE_TIME pos = int64_t(second) * 10000000;
            Check(SUCCEEDED(seeking->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning)), "graph seek");
            const int before = sink->count;
            Check(SUCCEEDED(control->Run()), "run after seek");
            WaitSamples(sink, before + 20);
        }
        Check(sink->valid, "continuous timestamps and bounded PCM after pause and seeks");
        Check(sink->discontinuities >= 6, "each seek starts a new segment");
        Check(SUCCEEDED(control->Stop()), "stop graph");
        Check(SUCCEEDED(control->Run()), "restart stopped graph");
        const int restart = sink->count;
        WaitSamples(sink, restart + 20);
        CComQIPtr<IMediaEvent> events = graph;
        long event = 0;
        Check(SUCCEEDED(events->WaitForCompletion(15000, &event)) && event == EC_COMPLETE, "end of stream");
        Check(sink->valid, "valid samples through EOS");
        control->Stop();
        std::cout << "PASS: DirectShow delivery, pause/resume, repeated seek, stop/restart, EOS, timestamps and PCM bounds\n";
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; result = 1; }
    CoUninitialize();
    return result;
}
