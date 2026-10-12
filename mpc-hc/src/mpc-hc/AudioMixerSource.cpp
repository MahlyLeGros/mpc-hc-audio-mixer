// SPDX-License-Identifier: GPL-3.0-or-later
#include "stdafx.h"
#include "AudioMixer.h"
#include "IGraphBuilder2.h"

namespace AudioMix {
namespace {
// {756298D1-F2AB-47A1-83D7-C1D268B89E16}
const CLSID CLSID_AudioMixerSource = {0x756298d1, 0xf2ab, 0x47a1, {0x83, 0xd7, 0xc1, 0xd2, 0x68, 0xb8, 0x9e, 0x16}};

class Stream final : public CSourceStream, public CSourceSeeking {
    CCritSec m_seekLock;
    std::shared_ptr<Session> m_session;
    std::vector<std::unique_ptr<Decoder>> m_decoders;
    std::vector<std::vector<float>> m_input;
    Mixer m_mixer;
    int64_t m_frame = 0;
    int64_t m_segmentFrame = 0;
    bool m_pendingSeek = true;
    bool m_discontinuity = true;

    HRESULT Restart(bool seek) {
        { std::lock_guard<std::mutex> lock(m_session->mutex); m_session->meters.Clear(); }
        if (ThreadExists()) {
            DeliverBeginFlush();
            Stop();
            m_pendingSeek = seek;
            DeliverEndFlush();
            Run();
        } else if (seek) {
            m_pendingSeek = true;
        }
        return S_OK;
    }
public:
    Stream(CSource* source, HRESULT* hr, std::shared_ptr<Session> session)
        : CSourceStream(NAME("Audio mixer output"), hr, source, L"Stereo mix")
        , CSourceSeeking(NAME("Audio mixer seeking"), static_cast<IPin*>(this), hr, &m_seekLock)
        , m_session(std::move(session)) {
        m_rtDuration = m_rtStop = m_session->duration;
        m_dwSeekingCaps = AM_SEEKING_CanSeekAbsolute | AM_SEEKING_CanSeekForwards |
                          AM_SEEKING_CanSeekBackwards | AM_SEEKING_CanGetDuration | AM_SEEKING_CanGetStopPos;
        int opened = 0;
        std::lock_guard<std::mutex> lock(m_session->mutex);
        for (auto& track : m_session->tracks) {
            auto decoder = std::make_unique<Decoder>();
            track.error.clear();
            if (!decoder->Open(m_session->path, track.streamIndex, track.error)) decoder.reset();
            else ++opened;
            m_decoders.push_back(std::move(decoder));
        }
        m_input.resize(m_decoders.size());
        if (!opened) *hr = VFW_E_CANNOT_RENDER;
    }
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** out) override {
        if (riid == IID_IMediaSeeking) return CSourceSeeking::NonDelegatingQueryInterface(riid, out);
        return CSourceStream::NonDelegatingQueryInterface(riid, out);
    }
    HRESULT GetMediaType(CMediaType* mt) override {
        if (!mt) return E_POINTER;
        WAVEFORMATEX wave = {};
        wave.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        wave.nChannels = Channels;
        wave.nSamplesPerSec = SampleRate;
        wave.wBitsPerSample = 32;
        wave.nBlockAlign = Channels * sizeof(float);
        wave.nAvgBytesPerSec = wave.nSamplesPerSec * wave.nBlockAlign;
        mt->InitMediaType();
        mt->SetType(&MEDIATYPE_Audio);
        mt->SetSubtype(&MEDIASUBTYPE_IEEE_FLOAT);
        mt->SetFormatType(&FORMAT_WaveFormatEx);
        mt->SetTemporalCompression(FALSE);
        mt->SetSampleSize(wave.nBlockAlign);
        return mt->SetFormat(reinterpret_cast<BYTE*>(&wave), sizeof(wave)) ? S_OK : E_OUTOFMEMORY;
    }
    HRESULT DecideBufferSize(IMemAllocator* allocator, ALLOCATOR_PROPERTIES* props) override {
        props->cBuffers = 4;
        props->cbBuffer = BlockFrames * Channels * sizeof(float);
        props->cbAlign = 1;
        ALLOCATOR_PROPERTIES actual = {};
        HRESULT hr = allocator->SetProperties(props, &actual);
        if (FAILED(hr)) return hr;
        return actual.cbBuffer >= props->cbBuffer ? S_OK : E_FAIL;
    }
    HRESULT OnThreadCreate() override { m_pendingSeek = true; return S_OK; }
    HRESULT OnThreadStartPlay() override {
        if (m_pendingSeek) {
            m_segmentFrame = m_frame = m_rtStart * SampleRate / 10000000;
            for (size_t i = 0; i < m_decoders.size(); ++i) {
                if (!m_decoders[i]) continue;
                std::wstring error;
                if (!m_decoders[i]->Seek(m_frame, error)) {
                    std::lock_guard<std::mutex> lock(m_session->mutex);
                    m_session->tracks[i].error = error;
                    m_decoders[i].reset();
                }
            }
            {
                std::lock_guard<std::mutex> lock(m_session->mutex);
                m_session->meters.Clear();
                m_mixer.Reset(m_session->tracks, m_session->master);
            }
            m_pendingSeek = false;
            m_discontinuity = true;
        }
        return DeliverNewSegment(m_rtStart, m_rtStop, m_dRateSeeking);
    }
    HRESULT ChangeStart() override { return Restart(true); }
    HRESULT ChangeStop() override { return Restart(false); }
    HRESULT ChangeRate() override { return Restart(true); }
    STDMETHODIMP SetRate(double rate) override {
        // v1 has one shared time base at normal speed. Reject unsupported rates
        // rather than quietly drifting relative to the video renderer.
        return rate == 1.0 ? S_OK : E_INVALIDARG;
    }
    HRESULT FillBuffer(IMediaSample* sample) override {
        int64_t stop;
        {
            CAutoLock lock(&m_seekLock);
            stop = m_rtStop * SampleRate / 10000000;
        }
        if (m_frame >= stop) return S_FALSE;
        const int frames = int(std::min<int64_t>(BlockFrames, stop - m_frame));
        BYTE* output = nullptr;
        HRESULT hr = sample->GetPointer(&output);
        if (FAILED(hr)) return hr;
        if (sample->GetSize() < frames * Channels * int(sizeof(float))) return E_FAIL;
        for (size_t i = 0; i < m_decoders.size(); ++i) {
            std::wstring error;
            if (!m_decoders[i]) m_input[i].assign(size_t(frames) * Channels, 0.0f);
            else if (!m_decoders[i]->Read(m_frame, frames, m_input[i], error)) {
                std::lock_guard<std::mutex> lock(m_session->mutex);
                m_session->tracks[i].error = error;
                m_input[i].assign(size_t(frames) * Channels, 0.0f);
                m_decoders[i].reset();
            }
        }
        {
            std::lock_guard<std::mutex> lock(m_session->mutex);
            m_mixer.Process(m_input, m_session->tracks, m_session->master, reinterpret_cast<float*>(output), frames);
            m_session->meters.Push(m_frame, frames, m_session->tracks);
        }
        REFERENCE_TIME begin = (m_frame - m_segmentFrame) * 10000000 / SampleRate;
        REFERENCE_TIME end = (m_frame + frames - m_segmentFrame) * 10000000 / SampleRate;
        sample->SetTime(&begin, &end);
        sample->SetActualDataLength(frames * Channels * sizeof(float));
        sample->SetSyncPoint(TRUE);
        sample->SetDiscontinuity(m_discontinuity);
        m_discontinuity = false;
        m_frame += frames;
        return S_OK;
    }
};

class Source final : public CSource {
public:
    Source(HRESULT* hr, const std::shared_ptr<Session>& session)
        : CSource(NAME("MPC-HC multi-track audio mixer"), nullptr, CLSID_AudioMixerSource, hr) {
        new Stream(this, hr, session); // CSource owns its streams.
    }
};
}

HRESULT AddSource(IGraphBuilder* graph, const std::shared_ptr<Session>& session) {
    if (!graph || !session) return E_POINTER;
    HRESULT hr = S_OK;
    auto source = new Source(&hr, session);
    CComPtr<IBaseFilter> filter = source;
    if (FAILED(hr)) return hr;
    hr = graph->AddFilter(filter, L"Multi-track audio mixer");
    if (SUCCEEDED(hr)) hr = graph->Render(source->GetPin(0));
    if (FAILED(hr)) {
        CComQIPtr<IGraphBuilder2> managedGraph = graph;
        if (managedGraph) managedGraph->NukeDownstream(filter);
        graph->RemoveFilter(filter);
    }
    return hr;
}
}
