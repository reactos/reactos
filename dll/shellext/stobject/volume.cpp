/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/stobject/volume.cpp
 * PURPOSE:     Volume notification icon handler
 * PROGRAMMERS: David Quintana <gigaherz@gmail.com>
 */

#include "precomp.h"

#include <mmddk.h>

HICON g_hIconVolume;
HICON g_hIconMute;

HMIXER g_hMixer;
UINT   g_mixerId;
DWORD  g_mixerLineID;
DWORD  g_muteControlID;

UINT g_mmDeviceChange;

static BOOL g_IsMute = FALSE;

static HRESULT __stdcall Volume_FindMixerControl(CSysTray * pSysTray)
{
    MMRESULT result;
    UINT mixerId = 0;
    DWORD waveOutId = 0;
    DWORD param2 = 0;

    TRACE("Volume_FindDefaultMixerID\n");

    result = waveOutMessage((HWAVEOUT)UlongToHandle(WAVE_MAPPER), DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR)&waveOutId, (DWORD_PTR)&param2);
    if (result)
        return E_FAIL;

    if (waveOutId == (DWORD)-1)
    {
        TRACE("WARNING: waveOut has no default device, trying with first available device...\n", waveOutId);

        mixerId = 0;
    }
    else
    {
        TRACE("waveOut default device is %d\n", waveOutId);

        result = mixerGetID((HMIXEROBJ)UlongToHandle(waveOutId), &mixerId, MIXER_OBJECTF_WAVEOUT);
        if (result)
            return E_FAIL;

        TRACE("mixerId for waveOut default device is %d\n", mixerId);
    }

    g_mixerId = mixerId;
    return S_OK;

    MIXERCAPS mixerCaps;
    MIXERLINE mixerLine;
    MIXERCONTROL mixerControl;
    MIXERLINECONTROLS mixerLineControls;

    g_mixerLineID = -1;
    g_muteControlID = -1;

    if (mixerGetDevCapsW(g_mixerId, &mixerCaps, sizeof(mixerCaps)))
        return E_FAIL;

    if (mixerCaps.cDestinations == 0)
        return S_FALSE;

    TRACE("mixerCaps.cDestinations %d\n", mixerCaps.cDestinations);

    DWORD idx;
    for (idx = 0; idx < mixerCaps.cDestinations; idx++)
    {
        mixerLine.cbStruct = sizeof(mixerLine);
        mixerLine.dwDestination = idx;
        if (!mixerGetLineInfoW((HMIXEROBJ)UlongToHandle(g_mixerId), &mixerLine, 0))
        {
            if (mixerLine.dwComponentType >= MIXERLINE_COMPONENTTYPE_DST_SPEAKERS &&
                mixerLine.dwComponentType <= MIXERLINE_COMPONENTTYPE_DST_HEADPHONES)
                break;
            TRACE("Destination %d was not speakers or headphones.\n");
        }
    }

    if (idx >= mixerCaps.cDestinations)
        return E_FAIL;

    TRACE("Valid destination %d found.\n");

    g_mixerLineID = mixerLine.dwLineID;

    mixerLineControls.cbStruct = sizeof(mixerLineControls);
    mixerLineControls.dwLineID = mixerLine.dwLineID;
    mixerLineControls.cControls = 1;
    mixerLineControls.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
    mixerLineControls.pamxctrl = &mixerControl;
    mixerLineControls.cbmxctrl = sizeof(mixerControl);

    if (mixerGetLineControlsW((HMIXEROBJ)UlongToHandle(g_mixerId), &mixerLineControls, MIXER_GETLINECONTROLSF_ONEBYTYPE))
        return E_FAIL;

    TRACE("Found control id %d for mute: %d\n", mixerControl.dwControlID);

    g_muteControlID = mixerControl.dwControlID;

    return S_OK;
}

HRESULT Volume_IsMute()
{
#if 0
    MIXERCONTROLDETAILS mixerControlDetails;

    if (g_mixerId != (UINT)-1 && g_muteControlID != (DWORD)-1)
    {
        BOOL detailsResult = 0;
        mixerControlDetails.cbStruct = sizeof(mixerControlDetails);
        mixerControlDetails.hwndOwner = 0;
        mixerControlDetails.dwControlID = g_muteControlID;
        mixerControlDetails.cChannels = 1;
        mixerControlDetails.paDetails = &detailsResult;
        mixerControlDetails.cbDetails = sizeof(detailsResult);
        if (mixerGetControlDetailsW((HMIXEROBJ) g_mixerId, &mixerControlDetails, 0))
            return E_FAIL;

        TRACE("Obtained mute status %d\n", detailsResult);

        g_IsMute = detailsResult != 0;
    }
#endif
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Volume_Init(_In_ CSysTray * pSysTray)
{
    HRESULT hr;
    WCHAR strTooltip[128];

    TRACE("Volume_Init\n");

    if (!g_hMixer)
    {
        hr = Volume_FindMixerControl(pSysTray);
        if (FAILED(hr))
            return hr;

        g_mmDeviceChange = RegisterWindowMessageW(L"winmm_devicechange");
    }

    g_hIconVolume = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_VOLUME));
    g_hIconMute = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_VOLMUTE));

    Volume_IsMute();

    HICON icon;
    if (g_IsMute)
        icon = g_hIconMute;
    else
        icon = g_hIconVolume;

    LoadStringW(g_hInstance, IDS_VOL_VOLUME, strTooltip, _countof(strTooltip));
    return pSysTray->NotifyIcon(NIM_ADD, ID_ICON_VOLUME, icon, strTooltip);
}

HRESULT STDMETHODCALLTYPE Volume_Update(_In_ CSysTray * pSysTray)
{
    BOOL PrevState;

    TRACE("Volume_Update\n");

    PrevState = g_IsMute;
    Volume_IsMute();

    if (PrevState != g_IsMute)
    {
        WCHAR strTooltip[128];
        HICON icon;
        if (g_IsMute)
        {
            icon = g_hIconMute;
            LoadStringW(g_hInstance, IDS_VOL_MUTED, strTooltip, _countof(strTooltip));
        }
        else
        {
            icon = g_hIconVolume;
            LoadStringW(g_hInstance, IDS_VOL_VOLUME, strTooltip, _countof(strTooltip));
        }

        return pSysTray->NotifyIcon(NIM_MODIFY, ID_ICON_VOLUME, icon, strTooltip);
    }
    else
    {
        return S_OK;
    }
}

HRESULT STDMETHODCALLTYPE Volume_Shutdown(_In_ CSysTray * pSysTray)
{
    TRACE("Volume_Shutdown\n");

    return pSysTray->NotifyIcon(NIM_DELETE, ID_ICON_VOLUME, NULL, NULL);
}

HRESULT Volume_OnDeviceChange(_In_ CSysTray * pSysTray, WPARAM wParam, LPARAM lParam)
{
    return Volume_FindMixerControl(pSysTray);
}

static void _RunVolume(BOOL bTray)
{
    ShellExecuteW(NULL, NULL, bTray ? L"sndvol32.exe /t" : L"sndvol32.exe", NULL, NULL, SW_SHOWNORMAL);
}

static void _RunMMCpl()
{
    ShellExecuteW(NULL, NULL, L"mmsys.cpl", NULL, NULL, SW_NORMAL);
}

static void _ShowContextMenu(CSysTray * pSysTray)
{
    WCHAR strAdjust[128];
    WCHAR strOpen[128];
    LoadStringW(g_hInstance, IDS_VOL_OPEN, strOpen, _countof(strOpen));
    LoadStringW(g_hInstance, IDS_VOL_ADJUST, strAdjust, _countof(strAdjust));

    HMENU hPopup = CreatePopupMenu();
    AppendMenuW(hPopup, MF_STRING, IDS_VOL_OPEN, strOpen);
    AppendMenuW(hPopup, MF_STRING, IDS_VOL_ADJUST, strAdjust);
    SetMenuDefaultItem(hPopup, IDS_VOL_OPEN, FALSE);

    DWORD flags = TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTALIGN | TPM_BOTTOMALIGN;
    POINT pt;
    SetForegroundWindow(pSysTray->GetHWnd());
    GetCursorPos(&pt);

    DWORD id = TrackPopupMenuEx(hPopup, flags,
        pt.x, pt.y,
        pSysTray->GetHWnd(), NULL);

    DestroyMenu(hPopup);

    switch (id)
    {
    case IDS_VOL_OPEN:
        _RunVolume(FALSE);
        break;
    case IDS_VOL_ADJUST:
        _RunMMCpl();
        break;
    }
}

HRESULT STDMETHODCALLTYPE Volume_Message(_In_ CSysTray * pSysTray, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult)
{
    if (uMsg == g_mmDeviceChange)
        return Volume_OnDeviceChange(pSysTray, wParam, lParam);

    switch (uMsg)
    {
        case WM_USER + 220:
            TRACE("Volume_Message: WM_USER+220\n");
            if (wParam == VOLUME_SERVICE_FLAG)
            {
                if (lParam)
                {
                    pSysTray->EnableService(VOLUME_SERVICE_FLAG, TRUE);
                    return Volume_Init(pSysTray);
                }
                else
                {
                    pSysTray->EnableService(VOLUME_SERVICE_FLAG, FALSE);
                    return Volume_Shutdown(pSysTray);
                }
            }
            return S_FALSE;

        case WM_USER + 221:
            TRACE("Volume_Message: WM_USER+221\n");
            if (wParam == VOLUME_SERVICE_FLAG)
            {
                lResult = (LRESULT)pSysTray->IsServiceEnabled(VOLUME_SERVICE_FLAG);
                return S_OK;
            }
            return S_FALSE;

        case WM_TIMER:
            if (wParam == VOLUME_TIMER_ID)
            {
                KillTimer(pSysTray->GetHWnd(), VOLUME_TIMER_ID);
                _RunVolume(TRUE);
            }
            break;

        case ID_ICON_VOLUME:
            TRACE("Volume_Message uMsg=%d, w=%x, l=%x\n", uMsg, wParam, lParam);

            Volume_Update(pSysTray);

            switch (lParam)
            {
                case WM_LBUTTONDOWN:
                    SetTimer(pSysTray->GetHWnd(), VOLUME_TIMER_ID, GetDoubleClickTime(), NULL);
                    break;

                case WM_LBUTTONUP:
                    break;

                case WM_LBUTTONDBLCLK:
                    KillTimer(pSysTray->GetHWnd(), VOLUME_TIMER_ID);
                    _RunVolume(FALSE);
                    break;

                case WM_RBUTTONDOWN:
                    break;

                case WM_RBUTTONUP:
                    _ShowContextMenu(pSysTray);
                    break;

                case WM_RBUTTONDBLCLK:
                    break;

                case WM_MOUSEMOVE:
                    break;
            }
            return S_OK;

        default:
            TRACE("Volume_Message received for unknown ID %d, ignoring.\n");
            return S_FALSE;
    }

    return S_FALSE;
}
