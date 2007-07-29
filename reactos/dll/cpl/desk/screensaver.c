/* 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            dll/cpl/desk/screensaver.c
 * PURPOSE:         Screen saver property page
 * 
 * PROGRAMMERS:     Trevor McCort (lycan359@gmail.com)
 *                  Ged Murphy (gedmurphy@reactos.org)
 */

#include "desk.h"

#define MAX_SCREENSAVERS 100

typedef struct
{
    BOOL  bIsScreenSaver; /* Is this background a wallpaper */
    TCHAR szFilename[MAX_PATH];
    TCHAR szDisplayName[256];
} ScreenSaverItem;


typedef struct _GLOBAL_DATA
{
    ScreenSaverItem ScreenSaverItems[MAX_SCREENSAVERS];
    PROCESS_INFORMATION PrevWindowPi;
    int Selection;
} GLOBAL_DATA, *PGLOBAL_DATA;


static LPTSTR
GetCurrentScreenSaverValue(LPTSTR lpValue)
{
    HKEY hKey;
    LPTSTR lpBuf = NULL;
    DWORD BufSize, Type = REG_SZ;
    LONG Ret;

    Ret = RegOpenKeyEx(HKEY_CURRENT_USER,
                       _T("Control Panel\\Desktop"),
                       0,
                       KEY_READ,
                       &hKey);
    if (Ret != ERROR_SUCCESS)
        return NULL;

    Ret = RegQueryValueEx(hKey,
                          lpValue,
                          0,
                          &Type,
                          NULL,
                          &BufSize);
    if (Ret == ERROR_SUCCESS)
    {
        lpBuf = HeapAlloc(GetProcessHeap(),
                          0,
                          BufSize);
        if (lpBuf)
        {
            Ret = RegQueryValueEx(hKey, 
                                  lpValue,
                                  0,
                                  &Type,
                                  (LPBYTE)lpBuf,
                                  &BufSize);
            if (Ret != ERROR_SUCCESS)
                lpBuf = NULL;
        }
    }

    RegCloseKey(hKey);
    
    return lpBuf;
}


static VOID
SelectionChanged(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    HWND hwndCombo;
    BOOL bEnable;
    INT i;

    hwndCombo = GetDlgItem(hwndDlg, IDC_SCREENS_LIST);

    i = (INT)SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
    i = (INT)SendMessage(hwndCombo, CB_GETITEMDATA, i, 0);

    pGlobalData->Selection = i;

    bEnable = (i != 0);

    EnableWindow(GetDlgItem(hwndDlg, IDC_SCREENS_SETTINGS), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SCREENS_TESTSC), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SCREENS_USEPASSCHK), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SCREENS_TIMEDELAY), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SCREENS_TIME), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_WAITTEXT), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MINTEXT), bEnable);
}


static VOID
SetScreenSaverPreviewBox(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    HWND hPreview = GetDlgItem(hwndDlg, IDC_SCREENS_PREVIEW);
    STARTUPINFO si;
    TCHAR szCmdline[2048];

    /* kill off the previous preview process*/
    if (pGlobalData->PrevWindowPi.hProcess)
    {
        TerminateProcess(pGlobalData->PrevWindowPi.hProcess, 0);
        CloseHandle(pGlobalData->PrevWindowPi.hProcess);
        CloseHandle(pGlobalData->PrevWindowPi.hThread);
        pGlobalData->PrevWindowPi.hThread = pGlobalData->PrevWindowPi.hProcess = NULL;
    }

    if (pGlobalData->Selection > 0)
    {
        _stprintf(szCmdline, 
                  _T("%s /p %u"),
                  pGlobalData->ScreenSaverItems[pGlobalData->Selection].szFilename,
                  hPreview);

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pGlobalData->PrevWindowPi, sizeof(pGlobalData->PrevWindowPi));

        if (!CreateProcess(NULL, 
                           szCmdline, 
                           NULL, 
                           NULL, 
                           FALSE, 
                           0, 
                           NULL, 
                           NULL, 
                           &si, 
                           &pGlobalData->PrevWindowPi))
        {
            pGlobalData->PrevWindowPi.hThread = pGlobalData->PrevWindowPi.hProcess = NULL;
        }
    }
}

static BOOL
WaitForSettingsDialog(HWND hwndDlg,
                      HANDLE hProcess)
{
    while (TRUE)
    {
        DWORD Ret;
        MSG msg;

        while (PeekMessage(&msg,
                           NULL,
                           0,
                           0,
                           PM_REMOVE))
        { 
            if (msg.message == WM_QUIT)
                return FALSE;

            if (IsDialogMessage(hwndDlg, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        Ret = MsgWaitForMultipleObjects(1,
                                        &hProcess,
                                        FALSE,
                                        INFINITE,
                                        QS_ALLINPUT);
        if (Ret == (WAIT_OBJECT_0))
        {
            return TRUE;
        } 
    }
}


static VOID
ScreensaverConfig(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    /*
       /c:<hwnd>  Run configuration, hwnd is handle of calling window
    */

    TCHAR szCmdline[2048];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    if (pGlobalData->Selection < 1)
        return;

    _stprintf(szCmdline, 
              _T("%s /c:%u"),
              pGlobalData->ScreenSaverItems[pGlobalData->Selection].szFilename,
              hwndDlg);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    if(CreateProcess(NULL,
                     szCmdline,
                     NULL,
                     NULL,
                     FALSE,
                     0,
                     NULL,
                     NULL,
                     &si,
                     &pi))
    {
        /* kill off the previous preview process */
        if (pGlobalData->PrevWindowPi.hProcess)
        {
            TerminateProcess(pGlobalData->PrevWindowPi.hProcess, 0);
            CloseHandle(pGlobalData->PrevWindowPi.hProcess);
            CloseHandle(pGlobalData->PrevWindowPi.hThread);
            pGlobalData->PrevWindowPi.hThread = pGlobalData->PrevWindowPi.hProcess = NULL;
        }

        if (WaitForSettingsDialog(hwndDlg, pi.hProcess))
            SetScreenSaverPreviewBox(hwndDlg, pGlobalData);
    }
}


static VOID
ScreensaverPreview(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    /*
       /s         Run normal
    */

    WCHAR szCmdline[2048];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    if (pGlobalData->Selection < 1)
        return;

    /* kill off the previous preview process*/
    if (pGlobalData->PrevWindowPi.hProcess)
    {
        TerminateProcess(pGlobalData->PrevWindowPi.hProcess, 0);
        CloseHandle(pGlobalData->PrevWindowPi.hProcess);
        CloseHandle(pGlobalData->PrevWindowPi.hThread);
        pGlobalData->PrevWindowPi.hThread = pGlobalData->PrevWindowPi.hProcess = NULL;
    }

    _stprintf(szCmdline, 
              _T("%s /s"),
              pGlobalData->ScreenSaverItems[pGlobalData->Selection].szFilename);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    if(CreateProcess(NULL,
                     szCmdline,
                     NULL,
                     NULL,
                     FALSE,
                     0,
                     NULL,
                     NULL,
                     &si,
                     &pi))
    {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}


static VOID
CheckRegScreenSaverIsSecure(HWND hwndDlg)
{
    HKEY hKey;
    TCHAR szBuffer[2];
    DWORD bufferSize = sizeof(szBuffer);
    DWORD varType = REG_SZ;
    LONG result;

    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     _T("Control Panel\\Desktop"),
                     0,
                     KEY_ALL_ACCESS,
                     &hKey) == ERROR_SUCCESS)
    {
        result = RegQueryValueEx(hKey,
                                _T("ScreenSaverIsSecure"),
                                0,
                                &varType,
                                (LPBYTE)szBuffer,
                                &bufferSize);
        RegCloseKey(hKey);

        if (result == ERROR_SUCCESS)
        {
            if (_ttoi(szBuffer) == 1)
            {
                SendDlgItemMessage(hwndDlg,
                                   IDC_SCREENS_USEPASSCHK,
                                   BM_SETCHECK,
                                   (WPARAM)BST_CHECKED,
                                   0);
                return;
            }
        }

        SendDlgItemMessage(hwndDlg,
                           IDC_SCREENS_USEPASSCHK,
                           BM_SETCHECK,
                           (WPARAM)BST_UNCHECKED,
                           0);
    }
}


static VOID
AddScreenSavers(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    HWND hwndScreenSavers = GetDlgItem(hwndDlg, IDC_SCREENS_LIST);
    WIN32_FIND_DATA fd;
    HANDLE hFind;
    TCHAR szSearchPath[MAX_PATH];
    INT i;
    int ScreenSaverCount = 0;
    ScreenSaverItem *ScreenSaverItem = NULL;
    HANDLE hModule = NULL;

    /* Add the "None" item */
    ScreenSaverItem = &pGlobalData->ScreenSaverItems[ScreenSaverCount];

    ScreenSaverItem->bIsScreenSaver = FALSE;

    LoadString(hApplet,
               IDS_NONE,
               ScreenSaverItem->szDisplayName,
               sizeof(ScreenSaverItem->szDisplayName) / sizeof(TCHAR));

    i = SendMessage(hwndScreenSavers,
                    CB_ADDSTRING,
                    0,
                    (LPARAM)ScreenSaverItem->szDisplayName);

    SendMessage(hwndScreenSavers,
                CB_SETITEMDATA,
                i,
                (LPARAM)ScreenSaverCount);

    ScreenSaverCount++;

    /* Add all the screensavers in the C:\ReactOS\System32 directory. */

    GetSystemDirectory(szSearchPath, MAX_PATH);
    _tcscat(szSearchPath, TEXT("\\*.scr"));

    hFind = FindFirstFile(szSearchPath, &fd);
    while (ScreenSaverCount < MAX_SCREENSAVERS-1 &&
           hFind != INVALID_HANDLE_VALUE)
    {
        /* Don't add any hidden screensavers */
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == 0)
        {
            TCHAR filename[MAX_PATH];

            GetSystemDirectory(filename, MAX_PATH);

            _tcscat(filename, TEXT("\\"));
            _tcscat(filename, fd.cFileName);

            ScreenSaverItem = &pGlobalData->ScreenSaverItems[ScreenSaverCount];

            ScreenSaverItem->bIsScreenSaver = TRUE;

            hModule = LoadLibraryEx(filename,
                                    NULL,
                                    DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
            if (hModule)
            {
               LoadString(hModule,
                          1,
                          ScreenSaverItem->szDisplayName,
                          sizeof(ScreenSaverItem->szDisplayName) / sizeof(TCHAR));
               FreeLibrary(hModule);
            }
            else
            {
               _tcscpy(ScreenSaverItem->szDisplayName, _T("Unknown"));
            }

            _tcscpy(ScreenSaverItem->szFilename, filename);

            i = SendMessage(hwndScreenSavers,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)ScreenSaverItem->szDisplayName);

            SendMessage(hwndScreenSavers,
                        CB_SETITEMDATA,
                        i,
                        (LPARAM)ScreenSaverCount);

            ScreenSaverCount++;
        }

        if (!FindNextFile(hFind, &fd))
            hFind = INVALID_HANDLE_VALUE;
    }
}


static VOID
SetScreenSaver(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    HKEY regKey;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, 
                     _T("Control Panel\\Desktop"),
                     0, 
                     KEY_ALL_ACCESS, 
                     &regKey) == ERROR_SUCCESS)
    {
        INT Time;
        BOOL bRet;
        TCHAR szTime[256], Sec;
        UINT Ret;

        /* set the screensaver */
        if (pGlobalData->ScreenSaverItems[pGlobalData->Selection].bIsScreenSaver)
        {
            RegSetValueEx(regKey, 
                          _T("SCRNSAVE.EXE"),
                          0,
                          REG_SZ,
                          (PBYTE)pGlobalData->ScreenSaverItems[pGlobalData->Selection].szFilename,
                          _tcslen(pGlobalData->ScreenSaverItems[pGlobalData->Selection].szFilename) * sizeof(TCHAR));
        }
        else
        {
            /* Windows deletes the value if no screensaver is set */
            RegDeleteValue(regKey, _T("SCRNSAVE.EXE"));
        }

        /* set the screensaver time delay */
        Time = GetDlgItemInt(hwndDlg, 
                             IDC_SCREENS_TIMEDELAY,
                             &bRet,
                             FALSE);
        if (Time == 0)
            Time = 60;
        else
            Time *= 60;

        _itot(Time, szTime, 10);
        RegSetValueEx(regKey,
                      _T("ScreenSaveTimeOut"),
                      0,
                      REG_SZ,
                      (PBYTE)szTime,
                      _tcslen(szTime) * sizeof(TCHAR));

        /* set the secure value */
        Ret = SendDlgItemMessage(hwndDlg,
                                 IDC_SCREENS_USEPASSCHK,
                                 BM_GETCHECK,
                                 0,
                                 0);
        Sec = (Ret == BST_CHECKED) ? _T('1') : _T('0');
        RegSetValueEx(regKey,
                      _T("ScreenSaverIsSecure"),
                      0,
                      REG_SZ,
                      (PBYTE)&Sec,
                      sizeof(TCHAR));

        RegCloseKey(regKey);
    }
}


static BOOL 
OnInitDialog(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    LPTSTR lpCurSs;
    HWND hwndSSCombo = GetDlgItem(hwndDlg, IDC_SCREENS_LIST);
    INT Num;

    pGlobalData = HeapAlloc(GetProcessHeap(), 
                            HEAP_ZERO_MEMORY, 
                            sizeof(GLOBAL_DATA));
    if (!pGlobalData)
    {
        EndDialog(hwndDlg, -1);
        return FALSE;
    }

    SetWindowLongPtr(hwndDlg,
                     DWLP_USER,
                     (LONG_PTR)pGlobalData);

    pGlobalData->Selection = -1;

    SendDlgItemMessage(hwndDlg,
                       IDC_SCREENS_TIME,
                       UDM_SETRANGE,
                       0,
                       MAKELONG
                       ((short) 240, (short) 1));

    AddScreenSavers(hwndDlg,
                    pGlobalData);

    CheckRegScreenSaverIsSecure(hwndDlg);

    /* set the current screensaver in the combo box */
    lpCurSs = GetCurrentScreenSaverValue(_T("SCRNSAVE.EXE"));
    if (lpCurSs)
    {
        BOOL bFound = FALSE;
        INT i;

        for (i = 0; i < MAX_SCREENSAVERS; i++)
        {
            if (!_tcscmp(lpCurSs, pGlobalData->ScreenSaverItems[i].szFilename))
            {
                bFound = TRUE;
                break;
            }
        }

        if (bFound)
        {
            Num = SendMessage(hwndSSCombo, 
                              CB_FINDSTRINGEXACT,
                              -1,
                              (LPARAM)pGlobalData->ScreenSaverItems[i].szDisplayName);
            if (Num != CB_ERR)
                SendMessage(hwndSSCombo,
                            CB_SETCURSEL,
                            Num,
                            0);
        }
        else
        {
            SendMessage(hwndSSCombo,
                        CB_SETCURSEL,
                        0,
                        0);
        }

        HeapFree(GetProcessHeap(),
                 0,
                 lpCurSs);
    }
    else
    {
        /* set screensaver to (none) */
        SendMessage(hwndSSCombo,
                    CB_SETCURSEL,
                    0,
                    0);
    }

    /* set the current timeout */
    lpCurSs = GetCurrentScreenSaverValue(_T("ScreenSaveTimeOut"));
    if (lpCurSs)
    {
        UINT Time = _ttoi(lpCurSs);

        Time /= 60;

        SendDlgItemMessage(hwndDlg,
                           IDC_SCREENS_TIME,
                           UDM_SETPOS32,
                           0,
                           Time);

        HeapFree(GetProcessHeap(),
                 0,
                 lpCurSs);

    }

    SelectionChanged(hwndDlg,
                     pGlobalData);

    return TRUE;
}


INT_PTR CALLBACK
ScreenSaverPageProc(HWND hwndDlg,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
    PGLOBAL_DATA pGlobalData;

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            OnInitDialog(hwndDlg, pGlobalData);
            break;
        }

        case WM_DESTROY:
        {
            if (pGlobalData->PrevWindowPi.hProcess)
            {
                TerminateProcess(pGlobalData->PrevWindowPi.hProcess, 0);
                CloseHandle(pGlobalData->PrevWindowPi.hProcess);
                CloseHandle(pGlobalData->PrevWindowPi.hThread);
            }
            HeapFree(GetProcessHeap(),
                     0,
                     pGlobalData);
            break;
        }

        case WM_ENDSESSION:
        {
            SetScreenSaverPreviewBox(hwndDlg,
                                     pGlobalData);
            break;
        }

        case WM_COMMAND:
        {
            DWORD controlId = LOWORD(wParam);
            DWORD command   = HIWORD(wParam);

            switch (controlId)
            {
                case IDC_SCREENS_LIST:
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        SelectionChanged(hwndDlg, pGlobalData);
                        SetScreenSaverPreviewBox(hwndDlg, pGlobalData);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
                }
                
                case IDC_SCREENS_TIMEDELAY:
                {
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }

                case IDC_SCREENS_POWER_BUTTON: // Start Powercfg.Cpl
                {
                    if (command == BN_CLICKED)
                        WinExec("rundll32 shell32.dll,Control_RunDLL powercfg.cpl",SW_SHOWNORMAL);
                    break;
                }

                case IDC_SCREENS_TESTSC: // Screensaver Preview
                {
                    if(command == BN_CLICKED)
                    {
                        ScreensaverPreview(hwndDlg, pGlobalData);
                        SetScreenSaverPreviewBox(hwndDlg, pGlobalData);
                    }
                    break;
                }

                case IDC_SCREENS_SETTINGS: // Screensaver Settings
                {
                    if (command == BN_CLICKED)
                        ScreensaverConfig(hwndDlg, pGlobalData);
                    break;
                }

                case IDC_SCREENS_USEPASSCHK: // Screensaver Is Secure
                {
                    if (command == BN_CLICKED)
                    {
                        MessageBox(NULL, TEXT("Feature not yet implemented"), TEXT("Sorry"), MB_OK);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
                }
            }
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch(lpnm->code)
            {
                case PSN_APPLY:
                {
                    SetScreenSaver(hwndDlg, pGlobalData);
                    return TRUE;
                }

                case PSN_SETACTIVE:
                {
                    /* activate screen saver support */
                    SystemParametersInfoW(SPI_SETSCREENSAVEACTIVE, TRUE, 0, SPIF_SENDCHANGE);
                    SetScreenSaverPreviewBox(hwndDlg, pGlobalData);
                    break;
                }

                case PSN_KILLACTIVE:
                {
                    /* Disable screensaver support */
                    SystemParametersInfoW(SPI_SETSCREENSAVEACTIVE, FALSE, 0, SPIF_SENDCHANGE);
                    if (pGlobalData->PrevWindowPi.hProcess)
                    {
                        TerminateProcess(pGlobalData->PrevWindowPi.hProcess, 0);
                        CloseHandle(pGlobalData->PrevWindowPi.hProcess);
                        CloseHandle(pGlobalData->PrevWindowPi.hThread);
                        pGlobalData->PrevWindowPi.hThread = pGlobalData->PrevWindowPi.hProcess = NULL;
                    }
                    break;
                }
            }
        }
        break;
    }

    return FALSE;
}
