/*
 * PROJECT:         ReactOS Multimedia Control Panel
 * FILE:            dll/cpl/mmsys/volume.c
 * PURPOSE:         ReactOS Multimedia Control Panel
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *                  Johannes Anderwald <janderwald@reactos.com>
 *                  Dmitry Chapyshev <dmitry@reactos.org>
 */

#include "mmsys.h"

#include <shellapi.h>


typedef struct _IMGINFO
{
    HBITMAP hBitmap;
    INT cxSource;
    INT cySource;
} IMGINFO, *PIMGINFO;


typedef struct _GLOBAL_DATA
{
    HMIXER hMixer;
    HICON hIconMuted;
    HICON hIconUnMuted;
    HICON hIconNoHW;

    LONG muteVal;
    DWORD muteControlID;

    DWORD volumeControlID;
    DWORD volumeChannels;
    DWORD volumeMinimum;
    DWORD volumeMaximum;
    DWORD volumeStep;

    DWORD maxVolume;
    PMIXERCONTROLDETAILS_UNSIGNED volumeInitialValues;
    PMIXERCONTROLDETAILS_UNSIGNED volumePreviousValues;
    PMIXERCONTROLDETAILS_UNSIGNED volumeCurrentValues;

} GLOBAL_DATA, *PGLOBAL_DATA;


static VOID
InitImageInfo(PIMGINFO ImgInfo)
{
    BITMAP bitmap;

    ZeroMemory(ImgInfo, sizeof(*ImgInfo));

    ImgInfo->hBitmap = LoadImageW(hApplet,
                                  MAKEINTRESOURCEW(IDB_SPEAKIMG),
                                  IMAGE_BITMAP,
                                  0,
                                  0,
                                  LR_DEFAULTCOLOR);

    if (ImgInfo->hBitmap != NULL)
    {
        GetObjectW(ImgInfo->hBitmap, sizeof(BITMAP), &bitmap);

        ImgInfo->cxSource = bitmap.bmWidth;
        ImgInfo->cySource = bitmap.bmHeight;
    }
}


VOID
GetMuteControl(PGLOBAL_DATA pGlobalData)
{
    MIXERLINEW mxln;
    MIXERCONTROLW mxc;
    MIXERLINECONTROLSW mxlctrl;

    if (pGlobalData->hMixer == NULL)
        return;

    mxln.cbStruct = sizeof(MIXERLINEW);
    mxln.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

    if (mixerGetLineInfoW((HMIXEROBJ)pGlobalData->hMixer, &mxln, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE)
        != MMSYSERR_NOERROR)
        return;

    mxlctrl.cbStruct = sizeof(MIXERLINECONTROLSW);
    mxlctrl.dwLineID = mxln.dwLineID;
    mxlctrl.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
    mxlctrl.cControls = 1;
    mxlctrl.cbmxctrl = sizeof(MIXERCONTROLW);
    mxlctrl.pamxctrl = &mxc;

    if (mixerGetLineControlsW((HMIXEROBJ)pGlobalData->hMixer, &mxlctrl, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE)
        != MMSYSERR_NOERROR)
        return;

    pGlobalData->muteControlID = mxc.dwControlID;
}


VOID
GetMuteState(PGLOBAL_DATA pGlobalData)
{
    MIXERCONTROLDETAILS_BOOLEAN mxcdMute;
    MIXERCONTROLDETAILS mxcd;

    if (pGlobalData->hMixer == NULL)
        return;

    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
    mxcd.dwControlID = pGlobalData->muteControlID;
    mxcd.cChannels = 1;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    mxcd.paDetails = &mxcdMute;

    if (mixerGetControlDetailsW((HMIXEROBJ)pGlobalData->hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE)
        != MMSYSERR_NOERROR)
        return;

    pGlobalData->muteVal = mxcdMute.fValue;
}


VOID
SwitchMuteState(PGLOBAL_DATA pGlobalData)
{
    MIXERCONTROLDETAILS_BOOLEAN mxcdMute;
    MIXERCONTROLDETAILS mxcd;

    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
    mxcd.dwControlID = pGlobalData->muteControlID;
    mxcd.cChannels = 1;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    mxcd.paDetails = &mxcdMute;

    mxcdMute.fValue = !pGlobalData->muteVal;
    if (mixerSetControlDetails((HMIXEROBJ)pGlobalData->hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE)
        != MMSYSERR_NOERROR)
        return;

    pGlobalData->muteVal = mxcdMute.fValue;
}


VOID
GetVolumeControl(PGLOBAL_DATA pGlobalData)
{
    MIXERLINEW mxln;
    MIXERCONTROLW mxc;
    MIXERLINECONTROLSW mxlc;

    if (pGlobalData->hMixer == NULL)
        return;

    mxln.cbStruct = sizeof(MIXERLINEW);
    mxln.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
    if (mixerGetLineInfoW((HMIXEROBJ)pGlobalData->hMixer, &mxln, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE)
        != MMSYSERR_NOERROR)
        return;

    pGlobalData->volumeChannels = mxln.cChannels;

    mxlc.cbStruct = sizeof(MIXERLINECONTROLSW);
    mxlc.dwLineID = mxln.dwLineID;
    mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
    mxlc.cControls = 1;
    mxlc.cbmxctrl = sizeof(MIXERCONTROLW);
    mxlc.pamxctrl = &mxc;
    if (mixerGetLineControlsW((HMIXEROBJ)pGlobalData->hMixer, &mxlc, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE)
        != MMSYSERR_NOERROR)
        return;

    pGlobalData->volumeMinimum = mxc.Bounds.dwMinimum;
    pGlobalData->volumeMaximum = mxc.Bounds.dwMaximum;
    pGlobalData->volumeControlID = mxc.dwControlID;
    pGlobalData->volumeStep = (pGlobalData->volumeMaximum - pGlobalData->volumeMinimum) / (VOLUME_MAX - VOLUME_MIN);

    pGlobalData->volumeInitialValues = HeapAlloc(GetProcessHeap(),
                                                 0,
                                                 mxln.cChannels * sizeof(MIXERCONTROLDETAILS_UNSIGNED));
    if (pGlobalData->volumeInitialValues == NULL)
        return;

    pGlobalData->volumePreviousValues = HeapAlloc(GetProcessHeap(),
                                                  0,
                                                  mxln.cChannels * sizeof(MIXERCONTROLDETAILS_UNSIGNED));
    if (pGlobalData->volumePreviousValues == NULL)
        return;

    pGlobalData->volumeCurrentValues = HeapAlloc(GetProcessHeap(),
                                                 0,
                                                 mxln.cChannels * sizeof(MIXERCONTROLDETAILS_UNSIGNED));
    if (pGlobalData->volumeCurrentValues == NULL)
        return;
}


VOID
GetVolumeValue(
    PGLOBAL_DATA pGlobalData,
    BOOL bInit)
{
    MIXERCONTROLDETAILS mxcd;
    DWORD i;

    if (pGlobalData->hMixer == NULL)
        return;

    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
    mxcd.dwControlID = pGlobalData->volumeControlID;
    mxcd.cChannels = pGlobalData->volumeChannels;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    mxcd.paDetails = pGlobalData->volumePreviousValues;

    if (mixerGetControlDetailsW((HMIXEROBJ)pGlobalData->hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE)
        != MMSYSERR_NOERROR)
        return;

    pGlobalData->maxVolume = 0;
    for (i = 0; i < pGlobalData->volumeChannels; i++)
    {
        pGlobalData->volumeCurrentValues[i].dwValue = pGlobalData->volumePreviousValues[i].dwValue;

        if (pGlobalData->volumePreviousValues[i].dwValue > pGlobalData->maxVolume)
            pGlobalData->maxVolume = pGlobalData->volumePreviousValues[i].dwValue;

        if (bInit)
            pGlobalData->volumeInitialValues[i].dwValue = pGlobalData->volumeCurrentValues[i].dwValue;
    }
}


VOID
SetVolumeValue(PGLOBAL_DATA pGlobalData,
               DWORD dwPosition)
{
    MIXERCONTROLDETAILS mxcd;
    DWORD dwVolume, i;

    if (pGlobalData->hMixer == NULL)
        return;

    if (dwPosition == VOLUME_MIN)
        dwVolume = pGlobalData->volumeMinimum;
    else if (dwPosition == VOLUME_MAX)
        dwVolume = pGlobalData->volumeMaximum;
    else
        dwVolume = (dwPosition * pGlobalData->volumeStep) + pGlobalData->volumeMinimum;

    for (i = 0; i < pGlobalData->volumeChannels; i++)
    {
        if (pGlobalData->volumePreviousValues[i].dwValue == pGlobalData->maxVolume)
        {
            pGlobalData->volumeCurrentValues[i].dwValue = dwVolume;
        }
        else
        {
            pGlobalData->volumeCurrentValues[i].dwValue =
                pGlobalData->volumePreviousValues[i].dwValue * dwVolume / pGlobalData->maxVolume;
        }
    }

    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
    mxcd.dwControlID = pGlobalData->volumeControlID;
    mxcd.cChannels = pGlobalData->volumeChannels;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    mxcd.paDetails = pGlobalData->volumeCurrentValues;

    if (mixerSetControlDetails((HMIXEROBJ)pGlobalData->hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE)
        != MMSYSERR_NOERROR)
        return;
}


static
VOID
RestoreVolumeValue(
    PGLOBAL_DATA pGlobalData)
{
    MIXERCONTROLDETAILS mxcd;

    if (pGlobalData->hMixer == NULL)
        return;

    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
    mxcd.dwControlID = pGlobalData->volumeControlID;
    mxcd.cChannels = pGlobalData->volumeChannels;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    mxcd.paDetails = pGlobalData->volumeInitialValues;

    mixerSetControlDetails((HMIXEROBJ)pGlobalData->hMixer,
                           &mxcd,
                           MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE);
}


static
VOID
SetSystrayVolumeIconState(BOOL bEnabled)
{
    HWND hwndTaskBar;

    hwndTaskBar = FindWindowW(L"SystemTray_Main", NULL);
    if (hwndTaskBar == NULL)
        return;

    SendMessageW(hwndTaskBar, WM_USER + 220, 4, bEnabled);
}

static
BOOL
GetSystrayVolumeIconState(VOID)
{
    HWND hwndTaskBar;

    hwndTaskBar = FindWindowW(L"SystemTray_Main", NULL);
    if (hwndTaskBar == NULL)
    {
        return FALSE;
    }

    return (BOOL)SendMessageW(hwndTaskBar, WM_USER + 221, 4, 0);
}

VOID
InitVolumeControls(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    UINT NumMixers;
    MIXERCAPSW mxc;
    WCHAR szNoDevices[256];

    LoadStringW(hApplet, IDS_NO_DEVICES, szNoDevices, _countof(szNoDevices));

    SendDlgItemMessageW(hwndDlg, IDC_VOLUME_TRACKBAR, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(VOLUME_MIN, VOLUME_MAX));
    SendDlgItemMessageW(hwndDlg, IDC_VOLUME_TRACKBAR, TBM_SETTICFREQ, VOLUME_TICFREQ, 0);
    SendDlgItemMessageW(hwndDlg, IDC_VOLUME_TRACKBAR, TBM_SETPAGESIZE, 0, VOLUME_PAGESIZE);

    NumMixers = mixerGetNumDevs();
    if (!NumMixers)
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_VOLUME_TRACKBAR), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_VOLUME_LOW),      FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_VOLUME_HIGH),     FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_MUTE_CHECKBOX),   FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ICON_IN_TASKBAR), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ADVANCED_BTN),    FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SPEAKER_VOL_BTN), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ADVANCED2_BTN),   FALSE);
        SendDlgItemMessageW(hwndDlg, IDC_MUTE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pGlobalData->hIconNoHW);
        SetDlgItemTextW(hwndDlg, IDC_DEVICE_NAME, szNoDevices);
        return;
    }

    if (mixerOpen(&pGlobalData->hMixer, 0, PtrToUlong(hwndDlg), 0, MIXER_OBJECTF_MIXER | CALLBACK_WINDOW) != MMSYSERR_NOERROR)
    {
        MessageBoxW(hwndDlg, L"Cannot open mixer", NULL, MB_OK);
        return;
    }

    ZeroMemory(&mxc, sizeof(MIXERCAPSW));
    if (mixerGetDevCapsW(PtrToUint(pGlobalData->hMixer), &mxc, sizeof(MIXERCAPSW)) != MMSYSERR_NOERROR)
    {
        MessageBoxW(hwndDlg, L"mixerGetDevCaps failed", NULL, MB_OK);
        return;
    }

    CheckDlgButton(hwndDlg,
                   IDC_ICON_IN_TASKBAR,
                   GetSystrayVolumeIconState() ? BST_CHECKED : BST_UNCHECKED);

    GetMuteControl(pGlobalData);
    GetMuteState(pGlobalData);
    if (pGlobalData->muteVal)
    {
        SendDlgItemMessageW(hwndDlg, IDC_MUTE_CHECKBOX, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_MUTE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pGlobalData->hIconMuted);
    }
    else
    {
        SendDlgItemMessageW(hwndDlg, IDC_MUTE_CHECKBOX, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_MUTE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pGlobalData->hIconUnMuted);
    }

    GetVolumeControl(pGlobalData);
    GetVolumeValue(pGlobalData, TRUE);

    SendDlgItemMessageW(hwndDlg, IDC_DEVICE_NAME, WM_SETTEXT, 0, (LPARAM)mxc.szPname);
    SendDlgItemMessageW(hwndDlg, IDC_VOLUME_TRACKBAR, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(pGlobalData->maxVolume - pGlobalData->volumeMinimum) / pGlobalData->volumeStep);
}

VOID
SaveData(HWND hwndDlg)
{
    BOOL bShowIcon;

    bShowIcon = (IsDlgButtonChecked(hwndDlg, IDC_ICON_IN_TASKBAR) == BST_CHECKED);

    SetSystrayVolumeIconState(bShowIcon);
}

VOID
LaunchSoundControl(HWND hwndDlg)
{
    if ((INT_PTR)ShellExecuteW(NULL, L"open", L"sndvol32.exe", NULL, NULL, SW_SHOWNORMAL) > 32)
        return;
    MessageBoxW(hwndDlg, L"Cannot run sndvol32.exe", NULL, MB_OK);
}

/* Volume property page dialog callback */
//static INT_PTR CALLBACK
INT_PTR CALLBACK
VolumeDlgProc(HWND hwndDlg,
              UINT uMsg,
              WPARAM wParam,
              LPARAM lParam)
{
    static IMGINFO ImgInfo;
    PGLOBAL_DATA pGlobalData;

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtrW(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case MM_MIXM_LINE_CHANGE:
        {
            GetMuteState(pGlobalData);
            if (pGlobalData->muteVal)
            {
                SendDlgItemMessageW(hwndDlg, IDC_MUTE_CHECKBOX, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
                SendDlgItemMessageW(hwndDlg, IDC_MUTE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pGlobalData->hIconMuted);
            }
            else
            {
                SendDlgItemMessageW(hwndDlg, IDC_MUTE_CHECKBOX, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
                SendDlgItemMessageW(hwndDlg, IDC_MUTE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pGlobalData->hIconUnMuted);
            }
            break;
        }
        case MM_MIXM_CONTROL_CHANGE:
        {
            GetVolumeValue(pGlobalData, FALSE);
            SendDlgItemMessageW(hwndDlg, IDC_VOLUME_TRACKBAR, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(pGlobalData->maxVolume - pGlobalData->volumeMinimum) / pGlobalData->volumeStep);
            break;
        }
        case WM_INITDIALOG:
        {
            pGlobalData = (PGLOBAL_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLOBAL_DATA));
            SetWindowLongPtrW(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

            pGlobalData->hIconUnMuted = LoadImageW(hApplet, MAKEINTRESOURCEW(IDI_CPLICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
            pGlobalData->hIconMuted = LoadImageW(hApplet, MAKEINTRESOURCEW(IDI_MUTED_ICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
            pGlobalData->hIconNoHW = LoadImageW(hApplet, MAKEINTRESOURCEW(IDI_NO_HW), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);

            InitImageInfo(&ImgInfo);
            InitVolumeControls(hwndDlg, pGlobalData);
            break;
        }

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpDrawItem;
            lpDrawItem = (LPDRAWITEMSTRUCT)lParam;
            if (lpDrawItem->CtlID == IDC_SPEAKIMG)
            {
                HDC hdcMem;
                LONG left;

                /* Position image in centre of dialog */
                left = (lpDrawItem->rcItem.right - ImgInfo.cxSource) / 2;

                hdcMem = CreateCompatibleDC(lpDrawItem->hDC);
                if (hdcMem != NULL)
                {
                    SelectObject(hdcMem, ImgInfo.hBitmap);
                    BitBlt(lpDrawItem->hDC,
                           left,
                           lpDrawItem->rcItem.top,
                           lpDrawItem->rcItem.right - lpDrawItem->rcItem.left,
                           lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top,
                           hdcMem,
                           0,
                           0,
                           SRCCOPY);
                    DeleteDC(hdcMem);
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_MUTE_CHECKBOX:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        SwitchMuteState(pGlobalData);
                        if (pGlobalData->muteVal)
                        {
                            SendDlgItemMessageW(hwndDlg, IDC_MUTE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pGlobalData->hIconMuted);
                        }
                        else
                        {
                            SendDlgItemMessageW(hwndDlg, IDC_MUTE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pGlobalData->hIconUnMuted);
                        }

                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_ICON_IN_TASKBAR:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_ADVANCED_BTN:
                    LaunchSoundControl(hwndDlg);
                    break;

                case IDC_SPEAKER_VOL_BTN:
                    SpeakerVolume(hwndDlg);
                    break;
            }
            break;
        }

        case WM_HSCROLL:
        {
            HWND hVolumeTrackbar = GetDlgItem(hwndDlg, IDC_VOLUME_TRACKBAR);
            if (hVolumeTrackbar == (HWND)lParam)
            {
                switch (LOWORD(wParam))
                {
                    case TB_THUMBPOSITION:
                        break;

                    case TB_ENDTRACK:
                        PlaySoundW((LPCWSTR)SND_ALIAS_SYSTEMDEFAULT, NULL, SND_ALIAS_ID | SND_ASYNC);
                        break;

                    default:
                        SetVolumeValue(pGlobalData,
                                       (DWORD)SendDlgItemMessageW(hwndDlg, IDC_VOLUME_TRACKBAR, TBM_GETPOS, 0, 0));
                        break;
                }
            }
            break;
        }

        case WM_DESTROY:
            if (pGlobalData)
            {
                if (pGlobalData->volumeCurrentValues)
                    HeapFree(GetProcessHeap(), 0, pGlobalData->volumeCurrentValues);

                if (pGlobalData->volumePreviousValues)
                    HeapFree(GetProcessHeap(), 0, pGlobalData->volumePreviousValues);

                if (pGlobalData->volumeInitialValues)
                    HeapFree(GetProcessHeap(), 0, pGlobalData->volumeInitialValues);

                mixerClose(pGlobalData->hMixer);
                DestroyIcon(pGlobalData->hIconMuted);
                DestroyIcon(pGlobalData->hIconUnMuted);
                DestroyIcon(pGlobalData->hIconNoHW);
                HeapFree(GetProcessHeap(), 0, pGlobalData);
            }
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case PSN_APPLY:
                    SaveData(hwndDlg);
                    break;

                case PSN_RESET:
                    RestoreVolumeValue(pGlobalData);
                    break;
            }
            return TRUE;
    }

    return FALSE;
}
