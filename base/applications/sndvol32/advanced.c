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
    MIXERCONTROLDETAILS_UNSIGNED UnsignedDetails;
    MIXERCONTROLDETAILS_BOOLEAN BooleanDetails;
    WCHAR szRawBuffer[256], szCookedBuffer[256];
    LPMIXERCONTROL Control = NULL;
    UINT ControlCount = 0, Index;
    DWORD i, dwStep, dwPosition;
    DWORD dwOtherControls = 0;
    RECT rect;
    LONG dy;

    /* Set the dialog title */
    LoadStringW(hAppInstance, IDS_ADVANCED_CONTROLS, szRawBuffer, ARRAYSIZE(szRawBuffer));
    StringCchPrintfW(szCookedBuffer, ARRAYSIZE(szCookedBuffer), szRawBuffer, Context->LineName);
    SetWindowTextW(hwndDlg, szCookedBuffer);

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
                /* Bass control */

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
                /* Treble control */

                if (SndMixerGetVolumeControlDetails(Context->Mixer, Control[Index].dwControlID, 1, sizeof(MIXERCONTROLDETAILS_UNSIGNED), (LPVOID)&UnsignedDetails) != -1)
                {
                    for (i = IDC_ADV_TREBLE_LOW; i<= IDC_ADV_TREBLE_SLIDER; i++)
                        EnableWindow(GetDlgItem(hwndDlg, i), TRUE);

                    dwStep = (Control[Index].Bounds.dwMaximum - Control[Index].Bounds.dwMinimum) / (VOLUME_MAX - VOLUME_MIN);
                    dwPosition = (UnsignedDetails.dwValue - Control[Index].Bounds.dwMinimum) / dwStep;
                    SendDlgItemMessageW(hwndDlg, IDC_ADV_TREBLE_SLIDER, TBM_SETPOS, (WPARAM)TRUE, dwPosition);
                }
            }
            else if (((Control[Index].dwControlType & (MIXERCONTROL_CT_CLASS_MASK | MIXERCONTROL_CT_SUBCLASS_MASK | MIXERCONTROL_CT_UNITS_MASK)) == MIXERCONTROL_CONTROLTYPE_BOOLEAN) &&
                     (Control[Index].dwControlType != MIXERCONTROL_CONTROLTYPE_MUTE))
            {
                /* All boolean controls but the Mute control (Maximum of 2) */

                if (dwOtherControls < 2)
                {
                    if (SndMixerGetVolumeControlDetails(Context->Mixer, Control[Index].dwControlID, 1, sizeof(MIXERCONTROLDETAILS_BOOLEAN), (LPVOID)&BooleanDetails) != -1)
                    {
                        LoadStringW(hAppInstance, IDS_OTHER_CONTROLS1 + dwOtherControls, szRawBuffer, ARRAYSIZE(szRawBuffer));
                        StringCchPrintfW(szCookedBuffer, ARRAYSIZE(szCookedBuffer), szRawBuffer, Control[Index].szName);
                        SetWindowTextW(GetDlgItem(hwndDlg, IDC_ADV_OTHER_CHECK1 + dwOtherControls), szCookedBuffer);

                        ShowWindow(GetDlgItem(hwndDlg, IDC_ADV_OTHER_CHECK1 + dwOtherControls), SW_SHOWNORMAL);

                        SendDlgItemMessageW(hwndDlg, IDC_ADV_OTHER_CHECK1 + dwOtherControls, BM_SETCHECK, (WPARAM)BooleanDetails.fValue, 0);

                        dwOtherControls++;
                    }
                }
            }
        }

        /* Free controls */
        HeapFree(GetProcessHeap(), 0, Control);
    }

    if (dwOtherControls != 0)
    {
        /* Show the 'Other controls' groupbox and text */
        ShowWindow(GetDlgItem(hwndDlg, IDC_ADV_OTHER_CONTROLS), SW_SHOWNORMAL);
        ShowWindow(GetDlgItem(hwndDlg, IDC_ADV_OTHER_TEXT), SW_SHOWNORMAL);

        /* Resize the dialog */
        GetWindowRect(hwndDlg, &rect);

        dy = MulDiv((dwOtherControls == 1) ? 73 : (73 + 15), Context->MixerWindow->baseUnit.cy, 8);
        rect.bottom += dy;

        SetWindowPos(hwndDlg, HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);

        /* Move the 'Close' button down */
        GetWindowRect(GetDlgItem(hwndDlg, IDOK), &rect);
        MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, 2);

        rect.top += dy;
        rect.bottom += dy;

        SetWindowPos(GetDlgItem(hwndDlg, IDOK), HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOSIZE | SWP_NOZORDER);

        if (dwOtherControls == 2)
        {
            /* Resize the 'Other Controls' groupbox */
            GetWindowRect(GetDlgItem(hwndDlg, IDC_ADV_OTHER_CONTROLS), &rect);
            MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, 2);

            dy = MulDiv(15, Context->MixerWindow->baseUnit.cy, 8);
            rect.bottom += dy;

            SetWindowPos(GetDlgItem(hwndDlg, IDC_ADV_OTHER_CONTROLS), HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);
        }
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
