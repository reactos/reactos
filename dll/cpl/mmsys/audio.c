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


VOID
InitAudioDlg(HWND hwnd)
{
    WAVEOUTCAPS waveOutputPaps;
    WAVEINCAPS waveInputPaps;
    MIDIOUTCAPS midiOutCaps;
    UINT DevsNum;
    UINT uIndex;
    HWND hCB;
    LRESULT Res;

    // Init sound playback devices list
    hCB = GetDlgItem(hwnd, IDC_DEVICE_PLAY_LIST);

    DevsNum = waveOutGetNumDevs();
    if (DevsNum < 1) return;

    for (uIndex = 0; uIndex < DevsNum; uIndex++)
    {
        if (waveOutGetDevCaps(uIndex, &waveOutputPaps, sizeof(waveOutputPaps)))
            continue;

        Res = SendMessage(hCB, CB_ADDSTRING, 0, (LPARAM) waveOutputPaps.szPname);

        if (CB_ERR != Res)
        {
            SendMessage(hCB, CB_SETITEMDATA, Res, (LPARAM) uIndex);
            // TODO: Getting default device
            SendMessage(hCB, CB_SETCURSEL, (WPARAM) Res, 0);
        }
    }

    // Init sound recording devices list
    hCB = GetDlgItem(hwnd, IDC_DEVICE_REC_LIST);

    DevsNum = waveInGetNumDevs();
    if (DevsNum < 1) return;

    for (uIndex = 0; uIndex < DevsNum; uIndex++)
    {
        if (waveInGetDevCaps(uIndex, &waveInputPaps, sizeof(waveInputPaps)))
            continue;

        Res = SendMessage(hCB, CB_ADDSTRING, 0, (LPARAM) waveInputPaps.szPname);

        if (CB_ERR != Res)
        {
            SendMessage(hCB, CB_SETITEMDATA, Res, (LPARAM) uIndex);
            // TODO: Getting default device
            SendMessage(hCB, CB_SETCURSEL, (WPARAM) Res, 0);
        }
    }

    // Init MIDI devices list
    hCB = GetDlgItem(hwnd, IDC_DEVICE_MIDI_LIST);

    DevsNum = midiOutGetNumDevs();
    if (DevsNum < 1) return;

    for (uIndex = 0; uIndex < DevsNum; uIndex++)
    {
        if (midiOutGetDevCaps(uIndex, &midiOutCaps, sizeof(midiOutCaps)))
            continue;

        Res = SendMessage(hCB, CB_ADDSTRING, 0, (LPARAM) midiOutCaps.szPname);

        if (CB_ERR != Res)
        {
            SendMessage(hCB, CB_SETITEMDATA, Res, (LPARAM) uIndex);
            // TODO: Getting default device
            SendMessage(hCB, CB_SETCURSEL, (WPARAM) Res, 0);
        }
    }
}

static UINT
GetDevNum(HWND hControl, DWORD Id)
{
    int iCurSel;
    UINT DevNum;

    iCurSel = SendMessage(hControl, CB_GETCURSEL, 0, 0);

    if (iCurSel == CB_ERR)
        return 0;

    DevNum = (UINT) SendMessage(hControl, CB_GETITEMDATA, iCurSel, 0);
    if (DevNum == (UINT) CB_ERR)
        return 0;

    if (mixerGetID((HMIXEROBJ) DevNum, &DevNum, Id) != MMSYSERR_NOERROR)
        return 0;

    return DevNum;
}

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
            else
            {
                InitAudioDlg(hwndDlg);
            }
        }
        break;

        case WM_COMMAND:
        {
            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            WCHAR szPath[MAX_PATH];

            switch(LOWORD(wParam))
            {
                case IDC_VOLUME1_BTN:
                {
                    wsprintf(szPath, L"sndvol32.exe -d %d",
                             GetDevNum(GetDlgItem(hwndDlg, IDC_DEVICE_PLAY_LIST), MIXER_OBJECTF_WAVEOUT));

                    ZeroMemory(&si, sizeof(si));
                    si.cb = sizeof(si);
                    si.wShowWindow = SW_SHOW;

                    CreateProcess(NULL, szPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
                }
                break;

                case IDC_ADV2_BTN:
                {
                
                }
                break;

                case IDC_VOLUME2_BTN:
                {
                    wsprintf(szPath, L"sndvol32.exe -r -d %d",
                             GetDevNum(GetDlgItem(hwndDlg, IDC_DEVICE_REC_LIST), MIXER_OBJECTF_WAVEIN));

                    ZeroMemory(&si, sizeof(si));
                    si.cb = sizeof(si);
                    si.wShowWindow = SW_SHOW;

                    CreateProcess(NULL, szPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
                }
                break;

                case IDC_ADV1_BTN:
                {
                
                }
                break;

                case IDC_VOLUME3_BTN:
                {
                    wsprintf(szPath, L"sndvol32.exe -d %d",
                             GetDevNum(GetDlgItem(hwndDlg, IDC_DEVICE_MIDI_LIST), MIXER_OBJECTF_MIDIOUT));

                    ZeroMemory(&si, sizeof(si));
                    si.cb = sizeof(si);
                    si.wShowWindow = SW_SHOW;

                    CreateProcess(NULL, szPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
                }
                break;

                case IDC_ADV3_BTN:
                {
                
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
