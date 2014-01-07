/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/misc.c
 * PURPOSE:         Misc functions
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "rapps.h"

/* SESSION Operation */
#define EXTRACT_FILLFILELIST  0x00000001
#define EXTRACT_EXTRACTFILES  0x00000002

static HANDLE hLog = NULL;

typedef struct
{
    int erfOper;
    int erfType;
    BOOL fError;
} ERF, *PERF;

struct FILELIST
{
    LPSTR FileName;
    struct FILELIST *next;
    BOOL DoExtract;
};

typedef struct
{
    INT FileSize;
    ERF Error;
    struct FILELIST *FileList;
    INT FileCount;
    INT Operation;
    CHAR Destination[MAX_PATH];
    CHAR CurrentFile[MAX_PATH];
    CHAR Reserved[MAX_PATH];
    struct FILELIST *FilterList;
} SESSION;

HRESULT (WINAPI *pfnExtract)(SESSION *dest, LPCSTR szCabName);


INT
GetSystemColorDepth(VOID)
{
    DEVMODE pDevMode;
    INT ColorDepth;

    pDevMode.dmSize = sizeof(DEVMODE);
    pDevMode.dmDriverExtra = 0;

    if (!EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &pDevMode))
    {
        /* TODO: Error message */
        return ILC_COLOR;
    }

    switch (pDevMode.dmBitsPerPel)
    {
        case 32: ColorDepth = ILC_COLOR32; break;
        case 24: ColorDepth = ILC_COLOR24; break;
        case 16: ColorDepth = ILC_COLOR16; break;
        case  8: ColorDepth = ILC_COLOR8;  break;
        case  4: ColorDepth = ILC_COLOR4;  break;
        default: ColorDepth = ILC_COLOR;   break;
    }

    return ColorDepth;
}

int
GetWindowWidth(HWND hwnd)
{
    RECT Rect;

    GetWindowRect(hwnd, &Rect);
    return (Rect.right - Rect.left);
}

int
GetWindowHeight(HWND hwnd)
{
    RECT Rect;

    GetWindowRect(hwnd, &Rect);
    return (Rect.bottom - Rect.top);
}

int
GetClientWindowWidth(HWND hwnd)
{
    RECT Rect;

    GetClientRect(hwnd, &Rect);
    return (Rect.right - Rect.left);
}

int
GetClientWindowHeight(HWND hwnd)
{
    RECT Rect;

    GetClientRect(hwnd, &Rect);
    return (Rect.bottom - Rect.top);
}

VOID
CopyTextToClipboard(LPCWSTR lpszText)
{
    if(OpenClipboard(NULL))
    {
        HGLOBAL ClipBuffer;
        WCHAR *Buffer;

        EmptyClipboard();
        ClipBuffer = GlobalAlloc(GMEM_DDESHARE, (wcslen(lpszText) + 1) * sizeof(TCHAR));
        Buffer = (WCHAR*)GlobalLock(ClipBuffer);
        wcscpy(Buffer, lpszText);
        GlobalUnlock(ClipBuffer);

        SetClipboardData(CF_UNICODETEXT, ClipBuffer);

        CloseClipboard();
    }
}

VOID
SetWelcomeText(VOID)
{
    WCHAR szText[MAX_STR_LEN*3];

    LoadStringW(hInst, IDS_WELCOME_TITLE, szText, sizeof(szText) / sizeof(WCHAR));
    NewRichEditText(szText, CFE_BOLD);

    LoadStringW(hInst, IDS_WELCOME_TEXT, szText, sizeof(szText) / sizeof(WCHAR));
    InsertRichEditText(szText, 0);

    LoadStringW(hInst, IDS_WELCOME_URL, szText, sizeof(szText) / sizeof(WCHAR));
    InsertRichEditText(szText, CFM_LINK);
}

VOID
ShowPopupMenu(HWND hwnd, UINT MenuID)
{
    HMENU hPopupMenu = GetSubMenu(LoadMenuW(hInst, MAKEINTRESOURCEW(MenuID)), 0);
    POINT pt;

    GetCursorPos(&pt);

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hPopupMenu, 0, pt.x, pt.y, 0, hMainWnd, NULL);

    DestroyMenu(hPopupMenu);
}

BOOL
StartProcess(LPWSTR lpPath, BOOL Wait)
{
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    DWORD dwRet;
    MSG msg;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOW;

    if (!CreateProcessW(NULL, lpPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        return FALSE;
    }

    CloseHandle(pi.hThread);
    if (Wait) EnableWindow(hMainWnd, FALSE);

    while (Wait)
    {
        dwRet = MsgWaitForMultipleObjects(1, &pi.hProcess, FALSE, INFINITE, QS_ALLEVENTS);
        if (dwRet == WAIT_OBJECT_0 + 1)
        {
            while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            if (dwRet == WAIT_OBJECT_0 || dwRet == WAIT_FAILED)
                break;
        }
    }

    CloseHandle(pi.hProcess);

    if (Wait)
    {
        EnableWindow(hMainWnd, TRUE);
        SetForegroundWindow(hMainWnd);
        SetFocus(hMainWnd);
    }

    return TRUE;
}


BOOL
ExtractFilesFromCab(LPWSTR lpCabName, LPWSTR lpOutputPath)
{
    HINSTANCE hCabinetDll;
    CHAR szCabName[MAX_PATH];
    SESSION Dest;
    HRESULT Result;

    hCabinetDll = LoadLibraryW(L"cabinet.dll");
    if (hCabinetDll)
    {
        pfnExtract = (void *) GetProcAddress(hCabinetDll, "Extract");
        if (pfnExtract)
        {
            ZeroMemory(&Dest, sizeof(SESSION));

            WideCharToMultiByte(CP_ACP, 0, lpOutputPath, -1, Dest.Destination, MAX_PATH, NULL, NULL);
            WideCharToMultiByte(CP_ACP, 0, lpCabName, -1, szCabName, MAX_PATH, NULL, NULL);
            Dest.Operation = EXTRACT_FILLFILELIST;

            Result = pfnExtract(&Dest, szCabName);
            if (Result == S_OK)
            {
                Dest.Operation = EXTRACT_EXTRACTFILES;
                Result = pfnExtract(&Dest, szCabName);
                if (Result == S_OK)
                {
                    FreeLibrary(hCabinetDll);
                    return TRUE;
                }
            }
        }
        FreeLibrary(hCabinetDll);
    }

    return FALSE;
}

VOID
InitLogs(VOID)
{
    WCHAR szBuf[MAX_PATH] = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\ReactOS Application Manager\\ReactOS Application Manager";
    WCHAR szPath[MAX_PATH];
    DWORD dwCategoryNum = 1;
    DWORD dwDisp, dwData;
    HKEY hKey;

    if (!SettingsInfo.bLogEnabled) return;

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                        szBuf, 0, NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE, NULL, &hKey, &dwDisp) != ERROR_SUCCESS) 
    {
        return;
    }

    if (!GetCurrentDirectoryW(MAX_PATH, szPath)) return;
    wcscat(szPath, L"\\rapps.exe");

    if (RegSetValueExW(hKey,
                       L"EventMessageFile",
                       0,
                       REG_EXPAND_SZ,
                       (LPBYTE)szPath,
                       (DWORD)(wcslen(szPath) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey); 
        return;
    }

    dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | 
             EVENTLOG_INFORMATION_TYPE; 
 
    if (RegSetValueExW(hKey,
                       L"TypesSupported",
                       0,
                       REG_DWORD,
                       (LPBYTE)&dwData,
                       sizeof(DWORD)) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey); 
        return;
    }

    if (RegSetValueExW(hKey,
                       L"CategoryMessageFile",
                       0,
                       REG_EXPAND_SZ,
                       (LPBYTE)szPath,
                       (DWORD)(wcslen(szPath) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey); 
        return;
    }

    if (RegSetValueExW(hKey,
                       L"CategoryCount",
                       0,
                       REG_DWORD,
                       (LPBYTE)&dwCategoryNum,
                       sizeof(DWORD)) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey); 
        return;
    }

    RegCloseKey(hKey);

    hLog = RegisterEventSourceW(NULL, L"ReactOS Application Manager");
}


VOID
FreeLogs(VOID)
{
    if (hLog) DeregisterEventSource(hLog);
}


BOOL
WriteLogMessage(WORD wType, DWORD dwEventID, LPWSTR lpMsg)
{
    if (!SettingsInfo.bLogEnabled) return TRUE;

    if (!ReportEventW(hLog,
                      wType,
                      0,
                      dwEventID,
                      NULL,
                      1,
                      0,
                      (LPCWSTR*)&lpMsg,
                      NULL))
    {
        return FALSE;
    }

    return TRUE;
}
