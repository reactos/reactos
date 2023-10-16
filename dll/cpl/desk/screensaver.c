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

static const TCHAR szPreviewWndClass[] = TEXT("SSDemoParent");

typedef struct
{
    BOOL  bIsScreenSaver; /* Is this a valid screensaver */
    TCHAR szFilename[MAX_PATH];
    TCHAR szDisplayName[256];
} SCREEN_SAVER_ITEM;


typedef struct _DATA
{
    SCREEN_SAVER_ITEM   ScreenSaverItems[MAX_SCREENSAVERS];
    PROCESS_INFORMATION PrevWindowPi;
    int                 Selection;
    WNDPROC             OldPreviewProc;
    UINT                ScreenSaverCount;
    HWND                ScreenSaverPreviewParent;
} DATA, *PDATA;


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
        lpBuf = HeapAlloc(GetProcessHeap(), 0, BufSize);
        if (lpBuf)
        {
            Ret = RegQueryValueEx(hKey,
                                  lpValue,
                                  0,
                                  &Type,
                                  (LPBYTE)lpBuf,
                                  &BufSize);
            if (Ret != ERROR_SUCCESS)
            {
                HeapFree(GetProcessHeap(), 0, lpBuf);
                lpBuf = NULL;
            }
        }
    }

    RegCloseKey(hKey);

    return lpBuf;
}


static VOID
SelectionChanged(HWND hwndDlg, PDATA pData)
{
    HWND hwndCombo;
    BOOL bEnable;
    INT i;

    hwndCombo = GetDlgItem(hwndDlg, IDC_SCREENS_LIST);

    i = (INT)SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
    i = (INT)SendMessage(hwndCombo, CB_GETITEMDATA, i, 0);

    pData->Selection = i;

    bEnable = (i != 0);

    EnableWindow(GetDlgItem(hwndDlg, IDC_SCREENS_SETTINGS), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SCREENS_TESTSC), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SCREENS_USEPASSCHK), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SCREENS_TIMEDELAY), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SCREENS_TIME), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_WAITTEXT), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MINTEXT), bEnable);
}


LRESULT CALLBACK
RedrawSubclassProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    HWND hwnd;
    PDATA pData;
    LRESULT Ret = FALSE;

    pData = (PDATA)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    if (!pData)
        return Ret;

    Ret = CallWindowProc(pData->OldPreviewProc, hwndDlg, uMsg, wParam, lParam);

    if (uMsg == WM_PAINT)
    {
        hwnd = pData->ScreenSaverPreviewParent;
        if (hwnd)
            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    }

    return Ret;
}


static VOID
ShowScreenSaverPreview(IN LPDRAWITEMSTRUCT draw, IN PDATA pData)
{
    HBRUSH hBrush;
    HDC hDC;
    HGDIOBJ hOldObj;
    RECT rcItem = {
        MONITOR_LEFT,
        MONITOR_TOP,
        MONITOR_RIGHT,
        MONITOR_BOTTOM
    };

    hDC = CreateCompatibleDC(draw->hDC);
    hOldObj = SelectObject(hDC, g_GlobalData.hMonitorBitmap);

    if (!IsWindowVisible(pData->ScreenSaverPreviewParent))
    {
        /* FIXME: Draw static bitmap inside monitor. */
        hBrush = CreateSolidBrush(g_GlobalData.desktop_color);
        FillRect(hDC, &rcItem, hBrush);
        DeleteObject(hBrush);
    }

    GdiTransparentBlt(draw->hDC,
                      draw->rcItem.left, draw->rcItem.top,
                      draw->rcItem.right - draw->rcItem.left + 1,
                      draw->rcItem.bottom - draw->rcItem.top + 1,
                      hDC,
                      0, 0,
                      g_GlobalData.bmMonWidth, g_GlobalData.bmMonHeight,
                      MONITOR_ALPHA);

    SelectObject(hDC, hOldObj);
    DeleteDC(hDC);
}


/*
 * /p:<hwnd>    Run preview, hwnd is handle of calling window
 */
static VOID
SetScreenSaverPreviewBox(HWND hwndDlg, PDATA pData)
{
    HWND hPreview = pData->ScreenSaverPreviewParent;
    HRESULT hr;
    STARTUPINFO si;
    TCHAR szCmdline[2048];

    /* Kill off the previous preview process */
    if (pData->PrevWindowPi.hProcess)
    {
        TerminateProcess(pData->PrevWindowPi.hProcess, 0);
        CloseHandle(pData->PrevWindowPi.hProcess);
        CloseHandle(pData->PrevWindowPi.hThread);
        pData->PrevWindowPi.hThread = pData->PrevWindowPi.hProcess = NULL;
    }
    ShowWindow(pData->ScreenSaverPreviewParent, SW_HIDE);

    if (pData->Selection < 1)
        return;

    hr = StringCbPrintf(szCmdline, sizeof(szCmdline),
                        TEXT("%s /p %Iu"),
                        pData->ScreenSaverItems[pData->Selection].szFilename,
                        (ULONG_PTR)hPreview);
    if (FAILED(hr))
        return;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pData->PrevWindowPi, sizeof(pData->PrevWindowPi));

    ShowWindow(pData->ScreenSaverPreviewParent, SW_SHOW);

    if (!CreateProcess(NULL,
                       szCmdline,
                       NULL,
                       NULL,
                       FALSE,
                       0,
                       NULL,
                       NULL,
                       &si,
                       &pData->PrevWindowPi))
    {
        pData->PrevWindowPi.hThread = pData->PrevWindowPi.hProcess = NULL;
    }
}

static BOOL
WaitForSettingsDialog(HWND hwndDlg,
                      HANDLE hProcess)
{
    DWORD dwResult;
    MSG msg;

    while (TRUE)
    {
        dwResult = MsgWaitForMultipleObjects(1,
                                             &hProcess,
                                             FALSE,
                                             INFINITE,
                                             QS_ALLINPUT);
        if (dwResult == WAIT_OBJECT_0 + 1)
        {
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    return FALSE;
                }
                if (IsDialogMessage(hwndDlg, &msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
        else if (dwResult == WAIT_OBJECT_0)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}


/*
 * /c:<hwnd>    Run configuration, hwnd is handle of calling window
 */
static VOID
ScreenSaverConfig(HWND hwndDlg, PDATA pData)
{
    HRESULT hr;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    TCHAR szCmdline[2048];

    if (pData->Selection < 1)
        return;

    hr = StringCbPrintf(szCmdline, sizeof(szCmdline),
                        TEXT("%s /c:%Iu"),
                        pData->ScreenSaverItems[pData->Selection].szFilename,
                        (ULONG_PTR)hwndDlg);
    if (FAILED(hr))
        return;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    if (CreateProcess(NULL,
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
        /* Kill off the previous preview process */
        if (pData->PrevWindowPi.hProcess)
        {
            TerminateProcess(pData->PrevWindowPi.hProcess, 0);
            CloseHandle(pData->PrevWindowPi.hProcess);
            CloseHandle(pData->PrevWindowPi.hThread);
            pData->PrevWindowPi.hThread = pData->PrevWindowPi.hProcess = NULL;
        }

        if (WaitForSettingsDialog(hwndDlg, pi.hProcess))
            SetScreenSaverPreviewBox(hwndDlg, pData);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

/*
 * /s   Run normal
 */
static VOID
ScreenSaverPreview(HWND hwndDlg, PDATA pData)
{
    HRESULT hr;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    TCHAR szCmdline[2048];

    if (pData->Selection < 1)
        return;

    /* Kill off the previous preview process */
    if (pData->PrevWindowPi.hProcess)
    {
        TerminateProcess(pData->PrevWindowPi.hProcess, 0);
        CloseHandle(pData->PrevWindowPi.hProcess);
        CloseHandle(pData->PrevWindowPi.hThread);
        pData->PrevWindowPi.hThread = pData->PrevWindowPi.hProcess = NULL;
    }

    hr = StringCbPrintf(szCmdline, sizeof(szCmdline),
                        TEXT("%s /s"),
                        pData->ScreenSaverItems[pData->Selection].szFilename);
    if (FAILED(hr))
        return;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    if (CreateProcess(NULL,
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


static BOOL
AddScreenSaverItem(
    _In_ HWND hwndScreenSavers,
    _In_ PDATA pData,
    _In_ SCREEN_SAVER_ITEM* ScreenSaverItem)
{
    UINT i;

    if (pData->ScreenSaverCount >= MAX_SCREENSAVERS)
        return FALSE;

    i = SendMessage(hwndScreenSavers,
                    CB_ADDSTRING,
                    0,
                    (LPARAM)ScreenSaverItem->szDisplayName);
    if ((i == CB_ERR) || (i == CB_ERRSPACE))
        return FALSE;

    SendMessage(hwndScreenSavers,
                CB_SETITEMDATA,
                i,
                (LPARAM)pData->ScreenSaverCount);

    pData->ScreenSaverCount++;
    return TRUE;
}

static BOOL
AddScreenSaver(
    _In_ HWND hwndScreenSavers,
    _In_ PDATA pData,
    _In_ LPCTSTR pszFilePath,
    _In_ LPCTSTR pszFileName)
{
    SCREEN_SAVER_ITEM* ScreenSaverItem;
    HANDLE hModule;
    HRESULT hr;

    if (pData->ScreenSaverCount >= MAX_SCREENSAVERS)
        return FALSE;

    ScreenSaverItem = pData->ScreenSaverItems + pData->ScreenSaverCount;

    ScreenSaverItem->bIsScreenSaver = TRUE;

    hModule = LoadLibraryEx(pszFilePath,
                            NULL,
                            DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
    if (hModule)
    {
        if (LoadString(hModule,
                       1,
                       ScreenSaverItem->szDisplayName,
                       _countof(ScreenSaverItem->szDisplayName)) == 0)
        {
            /* If the string does not exist, copy the file name */
            hr = StringCbCopy(ScreenSaverItem->szDisplayName,
                              sizeof(ScreenSaverItem->szDisplayName),
                              pszFileName);
            if (FAILED(hr))
            {
                FreeLibrary(hModule);
                return FALSE;
            }
            /* Remove the .scr extension */
            ScreenSaverItem->szDisplayName[_tcslen(pszFileName)-4] = _T('\0');
        }
        FreeLibrary(hModule);
    }
    else
    {
        hr = StringCbCopy(ScreenSaverItem->szDisplayName,
                          sizeof(ScreenSaverItem->szDisplayName),
                          _T("Unknown"));
        if (FAILED(hr))
            return FALSE;
    }

    hr = StringCbCopy(ScreenSaverItem->szFilename,
                      sizeof(ScreenSaverItem->szFilename),
                      pszFilePath);
    if (FAILED(hr))
        return FALSE;

    return AddScreenSaverItem(hwndScreenSavers, pData, ScreenSaverItem);
}

static VOID
SearchScreenSavers(
    _In_ HWND hwndScreenSavers,
    _In_ PDATA pData,
    _In_ LPCTSTR pszSearchPath)
{
    HRESULT hr;
    WIN32_FIND_DATA fd;
    HANDLE hFind;
    TCHAR szFilePath[MAX_PATH];

    hr = StringCbPrintf(szFilePath, sizeof(szFilePath),
                        TEXT("%s\\*.scr"), pszSearchPath);
    if (FAILED(hr))
        return;

    hFind = FindFirstFile(szFilePath, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do
    {
        /* Don't add any hidden screensavers */
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
            continue;

        if (pData->ScreenSaverCount >= MAX_SCREENSAVERS)
            break;

        hr = StringCbPrintf(szFilePath, sizeof(szFilePath),
                            TEXT("%s\\%s"), pszSearchPath, fd.cFileName);
        if (FAILED(hr))
            break;

        if (!AddScreenSaver(hwndScreenSavers, pData, szFilePath, fd.cFileName))
            break;

    } while (FindNextFile(hFind, &fd));

    FindClose(hFind);
}

static VOID
EnumScreenSavers(
    _In_ HWND hwndScreenSavers,
    _In_ PDATA pData)
{
    SCREEN_SAVER_ITEM* ScreenSaverItem;
    PTCHAR pBackSlash;
    TCHAR szSearchPath[MAX_PATH];
    TCHAR szLocalPath[MAX_PATH];

    /* Initialize the number of list items */
    pData->ScreenSaverCount = 0;

    /* Add the "(None)" item */
    ScreenSaverItem = pData->ScreenSaverItems;

    ScreenSaverItem->bIsScreenSaver = FALSE;

    LoadString(hApplet,
               IDS_NONE,
               ScreenSaverItem->szDisplayName,
               _countof(ScreenSaverItem->szDisplayName));

    AddScreenSaverItem(hwndScreenSavers, pData, ScreenSaverItem);

    /* Add all the screensavers where the applet is stored */
    GetModuleFileName(hApplet, szLocalPath, _countof(szLocalPath));
    pBackSlash = _tcsrchr(szLocalPath, _T('\\'));
    if (pBackSlash != NULL)
    {
        *pBackSlash = _T('\0');
        SearchScreenSavers(hwndScreenSavers, pData, szLocalPath);
    }

    /* Add all the screensavers in the C:\ReactOS\System32 directory */
    GetSystemDirectory(szSearchPath, _countof(szSearchPath));
    if (pBackSlash != NULL && _tcsicmp(szSearchPath, szLocalPath) != 0)
        SearchScreenSavers(hwndScreenSavers, pData, szSearchPath);

    /* Add all the screensavers in the C:\ReactOS directory */
    GetWindowsDirectory(szSearchPath, _countof(szSearchPath));
    if (pBackSlash != NULL && _tcsicmp(szSearchPath, szLocalPath) != 0)
        SearchScreenSavers(hwndScreenSavers, pData, szSearchPath);
}


static VOID
SetScreenSaver(HWND hwndDlg, PDATA pData)
{
    HKEY regKey;
    BOOL DeleteMode = FALSE;

    DBG_UNREFERENCED_LOCAL_VARIABLE(DeleteMode);

    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     _T("Control Panel\\Desktop"),
                     0,
                     KEY_ALL_ACCESS,
                     &regKey) == ERROR_SUCCESS)
    {
        INT Time;
        BOOL bRet;
        TCHAR Sec;
        UINT Ret;

        /* Set the screensaver */
        if (pData->ScreenSaverItems[pData->Selection].bIsScreenSaver)
        {
            SIZE_T Length = (_tcslen(pData->ScreenSaverItems[pData->Selection].szFilename) + 1) * sizeof(TCHAR);
            RegSetValueEx(regKey,
                          _T("SCRNSAVE.EXE"),
                          0,
                          REG_SZ,
                          (PBYTE)pData->ScreenSaverItems[pData->Selection].szFilename,
                          (DWORD)Length);

            SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, TRUE, 0, SPIF_UPDATEINIFILE);
        }
        else
        {
            /* Windows deletes the value if no screensaver is set */
            RegDeleteValue(regKey, _T("SCRNSAVE.EXE"));
            DeleteMode = TRUE;

            SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE, 0, SPIF_UPDATEINIFILE);
        }

        /* Set the secure value */
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

        /* Set the screensaver time delay */
        Time = GetDlgItemInt(hwndDlg,
                             IDC_SCREENS_TIMEDELAY,
                             &bRet,
                             FALSE);
        if (Time == 0)
            Time = 1;
        Time *= 60; // Convert to seconds

        SystemParametersInfoW(SPI_SETSCREENSAVETIMEOUT, Time, 0, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);

        RegCloseKey(regKey);
    }
}


static BOOL
OnInitDialog(HWND hwndDlg, PDATA pData)
{
    HWND hwndSSCombo = GetDlgItem(hwndDlg, IDC_SCREENS_LIST);
    LPTSTR pSsValue;
    INT iCurSs;
    WNDCLASS wc = {0};

    pData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DATA));
    if (!pData)
    {
        EndDialog(hwndDlg, -1);
        return FALSE;
    }

    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = hApplet;
    wc.hCursor = NULL;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = szPreviewWndClass;

    if (RegisterClass(&wc))
    {
        HWND hParent = GetDlgItem(hwndDlg, IDC_SCREENS_PREVIEW);
        HWND hChild;

        if (hParent != NULL)
        {
            pData->OldPreviewProc = (WNDPROC)GetWindowLongPtr(hParent, GWLP_WNDPROC);
            SetWindowLongPtr(hParent, GWLP_WNDPROC, (LONG_PTR)RedrawSubclassProc);
            SetWindowLongPtr(hParent, GWLP_USERDATA, (LONG_PTR)pData);
        }

        hChild = CreateWindowEx(0, szPreviewWndClass, NULL,
                                WS_CHILD | WS_CLIPCHILDREN,
                                0, 0, 0, 0, hParent,
                                NULL, hApplet, NULL);
        if (hChild != NULL)
        {
            RECT rc;
            GetClientRect(hParent, &rc);
            rc.left += MONITOR_LEFT;
            rc.top += MONITOR_TOP;
            MoveWindow(hChild, rc.left, rc.top, MONITOR_WIDTH, MONITOR_HEIGHT, FALSE);
        }

        pData->ScreenSaverPreviewParent = hChild;
    }

    SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pData);

    pData->Selection = -1;

    SendDlgItemMessage(hwndDlg,
                       IDC_SCREENS_TIME,
                       UDM_SETRANGE,
                       0,
                       MAKELONG(240, 1));

    EnumScreenSavers(hwndSSCombo, pData);

    CheckRegScreenSaverIsSecure(hwndDlg);

    /* Set the current screensaver in the combo box */
    iCurSs = 0; // Default to "(None)"
    pSsValue = GetCurrentScreenSaverValue(_T("SCRNSAVE.EXE"));
    if (pSsValue)
    {
        BOOL bFound = FALSE;
        INT i;

        /* Find whether the current screensaver is in the list */
        for (i = 0; i < pData->ScreenSaverCount; i++)
        {
            if (!_tcsicmp(pSsValue, pData->ScreenSaverItems[i].szFilename))
            {
                bFound = TRUE;
                break;
            }
        }

        if (!bFound)
        {
            /* The current screensaver is not in the list: add it */
            // i = pData->ScreenSaverCount;
            bFound = AddScreenSaver(hwndSSCombo, pData, pSsValue, _T("SCRNSAVE.EXE"));
            if (bFound)
                i = pData->ScreenSaverCount - 1;
        }

        HeapFree(GetProcessHeap(), 0, pSsValue);

        if (bFound)
        {
            /* The current screensaver should be in the list: select it */
            iCurSs = SendMessage(hwndSSCombo,
                                 CB_FINDSTRINGEXACT,
                                 -1,
                                 (LPARAM)pData->ScreenSaverItems[i].szDisplayName);
            if (iCurSs == CB_ERR)
                iCurSs = 0; // Default to "(None)"
        }
    }
    SendMessage(hwndSSCombo, CB_SETCURSEL, iCurSs, 0);

    /* Set the current timeout */
    pSsValue = GetCurrentScreenSaverValue(_T("ScreenSaveTimeOut"));
    if (pSsValue)
    {
        UINT Time = _ttoi(pSsValue) / 60;

        HeapFree(GetProcessHeap(), 0, pSsValue);

        SendDlgItemMessage(hwndDlg,
                           IDC_SCREENS_TIME,
                           UDM_SETPOS32,
                           0,
                           Time);
    }

    SelectionChanged(hwndDlg, pData);

    return TRUE;
}


INT_PTR CALLBACK
ScreenSaverPageProc(HWND hwndDlg,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
    PDATA pData;

    pData = (PDATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            OnInitDialog(hwndDlg, pData);
            break;
        }

        case WM_DESTROY:
        {
            if (pData->ScreenSaverPreviewParent)
            {
                SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_SCREENS_PREVIEW),
                                 GWLP_WNDPROC,
                                 (LONG_PTR)pData->OldPreviewProc);
                DestroyWindow(pData->ScreenSaverPreviewParent);
                pData->ScreenSaverPreviewParent = NULL;
            }
            UnregisterClass(szPreviewWndClass, hApplet);
            if (pData->PrevWindowPi.hProcess)
            {
                TerminateProcess(pData->PrevWindowPi.hProcess, 0);
                CloseHandle(pData->PrevWindowPi.hProcess);
                CloseHandle(pData->PrevWindowPi.hThread);
            }
            HeapFree(GetProcessHeap(), 0, pData);
            break;
        }

        case WM_ENDSESSION:
        {
            SetScreenSaverPreviewBox(hwndDlg, pData);
            break;
        }

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpDrawItem;
            lpDrawItem = (LPDRAWITEMSTRUCT)lParam;

            if (lpDrawItem->CtlID == IDC_SCREENS_PREVIEW)
                ShowScreenSaverPreview(lpDrawItem, pData);
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
                        SelectionChanged(hwndDlg, pData);
                        SetScreenSaverPreviewBox(hwndDlg, pData);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
                }

                case IDC_SCREENS_TIMEDELAY:
                {
                    if (command == EN_CHANGE)
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
                    if (command == BN_CLICKED)
                    {
                        ScreenSaverPreview(hwndDlg, pData);
                        SetScreenSaverPreviewBox(hwndDlg, pData);
                    }
                    break;
                }

                case IDC_SCREENS_SETTINGS: // Screensaver Settings
                {
                    if (command == BN_CLICKED)
                        ScreenSaverConfig(hwndDlg, pData);
                    break;
                }

                case IDC_SCREENS_USEPASSCHK: // Screensaver Is Secure
                {
                    if (command == BN_CLICKED)
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
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
                    SetScreenSaver(hwndDlg, pData);
                    return TRUE;
                }

                case PSN_SETACTIVE:
                {
                    /* Enable screensaver preview support */
                    SetScreenSaverPreviewBox(hwndDlg, pData);
                    break;
                }

                case PSN_KILLACTIVE:
                {
                    /* Kill running preview screensaver */
                    if (pData->PrevWindowPi.hProcess)
                    {
                        TerminateProcess(pData->PrevWindowPi.hProcess, 0);
                        CloseHandle(pData->PrevWindowPi.hProcess);
                        CloseHandle(pData->PrevWindowPi.hThread);
                        pData->PrevWindowPi.hThread = pData->PrevWindowPi.hProcess = NULL;
                    }
                    break;
                }
            }
        }
        break;
    }

    return FALSE;
}
