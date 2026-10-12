// SPDX-License-Identifier: GPL-3.0-or-later
#define NOMINMAX
#include <windows.h>
#include <dshow.h>
#include "../../src/mpc-hc/AudioMixer.h"
#include <iostream>
#include <stdexcept>

using namespace AudioMix;
void Check(bool b, const char* m) { if (!b) throw std::runtime_error(m); }
float Energy(const std::vector<float>& v) { double e=0; for (auto x:v) e+=x*x; return float(e / std::max<size_t>(1,v.size())); }
int wmain(int argc, wchar_t** argv) {
    try {
        Check(argc == 2, "usage: decoder_tests media-directory");
        std::wstring dir = argv[1], error;
        Check(!Probe(dir + L"\\missing.mkv", 0, error), "missing file rejected");
        Check(!Probe(L"https://example.com/video.mp4", 0, error), "network source rejected");
        Decoder unopened;
        std::vector<float> invalid;
        Check(!unopened.Read(0, 480, invalid, error), "unopened decoder fails safely");
        for (const auto* name : {L"two-tracks.mkv", L"three-tracks.mkv", L"three-tracks.mp4", L"three-tracks.mov", L"three-tracks.avi", L"three-tracks.webm"}) {
            auto session = Probe(dir + L"\\" + name, 1, error);
            if (!session) std::wcerr << error << L'\n';
            Check(bool(session), "probe");
            Check(session->tracks.size() == (std::wstring(name) == L"two-tracks.mkv" ? 2 : 3), "track count");
            Check(session->tracks[1].volume == 1 && session->tracks[0].volume == 0, "selected track only on initialization");
            std::vector<float> block;
            for (size_t i = 0; i < session->tracks.size(); ++i) {
                Decoder decoder;
                Check(decoder.Open(session->path, session->tracks[i].streamIndex, error), "open track");
                Check(decoder.Seek(0, error), "initial seek");
                Check(decoder.Read(0, SampleRate / 10, block, error), "first 100ms (allow codec priming)");
                if (i == 1) Check(Energy(block) < 1e-8, "delayed track has initial silence");
                else Check(Energy(block) > 1e-5, "active track has sound");
                for (int second : {2, 6, 1, 3, 0, 7, 2}) {
                    Check(decoder.Seek(int64_t(second) * SampleRate, error), "repeated seek");
                    Check(decoder.Read(int64_t(second) * SampleRate, BlockFrames, block, error), "read after seek");
                    if (i == 1 && (second < 1.2 || second > 4.3)) Check(Energy(block) < 1e-8, "silence outside short track");
                    else {
                        if (Energy(block) <= 1e-5) std::wcerr << name << L" track=" << i << L" seek=" << second << L" energy=" << Energy(block) << L'\n';
                        Check(Energy(block) > 1e-5, "sound after seek");
                    }
                }
            }
            // Check the actual placement of the delayed track, not just its energy.
            Decoder delayed;
            Check(delayed.Open(session->path, session->tracks[1].streamIndex, error), "open delayed track");
            Check(delayed.Seek(0, error), "seek delayed track");
            int64_t first = -1, last = -1;
            for (int64_t f = 0; f < 8 * SampleRate; f += BlockFrames) {
                Check(delayed.Read(f, BlockFrames, block, error), "decode delayed timeline");
                for (int k = 0; k < BlockFrames; ++k) {
                    if (std::abs(block[k * Channels]) > 0.001f) {
                        if (first < 0) first = f + k;
                        last = f + k;
                    }
                }
            }
            Check(std::abs(first - int64_t(1.25 * SampleRate)) < 0.04 * SampleRate, "delayed onset within 40ms");
            Check(std::abs(last - int64_t(4.25 * SampleRate)) < 0.04 * SampleRate, "delayed end within 40ms");
            std::wcout << L"PASS: " << name << L" (PCM/AAC/Opus, mono/stereo, delay, short track, seeks)\n";
        }
        auto longSession = Probe(dir + L"\\long-30min.mp4", 0, error);
        Check(bool(longSession), "probe long media");
        for (const auto& t : longSession->tracks) {
            Decoder decoder;
            Check(decoder.Open(longSession->path, t.streamIndex, error), "open long track");
            // Decode 30 minutes; each second has a 20ms sound matching a video flash.
            std::vector<float> block;
            Check(decoder.Seek(0, error), "seek long track");
            bool pulse = false;
            for (int64_t f = 0; f < int64_t(1800) * SampleRate; f += BlockFrames) {
                Check(decoder.Read(f, BlockFrames, block, error), "continuous long decode");
                const int64_t phase = f % SampleRate;
                if (phase == 0) pulse = false;
                if (phase < SampleRate * 0.04 && Energy(block) > 0.001f) pulse = true;
                if (phase == SampleRate / 20) Check(pulse, "audio marker within 40ms on the 30-minute timeline");
                if (phase > SampleRate / 3 && phase < SampleRate * 2 / 3) Check(Energy(block) < 1e-6f, "silence between timed markers");
            }
        }
        std::cout << "PASS: 30-minute continuous decoding of both tracks (offline timeline, not physical A/V measurement)\n";
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
