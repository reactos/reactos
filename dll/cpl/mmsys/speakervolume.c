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
    WCHAR szBuffer[256];
    MIXERLINEW mxln;
    MIXERCONTROLW mxc;
    MIXERLINECONTROLSW mxlctrl;
    MIXERCONTROLDETAILS mxcd;
    INT i, j;

    /* Open the mixer */
    if (mixerOpen(&pPageData->hMixer, 0, PtrToUlong(hwndDlg), 0, MIXER_OBJECTF_MIXER | CALLBACK_WINDOW) != MMSYSERR_NOERROR)
    {
        MessageBoxW(hwndDlg, L"Cannot open mixer", NULL, MB_OK);
        return FALSE;
    }

    /* Retrieve the mixer information */
    mxln.cbStruct = sizeof(MIXERLINEW);
    mxln.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

    if (mixerGetLineInfoW((HMIXEROBJ)pPageData->hMixer, &mxln, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE) != MMSYSERR_NOERROR)
        return FALSE;

    pPageData->volumeChannels = mxln.cChannels;

    /* Retrieve the line information */
    mxlctrl.cbStruct = sizeof(MIXERLINECONTROLSW);
    mxlctrl.dwLineID = mxln.dwLineID;
    mxlctrl.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
    mxlctrl.cControls = 1;
    mxlctrl.cbmxctrl = sizeof(MIXERCONTROLW);
    mxlctrl.pamxctrl = &mxc;

    if (mixerGetLineControlsW((HMIXEROBJ)pPageData->hMixer, &mxlctrl, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE) != MMSYSERR_NOERROR)
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

    if (mixerGetControlDetailsW((HMIXEROBJ)pPageData->hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE) != MMSYSERR_NOERROR)
        return FALSE;

    /* Initialize the channels */
    for (i = 0; i < min(mxln.cChannels, 5); i++)
    {
        j = i * 4;

        /* Set the channel name */
        LoadStringW(hApplet, IDS_SPEAKER_LEFT + i, szBuffer, _countof(szBuffer));
        SetWindowTextW(GetDlgItem(hwndDlg, 9472 + j), szBuffer);

        /* Initialize the channel trackbar */
        SendDlgItemMessageW(hwndDlg, 9475 + j, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(VOLUME_MIN, VOLUME_MAX));
        SendDlgItemMessageW(hwndDlg, 9475 + j, TBM_SETTICFREQ, VOLUME_TICFREQ, 0);
        SendDlgItemMessageW(hwndDlg, 9475 + j, TBM_SETPAGESIZE, 0, VOLUME_PAGESIZE);
        SendDlgItemMessageW(hwndDlg, 9475 + j, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(pPageData->volumeValues[i].dwValue - pPageData->volumeMinimum) / pPageData->volumeStep);
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

    if (mixerGetControlDetailsW((HMIXEROBJ)pPageData->hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE) != MMSYSERR_NOERROR)
        return;

    for (i = 0; i < pPageData->volumeChannels; i++)
    {
        j = i * 4;

        SendDlgItemMessageW(hwndDlg, 9475 + j, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(pPageData->volumeValues[i].dwValue - pPageData->volumeMinimum) / pPageData->volumeStep);
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

    id = (INT)GetWindowLongPtrW((HWND)lParam, GWLP_ID);
    if (id < 9475 || id > 9503)
        return;

    if ((id - 9475) % 4 != 0)
        return;

    dwPosition = (DWORD)SendDlgItemMessageW(hwndDlg, id, TBM_GETPOS, 0, 0);

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
                SendDlgItemMessageW(hwndDlg, j, TBM_SETPOS, (WPARAM)TRUE, dwPosition);

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

    pPageData = (PPAGE_DATA)GetWindowLongPtrW(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pPageData = (PPAGE_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PAGE_DATA));
            SetWindowLongPtrW(hwndDlg, DWLP_USER, (LONG_PTR)pPageData);

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
                SetWindowLongPtrW(hwndDlg, DWLP_USER, (LONG_PTR)NULL);
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
                        pPageData->volumeSync = (SendDlgItemMessageW(hwndDlg, 9504, BM_GETCHECK, 0, 0) == BST_CHECKED);
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
    PROPSHEETPAGEW psp[1];
    PROPSHEETHEADERW psh;

    ZeroMemory(&psh, sizeof(PROPSHEETHEADERW));
    psh.dwSize = sizeof(PROPSHEETHEADERW);
    psh.dwFlags =  PSH_PROPSHEETPAGE;
    psh.hwndParent = hwndDlg;
    psh.hInstance = hApplet;
    psh.pszCaption = MAKEINTRESOURCE(IDS_SPEAKER_VOLUME);
    psh.nPages = _countof(psp);
    psh.nStartPage = 0;
    psh.ppsp = psp;

    InitPropSheetPage(&psp[0], IDD_MULTICHANNEL, SpeakerVolumeDlgProc);
    psp[0].dwFlags |= PSP_USETITLE;
    psp[0].hInstance = hApplet;
    psp[0].pszTitle = MAKEINTRESOURCE(IDS_SPEAKER_VOLUME);

    return (LONG)(PropertySheetW(&psh) != -1);
}
