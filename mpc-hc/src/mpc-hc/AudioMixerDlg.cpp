// SPDX-License-Identifier: GPL-3.0-or-later
#include "stdafx.h"
#include "AudioMixerDlg.h"
#include "AudioMixerProfile.h"
#include <uxtheme.h>
#include "SVGImage.h"
#include "CMPCTheme.h"
#include "mplayerc.h"
#pragma comment(lib, "uxtheme.lib")

// Rasterize MPC-HC's native small-size SVG artwork at the final pixel size.
// Never resize the bitmap already rendered for the main toolbar.
void CAudioTrackMute::DrawItem(LPDRAWITEMSTRUCT item) {
    CDC dc; dc.Attach(item->hDC);
    CRect bounds(item->rcItem);
    dc.FillSolidRect(bounds, RGB(32,34,38));
    const int size = std::min(bounds.Width(), bounds.Height());
    if (size > 0 && size != m_iconSize) {
        m_nativeIcons.Destroy();
        const UINT resource = size < 24 ? IDF_SVG_BUTTONS16 : size < 32 ? IDF_SVG_BUTTONS24 :
                              size < 48 ? IDF_SVG_BUTTONS32 : size < 64 ? IDF_SVG_BUTTONS48 : IDF_SVG_BUTTONS64;
        const int base = size < 24 ? 16 : size < 32 ? 24 : size < 48 ? 32 : size < 64 ? 48 : 64;
        m_iconSize = SUCCEEDED(SVGImage::Load(resource, m_nativeIcons, float(size)/base)) ? size : 0;
        // NanoSVG returns straight alpha; GDI AlphaBlend requires premultiplied RGB.
        if (m_iconSize) {
            m_nativeIcons.SetHasAlphaChannel(true);
            for (int y=0; y<m_nativeIcons.GetHeight(); ++y) {
                auto* pixels = static_cast<RGBQUAD*>(m_nativeIcons.GetPixelAddress(0,y));
                for (int x=0; x<m_nativeIcons.GetWidth(); ++x) {
                    auto& pixel = pixels[x];
                    pixel.rgbRed = BYTE((int(pixel.rgbRed)*pixel.rgbReserved+127)/255);
                    pixel.rgbGreen = BYTE((int(pixel.rgbGreen)*pixel.rgbReserved+127)/255);
                    pixel.rgbBlue = BYTE((int(pixel.rgbBlue)*pixel.rgbReserved+127)/255);
                }
            }
        }
    }
    if (!m_nativeIcons.IsNull()) {
        // The mixer has a dark background, so use the native dark-theme row.
        const int row = IsWindowEnabled() ? 2 : 3;
        m_nativeIcons.AlphaBlend(item->hDC,
            bounds.left+(bounds.Width()-size)/2, bounds.top+(bounds.Height()-size)/2,
            size, size, (24+(m_muted ? 1 : 0))*size, row*size, size, size);
    }
    if (item->itemState & ODS_FOCUS) dc.DrawFocusRect(bounds);
    dc.Detach();
}

namespace {
const wchar_t* const IconLabels[] = {L"Aucune ic\u00f4ne",L"Micro",L"Manette",L"Personne",L"Groupe",L"\u00c9cran",L"Musique",L"Haut-parleur",L"Casque",L"Cam\u00e9ra",L"Navigateur",L"T\u00e9l\u00e9phone"};
bool LoadTrackIcon(int index, int size, CImage& image) {
    image.Destroy();
    if (FAILED(SVGImage::Load(UINT(7200+index),image,float(size)/24))) return false;
    image.SetHasAlphaChannel(true);
    for (int y=0; y<image.GetHeight(); ++y) {
        auto* pixels=static_cast<RGBQUAD*>(image.GetPixelAddress(0,y));
        for (int x=0; x<image.GetWidth(); ++x) {
            auto& c=pixels[x];
            c.rgbRed=BYTE((int(c.rgbRed)*c.rgbReserved+127)/255);
            c.rgbGreen=BYTE((int(c.rgbGreen)*c.rgbReserved+127)/255);
            c.rgbBlue=BYTE((int(c.rgbBlue)*c.rgbReserved+127)/255);
        }
    }
    return true;
}
}
void CAudioTrackIcon::DrawItem(LPDRAWITEMSTRUCT item) {
    CDC dc; dc.Attach(item->hDC);
    CRect bounds(item->rcItem); dc.FillSolidRect(bounds,RGB(32,34,38));
    const int size=std::min(bounds.Width(),bounds.Height());
    if (size>0 && size!=m_size) m_size=LoadTrackIcon(std::max(0,m_index),size,m_image)?size:0;
    if (!m_image.IsNull()) m_image.AlphaBlend(item->hDC,bounds.left+(bounds.Width()-size)/2,bounds.top+(bounds.Height()-size)/2,size,size,0,0,size,size);
    if (item->itemState & ODS_FOCUS) dc.DrawFocusRect(bounds);
    dc.Detach();
}

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
    if (!m_rows.empty()) {
        auto& row=*m_rows[m_first];
        CWnd* target=row.volume.IsWindowEnabled()?static_cast<CWnd*>(&row.volume):&row.meter;
        target->SetFocus();
    }
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
    const int width = std::min(Rect(0,0,342,1).Width()+2, work.Width()-8);
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
    if (m_iconMenuOpen) return;
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
    m_iconTips.Create(this, TTS_ALWAYSTIP);
    m_iconTips.Activate(TRUE);
    const DWORD child = WS_CHILD | WS_VISIBLE;
    {
        std::lock_guard<std::mutex> lock(m_session->mutex);
        for (size_t i = 0; i < m_session->tracks.size(); ++i) {
            auto r = std::make_unique<Row>();
            r->name.Create(child | WS_TABSTOP | ES_AUTOHSCROLL,Rect(0,0,1,1),this,UINT(54000+i));
            r->name.LimitText(64);
            m_iconTips.AddTool(&r->name,L"Cliquer pour renommer. Entr\u00e9e : valider ; \u00c9chap : annuler. Vide : nom d'origine.");
            r->meter.Create(L"Niveau audio", child | SS_OWNERDRAW, Rect(0,0,1,1), this, UINT(5101+i*4));
            r->mute.Create(L"Activer/couper le son de la piste", child | WS_TABSTOP | BS_OWNERDRAW, Rect(0,0,1,1), this, UINT(5102+i*4));
            r->icon.Create(L"Choisir une ic\u00f4ne de piste",child | WS_TABSTOP | BS_OWNERDRAW,Rect(0,0,1,1),this,UINT(5103+i*4));
            m_iconTips.AddTool(&r->icon,L"Choisir une ic\u00f4ne de piste");
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
    const HWND focused=::GetFocus();
    for (int i=0; i<int(m_rows.size()); ++i) {
        if (i>=m_first && i<m_first+m_visibleRows) continue;
        auto& row=*m_rows[i];
        if (focused==row.name.GetSafeHwnd() || focused==row.icon.GetSafeHwnd() ||
            focused==row.mute.GetSafeHwnd() || focused==row.volume.GetSafeHwnd() ||
            focused==row.meter.GetSafeHwnd()) {
            // Hiding a focused control makes Windows select the next edit's entire name.
            auto& visible=*m_rows[m_first];
            CWnd* target=visible.volume.IsWindowEnabled()?static_cast<CWnd*>(&visible.volume):&visible.meter;
            target->ShowWindow(SW_SHOW);
            target->SetFocus();
            break;
        }
    }
    CRect client; GetClientRect(client);
    const int pad = Rect(0,0,8,1).Width();
    const int labelWidth = std::min(Rect(0,0,92,1).Width(),client.Width()/3);
    const int meterWidth = Rect(0,0,50,1).Width();
    const int sliderLeft = pad+labelWidth+meterWidth+pad;
    for (int i=0; i<int(m_rows.size()); ++i) {
        auto& r = *m_rows[i];
        const bool visible = i>=m_first && i<m_first+m_visibleRows;
        for (CWnd* w : std::initializer_list<CWnd*>{&r.name,&r.icon,&r.mute,&r.meter,&r.volume}) w->ShowWindow(visible ? SW_SHOW : SW_HIDE);
        if (!visible) continue;
        const int y = 6+(i-m_first)*22;
        r.name.MoveWindow(pad,Rect(0,y,1,1).top,labelWidth-pad-Rect(0,0,40,1).Width(),Rect(0,0,1,12).Height());
        r.icon.MoveWindow(pad+labelWidth-Rect(0,0,40,1).Width(),Rect(0,y-1,1,1).top,Rect(0,0,16,1).Width(),Rect(0,0,1,14).Height());
        r.mute.MoveWindow(pad+labelWidth-Rect(0,0,20,1).Width(),Rect(0,y-1,1,1).top,Rect(0,0,16,1).Width(),Rect(0,0,1,14).Height());
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
        if (GetFocus()!=&r.name) {
            CString shown((t.customName.empty()?t.name:t.customName).c_str()), current;
            r.name.GetWindowText(current);
            if (shown!=current) {
                r.updatingName=true;
                r.name.SetWindowText(shown);
                r.updatingName=false;
            }
        }
        m_iconTips.UpdateTipText(label+L" - cliquer pour renommer",&r.name);
        const int iconIndex=AudioMix::IconIndex(t.icon);
        if (r.iconIndex!=iconIndex) {
            r.iconIndex=iconIndex;
            r.icon.SetIconIndex(iconIndex);
            CString tip; tip.Format(L"Ic\u00f4ne de la piste %u : %s \u2014 cliquer pour choisir",unsigned(i+1),IconLabels[iconIndex]);
            r.icon.SetWindowText(tip);
            m_iconTips.UpdateTipText(tip,&r.icon);
        }
        r.icon.EnableWindow(usable && m_session->enabled);
        r.mute.SetMuted(t.mute);
        r.mute.SetWindowText(t.mute ? L"R\u00e9tablir le son de la piste" : L"Couper le son de la piste");
        r.mute.EnableWindow(usable && m_session->enabled && t.error.empty());
        r.volume.SetPos(int(std::lround(t.volume * 100)));
        r.volume.EnableWindow(usable && m_session->enabled && t.error.empty());
        r.meter.SetLevel(usable && m_session->enabled && t.error.empty() ? levels[i] : 0.0f);
    }
}
BOOL CAudioMixerDlg::PreTranslateMessage(MSG* message) {
    if (m_iconTips.GetSafeHwnd()) m_iconTips.RelayEvent(message);
    if (message->message==WM_KEYDOWN && (message->wParam==VK_RETURN || message->wParam==VK_ESCAPE)) {
        for (size_t i=0; i<m_rows.size(); ++i) {
            auto& row=*m_rows[i];
            if (message->hwnd!=row.name.GetSafeHwnd()) continue;
            if (message->wParam==VK_ESCAPE) {
                { std::lock_guard<std::mutex> lock(m_session->mutex); m_session->tracks[i].customName=row.nameBeforeEdit; }
                AudioMix::SavePreferences(m_session);
            }
            row.volume.SetFocus();
            Refresh();
            return TRUE;
        }
    }
    return CDialog::PreTranslateMessage(message);
}
BOOL CAudioMixerDlg::OnCommand(WPARAM wParam, LPARAM lParam) {
    for (size_t i=0; i<m_rows.size(); ++i) {
        auto& row=*m_rows[i];
        if (!lParam || reinterpret_cast<HWND>(lParam)!=row.name.GetSafeHwnd()) continue;
        if (row.updatingName) return TRUE;
        const UINT notification=HIWORD(wParam);
        if (notification==EN_SETFOCUS) {
            std::lock_guard<std::mutex> lock(m_session->mutex);
            row.nameBeforeEdit=m_session->tracks[i].customName;
        } else if (notification==EN_CHANGE) {
            CString text; row.name.GetWindowText(text);
            {
                std::lock_guard<std::mutex> lock(m_session->mutex);
                m_session->tracks[i].customName=AudioMix::NormalizeTrackName(text.GetString());
            }
            AudioMix::SavePreferences(m_session);
        } else if (notification==EN_KILLFOCUS) Refresh();
        return TRUE;
    }
    const UINT command=LOWORD(wParam);
    if (!lParam && command>=53000 && command<53012 && m_iconMenuRow<m_rows.size()) {
        {
            std::lock_guard<std::mutex> lock(m_session->mutex);
            if (m_busy || !m_session->active || !m_session->enabled) return TRUE;
            m_session->tracks[m_iconMenuRow].icon=AudioMix::IconIds[command-53000];
        }
        AudioMix::SavePreferences(m_session);
        Refresh();
        return TRUE;
    }
    if (HIWORD(wParam) == BN_CLICKED) {
        for (size_t i=0; i<m_rows.size(); ++i) {
            if (reinterpret_cast<HWND>(lParam)!=m_rows[i]->icon.GetSafeHwnd()) continue;
            CMenu menu; menu.CreatePopupMenu();
            std::vector<std::unique_ptr<CBitmap>> bitmaps;
            CRect bounds; m_rows[i]->icon.GetWindowRect(bounds);
            const int size=std::max(16,bounds.Height());
            for (int k=0; k<12; ++k) {
                menu.AppendMenu(MF_STRING | (m_rows[i]->iconIndex==k?MF_CHECKED:0),UINT(1+k),IconLabels[k]);
                CImage image;
                if (LoadTrackIcon(k,size,image)) {
                    auto bitmap=std::make_unique<CBitmap>(); bitmap->Attach(image.Detach());
                    menu.SetMenuItemBitmaps(UINT(1+k),MF_BYCOMMAND,bitmap.get(),bitmap.get());
                    bitmaps.push_back(std::move(bitmap));
                }
            }
            m_iconMenuRow=i;
            m_iconMenuOpen=true;
            const UINT choice=menu.TrackPopupMenu(TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN,bounds.left,bounds.bottom,this);
            m_iconMenuOpen=false;
            if (choice>=1 && choice<=12) {
                OnCommand(MAKEWPARAM(53000+choice-1,0),0);
            }
            return TRUE;
        }
        for (size_t i=0; i<m_rows.size(); ++i) {
            if (reinterpret_cast<HWND>(lParam) != m_rows[i]->mute.GetSafeHwnd()) continue;
            {
                std::lock_guard<std::mutex> lock(m_session->mutex);
                auto& track = m_session->tracks[i];
                if (!m_busy && m_session->active && m_session->enabled && track.error.empty()) track.mute = !track.mute;
            }
            AudioMix::SavePreferences(m_session);
            Refresh();
            return TRUE;
        }
    }
    return CDialog::OnCommand(wParam, lParam);
}
void CAudioMixerDlg::OnHScroll(UINT code, UINT pos, CScrollBar* bar) {
    if (bar) {
        {
        std::lock_guard<std::mutex> lock(m_session->mutex);
        for (size_t i = 0; i < m_rows.size(); ++i) if (bar->GetSafeHwnd() == m_rows[i]->volume.GetSafeHwnd())
            m_session->tracks[i].volume = m_rows[i]->volume.GetPos() / 100.0f;
        }
        AudioMix::SavePreferences(m_session);
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
    CVolumeCtrl* target = nullptr;
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
