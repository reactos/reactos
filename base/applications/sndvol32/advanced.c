/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Sound Volume Control
 * FILE:        base/applications/sndvol32/advanced.c
 * PROGRAMMERS: Eric Kohl <eric.kohl@reactos.org>
 */

#include "sndvol32.h"

static
VOID
OnInitDialog(
    HWND hwndDlg,
    PADVANCED_CONTEXT Context)
{
    WCHAR szRawTitle[256], szCookedTitle[256];
    MIXERCONTROLDETAILS_UNSIGNED UnsignedDetails;
    LPMIXERCONTROL Control = NULL;
    UINT ControlCount = 0, Index;
    DWORD i, dwStep, dwPosition;

    /* Set the dialog title */
    LoadStringW(hAppInstance, IDS_ADVANCED_CONTROLS, szRawTitle, ARRAYSIZE(szRawTitle));
    StringCchPrintfW(szCookedTitle, ARRAYSIZE(szCookedTitle), szRawTitle, Context->LineName);
    SetWindowTextW(hwndDlg, szCookedTitle);

    /* Disable the tone controls */
    for (i = IDC_ADV_BASS_LOW; i<= IDC_ADV_TREBLE_SLIDER; i++)
        EnableWindow(GetDlgItem(hwndDlg, i), FALSE);

    /* Initialize the bass and treble trackbars */
    SendDlgItemMessageW(hwndDlg, IDC_ADV_BASS_SLIDER, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(VOLUME_MIN, VOLUME_MAX));
    SendDlgItemMessageW(hwndDlg, IDC_ADV_TREBLE_SLIDER, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(VOLUME_MIN, VOLUME_MAX));
    SendDlgItemMessageW(hwndDlg, IDC_ADV_BASS_SLIDER, TBM_SETPAGESIZE, 0, (LPARAM)VOLUME_PAGE_SIZE);
    SendDlgItemMessageW(hwndDlg, IDC_ADV_TREBLE_SLIDER, TBM_SETPAGESIZE, 0, (LPARAM)VOLUME_PAGE_SIZE);
    SendDlgItemMessageW(hwndDlg, IDC_ADV_BASS_SLIDER, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)0);
    SendDlgItemMessageW(hwndDlg, IDC_ADV_TREBLE_SLIDER, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)0);

    /* Calculate and set ticks */
    dwStep = (VOLUME_MAX / (VOLUME_TICKS + 1));
    if (VOLUME_MAX % (VOLUME_TICKS + 1) != 0)
        dwStep++;
    for (i = dwStep; i < VOLUME_MAX; i += dwStep)
    {
        SendDlgItemMessageW(hwndDlg, IDC_ADV_BASS_SLIDER, TBM_SETTIC, 0, (LPARAM)i);
        SendDlgItemMessageW(hwndDlg, IDC_ADV_TREBLE_SLIDER, TBM_SETTIC, 0, (LPARAM)i);
    }

    /* Hide the other controls */
    for (i = IDC_ADV_OTHER_CONTROLS; i<= IDC_ADV_OTHER_CHECK2; i++)
        ShowWindow(GetDlgItem(hwndDlg, i), SW_HIDE);

    if (SndMixerQueryControls(Context->Mixer, &ControlCount, Context->Line, &Control))
    {
        for (Index = 0; Index < ControlCount; Index++)
        {
            if (Control[Index].dwControlType == MIXERCONTROL_CONTROLTYPE_BASS)
            {
                if (SndMixerGetVolumeControlDetails(Context->Mixer, Control[Index].dwControlID, 1, sizeof(MIXERCONTROLDETAILS_UNSIGNED), (LPVOID)&UnsignedDetails) != -1)
                {
                    for (i = IDC_ADV_BASS_LOW; i<= IDC_ADV_BASS_SLIDER; i++)
                        EnableWindow(GetDlgItem(hwndDlg, i), TRUE);

                    dwStep = (Control[Index].Bounds.dwMaximum - Control[Index].Bounds.dwMinimum) / (VOLUME_MAX - VOLUME_MIN);
                    dwPosition = (UnsignedDetails.dwValue - Control[Index].Bounds.dwMinimum) / dwStep;
                    SendDlgItemMessageW(hwndDlg, IDC_ADV_BASS_SLIDER, TBM_SETPOS, (WPARAM)TRUE, dwPosition);
                }
            }
            else if (Control[Index].dwControlType == MIXERCONTROL_CONTROLTYPE_TREBLE)
            {
                if (SndMixerGetVolumeControlDetails(Context->Mixer, Control[Index].dwControlID, 1, sizeof(MIXERCONTROLDETAILS_UNSIGNED), (LPVOID)&UnsignedDetails) != -1)
                {
                    for (i = IDC_ADV_TREBLE_LOW; i<= IDC_ADV_TREBLE_SLIDER; i++)
                        EnableWindow(GetDlgItem(hwndDlg, i), TRUE);

                    dwStep = (Control[Index].Bounds.dwMaximum - Control[Index].Bounds.dwMinimum) / (VOLUME_MAX - VOLUME_MIN);
                    dwPosition = (UnsignedDetails.dwValue - Control[Index].Bounds.dwMinimum) / dwStep;
                    SendDlgItemMessageW(hwndDlg, IDC_ADV_TREBLE_SLIDER, TBM_SETPOS, (WPARAM)TRUE, dwPosition);
                }
            }
            else if (Control[Index].dwControlType != MIXERCONTROL_CONTROLTYPE_VOLUME &&
                     Control[Index].dwControlType != MIXERCONTROL_CONTROLTYPE_MUTE)
            {
                ShowWindow(GetDlgItem(hwndDlg, IDC_ADV_OTHER_CONTROLS), SW_SHOWNORMAL);
                ShowWindow(GetDlgItem(hwndDlg, IDC_ADV_OTHER_TEXT), SW_SHOWNORMAL);

            }
        }

        /* free controls */
        HeapFree(GetProcessHeap(), 0, Control);
    }
}


INT_PTR
CALLBACK
AdvancedDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PADVANCED_CONTEXT Context;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)lParam);
            Context = (PADVANCED_CONTEXT)((LONG_PTR)lParam);
            OnInitDialog(hwndDlg, Context);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwndDlg, IDOK);
                    break;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwndDlg, IDCANCEL);
            break;
    }

    return FALSE;
}

/* EOF */
