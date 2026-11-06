// SPDX-License-Identifier: GPL-3.0-or-later
#include "../../src/mpc-hc/AudioMixerPreferences.h"
#include <cassert>
#include <iostream>
using namespace AudioMix;
int main() {
    std::vector<Track> tracks(4);
    std::vector<TrackPreference> saved={{0,true,L"microphone"},{100,false,L"gamepad"},{37,true,L"person"}};
    ApplyPreferences(tracks,saved);
    assert(tracks[0].volume==0 && tracks[0].mute && tracks[0].icon==L"microphone");
    assert(tracks[1].volume==1 && !tracks[1].mute && tracks[1].icon==L"gamepad");
    assert(tracks[2].mute && tracks[2].volume>.369f && tracks[2].volume<.371f);
    assert(tracks[3].volume==0 && !tracks[3].mute && tracks[3].icon==L"none");
    tracks.resize(1); tracks[0].name=L"Other name"; tracks[0].streamIndex=9;
    ApplyPreferences(tracks,saved); assert(tracks[0].icon==L"microphone");
    tracks.resize(4); ApplyPreferences(tracks,saved); assert(tracks[2].icon==L"person");
    saved={{-30,false,L"bad"},{900,true,L"phone"}};
    ApplyPreferences(tracks,saved); assert(tracks[0].volume==0 && tracks[0].icon==L"none");
    assert(tracks[1].volume==1 && tracks[1].mute);
    for (int i=0;i<12;++i) assert(IconIndex(IconIds[i])==i);
    saved={{25,false,L"microphone"},{75,true,L"microphone"}};
    ApplyPreferences(tracks,saved); assert(tracks[0].volume!=tracks[1].volume && tracks[0].icon==tracks[1].icon);
    saved={{45,true,L"microphone",L"  Micro personnel  "}};
    tracks[0].name=L"Source title";
    ApplyPreferences(tracks,saved);
    assert(tracks[0].customName==L"Micro personnel" && tracks[0].name==L"Source title");
    tracks[0].name=L"Another video"; ApplyPreferences(tracks,saved);
    assert(tracks[0].customName==L"Micro personnel");
    assert(NormalizeTrackName(L"   ").empty());
    assert(NormalizeTrackName(L"a\r\nb\tc")==L"abc");
    assert(NormalizeTrackName(std::wstring(100,L'x')).size()==64);
    assert(NormalizeTrackName(L"Voix \u00e9quipe")==L"Voix \u00e9quipe");
    saved[0].customName=L""; ApplyPreferences(tracks,saved);
    assert(tracks[0].customName.empty() && tracks[0].name==L"Another video");
    std::cout<<"PASS: ordinal preferences, missing/extra/renamed tracks, bounds, mute, 12 icons and duplicate icons\n";
}
