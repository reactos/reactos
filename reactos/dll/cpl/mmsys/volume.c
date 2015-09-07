/*
 * PROJECT:         ReactOS Multimedia Control Panel
 * FILE:            dll/cpl/mmsys/mmsys.c
 * PURPOSE:         ReactOS Multimedia Control Panel
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *                  Johannes Anderwald <janderwald@reactos.com>
 *                  Dmitry Chapyshev <dmitry@reactos.org>
 */

#include "mmsys.h"

#include <shellapi.h>

#define VOLUME_DIVIDER 0xFFF

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
    DWORD volumeMinimum;
    DWORD volumeMaximum;
    DWORD volumeValue;

} GLOBAL_DATA, *PGLOBAL_DATA;


static VOID
InitImageInfo(PIMGINFO ImgInfo)
{
    BITMAP bitmap;

    ZeroMemory(ImgInfo, sizeof(*ImgInfo));

    ImgInfo->hBitmap = LoadImage(hApplet,
                                 MAKEINTRESOURCE(IDB_SPEAKIMG),
                                 IMAGE_BITMAP,
                                 0,
                                 0,
                                 LR_DEFAULTCOLOR);

    if (ImgInfo->hBitmap != NULL)
    {
        GetObject(ImgInfo->hBitmap, sizeof(BITMAP), &bitmap);

        ImgInfo->cxSource = bitmap.bmWidth;
        ImgInfo->cySource = bitmap.bmHeight;
    }
}


VOID
GetMuteControl(PGLOBAL_DATA pGlobalData)
{
    MIXERLINE mxln;
    MIXERCONTROL mxc;
    MIXERLINECONTROLS mxlctrl;

    if (pGlobalData->hMixer == NULL)
        return;

    mxln.cbStruct = sizeof(MIXERLINE);
    mxln.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

    if (mixerGetLineInfo((HMIXEROBJ)pGlobalData->hMixer, &mxln, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE)
        != MMSYSERR_NOERROR) return;

    mxlctrl.cbStruct = sizeof(MIXERLINECONTROLS);
    mxlctrl.dwLineID = mxln.dwLineID;
    mxlctrl.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
    mxlctrl.cControls = 1;
    mxlctrl.cbmxctrl = sizeof(MIXERCONTROL);
    mxlctrl.pamxctrl = &mxc;

    if (mixerGetLineControls((HMIXEROBJ)pGlobalData->hMixer, &mxlctrl, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE)
        != MMSYSERR_NOERROR) return;

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

    if (mixerGetControlDetails((HMIXEROBJ)pGlobalData->hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE)
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
    MIXERLINE mxln;
    MIXERCONTROL mxc;
    MIXERLINECONTROLS mxlc;

    if (pGlobalData->hMixer == NULL)
        return;

    mxln.cbStruct = sizeof(MIXERLINE);
    mxln.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
    if (mixerGetLineInfo((HMIXEROBJ)pGlobalData->hMixer, &mxln, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE)
        != MMSYSERR_NOERROR)
        return;

    mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
    mxlc.dwLineID = mxln.dwLineID;
    mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
    mxlc.cControls = 1;
    mxlc.cbmxctrl = sizeof(MIXERCONTROL);
    mxlc.pamxctrl = &mxc;
    if (mixerGetLineControls((HMIXEROBJ)pGlobalData->hMixer, &mxlc, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE)
        != MMSYSERR_NOERROR)
        return;

    pGlobalData->volumeMinimum = mxc.Bounds.dwMinimum;
    pGlobalData->volumeMaximum = mxc.Bounds.dwMaximum;
    pGlobalData->volumeControlID = mxc.dwControlID;
}


VOID
GetVolumeValue(PGLOBAL_DATA pGlobalData)
{
    MIXERCONTROLDETAILS_UNSIGNED mxcdVolume;
    MIXERCONTROLDETAILS mxcd;

    if (pGlobalData->hMixer == NULL)
        return;

    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
    mxcd.dwControlID = pGlobalData->volumeControlID;
    mxcd.cChannels = 1;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    mxcd.paDetails = &mxcdVolume;

    if (mixerGetControlDetails((HMIXEROBJ)pGlobalData->hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE)
        != MMSYSERR_NOERROR)
        return;

    pGlobalData->volumeValue = mxcdVolume.dwValue;
}


VOID
SetVolumeValue(PGLOBAL_DATA pGlobalData){
    MIXERCONTROLDETAILS_UNSIGNED mxcdVolume;
    MIXERCONTROLDETAILS mxcd;

    if (pGlobalData->hMixer == NULL)
        return;

    mxcdVolume.dwValue = pGlobalData->volumeValue;
    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
    mxcd.dwControlID = pGlobalData->volumeControlID;
    mxcd.cChannels = 1;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    mxcd.paDetails = &mxcdVolume;

    if (mixerSetControlDetails((HMIXEROBJ)pGlobalData->hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE)
        != MMSYSERR_NOERROR)
        return;

    pGlobalData->volumeValue = mxcdVolume.dwValue;
}


VOID
InitVolumeControls(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    UINT NumMixers;
    MIXERCAPS mxc;
    TCHAR szNoDevices[256];

    LoadString(hApplet, IDS_NO_DEVICES, szNoDevices, _countof(szNoDevices));

    NumMixers = mixerGetNumDevs();
    if (!NumMixers)
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_VOLUME_TRACKBAR), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_MUTE_CHECKBOX),   FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ICON_IN_TASKBAR), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ADVANCED_BTN),    FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SPEAKER_VOL_BTN), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_ADVANCED2_BTN),   FALSE);
        SendDlgItemMessage(hwndDlg, IDC_MUTE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pGlobalData->hIconNoHW);
        SetDlgItemText(hwndDlg, IDC_DEVICE_NAME, szNoDevices);
        return;
    }

    if (mixerOpen(&pGlobalData->hMixer, 0, PtrToUlong(hwndDlg), 0, MIXER_OBJECTF_MIXER | CALLBACK_WINDOW) != MMSYSERR_NOERROR)
    {
        MessageBox(hwndDlg, _T("Cannot open mixer"), NULL, MB_OK);
        return;
    }

    ZeroMemory(&mxc, sizeof(MIXERCAPS));
    if (mixerGetDevCaps(PtrToUint(pGlobalData->hMixer), &mxc, sizeof(MIXERCAPS)) != MMSYSERR_NOERROR)
    {
        MessageBox(hwndDlg, _T("mixerGetDevCaps failed"), NULL, MB_OK);
        return;
    }

    GetMuteControl(pGlobalData);
    GetMuteState(pGlobalData);
    if (pGlobalData->muteVal)
    {
        SendDlgItemMessage(hwndDlg, IDC_MUTE_CHECKBOX, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
        SendDlgItemMessage(hwndDlg, IDC_MUTE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pGlobalData->hIconMuted);
    }
    else
    {
        SendDlgItemMessage(hwndDlg, IDC_MUTE_CHECKBOX, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
        SendDlgItemMessage(hwndDlg, IDC_MUTE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pGlobalData->hIconUnMuted);
    }

    GetVolumeControl(pGlobalData);
    GetVolumeValue(pGlobalData);

    SendDlgItemMessage(hwndDlg, IDC_DEVICE_NAME, WM_SETTEXT, 0, (LPARAM)mxc.szPname);
    SendDlgItemMessage(hwndDlg, IDC_VOLUME_TRACKBAR, TBM_SETRANGE, (WPARAM)TRUE,
        (LPARAM)MAKELONG(pGlobalData->volumeMinimum, pGlobalData->volumeMaximum/VOLUME_DIVIDER));
    SendDlgItemMessage(hwndDlg, IDC_VOLUME_TRACKBAR, TBM_SETPAGESIZE, (WPARAM)FALSE, (LPARAM)1);
    SendDlgItemMessage(hwndDlg, IDC_VOLUME_TRACKBAR, TBM_SETSEL, (WPARAM)FALSE,
        (LPARAM)MAKELONG(pGlobalData->volumeMinimum, pGlobalData->volumeValue/VOLUME_DIVIDER));
    SendDlgItemMessage(hwndDlg, IDC_VOLUME_TRACKBAR, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)pGlobalData->volumeValue/VOLUME_DIVIDER);
}


VOID
LaunchSoundControl(HWND hwndDlg)
{
    if ((INT_PTR)ShellExecuteW(NULL, L"open", L"sndvol32.exe", NULL, NULL, SW_SHOWNORMAL) > 32)
        return;
    MessageBox(hwndDlg, _T("Cannot run sndvol32.exe"), NULL, MB_OK);
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
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);



    switch(uMsg)
    {
        case MM_MIXM_LINE_CHANGE:
        {
            GetMuteState(pGlobalData);
            if (pGlobalData->muteVal)
            {
                SendDlgItemMessage(hwndDlg, IDC_MUTE_CHECKBOX, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
                SendDlgItemMessage(hwndDlg, IDC_MUTE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pGlobalData->hIconMuted);
            }
            else
            {
                SendDlgItemMessage(hwndDlg, IDC_MUTE_CHECKBOX, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
                SendDlgItemMessage(hwndDlg, IDC_MUTE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pGlobalData->hIconUnMuted);
            }
            break;
        }
        case MM_MIXM_CONTROL_CHANGE:
        {
            GetVolumeValue(pGlobalData);
            SendDlgItemMessage(hwndDlg, IDC_VOLUME_TRACKBAR, TBM_SETSEL, (WPARAM)FALSE, (LPARAM)MAKELONG(pGlobalData->volumeMinimum, pGlobalData->volumeValue/VOLUME_DIVIDER));
            SendDlgItemMessage(hwndDlg, IDC_VOLUME_TRACKBAR, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)pGlobalData->volumeValue/VOLUME_DIVIDER);
            break;
        }
        case WM_INITDIALOG:
        {
            pGlobalData = (GLOBAL_DATA*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLOBAL_DATA));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

            pGlobalData->hIconUnMuted = LoadImage(hApplet, MAKEINTRESOURCE(IDI_CPLICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
            pGlobalData->hIconMuted = LoadImage(hApplet, MAKEINTRESOURCE(IDI_MUTED_ICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
            pGlobalData->hIconNoHW = LoadImage(hApplet, MAKEINTRESOURCE(IDI_NO_HW), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);

            InitImageInfo(&ImgInfo);
            InitVolumeControls(hwndDlg, pGlobalData);
            break;
        }

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpDrawItem;
            lpDrawItem = (LPDRAWITEMSTRUCT) lParam;
            if(lpDrawItem->CtlID == IDC_SPEAKIMG)
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
                    SwitchMuteState(pGlobalData);
                    if (pGlobalData->muteVal)
                        {
                            SendDlgItemMessage(hwndDlg, IDC_MUTE_CHECKBOX, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
                            SendDlgItemMessage(hwndDlg, IDC_MUTE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pGlobalData->hIconMuted);
                        }
                        else
                        {
                            SendDlgItemMessage(hwndDlg, IDC_MUTE_CHECKBOX, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
                            SendDlgItemMessage(hwndDlg, IDC_MUTE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pGlobalData->hIconUnMuted);
                        }
                    break;
                case IDC_ADVANCED_BTN:
                    LaunchSoundControl(hwndDlg);
                    break;
            }
            break;
        }

        case WM_HSCROLL:
        {
            HWND hVolumeTrackbar = GetDlgItem(hwndDlg, IDC_VOLUME_TRACKBAR);
            if (hVolumeTrackbar == (HWND)lParam)
            {
                pGlobalData->volumeValue = (DWORD)SendDlgItemMessage(hwndDlg, IDC_VOLUME_TRACKBAR, TBM_GETPOS, 0, 0)*VOLUME_DIVIDER;
                SetVolumeValue(pGlobalData);
                SendDlgItemMessage(hwndDlg, IDC_VOLUME_TRACKBAR, TBM_SETSEL, (WPARAM)TRUE,
                    (LPARAM)MAKELONG(pGlobalData->volumeMinimum, pGlobalData->volumeValue/VOLUME_DIVIDER));
            }
            break;
        }

        case WM_DESTROY:
            mixerClose(pGlobalData->hMixer);
            DestroyIcon(pGlobalData->hIconMuted);
            DestroyIcon(pGlobalData->hIconUnMuted);
            DestroyIcon(pGlobalData->hIconNoHW);
            HeapFree(GetProcessHeap(), 0, pGlobalData);
            break;
    }

    return FALSE;
}
