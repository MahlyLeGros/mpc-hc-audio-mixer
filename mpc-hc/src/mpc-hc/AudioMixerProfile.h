// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "AudioMixer.h"
#include "AudioMixerPreferences.h"
#include "mplayerc.h"
namespace AudioMix {
inline bool AutoApply() { return AfxGetMyApp()->GetProfileInt(L"AudioMixer",L"AutoApply",0)==1; }
inline void LoadPreferences(const std::shared_ptr<Session>& session) {
    auto* app=AfxGetMyApp();
    if (!app->GetProfileInt(L"AudioMixer",L"HasProfile",0)) return;
    std::vector<TrackPreference> saved(session->tracks.size());
    for (size_t i=0; i<saved.size(); ++i) {
        CString key; key.Format(L"Track%u",unsigned(i+1));
        saved[i].volume=int(app->GetProfileInt(L"AudioMixer",key+L"Volume",0));
        saved[i].mute=app->GetProfileInt(L"AudioMixer",key+L"Mute",0)==1;
        saved[i].icon=app->GetProfileString(L"AudioMixer",key+L"Icon",L"none").GetString();
        saved[i].customName=app->GetProfileString(L"AudioMixer",key+L"Name",L"").GetString();
    }
    ApplyPreferences(session->tracks,saved);
    session->initialized=true;
}
inline void SavePreferences(const std::shared_ptr<Session>& session) {
    std::vector<TrackPreference> saved;
    {
        std::lock_guard<std::mutex> lock(session->mutex);
        if (!session->active || !session->enabled) return;
        for (const auto& t: session->tracks) saved.push_back({int(std::lround(t.volume*100)),t.mute,t.icon,t.customName});
    }
    auto* app=AfxGetMyApp();
    for (size_t i=0; i<saved.size(); ++i) {
        CString key; key.Format(L"Track%u",unsigned(i+1));
        app->WriteProfileInt(L"AudioMixer",key+L"Volume",saved[i].volume);
        app->WriteProfileInt(L"AudioMixer",key+L"Mute",saved[i].mute);
        app->WriteProfileString(L"AudioMixer",key+L"Icon",saved[i].icon.c_str());
        app->WriteProfileString(L"AudioMixer",key+L"Name",saved[i].customName.c_str());
    }
    app->WriteProfileInt(L"AudioMixer",L"HasProfile",1);
}
}
