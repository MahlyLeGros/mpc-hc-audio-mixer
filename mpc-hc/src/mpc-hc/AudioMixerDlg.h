// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "AudioMixer.h"
#include "resource.h"
#include "VolumeCtrl.h"
#include <functional>

class CAudioTrackMeter : public CStatic {
    int m_segments = 0;
    void DrawItem(LPDRAWITEMSTRUCT item) override;
public:
    void SetLevel(float peak);
};

class CAudioTrackMute : public CButton {
    bool m_muted = false;
    CImage m_nativeIcons;
    int m_iconSize = 0;
    void DrawItem(LPDRAWITEMSTRUCT item) override;
public:
    void SetMuted(bool muted) {
        if (m_muted != muted) { m_muted = muted; Invalidate(FALSE); }
    }
};

class CAudioMixerDlg : public CDialog {
    struct Row {
        CStatic name;
        CAudioTrackMute mute;
        CAudioTrackMeter meter;
        CVolumeCtrl volume{true, true};
    };
    std::shared_ptr<AudioMix::Session> m_session;
    std::function<void(bool)> m_toggle;
    std::function<int64_t()> m_playbackPosition;
    std::vector<std::unique_ptr<Row>> m_rows;
    CBrush m_background;
    int m_wheelDelta = 0;
    HWND m_wheelTarget = nullptr;
    int m_visibleRows = 2;
    std::function<void()> m_hide;
    int m_first = 0;
    bool m_busy = false;
    bool m_attached = false;
    CWnd* m_anchor = nullptr;
    CRect Rect(int x, int y, int w, int h);
    void LayoutRows();
    void Refresh();
protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WPARAM wParam, LPARAM lParam) override;
    void OnCancel() override { if (m_hide) m_hide(); }
    void OnOK() override { OnCancel(); }
    afx_msg void OnHScroll(UINT code, UINT pos, CScrollBar* bar);
    afx_msg void OnVScroll(UINT code, UINT pos, CScrollBar* bar);
    afx_msg void OnTimer(UINT_PTR id);
    afx_msg void OnSize(UINT type, int cx, int cy);
    afx_msg BOOL OnMouseWheel(UINT flags, short delta, CPoint point);
    afx_msg HBRUSH OnCtlColor(CDC* dc, CWnd* wnd, UINT type);
    afx_msg BOOL OnEraseBkgnd(CDC* dc);
    afx_msg void OnActivate(UINT state, CWnd* other, BOOL minimized);
    DECLARE_MESSAGE_MAP()
public:
    CAudioMixerDlg(CWnd* parent, std::shared_ptr<AudioMix::Session> session, std::function<void(bool)> toggle, std::function<int64_t()> playbackPosition = {})
        : CDialog(IDD_AUDIO_MIXER, parent), m_session(std::move(session)), m_toggle(std::move(toggle)), m_playbackPosition(std::move(playbackPosition)) {}
    BOOL CreateMixer(CWnd* parent, bool attached = false);
    void ShowAttached(CWnd* anchor);
    void PositionAttached();
    void SetCollapseHandler(std::function<void()> hide) { m_hide = std::move(hide); }
    int PanelHeight() { return Rect(0, 0, 1, 24 + (m_visibleRows - 1) * 22).Height(); }
    BOOL HandleWheel(short delta, CPoint point);
    void SetBusy(bool busy) { m_busy = busy; if (GetSafeHwnd()) Refresh(); }
};

// A fixed, non-floating control bar: the frame reserves space outside the video.
class CAudioMixerPanel : public CDialogBar {
    CAudioMixerDlg* m_dialog = nullptr;
    int m_height = 0;
protected:
    afx_msg void OnSize(UINT type, int cx, int cy);
    DECLARE_MESSAGE_MAP()
public:
    BOOL CreatePanel(CFrameWnd* frame);
    void Attach(CAudioMixerDlg* dialog);
    void Detach() { m_dialog = nullptr; }
    void OnUpdateCmdUI(CFrameWnd*, BOOL) override {} // Dialog refresh owns control enablement.
    CSize CalcFixedLayout(BOOL stretch, BOOL horizontal) override;
};
