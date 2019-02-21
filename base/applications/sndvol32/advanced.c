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
    LPMIXERCONTROL Control = NULL;
    UINT ControlCount = 0, Index, i;

    /* Set the dialog title */
    LoadStringW(hAppInstance, IDS_ADVANCED_CONTROLS, szRawTitle, ARRAYSIZE(szRawTitle));
//    swprintf(szCookedTitle, szRawTitle, Context->LineName);
    StringCchPrintfW(szCookedTitle, ARRAYSIZE(szCookedTitle), szRawTitle, Context->LineName);
    SetWindowTextW(hwndDlg, szCookedTitle);

    /* Disable the tone controls */
    for (i = IDC_ADV_BASS_LOW; i<= IDC_ADV_TREBLE_SLIDER; i++)
        EnableWindow(GetDlgItem(hwndDlg, i), FALSE);

    /* Hide the other controls */
    for (i = IDC_ADV_OTHER_CONTROLS; i<= IDC_ADV_OTHER_CHECK2; i++)
        ShowWindow(GetDlgItem(hwndDlg, i), SW_HIDE);

    if (SndMixerQueryControls(Context->Mixer, &ControlCount, Context->Line, &Control))
    {
        for (Index = 0; Index < ControlCount; Index++)
        {
            if (Control[Index].dwControlType == MIXERCONTROL_CONTROLTYPE_BASS)
            {
                for (i = IDC_ADV_BASS_LOW; i<= IDC_ADV_BASS_SLIDER; i++)
                    EnableWindow(GetDlgItem(hwndDlg, i), TRUE);

            }
            else if (Control[Index].dwControlType == MIXERCONTROL_CONTROLTYPE_TREBLE)
            {
                for (i = IDC_ADV_TREBLE_LOW; i<= IDC_ADV_TREBLE_SLIDER; i++)
                    EnableWindow(GetDlgItem(hwndDlg, i), TRUE);

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
