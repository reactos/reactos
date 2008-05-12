/*
 *
 * PROJECT:         ReactOS Multimedia Control Panel
 * FILE:            lib/cpl/mmsys/mmsys.c
 * PURPOSE:         ReactOS Multimedia Control Panel
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *                  Johannes Anderwald <janderwald@reactos.com>
 *                  Dmitry Chapyshev <dmitry@reactos.org>
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>
#include <stdio.h>
#include "mmsys.h"
#include "resource.h"

/* Audio property page dialog callback */
INT_PTR CALLBACK
AudioDlgProc(HWND hwndDlg,
             UINT uMsg,
             WPARAM wParam,
             LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            UINT NumWavOut;

            NumWavOut = waveOutGetNumDevs();
            if (!NumWavOut)
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_DEVICE_PLAY_LIST),     FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_DEVICE_REC_LIST),      FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_DEVICE_MIDI_LIST),     FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_DEFAULT_DEV_CHECKBOX), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VOLUME1_BTN),          FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ADV2_BTN),             FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VOLUME2_BTN),          FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ADV1_BTN),             FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VOLUME3_BTN),          FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ADV3_BTN),             FALSE);
            }
        }
        break;
    }

    return FALSE;
}
