// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "AudioMixerCore.h"
namespace AudioMix {
inline const wchar_t* const IconIds[] = {L"none",L"microphone",L"gamepad",L"person",L"group",L"screen",L"music",L"speaker",L"headphones",L"camera",L"browser",L"phone"};
inline int IconIndex(const std::wstring& id) {
    for (int i=0; i<12; ++i) if (id == IconIds[i]) return i;
    return 0;
}
inline std::wstring NormalizeTrackName(const std::wstring& name) {
    std::wstring result;
    for (wchar_t c : name) if (!std::iswcntrl(c)) result += c;
    const auto begin=result.find_first_not_of(L" ");
    if (begin==std::wstring::npos) return {};
    result=result.substr(begin,result.find_last_not_of(L" ")-begin+1);
    if (result.size()>64) {
        result.resize(64);
        if (result.back()>=0xd800 && result.back()<=0xdbff) result.pop_back();
    }
    return result;
}
struct TrackPreference { int volume = 0; bool mute = false; std::wstring icon = L"none"; std::wstring customName; };
inline void ApplyPreferences(std::vector<Track>& tracks, const std::vector<TrackPreference>& saved) {
    for (size_t i=0; i<tracks.size(); ++i) {
        const auto value = i<saved.size() ? saved[i] : TrackPreference{};
        tracks[i].volume = std::clamp(value.volume,0,100)/100.0f;
        tracks[i].mute = value.mute;
        tracks[i].icon = IconIds[IconIndex(value.icon)];
        tracks[i].customName = NormalizeTrackName(value.customName);
    }
}
}
