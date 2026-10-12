// SPDX-License-Identifier: GPL-3.0-or-later
#include "stdafx.h"
#include "AudioMixerDlg.h"
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")

// Segmented, logarithmic level display: green, yellow, then red near clipping.
void CAudioTrackMeter::SetLevel(float peak) {
    const float db = peak > 0 ? 20.0f * std::log10(peak) : -60.0f;
    const int segments = int(std::ceil(std::clamp((db + 60.0f) / 60.0f, 0.0f, 1.0f) * 24));
    if (m_segments != segments) { m_segments = segments; Invalidate(FALSE); }
}
void CAudioTrackMeter::DrawItem(LPDRAWITEMSTRUCT item) {
    CDC dc; dc.Attach(item->hDC);
    CRect bounds(item->rcItem);
    dc.FillSolidRect(bounds, RGB(24,26,29));
    for (int i=0; i<24; ++i) {
        const float db = -60.0f + (i+1) * 2.5f;
        const COLORREF color = db > -6 ? RGB(224,63,72) : db > -18 ? RGB(220,185,55) : RGB(54,198,91);
        const COLORREF dim = db > -6 ? RGB(62,29,34) : db > -18 ? RGB(59,52,25) : RGB(25,53,34);
        CRect segment(bounds.left+i*bounds.Width()/24, bounds.top,
                      bounds.left+(i+1)*bounds.Width()/24-1, bounds.bottom);
        dc.FillSolidRect(segment, i<m_segments ? color : dim);
    }
    dc.Detach();
}

BEGIN_MESSAGE_MAP(CAudioMixerDlg, CDialog)
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_WM_TIMER()
    ON_WM_MOUSEWHEEL()
    ON_WM_CTLCOLOR()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_WM_ACTIVATE()
END_MESSAGE_MAP()

BOOL CAudioMixerDlg::CreateMixer(CWnd* parent, bool attached) {
    // Copy the executable resource so translated DLLs and the original template stay intact.
    HINSTANCE module = AfxGetInstanceHandle();
    HRSRC resource = FindResource(module, MAKEINTRESOURCE(IDD_AUDIO_MIXER), RT_DIALOG);
    HGLOBAL data = resource ? LoadResource(module, resource) : nullptr;
    if (!data) return FALSE;
    m_attached = attached;
    const auto bytes = static_cast<const BYTE*>(LockResource(data));
    if (!attached) return CreateIndirect(reinterpret_cast<LPCDLGTEMPLATE>(bytes), parent);
    std::vector<BYTE> copy(bytes, bytes + SizeofResource(module, resource));
    WORD signature; memcpy(&signature, copy.data() + 2, sizeof(signature));
    const size_t styleOffset = signature == 0xffff ? 12 : 0;
    const size_t exOffset = signature == 0xffff ? 8 : 4;
    DWORD style, extended;
    memcpy(&style, copy.data() + styleOffset, sizeof(style));
    memcpy(&extended, copy.data() + exOffset, sizeof(extended));
    style = (style & ~(WS_CHILD | DS_CONTROL | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME)) | WS_POPUP | WS_BORDER;
    extended = (extended & ~WS_EX_APPWINDOW) | WS_EX_TOOLWINDOW;
    memcpy(copy.data() + styleOffset, &style, sizeof(style));
    memcpy(copy.data() + exOffset, &extended, sizeof(extended));
    return CreateIndirect(reinterpret_cast<LPCDLGTEMPLATE>(copy.data()), parent);
}
void CAudioMixerDlg::ShowAttached(CWnd* anchor) {
    m_anchor = anchor;
    PositionAttached();
    ShowWindow(SW_SHOW);
    SetActiveWindow();
    if (!m_rows.empty()) m_rows.front()->volume.SetFocus();
}
void CAudioMixerDlg::PositionAttached() {
    if (!m_attached || !m_anchor || !m_anchor->GetSafeHwnd()) return;
    CRect anchor; m_anchor->GetWindowRect(anchor);
    MONITORINFO monitor = {sizeof(monitor)};
    if (!GetMonitorInfo(MonitorFromRect(anchor, MONITOR_DEFAULTTONEAREST), &monitor)) return;
    CRect work(monitor.rcWork);
    CRect ownerBounds, intersection;
    if (GetOwner()) {
        GetOwner()->GetWindowRect(ownerBounds);
        if (!intersection.IntersectRect(work, ownerBounds) && !GetOwner()->IsIconic()) work = ownerBounds;
    }
    const int width = std::min(Rect(0,0,320,1).Width()+2, work.Width()-8);
    const int height = std::min(PanelHeight()+2, work.Height()-8);
    const int x = std::clamp(anchor.left, work.left+4, work.right-width-4);
    int y = anchor.top-height-6;
    if (y < work.top+4) y = std::min(anchor.bottom+6, work.bottom-height-4);
    y = std::max<int>(work.top+4, y);
    const CRect desired(x,y,x+width,y+height);
    CRect current; GetWindowRect(current);
    if (current != desired) SetWindowPos(nullptr,x,y,width,height,SWP_NOZORDER|SWP_NOACTIVATE);
}
void CAudioMixerDlg::OnActivate(UINT state, CWnd* other, BOOL minimized) {
    CDialog::OnActivate(state, other, minimized);
    if (m_attached && state == WA_INACTIVE && !m_busy) {
        OnCancel();
    }
}

CRect CAudioMixerDlg::Rect(int x, int y, int w, int h) {
    CRect rect(x, y, x + w, y + h);
    MapDialogRect(&rect);
    return rect;
}
BOOL CAudioMixerDlg::OnInitDialog() {
    CDialog::OnInitDialog();
    SetWindowText(L"M\u00e9langeur audio");
    m_background.CreateSolidBrush(RGB(32, 34, 38));
    const DWORD child = WS_CHILD | WS_VISIBLE;
    {
        std::lock_guard<std::mutex> lock(m_session->mutex);
        for (size_t i = 0; i < m_session->tracks.size(); ++i) {
            auto r = std::make_unique<Row>();
            r->name.Create(L"", child, Rect(0, 0, 1, 1), this);
            r->meter.Create(L"Niveau audio", child | SS_OWNERDRAW, Rect(0,0,1,1), this, UINT(5101+i*4));
            r->volume.Create(this, UINT(5100 + i * 4));
            r->volume.ModifyStyle(0, WS_TABSTOP);
            m_rows.push_back(std::move(r));
        }
    }
    for (CWnd* w = GetWindow(GW_CHILD); w; w = w->GetNextWindow()) w->SetFont(GetFont());
    SCROLLINFO si = {sizeof(si), SIF_RANGE | SIF_PAGE | SIF_POS};
    si.nMax = int(m_rows.size()) - 1;
    m_visibleRows = std::max(1, std::min(m_attached ? 4 : 2, int(m_rows.size())));
    si.nPage = m_visibleRows;
    SetScrollInfo(SB_VERT, &si);
    LayoutRows();
    Refresh();
    SetTimer(1, 33, nullptr);
    return TRUE;
}
void CAudioMixerDlg::LayoutRows() {
    if (m_rows.empty()) return;
    CRect client; GetClientRect(client);
    const int pad = Rect(0,0,8,1).Width();
    const int labelWidth = std::min(Rect(0,0,70,1).Width(),client.Width()/3);
    const int meterWidth = Rect(0,0,50,1).Width();
    const int sliderLeft = pad+labelWidth+meterWidth+pad;
    for (int i=0; i<int(m_rows.size()); ++i) {
        auto& r = *m_rows[i];
        const bool visible = i>=m_first && i<m_first+m_visibleRows;
        for (CWnd* w : std::initializer_list<CWnd*>{&r.name,&r.meter,&r.volume}) w->ShowWindow(visible ? SW_SHOW : SW_HIDE);
        if (!visible) continue;
        const int y = 6+(i-m_first)*22;
        r.name.MoveWindow(pad,Rect(0,y,1,1).top,labelWidth-pad,Rect(0,0,1,12).Height());
        r.meter.MoveWindow(pad+labelWidth,Rect(0,y+3,1,1).top,meterWidth,Rect(0,0,1,6).Height());
        r.volume.MoveWindow(sliderLeft,Rect(0,y,1,1).top,std::max(30,client.Width()-sliderLeft-pad),Rect(0,0,1,12).Height());
    }
    Invalidate(FALSE);
}
void CAudioMixerDlg::OnSize(UINT type, int cx, int cy) {
    CDialog::OnSize(type, cx, cy);
    LayoutRows();
}
void CAudioMixerDlg::Refresh() {
    const int64_t position = m_playbackPosition ? m_playbackPosition() : -1;
    std::lock_guard<std::mutex> lock(m_session->mutex);
    const bool usable = m_session->active && !m_busy;
    const auto levels = m_session->meters.Levels(position < 0 ? -1 : position * AudioMix::SampleRate / 10000000, m_rows.size());
    for (size_t i = 0; i < m_rows.size(); ++i) {
        auto& r = *m_rows[i];
        const auto& t = m_session->tracks[i];
        CString label(t.name.c_str());
        if (!t.language.empty() && t.language != L"und") label += L" [" + CString(t.language.c_str()) + L"]";
        if (!t.error.empty()) label += L" - " + CString(t.error.c_str());
        r.name.SetWindowText(label);
        r.volume.SetPos(int(std::lround(t.volume * 100)));
        r.volume.EnableWindow(usable && m_session->enabled && t.error.empty());
        r.meter.SetLevel(usable && m_session->enabled && t.error.empty() ? levels[i] : 0.0f);
    }
}
BOOL CAudioMixerDlg::OnCommand(WPARAM wParam, LPARAM lParam) {
    return CDialog::OnCommand(wParam, lParam);
}
void CAudioMixerDlg::OnHScroll(UINT code, UINT pos, CScrollBar* bar) {
    if (bar) {
        std::lock_guard<std::mutex> lock(m_session->mutex);
        for (size_t i = 0; i < m_rows.size(); ++i) if (bar->GetSafeHwnd() == m_rows[i]->volume.GetSafeHwnd())
            m_session->tracks[i].volume = m_rows[i]->volume.GetPos() / 100.0f;
    }
    CDialog::OnHScroll(code, pos, bar);
}
void CAudioMixerDlg::OnVScroll(UINT code, UINT pos, CScrollBar* bar) {
    switch (code) {
        case SB_TOP: m_first = 0; break;
        case SB_BOTTOM: m_first = int(m_rows.size()); break;
        case SB_LINEUP: --m_first; break;
        case SB_LINEDOWN: ++m_first; break;
        case SB_PAGEUP: m_first -= m_visibleRows; break;
        case SB_PAGEDOWN: m_first += m_visibleRows; break;
        case SB_THUMBTRACK: case SB_THUMBPOSITION: m_first = int(pos); break;
    }
    m_first = std::clamp(m_first, 0, std::max(0, int(m_rows.size()) - m_visibleRows));
    SetScrollPos(SB_VERT, m_first);
    LayoutRows();
    CDialog::OnVScroll(code, pos, bar);
}
BOOL CAudioMixerDlg::HandleWheel(short delta, CPoint point) {
    CSliderCtrl* target = nullptr;
    auto hit = [&](CSliderCtrl& slider) {
        CRect bounds; slider.GetWindowRect(bounds);
        return (slider.GetStyle() & WS_VISIBLE) && bounds.PtInRect(point);
    };
    for (auto& row : m_rows) if (hit(row->volume)) target = &row->volume;
    const HWND handle = target ? target->GetSafeHwnd() : nullptr;
    if (handle != m_wheelTarget) { m_wheelTarget = handle; m_wheelDelta = 0; }
    m_wheelDelta += delta;
    const int steps = m_wheelDelta / WHEEL_DELTA;
    m_wheelDelta %= WHEEL_DELTA;
    if (target) {
        if (target->IsWindowEnabled() && steps) {
            target->SetPos(std::clamp(target->GetPos() + steps * 5, 0, 100));
            OnHScroll(SB_THUMBPOSITION, target->GetPos(), reinterpret_cast<CScrollBar*>(target));
        }
    } else if (steps) {
        m_first = std::clamp(m_first - steps, 0, std::max(0,int(m_rows.size())-m_visibleRows));
        SetScrollPos(SB_VERT, m_first); LayoutRows();
    }
    // Always consume wheel input over the mixer, including disabled sliders.
    return TRUE;
}
BOOL CAudioMixerDlg::OnMouseWheel(UINT, short delta, CPoint point) { return HandleWheel(delta, point); }
HBRUSH CAudioMixerDlg::OnCtlColor(CDC* dc, CWnd*, UINT) {
    dc->SetTextColor(RGB(235,235,235)); dc->SetBkColor(RGB(32,34,38));
    return HBRUSH(m_background.GetSafeHandle());
}
BOOL CAudioMixerDlg::OnEraseBkgnd(CDC* dc) {
    CRect bounds; GetClientRect(bounds); dc->FillSolidRect(bounds, RGB(32,34,38)); return TRUE;
}
void CAudioMixerDlg::OnTimer(UINT_PTR id) { if (id == 1) { Refresh(); if (m_attached && IsWindowVisible()) PositionAttached(); } CDialog::OnTimer(id); }

BEGIN_MESSAGE_MAP(CAudioMixerPanel, CDialogBar)
    ON_WM_SIZE()
END_MESSAGE_MAP()
BOOL CAudioMixerPanel::CreatePanel(CFrameWnd* frame) {
    const auto previous = AfxGetResourceHandle();
    AfxSetResourceHandle(AfxGetInstanceHandle());
    const BOOL created = Create(frame, IDD_AUDIO_MIXER, CBRS_BOTTOM | CBRS_HIDE_INPLACE, ID_AUDIO_MIXER);
    AfxSetResourceHandle(previous);
    if (created) ModifyStyle(WS_VSCROLL, 0);
    return created;
}
void CAudioMixerPanel::Attach(CAudioMixerDlg* dialog) {
    m_dialog = dialog;
    m_height = dialog->PanelHeight();
    CRect client; GetClientRect(client);
    dialog->MoveWindow(0, 0, client.Width(), m_height);
    dialog->ShowWindow(SW_SHOW);
}
CSize CAudioMixerPanel::CalcFixedLayout(BOOL stretch, BOOL horizontal) {
    return CSize(stretch && horizontal ? 32767 : m_sizeDefault.cx, m_height);
}
void CAudioMixerPanel::OnSize(UINT type, int cx, int cy) {
    CDialogBar::OnSize(type, cx, cy);
    if (m_dialog && m_dialog->GetSafeHwnd()) m_dialog->MoveWindow(0, 0, cx, cy);
}
