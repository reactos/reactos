/*
 *
 * PROJECT:         ReactOS Multimedia Control Panel
 * FILE:            dll/cpl/mmsys/voice.c
 * PURPOSE:         ReactOS Multimedia Control Panel
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *                  Johannes Anderwald <janderwald@reactos.com>
 *                  Dmitry Chapyshev <dmitry@reactos.org>
 */

#include "mmsys.h"

/* Voice property page dialog callback */
INT_PTR CALLBACK
VoiceDlgProc(HWND hwndDlg,
             UINT uMsg,
             WPARAM wParam,
             LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            UINT NumWavOut;

            NumWavOut = waveOutGetNumDevs();
            if (!NumWavOut)
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_DEVICE_VOICE_LIST),     FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_DEVICE_VOICE_REC_LIST), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VOLUME4_BTN),           FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ADV4_BTN),              FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VOLUME5_BTN),           FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ADV5_BTN),              FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_TEST_HARDWARE),         FALSE);
            }
        }
        break;
    }

    return FALSE;
}
