/*
 * PROJECT:         ReactOS Multimedia Control Panel
 * FILE:            dll/cpl/mmsys/speakervolume.c
 * PURPOSE:         ReactOS Multimedia Control Panel
 * PROGRAMMER:      Eric Kohl <eric.kohl@reactos.com>
 */

#include "mmsys.h"

static
BOOL
OnInitDialog(
    HWND hwndDlg,
    LPARAM lParam)
{
    TCHAR szBuffer[256];
    MIXERLINE mxln;
    MIXERCONTROL mxc;
    MIXERLINECONTROLS mxlctrl;
    MIXERCONTROLDETAILS mxcd;
    MIXERCONTROLDETAILS_UNSIGNED mxcdVolume[8];
    INT i, j;
    DWORD dwStep;
    HMIXER hMixer;

    hMixer = (HMIXER)((LPPROPSHEETPAGE)lParam)->lParam;

    mxln.cbStruct = sizeof(MIXERLINE);
    mxln.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

    if (mixerGetLineInfo((HMIXEROBJ)hMixer, &mxln, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE) != MMSYSERR_NOERROR)
        return FALSE;

    mxlctrl.cbStruct = sizeof(MIXERLINECONTROLS);
    mxlctrl.dwLineID = mxln.dwLineID;
    mxlctrl.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
    mxlctrl.cControls = 1;
    mxlctrl.cbmxctrl = sizeof(MIXERCONTROL);
    mxlctrl.pamxctrl = &mxc;

    if (mixerGetLineControls((HMIXEROBJ)hMixer, &mxlctrl, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE) != MMSYSERR_NOERROR)
        return FALSE;

//    pGlobalData->volumeMinimum = mxc.Bounds.dwMinimum;
//    pGlobalData->volumeMaximum = mxc.Bounds.dwMaximum;
//    pGlobalData->volumeControlID = mxc.dwControlID;
//    pGlobalData->volumeStep = (pGlobalData->volumeMaximum - pGlobalData->volumeMinimum) / (VOLUME_MAX - VOLUME_MIN);
    dwStep = (mxc.Bounds.dwMaximum - mxc.Bounds.dwMinimum) / (VOLUME_MAX - VOLUME_MIN);

    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
    mxcd.dwControlID = mxc.dwControlID;
    mxcd.cChannels = mxln.cChannels;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    mxcd.paDetails = &mxcdVolume;

    if (mixerGetControlDetails((HMIXEROBJ)hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE) != MMSYSERR_NOERROR)
        return FALSE;

//    pGlobalData->volumeValue[i] = mxcdVolume[i].dwValue;

    /* Initialize the channels */
    for (i = 0; i < min(mxln.cChannels, 5); i++)
    {
        j = i * 4;
        LoadString(hApplet, 5792 + i, szBuffer, _countof(szBuffer));
        SetWindowText(GetDlgItem(hwndDlg, 9472 + j), szBuffer);

        SendDlgItemMessage(hwndDlg, 9475 + j, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(VOLUME_MIN, VOLUME_MAX));
        SendDlgItemMessage(hwndDlg, 9475 + j, TBM_SETTICFREQ, VOLUME_TICFREQ, 0);
        SendDlgItemMessage(hwndDlg, 9475 + j, TBM_SETPAGESIZE, 0, VOLUME_PAGESIZE);
//        SendDlgItemMessage(hwndDlg, 9475 + j, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(pGlobalData->volumeValue - pGlobalData->volumeMinimum) / pGlobalData->volumeStep);
        SendDlgItemMessage(hwndDlg, 9475 + j, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(mxcdVolume[i].dwValue - mxc.Bounds.dwMinimum) / dwStep);
    }

    /* Hide the unused controls */
    for (i = mxln.cChannels; i < 8; i++)
    {
        j = i * 4;
        ShowWindow(GetDlgItem(hwndDlg, 9472 + j), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, 9473 + j), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, 9474 + j), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, 9475 + j), SW_HIDE);
    }

    return TRUE;
}


INT_PTR
CALLBACK
SpeakerVolumeDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    switch(uMsg)
    {
        case WM_INITDIALOG:
            OnInitDialog(hwndDlg, lParam);
            break;

        case WM_DESTROY:
            break;

        case WM_NOTIFY:
            return TRUE;
    }

    return FALSE;
}


INT_PTR
SpeakerVolume(
    HWND hwnd,
    HMIXER hMixer)
{
    PROPSHEETPAGE psp[1];
    PROPSHEETHEADER psh;
    TCHAR Caption[256];

    LoadString(hApplet, IDS_SPEAKER_VOLUME, Caption, _countof(Caption));

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE;
    psh.hwndParent = hwnd;
    psh.hInstance = hApplet;
    psh.pszCaption = Caption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = psp;

    InitPropSheetPage(&psp[0], IDD_MULTICHANNEL, SpeakerVolumeDlgProc, (LPARAM)hMixer);

    return (LONG)(PropertySheet(&psh) != -1);
}
