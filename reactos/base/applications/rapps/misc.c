/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/misc.c
 * PURPOSE:         Misc functions
 * PROGRAMMERS:     Dmitry Chapyshev           (dmitry@reactos.org)
 *                  Ismael Ferreras Morezuelas (swyterzone+ros@gmail.com)
 */

#include "rapps.h"
#include <sha1.h>

/* SESSION Operation */
#define EXTRACT_FILLFILELIST  0x00000001
#define EXTRACT_EXTRACTFILES  0x00000002

static HANDLE hLog = NULL;
WCHAR szCachedINISectionLocale[MAX_PATH] = L"Section.";
WCHAR szCachedINISectionLocaleNeutral[MAX_PATH] = {0};
BYTE bCachedSectionStatus = FALSE;

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
    HRESULT hr;

    if (OpenClipboard(NULL))
    {
        HGLOBAL ClipBuffer;
        WCHAR *Buffer;
        DWORD cchBuffer;

        EmptyClipboard();
        cchBuffer = wcslen(lpszText) + 1;
        ClipBuffer = GlobalAlloc(GMEM_DDESHARE, cchBuffer * sizeof(WCHAR));
        Buffer = GlobalLock(ClipBuffer);
        hr = StringCchCopyW(Buffer, cchBuffer, lpszText);
        GlobalUnlock(ClipBuffer);

        if (SUCCEEDED(hr))
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
ShowPopupMenu(HWND hwnd, UINT MenuID, UINT DefaultItem)
{
    HMENU hMenu = NULL;
    HMENU hPopupMenu;
    MENUITEMINFO mii;
    POINT pt;

    if (MenuID)
    {
        hMenu = LoadMenuW(hInst, MAKEINTRESOURCEW(MenuID));
        hPopupMenu = GetSubMenu(hMenu, 0);
    }
    else
        hPopupMenu = GetMenu(hwnd);

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STATE;
    GetMenuItemInfo(hPopupMenu, DefaultItem, FALSE, &mii);

    if (!(mii.fState & MFS_GRAYED))
        SetMenuDefaultItem(hPopupMenu, DefaultItem, FALSE);

    GetCursorPos(&pt);

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hPopupMenu, 0, pt.x, pt.y, 0, hMainWnd, NULL);

    if (hMenu)
        DestroyMenu(hMenu);
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
GetStorageDirectory(PWCHAR lpDirectory, DWORD cch)
{
    if (cch < MAX_PATH)
        return FALSE;

    if (!SHGetSpecialFolderPathW(NULL, lpDirectory, CSIDL_LOCAL_APPDATA, TRUE))
        return FALSE;

    if (FAILED(StringCchCatW(lpDirectory, cch, L"\\rapps")))
        return FALSE;

    if (!CreateDirectoryW(lpDirectory, NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS)
    {
        return FALSE;
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
    WCHAR szBuf[MAX_PATH] = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\ReactOS Application Manager";
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

    if (!GetModuleFileName(NULL, szPath, sizeof(szPath) / sizeof(szPath[0])))
        return;

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


LPWSTR GetINIFullPath(LPCWSTR lpFileName)
{
           WCHAR szDir[MAX_PATH];
    static WCHAR szBuffer[MAX_PATH];

    GetStorageDirectory(szDir, _countof(szDir));
    StringCbPrintfW(szBuffer, sizeof(szBuffer), L"%ls\\rapps\\%ls", szDir, lpFileName);

    return szBuffer;
}


UINT ParserGetString(LPCWSTR lpKeyName, LPWSTR lpReturnedString, UINT nSize, LPCWSTR lpFileName)
{
    PWSTR lpFullFileName = GetINIFullPath(lpFileName);
    DWORD dwResult;

    /* we don't have cached section strings for the current system language, create them */
    if(bCachedSectionStatus == FALSE)
    {
        WCHAR szLocale[4 + 1];
        DWORD len;

        /* find out what is the current system lang code (e.g. "0a") and append it to SectionLocale */
        GetLocaleInfoW(GetUserDefaultLCID(), LOCALE_ILANGUAGE,
                       szLocale, _countof(szLocale));

        StringCbCatW(szCachedINISectionLocale, sizeof(szCachedINISectionLocale), szLocale);

        /* copy the locale-dependent string into the buffer of the future neutral one */
        StringCbCopyW(szCachedINISectionLocaleNeutral,
                      sizeof(szCachedINISectionLocale),
                      szCachedINISectionLocale);

        /* turn "Section.0c0a" into "Section.0a", keeping just the neutral lang part */
        len = wcslen(szCachedINISectionLocale);

        memmove((szCachedINISectionLocaleNeutral + len) - 4,
                (szCachedINISectionLocaleNeutral + len) - 2,
                (2 * sizeof(WCHAR)) + sizeof(UNICODE_NULL));

        /* finally, mark us as cache-friendly for the next time */
        bCachedSectionStatus = TRUE;
    }

    /* 1st - find localized strings (e.g. "Section.0c0a") */
    dwResult = GetPrivateProfileStringW(szCachedINISectionLocale,
                                        lpKeyName,
                                        NULL,
                                        lpReturnedString,
                                        nSize,
                                        lpFullFileName);

    if (dwResult != 0)
        return TRUE;

    /* 2nd - if they weren't present check for neutral sub-langs/ generic translations (e.g. "Section.0a") */
    dwResult = GetPrivateProfileStringW(szCachedINISectionLocaleNeutral,
                                        lpKeyName,
                                        NULL,
                                        lpReturnedString,
                                        nSize,
                                        lpFullFileName);

    if (dwResult != 0)
        return TRUE;

    /* 3rd - if they weren't present fallback to standard english strings (just "Section") */
    dwResult = GetPrivateProfileStringW(L"Section",
                                        lpKeyName,
                                        NULL,
                                        lpReturnedString,
                                        nSize,
                                        lpFullFileName);

    return (dwResult != 0 ? TRUE : FALSE);
}

UINT ParserGetInt(LPCWSTR lpKeyName, LPCWSTR lpFileName)
{
    WCHAR Buffer[30];
    UNICODE_STRING BufferW;
    ULONG Result;

    /* grab the text version of our entry */
    if (!ParserGetString(lpKeyName, Buffer, _countof(Buffer), lpFileName))
        return FALSE;

    if (!Buffer[0])
        return FALSE;

    /* convert it to an actual integer */
    RtlInitUnicodeString(&BufferW, Buffer);
    RtlUnicodeStringToInteger(&BufferW, 0, &Result);

    return Result;
}

BOOL VerifyInteg(LPCWSTR lpSHA1Hash, LPCWSTR lpFileName)
{
    BOOL ret = FALSE;
    const unsigned char *file_map;
    HANDLE file, map;

    ULONG sha[5];
    WCHAR buf[40 + 1];
    SHA_CTX ctx;

    LARGE_INTEGER size;
    UINT i;

    /* first off, does it exist at all? */
    file = CreateFileW(lpFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);

    if (file == INVALID_HANDLE_VALUE)
        return FALSE;

    /* let's grab the actual file size to organize the mmap'ing rounds */
    GetFileSizeEx(file, &size);

    /* retrieve a handle to map the file contents to memory */
    map = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!map)
        goto cleanup;

    /* initialize the SHA-1 context */
    A_SHAInit(&ctx);

    /* map that thing in address space */
    file_map = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    if (!file_map)
        goto cleanup;

    /* feed the data to the cookie monster */
    A_SHAUpdate(&ctx, file_map, size.LowPart);

    /* cool, we don't need this anymore */
    UnmapViewOfFile(file_map);

    /* we're done, compute the final hash */
    A_SHAFinal(&ctx, sha);

    for (i = 0; i < sizeof(sha); i++)
        swprintf(buf + 2 * i, L"%02x", ((unsigned char *)sha)[i]);

    /* does the resulting SHA1 match with the provided one? */
    if (!_wcsicmp(buf, lpSHA1Hash))
        ret = TRUE;

cleanup:
    CloseHandle(map);
    CloseHandle(file);

    return ret;
}
