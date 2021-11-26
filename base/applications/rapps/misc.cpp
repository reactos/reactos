/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Misc functions
 * COPYRIGHT:   Copyright 2009 Dmitry Chapyshev           (dmitry@reactos.org)
 *              Copyright 2015 Ismael Ferreras Morezuelas (swyterzone+ros@gmail.com)
 *              Copyright 2017 Alexander Shaposhnikov     (sanchaez@reactos.org)
 */
#include "rapps.h"

#include "misc.h"

static HANDLE hLog = NULL;

static BOOL bIsSys64ResultCached = FALSE;
static BOOL bIsSys64Result = FALSE;

VOID CopyTextToClipboard(LPCWSTR lpszText)
{
    if (!OpenClipboard(NULL))
    {
        return;
    }

    HRESULT hr;
    HGLOBAL ClipBuffer;
    LPWSTR Buffer;
    DWORD cchBuffer;

    EmptyClipboard();
    cchBuffer = wcslen(lpszText) + 1;
    ClipBuffer = GlobalAlloc(GMEM_DDESHARE, cchBuffer * sizeof(WCHAR));

    Buffer = (PWCHAR) GlobalLock(ClipBuffer);
    hr = StringCchCopyW(Buffer, cchBuffer, lpszText);
    GlobalUnlock(ClipBuffer);

    if (SUCCEEDED(hr))
        SetClipboardData(CF_UNICODETEXT, ClipBuffer);

    CloseClipboard();
}

VOID ShowPopupMenuEx(HWND hwnd, HWND hwndOwner, UINT MenuID, UINT DefaultItem)
{
    HMENU hMenu = NULL;
    HMENU hPopupMenu;
    MENUITEMINFO ItemInfo;
    POINT pt;

    if (MenuID)
    {
        hMenu = LoadMenuW(hInst, MAKEINTRESOURCEW(MenuID));
        hPopupMenu = GetSubMenu(hMenu, 0);
    }
    else
    {
        hPopupMenu = GetMenu(hwnd);
    }

    ZeroMemory(&ItemInfo, sizeof(ItemInfo));
    ItemInfo.cbSize = sizeof(ItemInfo);
    ItemInfo.fMask = MIIM_STATE;

    GetMenuItemInfoW(hPopupMenu, DefaultItem, FALSE, &ItemInfo);

    if (!(ItemInfo.fState & MFS_GRAYED))
    {
        SetMenuDefaultItem(hPopupMenu, DefaultItem, FALSE);
    }

    GetCursorPos(&pt);

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hPopupMenu, 0, pt.x, pt.y, 0, hwndOwner, NULL);

    if (hMenu)
    {
        DestroyMenu(hMenu);
    }
}

BOOL StartProcess(const ATL::CStringW& Path, BOOL Wait)
{
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    DWORD dwRet;
    MSG msg;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOW;

    // The Unicode version of CreateProcess can modify the contents of this string.
    CStringW Tmp = Path;
    BOOL fSuccess = CreateProcessW(NULL, Tmp.GetBuffer(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    Tmp.ReleaseBuffer();
    if (!fSuccess)
    {
        return FALSE;
    }

    CloseHandle(pi.hThread);

    if (Wait)
    {
        EnableWindow(hMainWnd, FALSE);
    }

    while (Wait)
    {
        dwRet = MsgWaitForMultipleObjects(1, &pi.hProcess, FALSE, INFINITE, QS_ALLEVENTS);
        if (dwRet == WAIT_OBJECT_0 + 1)
        {
            while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
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

BOOL GetStorageDirectory(ATL::CStringW& Directory)
{
    static CStringW CachedDirectory;
    static BOOL CachedDirectoryInitialized = FALSE;

    if (!CachedDirectoryInitialized)
    {
        LPWSTR DirectoryStr = CachedDirectory.GetBuffer(MAX_PATH);
        BOOL bHasPath = SHGetSpecialFolderPathW(NULL, DirectoryStr, CSIDL_LOCAL_APPDATA, TRUE);
        if (bHasPath)
        {
            PathAppendW(DirectoryStr, L"rapps");
        }
        CachedDirectory.ReleaseBuffer();

        if (bHasPath)
        {
            if (!CreateDirectoryW(CachedDirectory, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
            {
                CachedDirectory.Empty();
            }
        }
        else
        {
            CachedDirectory.Empty();
        }

        CachedDirectoryInitialized = TRUE;
    }

    Directory = CachedDirectory;
    return !Directory.IsEmpty();
}

VOID InitLogs()
{
    if (!SettingsInfo.bLogEnabled)
    {
        return;
    }

    WCHAR szPath[MAX_PATH];
    DWORD dwCategoryNum = 1;
    DWORD dwDisp, dwData;
    ATL::CRegKey key;

    if (key.Create(HKEY_LOCAL_MACHINE,
                   L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\ReactOS Application Manager",
                   REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &dwDisp) != ERROR_SUCCESS)
    {
        return;
    }

    if (!GetModuleFileNameW(NULL, szPath, _countof(szPath)))
    {
        return;
    }

    dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE |
        EVENTLOG_INFORMATION_TYPE;

    if ((key.SetStringValue(L"EventMessageFile",
                            szPath,
                            REG_EXPAND_SZ) == ERROR_SUCCESS)
        && (key.SetStringValue(L"CategoryMessageFile",
                               szPath,
                               REG_EXPAND_SZ) == ERROR_SUCCESS)
        && (key.SetDWORDValue(L"TypesSupported",
                              dwData) == ERROR_SUCCESS)
        && (key.SetDWORDValue(L"CategoryCount",
                              dwCategoryNum) == ERROR_SUCCESS))

    {
        hLog = RegisterEventSourceW(NULL, L"ReactOS Application Manager");
    }
}


VOID FreeLogs()
{
    if (hLog)
    {
        DeregisterEventSource(hLog);
    }
}


BOOL WriteLogMessage(WORD wType, DWORD dwEventID, LPCWSTR lpMsg)
{
    if (!SettingsInfo.bLogEnabled)
    {
        return TRUE;
    }

    if (!ReportEventW(hLog, wType, 0, dwEventID,
                      NULL, 1, 0, &lpMsg, NULL))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL GetInstalledVersion_WowUser(ATL::CStringW* szVersionResult,
                                 const ATL::CStringW& szRegName,
                                 BOOL IsUserKey,
                                 REGSAM keyWow)
{
    BOOL bHasSucceded = FALSE;
    ATL::CRegKey key;
    ATL::CStringW szVersion;
    ATL::CStringW szPath = ATL::CStringW(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%ls") + szRegName;

    if (key.Open(IsUserKey ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE,
                 szPath.GetString(),
                 keyWow | KEY_READ) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    if (szVersionResult != NULL)
    {
        ULONG dwSize = MAX_PATH * sizeof(WCHAR);

        if (key.QueryStringValue(L"DisplayVersion",
                                 szVersion.GetBuffer(MAX_PATH),
                                 &dwSize) == ERROR_SUCCESS)
        {
            szVersion.ReleaseBuffer();
            *szVersionResult = szVersion;
            bHasSucceded = TRUE;
        }
        else
        {
            szVersion.ReleaseBuffer();
        }
    }
    else
    {
        bHasSucceded = TRUE;
        szVersion.ReleaseBuffer();
    }

    return bHasSucceded;
}

BOOL GetInstalledVersion(ATL::CStringW *pszVersion, const ATL::CStringW &szRegName)
{
    return (!szRegName.IsEmpty()
            && (GetInstalledVersion_WowUser(pszVersion, szRegName, TRUE, KEY_WOW64_32KEY)
                || GetInstalledVersion_WowUser(pszVersion, szRegName, FALSE, KEY_WOW64_32KEY)
                || GetInstalledVersion_WowUser(pszVersion, szRegName, TRUE, KEY_WOW64_64KEY)
                || GetInstalledVersion_WowUser(pszVersion, szRegName, FALSE, KEY_WOW64_64KEY)));
}

BOOL PathAppendNoDirEscapeW(LPWSTR pszPath, LPCWSTR pszMore)
{
    WCHAR pszPathBuffer[MAX_PATH]; // buffer to store result
    WCHAR pszPathCopy[MAX_PATH];

    if (!PathCanonicalizeW(pszPathCopy, pszPath))
    {
        return FALSE;
    }

    PathRemoveBackslashW(pszPathCopy);

    if (StringCchCopyW(pszPathBuffer, _countof(pszPathBuffer), pszPathCopy) != S_OK)
    {
        return FALSE;
    }

    if (!PathAppendW(pszPathBuffer, pszMore))
    {
        return FALSE;
    }

    size_t PathLen;
    if (StringCchLengthW(pszPathCopy, _countof(pszPathCopy), &PathLen) != S_OK)
    {
        return FALSE;
    }
    int CommonPrefixLen = PathCommonPrefixW(pszPathCopy, pszPathBuffer, NULL);

    if ((unsigned int)CommonPrefixLen != PathLen)
    {
        // pszPathBuffer should be a file/folder under pszPath.
        // but now common prefix len is smaller than length of pszPathCopy
        // hacking use ".." ?
        return FALSE;
    }

    if (StringCchCopyW(pszPath, MAX_PATH, pszPathBuffer) != S_OK)
    {
        return FALSE;
    }

    return TRUE;
}

BOOL IsSystem64Bit()
{
    if (bIsSys64ResultCached)
    {
        // just return cached result
        return bIsSys64Result;
    }

    SYSTEM_INFO si;
    typedef void (WINAPI *LPFN_PGNSI)(LPSYSTEM_INFO);
    LPFN_PGNSI pGetNativeSystemInfo = (LPFN_PGNSI)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GetNativeSystemInfo");
    if (pGetNativeSystemInfo)
    {
        pGetNativeSystemInfo(&si);
        if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
        {
            bIsSys64Result = TRUE;
        }
    }
    else
    {
        bIsSys64Result = FALSE;
    }

    bIsSys64ResultCached = TRUE; // next time calling this function, it will directly return bIsSys64Result
    return bIsSys64Result;
}

INT GetSystemColorDepth()
{
    DEVMODEW pDevMode;
    INT ColorDepth;

    pDevMode.dmSize = sizeof(pDevMode);
    pDevMode.dmDriverExtra = 0;

    if (!EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &pDevMode))
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

void UnixTimeToFileTime(DWORD dwUnixTime, LPFILETIME pFileTime)
{
    // Note that LONGLONG is a 64-bit value
    LONGLONG ll;

    ll = Int32x32To64(dwUnixTime, 10000000) + 116444736000000000;
    pFileTime->dwLowDateTime = (DWORD)ll;
    pFileTime->dwHighDateTime = ll >> 32;
}

BOOL SearchPatternMatch(LPCWSTR szHaystack, LPCWSTR szNeedle)
{
    if (!*szNeedle)
        return TRUE;
    /* TODO: Improve pattern search beyond a simple case-insensitive substring search. */
    return StrStrIW(szHaystack, szNeedle) != NULL;
}
