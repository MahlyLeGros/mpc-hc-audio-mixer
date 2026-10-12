// SPDX-License-Identifier: GPL-3.0-or-later
#include "../../src/mpc-hc/AudioMixerCore.h"
#include <iostream>
#include <stdexcept>
#include <limits>

using namespace AudioMix;
void Check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
void Near(float a, float b, const char* message) { Check(std::abs(a - b) < 0.0001f, message); }

int main() {
    try {
        std::vector<Track> tracks(3);
        tracks[0].volume = 1;
        tracks[1].volume = 0.5f;
        tracks[2].volume = 0;
        std::vector<std::vector<float>> input(3, std::vector<float>(BlockFrames * Channels, 0.2f));
        std::vector<float> out(BlockFrames * Channels);
        Mixer mix;
        mix.Reset(tracks, 1);
        mix.Process(input, tracks, 1, out.data(), BlockFrames);
        Near(out.back(), 0.3f, "independent volumes");
        Near(tracks[0].peak, 0.2f, "track meter");
        Near(tracks[1].peak, 0.2f, "meter unaffected by half volume");
        Near(tracks[2].peak, 0.2f, "meter active at zero volume");
        tracks[0].mute = true;
        mix.Process(input, tracks, 1, out.data(), BlockFrames);
        Near(out.back(), 0.1f, "mute");
        Near(tracks[0].peak, 0.2f, "meter active while muted");
        tracks[1].solo = true;
        tracks[2].solo = true;
        tracks[2].volume = 1;
        mix.Process(input, tracks, 1, out.data(), BlockFrames);
        Near(out.back(), 0.3f, "multiple solos");
        tracks[1].mute = true;
        mix.Process(input, tracks, 1, out.data(), BlockFrames);
        Near(out.back(), 0.2f, "mute takes priority over solo");
        mix.Process(input, tracks, 0.5f, out.data(), BlockFrames);
        Near(out.back(), 0.1f, "master gain");
        Near(tracks[1].peak, 0.2f, "meter independent of solo and master gain");
        for (auto& t : tracks) { t.volume = 1; t.mute = t.solo = false; }
        for (auto& plane : input) std::fill(plane.begin(), plane.end(), 0.9f);
        mix.Reset(tracks, 1);
        mix.Process(input, tracks, 1, out.data(), BlockFrames);
        Check(*std::max_element(out.begin(), out.end()) <= 0.98001f, "limiter prevents clipping");
        input[0][0] = std::numeric_limits<float>::quiet_NaN();
        input[1][0] = std::numeric_limits<float>::infinity();
        mix.Process(input, tracks, 1, out.data(), BlockFrames);
        for (float v : out) Check(std::isfinite(v), "invalid input sanitized");
        for (auto& plane : input) std::fill(plane.begin(), plane.end(), std::numeric_limits<float>::max());
        mix.Process(input, tracks, 1, out.data(), BlockFrames);
        for (float v : out) Check(std::isfinite(v) && std::abs(v) <= 0.98001f, "extreme float PCM remains bounded");
        tracks.assign(1, Track{});
        input.assign(1, std::vector<float>(BlockFrames * Channels, 0.5f));
        mix.Reset(tracks, 1);
        tracks[0].volume = 1;
        mix.Process(input, tracks, 1, out.data(), BlockFrames);
        Check(out[0] > 0 && out[0] < 0.01f && out[1] == out[0], "gain ramp avoids clicks and preserves stereo");
        Near(out.back(), 0.5f, "gain ramp reaches target");
        tracks[0].error = L"codec failure";
        mix.Process(input, tracks, 1, out.data(), BlockFrames);
        Near(out.back(), 0, "failed track is silent");
        std::vector<float> timeline(20, 0), block(8, 0.75f);
        CopyOverlap(block, 3, timeline, 0);
        Near(timeline[5], 0, "late track begins with silence");
        Near(timeline[6], 0.75f, "late track aligned to timestamp");
        Near(timeline[14], 0, "short track ends with silence");
        timeline.assign(8, 0);
        CopyOverlap(block, -2, timeline, 0);
        Near(timeline[0], 0.75f, "negative preroll discarded");
        Near(timeline[4], 0, "overlap bounded");
        MeterTimeline meter;
        tracks[0].peak = 0.8f;
        meter.Push(48000, 480, tracks); // Decoder has reached 1 second ahead of playback.
        Near(meter.Levels(0,3)[0], 0, "future decoded audio must not light meter");
        Near(meter.Levels(48000,3)[0], 0.8f, "meter follows media timestamp");
        Near(meter.Levels(48000+2300,3)[0], 0.8f, "short transient survives UI polling");
        Near(meter.Levels(48000+3000,3)[0], 0, "meter clears after transient");
        Near(meter.Levels(-1,3)[0], 0, "paused meter is silent");
        meter.Clear();
        Near(meter.Levels(48000,3)[0], 0, "seek clears old meter levels");
        // 30 minutes of timestamp arithmetic: no accumulation of rounded 100ns samples.
        const int64_t end = int64_t(30 * 60) * SampleRate;
        for (int64_t frame = 0; frame < end; frame += BlockFrames) {
            int64_t rt = frame * 10000000 / SampleRate;
            Check(std::abs(rt * SampleRate / 10000000 - frame) <= 1, "long playback timestamp precision");
        }
        std::cout << "PASS: gains, mute, solo, master, meters, limiter, ramps, failures, timestamp overlap, 30-minute arithmetic\n";
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
