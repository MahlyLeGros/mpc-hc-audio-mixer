// SPDX-License-Identifier: GPL-3.0-or-later
#include "stdafx.h"
#include "MainFrm.h"

bool CMainFrame::CanMixAudio() {
    if (!IsStateLoaded() || GetPlaybackMode() != PM_FILE || m_audioMixReload) return false;
    CString ext = lastOpenFile.Right(4); ext.MakeLower();
    if (ext != L".mkv" && ext != L".mp4") return false;
    if (m_audioMixSession) return m_audioMixSession->tracks.size() > 1;
    DWORD count = 0;
    if (m_pAudioSwitcherSS && SUCCEEDED(m_pAudioSwitcherSS->Count(&count))) return count > 1;
    if (m_pSplitterSS && SUCCEEDED(m_pSplitterSS->Count(&count))) {
        int audio = 0;
        for (DWORD i=0; i<count; ++i) {
            DWORD group = 0;
            if (SUCCEEDED(m_pSplitterSS->Info(i,nullptr,nullptr,nullptr,&group,nullptr,nullptr,nullptr)) && group==1) ++audio;
        }
        return audio > 1;
    }
    return false;
}

void CMainFrame::OnUpdateAudioMixer(CCmdUI* ui) {
    m_wndToolBar.m_audioMixerButton.popupVisible = [this]() {
        return m_audioMixDialog && m_audioMixDialog->IsWindowVisible();
    };
    ui->SetCheck(m_audioMixDialog && m_audioMixDialog->IsWindowVisible());
    ui->Enable(CanMixAudio());
}

void CMainFrame::OnAudioMixer() {
    if (!CanMixAudio()) return;
    const auto clickVisibility = m_wndToolBar.m_audioMixerButton.ClickVisibility();
    if (clickVisibility.value_or(m_audioMixDialog && m_audioMixDialog->IsWindowVisible())) {
        if (m_audioMixDialog) m_audioMixDialog->ShowWindow(SW_HIDE);
        return;
    }
    if (!m_audioMixSession) {
        CWaitCursor wait;
        std::wstring error;
        m_audioMixSession = AudioMix::Probe(lastOpenFile.GetString(), GetCurrentAudioTrackIdx(), error);
        if (!m_audioMixSession) {
            AfxMessageBox(error.c_str(), MB_OK | MB_ICONINFORMATION);
            return;
        }
    }
    if (!m_audioMixDialog) {
        m_audioMixDialog = std::make_unique<CAudioMixerDlg>(this, m_audioMixSession,
            [this](bool enabled) { ToggleAudioMixer(enabled); },
            [this]() -> int64_t {
                REFERENCE_TIME position = 0;
                if (!m_pMS || m_audioMixReload || GetMediaState() != State_Running ||
                    FAILED(m_pMS->GetCurrentPosition(&position))) return -1;
                return position;
            });
        if (!m_audioMixDialog->CreateMixer(this, true)) { m_audioMixDialog.reset(); return; }
        m_audioMixDialog->SetCollapseHandler([this]() { m_audioMixDialog->ShowWindow(SW_HIDE); });
    }
    m_audioMixDialog->ShowAttached(&m_wndToolBar.m_audioMixerButton);
    if (!m_audioMixSession->enabled) ToggleAudioMixer(true);
}

void CMainFrame::ToggleAudioMixer(bool enabled) {
    if (!IsStateLoaded() || m_audioMixReload || !m_audioMixSession) return;
    const int selected = GetCurrentAudioTrackIdx();
    {
        std::lock_guard<std::mutex> lock(m_audioMixSession->mutex);
        if (!m_audioMixSession->active) return;
        if (enabled && !m_audioMixSession->initialized) {
            if (selected >= 0 && size_t(selected) < m_audioMixSession->tracks.size()) {
                m_audioMixSession->selected = selected;
                for (size_t i = 0; i < m_audioMixSession->tracks.size(); ++i)
                    m_audioMixSession->tracks[i].volume = int(i) == selected ? 1.0f : 0.0f;
            }
            m_audioMixSession->initialized = true;
        }
        m_audioMixSession->enabled = enabled;
        m_audioMixSession->error.clear();
        m_iReloadAudioIdx = m_audioMixSession->selected;
    }
    m_audioMixResumeState = GetMediaState();
    if (enabled) {
        if (m_pMS) m_pMS->SetRate(1.0);
        m_dSpeedRate = 1.0;
    }
    m_audioMixReload = true;
    if (m_audioMixDialog) m_audioMixDialog->SetBusy(true);
    m_rtReloadPos = m_wndSeekBar.HasDuration() ? m_wndSeekBar.GetPos() : 0;
    m_iReloadSubIdx = GetCurrentSubtitleTrackIdx();
    OnFileReopen();
}
