/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Sound Volume Control
 * FILE:        base/applications/sndvol32/tray.c
 * PROGRAMMERS: Eric Kohl <eric.kohl@reactos.org>
 */

#include "sndvol32.h"

typedef struct _DIALOG_DATA
{
    HMIXER hMixer;
    DWORD volumeControlID;
    DWORD volumeChannels;
    DWORD volumeMinimum;
    DWORD volumeMaximum;
    DWORD volumeStep;

    DWORD maxVolume;
    DWORD maxChannel;
    PMIXERCONTROLDETAILS_UNSIGNED volumeInitValues;
    PMIXERCONTROLDETAILS_UNSIGNED volumeCurrentValues;

    DWORD muteControlID;
} DIALOG_DATA, *PDIALOG_DATA;


static VOID
OnTrayInitDialog(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam)
{
    POINT ptCursor;
    RECT rcWindow;
    RECT rcScreen;
    LONG x, y, cx, cy;

    GetCursorPos(&ptCursor);

    GetWindowRect(hwnd, &rcWindow);

    GetWindowRect(GetDesktopWindow(), &rcScreen);

    cx = rcWindow.right - rcWindow.left;
    cy = rcWindow.bottom - rcWindow.top;

    if (ptCursor.y + cy > rcScreen.bottom)
        y = ptCursor.y - cy;
    else
        y = ptCursor.y;

    if (ptCursor.x + cx > rcScreen.right)
        x = ptCursor.x - cx;
    else
        x = ptCursor.x;

    SetWindowPos(hwnd, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE);
}


static
VOID
OnTrayInitMixer(
    PDIALOG_DATA pDialogData,
    HWND hwndDlg)
{
    MIXERLINE mxln;
    MIXERCONTROL mxc;
    MIXERLINECONTROLS mxlctrl;
    MIXERCONTROLDETAILS mxcd;
    MIXERCONTROLDETAILS_BOOLEAN mxcdBool;
    DWORD i;

    /* Open the mixer */
    if (mixerOpen(&pDialogData->hMixer, 0, PtrToUlong(hwndDlg), 0, MIXER_OBJECTF_MIXER | CALLBACK_WINDOW) != MMSYSERR_NOERROR)
        return;

    /* Retrieve the mixer information */
    mxln.cbStruct = sizeof(MIXERLINE);
    mxln.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

    if (mixerGetLineInfo((HMIXEROBJ)pDialogData->hMixer, &mxln, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE) != MMSYSERR_NOERROR)
        return;

    pDialogData->volumeChannels = mxln.cChannels;

    /* Retrieve the line information */
    mxlctrl.cbStruct = sizeof(MIXERLINECONTROLS);
    mxlctrl.dwLineID = mxln.dwLineID;
    mxlctrl.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
    mxlctrl.cControls = 1;
    mxlctrl.cbmxctrl = sizeof(MIXERCONTROL);
    mxlctrl.pamxctrl = &mxc;

    if (mixerGetLineControls((HMIXEROBJ)pDialogData->hMixer, &mxlctrl, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE) != MMSYSERR_NOERROR)
        return;

    pDialogData->volumeControlID = mxc.dwControlID;
    pDialogData->volumeMinimum = mxc.Bounds.dwMinimum;
    pDialogData->volumeMaximum = mxc.Bounds.dwMaximum;
    pDialogData->volumeStep = (pDialogData->volumeMaximum - pDialogData->volumeMinimum) / (VOLUME_MAX - VOLUME_MIN);

    /* Allocate a buffer for all channel volume values */
    pDialogData->volumeInitValues = HeapAlloc(GetProcessHeap(),
                                              0,
                                              mxln.cChannels * sizeof(MIXERCONTROLDETAILS_UNSIGNED));
    if (pDialogData->volumeInitValues == NULL)
        return;

    pDialogData->volumeCurrentValues = HeapAlloc(GetProcessHeap(),
                                                 0,
                                                 mxln.cChannels * sizeof(MIXERCONTROLDETAILS_UNSIGNED));
    if (pDialogData->volumeCurrentValues == NULL)
        return;

    /* Retrieve the channel volume values */
    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
    mxcd.dwControlID = mxc.dwControlID;
    mxcd.cChannels = mxln.cChannels;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    mxcd.paDetails = pDialogData->volumeInitValues;

    if (mixerGetControlDetails((HMIXEROBJ)pDialogData->hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE) != MMSYSERR_NOERROR)
        return;

    pDialogData->maxVolume = pDialogData->volumeInitValues[0].dwValue;
    pDialogData->maxChannel = 0;
    for (i = 1; i < pDialogData->volumeChannels; i++)
    {
        if (pDialogData->volumeInitValues[i].dwValue > pDialogData->maxVolume)
        {
            pDialogData->maxVolume = pDialogData->volumeInitValues[i].dwValue;
            pDialogData->maxChannel = i;
        }
    }

    /* Initialize the volume trackbar */
    SendDlgItemMessage(hwndDlg, IDC_LINE_SLIDER_VERT, TBM_SETRANGE, TRUE, MAKELONG(VOLUME_MIN, VOLUME_MAX));
    SendDlgItemMessage(hwndDlg, IDC_LINE_SLIDER_VERT, TBM_SETPAGESIZE, 0, VOLUME_PAGE_SIZE);
    SendDlgItemMessage(hwndDlg, IDC_LINE_SLIDER_VERT, TBM_SETPOS, TRUE, VOLUME_MAX -(pDialogData->maxVolume - pDialogData->volumeMinimum) / pDialogData->volumeStep);

    /* Retrieve the mute control information */
    mxlctrl.cbStruct = sizeof(MIXERLINECONTROLS);
    mxlctrl.dwLineID = mxln.dwLineID;
    mxlctrl.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
    mxlctrl.cControls = 1;
    mxlctrl.cbmxctrl = sizeof(MIXERCONTROL);
    mxlctrl.pamxctrl = &mxc;

    if (mixerGetLineControls((HMIXEROBJ)pDialogData->hMixer, &mxlctrl, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE) != MMSYSERR_NOERROR)
        return;

    pDialogData->muteControlID = mxc.dwControlID;

    /* Retrieve the mute value */
    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
    mxcd.dwControlID = mxc.dwControlID;
    mxcd.cChannels = 1;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    mxcd.paDetails = &mxcdBool;

    if (mixerGetControlDetails((HMIXEROBJ)pDialogData->hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE) != MMSYSERR_NOERROR)
        return;

    /* Initialize the mute checkbox */
    SendDlgItemMessage(hwndDlg, IDC_LINE_SWITCH, BM_SETCHECK, (WPARAM)(mxcdBool.fValue ? BST_CHECKED : BST_UNCHECKED), 0);
}


static
VOID
OnCommand(
    PDIALOG_DATA pDialogData,
    HWND hwndDlg,
    WPARAM wParam,
    LPARAM lParam)
{
    MIXERCONTROLDETAILS mxcd;
    MIXERCONTROLDETAILS_BOOLEAN mxcdMute;

    if ((LOWORD(wParam) == IDC_LINE_SWITCH) &&
        (HIWORD(wParam) == BN_CLICKED))
    {
        mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
        mxcd.dwControlID = pDialogData->muteControlID;
        mxcd.cChannels = 1;
        mxcd.cMultipleItems = 0;
        mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
        mxcd.paDetails = &mxcdMute;

        mxcdMute.fValue = (SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED);

        mixerSetControlDetails((HMIXEROBJ)pDialogData->hMixer,
                               &mxcd,
                               MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE);
    }
}


static
VOID
OnVScroll(
    PDIALOG_DATA pDialogData,
    HWND hwndDlg,
    WPARAM wParam,
    LPARAM lParam)
{
    MIXERCONTROLDETAILS mxcd;
    DWORD dwPos, dwVolume, i;

    switch (LOWORD(wParam))
    {
        case TB_THUMBTRACK:

            dwPos = VOLUME_MAX - (DWORD)SendDlgItemMessage(hwndDlg, IDC_LINE_SLIDER_VERT, TBM_GETPOS, 0, 0);
            dwVolume = (dwPos * pDialogData->volumeStep) + pDialogData->volumeMinimum;

            for (i = 0; i < pDialogData->volumeChannels; i++)
            {
                if (i == pDialogData->maxChannel)
                {
                    pDialogData->volumeCurrentValues[i].dwValue = dwVolume;
                }
                else
                {
                    pDialogData->volumeCurrentValues[i].dwValue =
                        pDialogData->volumeInitValues[i].dwValue * dwVolume / pDialogData-> maxVolume;
                }
            }

            mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
            mxcd.dwControlID = pDialogData->volumeControlID;
            mxcd.cChannels = pDialogData->volumeChannels;
            mxcd.cMultipleItems = 0;
            mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
            mxcd.paDetails = pDialogData->volumeCurrentValues;

            mixerSetControlDetails((HMIXEROBJ)pDialogData->hMixer,
                                   &mxcd,
                                   MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE);
            break;

        case TB_ENDTRACK:
            PlaySound((LPCTSTR)SND_ALIAS_SYSTEMDEFAULT, NULL, SND_ASYNC | SND_ALIAS_ID);
            break;

        default:
            break;
    }
}



INT_PTR
CALLBACK
TrayDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PDIALOG_DATA pDialogData;

    pDialogData = (PDIALOG_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            OnTrayInitDialog(hwndDlg, wParam, lParam);

            pDialogData = (PDIALOG_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DIALOG_DATA));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pDialogData);

            if (pDialogData)
                OnTrayInitMixer(pDialogData, hwndDlg);
            break;

        case WM_COMMAND:
            if (pDialogData)
                OnCommand(pDialogData, hwndDlg, wParam, lParam);
            break;

        case WM_VSCROLL:
            if (pDialogData)
                OnVScroll(pDialogData, hwndDlg, wParam, lParam);
            break;

        case WM_DESTROY:
            if (pDialogData)
            {
                if (pDialogData->volumeInitValues)
                    HeapFree(GetProcessHeap(), 0, pDialogData->volumeInitValues);

                if (pDialogData->volumeCurrentValues)
                    HeapFree(GetProcessHeap(), 0, pDialogData->volumeCurrentValues);

                if (pDialogData->hMixer)
                    mixerClose(pDialogData->hMixer);

                HeapFree(GetProcessHeap(), 0, pDialogData);
                pDialogData = NULL;
                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)NULL);
            }
            break;

        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE)
                EndDialog(hwndDlg, IDOK);
            break;
    }

    return 0;
}

/* EOF */
