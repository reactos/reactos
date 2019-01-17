/*
 *
 * PROJECT:         ReactOS Multimedia Control Panel
 * FILE:            dll/cpl/mmsys/audio.c
 * PURPOSE:         ReactOS Multimedia Control Panel
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *                  Johannes Anderwald <janderwald@reactos.com>
 *                  Dmitry Chapyshev <dmitry@reactos.org>
 */

#include "mmsys.h"

typedef struct _GLOBAL_DATA
{
    BOOL bNoAudioOut;
    BOOL bNoAudioIn;
    BOOL bNoMIDIOut;

    BOOL bAudioOutChanged;
    BOOL bAudioInChanged;
    BOOL bMIDIOutChanged;

} GLOBAL_DATA, *PGLOBAL_DATA;

VOID
InitAudioDlg(HWND hwnd, PGLOBAL_DATA pGlobalData)
{
    WAVEOUTCAPSW waveOutputPaps;
    WAVEINCAPS waveInputPaps;
    MIDIOUTCAPS midiOutCaps;
    TCHAR szNoDevices[256];
    UINT DevsNum;
    UINT uIndex;
    HWND hCB;
    LRESULT Res;

    LoadString(hApplet, IDS_NO_DEVICES, szNoDevices, _countof(szNoDevices));

    // Init sound playback devices list
    hCB = GetDlgItem(hwnd, IDC_DEVICE_PLAY_LIST);

    DevsNum = waveOutGetNumDevs();
    if (DevsNum < 1)
    {
        Res = SendMessage(hCB, CB_ADDSTRING, 0, (LPARAM)szNoDevices);
        SendMessage(hCB, CB_SETCURSEL, (WPARAM) Res, 0);
        pGlobalData->bNoAudioOut = TRUE;
    }
    else
    {
        WCHAR DefaultDevice[MAX_PATH] = {0};
        HKEY hKey;
        DWORD dwSize = sizeof(DefaultDevice);
        UINT DefaultIndex = 0;

        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Multimedia\\Sound Mapper", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            RegQueryValueExW(hKey, L"Playback", NULL, NULL, (LPBYTE)DefaultDevice, &dwSize);
            DefaultDevice[MAX_PATH-1] = L'\0';
            RegCloseKey(hKey);
        }

        for (uIndex = 0; uIndex < DevsNum; uIndex++)
        {
            if (waveOutGetDevCapsW(uIndex, &waveOutputPaps, sizeof(waveOutputPaps)))
                continue;

            Res = SendMessageW(hCB, CB_ADDSTRING, 0, (LPARAM) waveOutputPaps.szPname);

            if (CB_ERR != Res)
            {
                SendMessage(hCB, CB_SETITEMDATA, Res, (LPARAM) uIndex);
                if (!wcsicmp(waveOutputPaps.szPname, DefaultDevice))
                    DefaultIndex = Res;
            }
        }
        SendMessage(hCB, CB_SETCURSEL, (WPARAM) DefaultIndex, 0);
    }

    // Init sound recording devices list
    hCB = GetDlgItem(hwnd, IDC_DEVICE_REC_LIST);

    DevsNum = waveInGetNumDevs();
    if (DevsNum < 1)
    {
        Res = SendMessage(hCB, CB_ADDSTRING, 0, (LPARAM)szNoDevices);
        SendMessage(hCB, CB_SETCURSEL, (WPARAM) Res, 0);
        pGlobalData->bNoAudioIn = TRUE;
    }
    else
    {
        WCHAR DefaultDevice[MAX_PATH] = {0};
        HKEY hKey;
        DWORD dwSize = sizeof(DefaultDevice);
        UINT DefaultIndex = 0;

        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Multimedia\\Sound Mapper", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            RegQueryValueExW(hKey, L"Record", NULL, NULL, (LPBYTE)DefaultDevice, &dwSize);
            DefaultDevice[MAX_PATH-1] = L'\0';
            RegCloseKey(hKey);
        }


        for (uIndex = 0; uIndex < DevsNum; uIndex++)
        {
            if (waveInGetDevCaps(uIndex, &waveInputPaps, sizeof(waveInputPaps)))
                continue;

            Res = SendMessage(hCB, CB_ADDSTRING, 0, (LPARAM) waveInputPaps.szPname);

            if (CB_ERR != Res)
            {
                SendMessage(hCB, CB_SETITEMDATA, Res, (LPARAM) uIndex);
                if (!wcsicmp(waveInputPaps.szPname, DefaultDevice))
                    DefaultIndex = Res;
            }
        }
        SendMessage(hCB, CB_SETCURSEL, (WPARAM) DefaultIndex, 0);
    }

    // Init MIDI devices list
    hCB = GetDlgItem(hwnd, IDC_DEVICE_MIDI_LIST);

    DevsNum = midiOutGetNumDevs();
    if (DevsNum < 1)
    {
        Res = SendMessage(hCB, CB_ADDSTRING, 0, (LPARAM)szNoDevices);
        SendMessage(hCB, CB_SETCURSEL, (WPARAM) Res, 0);
        pGlobalData->bNoMIDIOut = TRUE;
    }
    else
    {
        WCHAR DefaultDevice[MAX_PATH] = {0};
        HKEY hKey;
        DWORD dwSize = sizeof(DefaultDevice);
        UINT DefaultIndex = 0;

        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Multimedia\\MIDIMap", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            RegQueryValueExW(hKey, L"szPname", NULL, NULL, (LPBYTE)DefaultDevice, &dwSize);
            DefaultDevice[MAX_PATH-1] = L'\0';
            RegCloseKey(hKey);
        }

        for (uIndex = 0; uIndex < DevsNum; uIndex++)
        {
            if (midiOutGetDevCaps(uIndex, &midiOutCaps, sizeof(midiOutCaps)))
                continue;

            Res = SendMessage(hCB, CB_ADDSTRING, 0, (LPARAM) midiOutCaps.szPname);

            if (CB_ERR != Res)
            {
                SendMessage(hCB, CB_SETITEMDATA, Res, (LPARAM) uIndex);
                if (!wcsicmp(midiOutCaps.szPname, DefaultDevice))
                    DefaultIndex = Res;
            }
        }
        SendMessage(hCB, CB_SETCURSEL, (WPARAM) DefaultIndex, 0);
    }
}

VOID
UpdateRegistryString(HWND hwnd, INT ctrl, LPWSTR key, LPWSTR value)
{
    HWND hwndCombo = GetDlgItem(hwnd, ctrl);
    INT CurSel = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
    UINT TextLen;
    WCHAR SelectedDevice[MAX_PATH] = {0};
    HKEY hKey;

    if (CurSel == CB_ERR)
        return;

    TextLen = SendMessageW(hwndCombo, CB_GETLBTEXTLEN, CurSel, 0) + 1;

    if (TextLen > _countof(SelectedDevice))
        return;

    SendMessageW(hwndCombo, CB_GETLBTEXT, CurSel, (LPARAM)SelectedDevice);

    if (RegCreateKeyExW(HKEY_CURRENT_USER, key, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
        return;

    RegSetValueExW(hKey, value, 0, REG_SZ, (BYTE *)SelectedDevice, (wcslen(SelectedDevice) + 1) * sizeof(WCHAR));
    RegCloseKey(hKey);
}

VOID
SaveAudioDlg(HWND hwnd, PGLOBAL_DATA pGlobalData)
{
    if (pGlobalData->bAudioOutChanged)
    {
        UpdateRegistryString(hwnd,
                             IDC_DEVICE_PLAY_LIST,
                             L"Software\\Microsoft\\Multimedia\\Sound Mapper",
                             L"Playback");
    }

    if (pGlobalData->bAudioInChanged)
    {
        UpdateRegistryString(hwnd,
                             IDC_DEVICE_REC_LIST,
                             L"Software\\Microsoft\\Multimedia\\Sound Mapper",
                             L"Record");
    }

    if (pGlobalData->bMIDIOutChanged)
    {
        UpdateRegistryString(hwnd,
                             IDC_DEVICE_MIDI_LIST,
                             L"Software\\Microsoft\\Windows\\CurrentVersion\\Multimedia\\MIDIMap",
                             L"szPname");
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

    if (mixerGetID((HMIXEROBJ)IntToPtr(DevNum), &DevNum, Id) != MMSYSERR_NOERROR)
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
    PGLOBAL_DATA pGlobalData;

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            UINT NumWavOut = waveOutGetNumDevs();

            pGlobalData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLOBAL_DATA));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

            if (!pGlobalData)
                break;

            InitAudioDlg(hwndDlg, pGlobalData);

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

            if (pGlobalData->bNoAudioOut)
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_DEVICE_PLAY_LIST),     FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VOLUME1_BTN),          FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ADV2_BTN),             FALSE);
            }

            if (pGlobalData->bNoAudioIn)
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_DEVICE_REC_LIST),      FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VOLUME2_BTN),          FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ADV1_BTN),             FALSE);
            }

            if (pGlobalData->bNoMIDIOut)
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_DEVICE_MIDI_LIST),     FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VOLUME3_BTN),          FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ADV3_BTN),             FALSE);
            }
        }
        break;

        case WM_COMMAND:
        {
            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            WCHAR szPath[MAX_PATH];

            if (!pGlobalData)
                break;

            switch(LOWORD(wParam))
            {
                case IDC_VOLUME1_BTN:
                {
                    wsprintf(szPath, L"sndvol32.exe -d %d",
                             GetDevNum(GetDlgItem(hwndDlg, IDC_DEVICE_PLAY_LIST), MIXER_OBJECTF_WAVEOUT));

                    ZeroMemory(&si, sizeof(si));
                    si.cb = sizeof(si);
                    si.dwFlags = STARTF_USESHOWWINDOW;
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
                    si.dwFlags = STARTF_USESHOWWINDOW;
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
                    si.dwFlags = STARTF_USESHOWWINDOW;
                    si.wShowWindow = SW_SHOW;

                    CreateProcess(NULL, szPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
                }
                break;

                case IDC_ADV3_BTN:
                {

                }
                break;

                case IDC_DEVICE_PLAY_LIST:
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        pGlobalData->bAudioOutChanged = TRUE;
                    }
                }
                break;

                case IDC_DEVICE_REC_LIST:
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        pGlobalData->bAudioInChanged = TRUE;
                    }
                }

                case IDC_DEVICE_MIDI_LIST:
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        pGlobalData->bMIDIOutChanged = TRUE;
                    }
                }
                break;
            }
        }
        break;

        case WM_DESTROY:
            if (!pGlobalData)
                break;

            HeapFree(GetProcessHeap(), 0, pGlobalData);
            break;

        case WM_NOTIFY:
            if (!pGlobalData)
                break;

            if (((LPNMHDR)lParam)->code == (UINT)PSN_APPLY)
            {
                SaveAudioDlg(hwndDlg, pGlobalData);
            }
            return TRUE;
    }

    return FALSE;
}
