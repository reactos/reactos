/*
 * PROJECT:         ReactOS Multimedia Control Panel
 * FILE:            dll/cpl/mmsys/speakervolume.c
 * PURPOSE:         ReactOS Multimedia Control Panel
 * PROGRAMMER:      Eric Kohl <eric.kohl@reactos.com>
 */

#include "mmsys.h"

typedef struct _PAGE_DATA
{
    HMIXER hMixer;
    DWORD volumeControlID;
    DWORD volumeChannels;
    DWORD volumeMinimum;
    DWORD volumeMaximum;
    DWORD volumeStep;
    PMIXERCONTROLDETAILS_UNSIGNED volumeValues;
    BOOL volumeSync;
} PAGE_DATA, *PPAGE_DATA;


static
BOOL
OnInitDialog(
    PPAGE_DATA pPageData,
    HWND hwndDlg)
{
    TCHAR szBuffer[256];
    MIXERLINE mxln;
    MIXERCONTROL mxc;
    MIXERLINECONTROLS mxlctrl;
    MIXERCONTROLDETAILS mxcd;
    INT i, j;

    /* Open the mixer */
    if (mixerOpen(&pPageData->hMixer, 0, PtrToUlong(hwndDlg), 0, MIXER_OBJECTF_MIXER | CALLBACK_WINDOW) != MMSYSERR_NOERROR)
    {
        MessageBox(hwndDlg, _T("Cannot open mixer"), NULL, MB_OK);
        return FALSE;
    }

    /* Retrieve the mixer information */
    mxln.cbStruct = sizeof(MIXERLINE);
    mxln.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

    if (mixerGetLineInfo((HMIXEROBJ)pPageData->hMixer, &mxln, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE) != MMSYSERR_NOERROR)
        return FALSE;

    pPageData->volumeChannels = mxln.cChannels;

    /* Retrieve the line information */
    mxlctrl.cbStruct = sizeof(MIXERLINECONTROLS);
    mxlctrl.dwLineID = mxln.dwLineID;
    mxlctrl.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
    mxlctrl.cControls = 1;
    mxlctrl.cbmxctrl = sizeof(MIXERCONTROL);
    mxlctrl.pamxctrl = &mxc;

    if (mixerGetLineControls((HMIXEROBJ)pPageData->hMixer, &mxlctrl, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE) != MMSYSERR_NOERROR)
        return FALSE;

    pPageData->volumeControlID = mxc.dwControlID;
    pPageData->volumeMinimum = mxc.Bounds.dwMinimum;
    pPageData->volumeMaximum = mxc.Bounds.dwMaximum;
    pPageData->volumeStep = (pPageData->volumeMaximum - pPageData->volumeMinimum) / (VOLUME_MAX - VOLUME_MIN);

    /* Allocate a buffer for all channel volume values */
    pPageData->volumeValues = HeapAlloc(GetProcessHeap(),
                                        0,
                                        mxln.cChannels * sizeof(MIXERCONTROLDETAILS_UNSIGNED));
    if (pPageData->volumeValues == NULL)
        return FALSE;

    /* Retrieve the channel volume values */
    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
    mxcd.dwControlID = mxc.dwControlID;
    mxcd.cChannels = mxln.cChannels;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    mxcd.paDetails = pPageData->volumeValues;

    if (mixerGetControlDetails((HMIXEROBJ)pPageData->hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE) != MMSYSERR_NOERROR)
        return FALSE;

    /* Initialize the channels */
    for (i = 0; i < min(mxln.cChannels, 5); i++)
    {
        j = i * 4;

        /* Set the channel name */
        LoadString(hApplet, IDS_SPEAKER_LEFT + i, szBuffer, _countof(szBuffer));
        SetWindowText(GetDlgItem(hwndDlg, 9472 + j), szBuffer);

        /* Initialize the channel trackbar */
        SendDlgItemMessage(hwndDlg, 9475 + j, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(VOLUME_MIN, VOLUME_MAX));
        SendDlgItemMessage(hwndDlg, 9475 + j, TBM_SETTICFREQ, VOLUME_TICFREQ, 0);
        SendDlgItemMessage(hwndDlg, 9475 + j, TBM_SETPAGESIZE, 0, VOLUME_PAGESIZE);
        SendDlgItemMessage(hwndDlg, 9475 + j, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(pPageData->volumeValues[i].dwValue - pPageData->volumeMinimum) / pPageData->volumeStep);
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


static
VOID
OnMixerControlChange(
    PPAGE_DATA pPageData,
    HWND hwndDlg)
{
    MIXERCONTROLDETAILS mxcd;
    INT i, j;

    /* Retrieve the channel volume values */
    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
    mxcd.dwControlID = pPageData->volumeControlID;
    mxcd.cChannels = pPageData->volumeChannels;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    mxcd.paDetails = pPageData->volumeValues;

    if (mixerGetControlDetails((HMIXEROBJ)pPageData->hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE) != MMSYSERR_NOERROR)
        return;

    for (i = 0; i < pPageData->volumeChannels; i++)
    {
        j = i * 4;

        SendDlgItemMessage(hwndDlg, 9475 + j, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(pPageData->volumeValues[i].dwValue - pPageData->volumeMinimum) / pPageData->volumeStep);
    }
}


static
VOID
OnHScroll(
    PPAGE_DATA pPageData,
    HWND hwndDlg,
    WPARAM wParam,
    LPARAM lParam)
{
    MIXERCONTROLDETAILS mxcd;
    DWORD dwValue, dwPosition;
    INT id, idx, i, j;

    id = (INT)GetWindowLongPtr((HWND)lParam, GWLP_ID);
    if (id < 9475 && id > 9503)
        return;

    if ((id - 9475) % 4 != 0)
        return;

    dwPosition = (DWORD)SendDlgItemMessage(hwndDlg, id, TBM_GETPOS, 0, 0);

    if (dwPosition == VOLUME_MIN)
        dwValue = pPageData->volumeMinimum;
    else if (dwPosition == VOLUME_MAX)
        dwValue = pPageData->volumeMaximum;
    else
        dwValue = (dwPosition * pPageData->volumeStep) + pPageData->volumeMinimum;

    if (pPageData->volumeSync)
    {
        for (i = 0; i < pPageData->volumeChannels; i++)
        {
            j = 9475 + (i * 4);
            if (j != id)
                SendDlgItemMessage(hwndDlg, j, TBM_SETPOS, (WPARAM)TRUE, dwPosition);

            pPageData->volumeValues[i].dwValue = dwValue;
        }
    }
    else
    {
        idx = (id - 9475) / 4;
        pPageData->volumeValues[idx].dwValue = dwValue;
    }

    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
    mxcd.dwControlID = pPageData->volumeControlID;
    mxcd.cChannels = pPageData->volumeChannels;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    mxcd.paDetails = pPageData->volumeValues;

    if (mixerSetControlDetails((HMIXEROBJ)pPageData->hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE) != MMSYSERR_NOERROR)
        return;
}


static
VOID
OnSetDefaults(
    PPAGE_DATA pPageData,
    HWND hwndDlg)
{
    MIXERCONTROLDETAILS mxcd;
    DWORD dwValue, i;

    dwValue = ((VOLUME_MAX - VOLUME_MIN) / 2 * pPageData->volumeStep) + pPageData->volumeMinimum;

    for (i = 0; i < pPageData->volumeChannels; i++)
        pPageData->volumeValues[i].dwValue = dwValue;

    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
    mxcd.dwControlID = pPageData->volumeControlID;
    mxcd.cChannels = pPageData->volumeChannels;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    mxcd.paDetails = pPageData->volumeValues;

    if (mixerSetControlDetails((HMIXEROBJ)pPageData->hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE) != MMSYSERR_NOERROR)
        return;
}


INT_PTR
CALLBACK
SpeakerVolumeDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PPAGE_DATA pPageData;

    UNREFERENCED_PARAMETER(wParam);

    pPageData = (PPAGE_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch(uMsg)
    {
        case WM_INITDIALOG:
            pPageData = (PPAGE_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PAGE_DATA));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pPageData);

            if (pPageData)
            {
                OnInitDialog(pPageData, hwndDlg);
            }
            break;

        case WM_DESTROY:
            if (pPageData)
            {
                if (pPageData->volumeValues)
                    HeapFree(GetProcessHeap(), 0, pPageData->volumeValues);

                if (pPageData->hMixer)
                    mixerClose(pPageData->hMixer);

                HeapFree(GetProcessHeap(), 0, pPageData);
                pPageData = NULL;
                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)NULL);
            }
            break;

        case WM_HSCROLL:
            if (pPageData)
            {
                OnHScroll(pPageData, hwndDlg, wParam, lParam);
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case 9504:
                    if (HIWORD(wParam) == BN_CLICKED)
                        pPageData->volumeSync = (SendDlgItemMessage(hwndDlg, 9504, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    break;

                case 9505:
                    OnSetDefaults(pPageData, hwndDlg);
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
            }
            break;

        case WM_NOTIFY:
            return TRUE;

        case MM_MIXM_CONTROL_CHANGE:
            if (pPageData)
                OnMixerControlChange(pPageData, hwndDlg);
            break;
    }

    return FALSE;
}


INT_PTR
SpeakerVolume(
    HWND hwndDlg)
{
    PROPSHEETPAGE psp[1];
    PROPSHEETHEADER psh;
    TCHAR Caption[256];

    LoadString(hApplet, IDS_SPEAKER_VOLUME, Caption, _countof(Caption));

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE;
    psh.hwndParent = hwndDlg;
    psh.hInstance = hApplet;
    psh.pszCaption = Caption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = psp;

    InitPropSheetPage(&psp[0], IDD_MULTICHANNEL, SpeakerVolumeDlgProc);
    psp[0].dwFlags |= PSP_USETITLE;
    psp[0].pszTitle = Caption;

    return (LONG)(PropertySheet(&psh) != -1);
}
