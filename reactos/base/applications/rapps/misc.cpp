/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/misc.cpp
 * PURPOSE:         Misc functions
 * PROGRAMMERS:     Dmitry Chapyshev           (dmitry@reactos.org)
 *                  Ismael Ferreras Morezuelas (swyterzone+ros@gmail.com)
 *                  Alexander Shaposhnikov     (chaez.san@gmail.com)
 */
#include "defines.h"

#include "gui.h"
#include "misc.h"

 /* SESSION Operation */
#define EXTRACT_FILLFILELIST  0x00000001
#define EXTRACT_EXTRACTFILES  0x00000002

static HANDLE hLog = NULL;

struct ERF
{
    INT erfOper;
    INT erfType;
    BOOL fError;
};

struct FILELIST
{
    LPSTR FileName;
    FILELIST *next;
    BOOL DoExtract;
};

struct SESSION
{
    INT FileSize;
    ERF Error;
    FILELIST *FileList;
    INT FileCount;
    INT Operation;
    CHAR Destination[MAX_PATH];
    CHAR CurrentFile[MAX_PATH];
    CHAR Reserved[MAX_PATH];
    FILELIST *FilterList;
};

typedef HRESULT(WINAPI *fnExtract)(SESSION *dest, LPCSTR szCabName);
fnExtract pfnExtract;

INT GetWindowWidth(HWND hwnd)
{
    RECT Rect;

    GetWindowRect(hwnd, &Rect);
    return (Rect.right - Rect.left);
}

INT GetWindowHeight(HWND hwnd)
{
    RECT Rect;

    GetWindowRect(hwnd, &Rect);
    return (Rect.bottom - Rect.top);
}

INT GetClientWindowWidth(HWND hwnd)
{
    RECT Rect;

    GetClientRect(hwnd, &Rect);
    return (Rect.right - Rect.left);
}

INT GetClientWindowHeight(HWND hwnd)
{
    RECT Rect;

    GetClientRect(hwnd, &Rect);
    return (Rect.bottom - Rect.top);
}

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

VOID SetWelcomeText()
{
    ATL::CStringW szText;

    szText.LoadStringW(hInst, IDS_WELCOME_TITLE);
    NewRichEditText(szText, CFE_BOLD);

    szText.LoadStringW(hInst, IDS_WELCOME_TEXT);
    InsertRichEditText(szText, 0);

    szText.LoadStringW(hInst, IDS_WELCOME_URL);
    InsertRichEditText(szText, CFM_LINK);
}

VOID ShowPopupMenu(HWND hwnd, UINT MenuID, UINT DefaultItem)
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
    TrackPopupMenu(hPopupMenu, 0, pt.x, pt.y, 0, hMainWnd, NULL);

    if (hMenu)
    {
        DestroyMenu(hMenu);
    }
}

BOOL StartProcess(ATL::CStringW &Path, BOOL Wait)
{
    return StartProcess(const_cast<LPWSTR>(Path.GetString()), Wait);;
}

BOOL StartProcess(LPWSTR lpPath, BOOL Wait)
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
    if (!SHGetSpecialFolderPathW(NULL, Directory.GetBuffer(MAX_PATH), CSIDL_LOCAL_APPDATA, TRUE))
    {
        Directory.ReleaseBuffer();
        return FALSE;
    }

    Directory.ReleaseBuffer();
    Directory += L"\\rapps";

    return (CreateDirectoryW(Directory.GetString(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS);
}

BOOL ExtractFilesFromCab(const ATL::CStringW &CabName, const ATL::CStringW &OutputPath)
{
    return ExtractFilesFromCab(CabName.GetString(), OutputPath.GetString());
}

BOOL ExtractFilesFromCab(LPCWSTR lpCabName, LPCWSTR lpOutputPath)
{
    HINSTANCE hCabinetDll;
    CHAR szCabName[MAX_PATH];
    SESSION Dest;
    HRESULT Result;

    hCabinetDll = LoadLibraryW(L"cabinet.dll");
    if (hCabinetDll)
    {
        pfnExtract = (fnExtract) GetProcAddress(hCabinetDll, "Extract");
        if (pfnExtract)
        {
            ZeroMemory(&Dest, sizeof(Dest));

            WideCharToMultiByte(CP_ACP, 0, lpOutputPath, -1, Dest.Destination, MAX_PATH, NULL, NULL);
            WideCharToMultiByte(CP_ACP, 0, lpCabName, -1, szCabName, MAX_PATH, NULL, NULL);
            Dest.Operation = EXTRACT_FILLFILELIST;

            Result = pfnExtract(&Dest, szCabName);
            if (Result == S_OK)
            {
                Dest.Operation = EXTRACT_EXTRACTFILES;
                CreateDirectoryW(lpOutputPath, NULL);

                Result = pfnExtract(&Dest, szCabName);
                if (Result == S_OK)
                {
                    FreeLibrary(hCabinetDll);
                    return TRUE;
                }
                else
                {
                    RemoveDirectoryW(lpOutputPath);
                }
            }
        }
        FreeLibrary(hCabinetDll);
    }

    return FALSE;
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

    key.Close();
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
                                 const ATL::CStringW& RegName,
                                 BOOL IsUserKey,
                                 REGSAM keyWow)
{
    BOOL bHasSucceded = FALSE;
    ATL::CRegKey key;
    ATL::CStringW szVersion;
    ATL::CStringW szPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + RegName;

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
    key.Close();

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

// CConfigParser
ATL::CStringW CConfigParser::m_szLocaleID;
ATL::CStringW CConfigParser::m_szCachedINISectionLocale;
ATL::CStringW CConfigParser::m_szCachedINISectionLocaleNeutral;

CConfigParser::CConfigParser(const ATL::CStringW& FileName) : szConfigPath(GetINIFullPath(FileName))
{
    // we don't have cached section strings for the current system language, create them, lazy
    CacheINILocaleLazy();
}

ATL::CStringW CConfigParser::GetINIFullPath(const ATL::CStringW& FileName)
{
    ATL::CStringW szDir;
    ATL::CStringW szBuffer;

    GetStorageDirectory(szDir);
    szBuffer.Format(L"%ls\\rapps\\%ls", szDir, FileName);

    return szBuffer;
}

VOID CConfigParser::CacheINILocaleLazy()
{
    if (m_szLocaleID.IsEmpty())
    {
        // TODO: Set default locale if call fails
        // find out what is the current system lang code (e.g. "0a") and append it to SectionLocale
        GetLocaleInfoW(GetUserDefaultLCID(), LOCALE_ILANGUAGE,
                       m_szLocaleID.GetBuffer(m_cchLocaleSize), m_cchLocaleSize);

        m_szLocaleID.ReleaseBuffer();
        m_szCachedINISectionLocale = L"Section." + m_szLocaleID;

        // turn "Section.0c0a" into "Section.0a", keeping just the neutral lang part
        m_szCachedINISectionLocaleNeutral = m_szCachedINISectionLocale + m_szLocaleID.Right(2);
    }
}

const ATL::CStringW& CConfigParser::GetLocale()
{
    CacheINILocaleLazy();
    return m_szLocaleID;
}

INT CConfigParser::GetLocaleSize()
{
    return m_cchLocaleSize;
}

UINT CConfigParser::GetString(const ATL::CStringW& KeyName, ATL::CStringW& ResultString)
{
    DWORD dwResult;

    LPWSTR ResultStringBuffer = ResultString.GetBuffer(MAX_PATH);
    // 1st - find localized strings (e.g. "Section.0c0a")
    dwResult = GetPrivateProfileStringW(m_szCachedINISectionLocale.GetString(),
                                        KeyName.GetString(),
                                        NULL,
                                        ResultStringBuffer,
                                        MAX_PATH,
                                        szConfigPath.GetString());

    if (!dwResult)
    {
        // 2nd - if they weren't present check for neutral sub-langs/ generic translations (e.g. "Section.0a")
        dwResult = GetPrivateProfileStringW(m_szCachedINISectionLocaleNeutral.GetString(),
                                            KeyName.GetString(),
                                            NULL,
                                            ResultStringBuffer,
                                            MAX_PATH,
                                            szConfigPath.GetString());
        if (!dwResult)
        {
            // 3rd - if they weren't present fallback to standard english strings (just "Section")
            dwResult = GetPrivateProfileStringW(L"Section",
                                                KeyName.GetString(),
                                                NULL,
                                                ResultStringBuffer,
                                                MAX_PATH,
                                                szConfigPath.GetString());
        }
    }

    ResultString.ReleaseBuffer();
    return (dwResult != 0 ? TRUE : FALSE);
}

UINT CConfigParser::GetInt(const ATL::CStringW& KeyName)
{
    ATL::CStringW Buffer;

    // grab the text version of our entry
    if (!GetString(KeyName, Buffer))
        return FALSE;

    if (Buffer.IsEmpty())
        return FALSE;

    // convert it to an actual integer
    INT result = StrToIntW(Buffer.GetString());

    return (UINT) (result <= 0) ? 0 : result;
}
// CConfigParser
