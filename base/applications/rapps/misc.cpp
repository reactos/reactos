/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Misc functions
 * COPYRIGHT:   Copyright 2009 Dmitry Chapyshev (dmitry@reactos.org)
 *              Copyright 2015 Ismael Ferreras Morezuelas (swyterzone+ros@gmail.com)
 *              Copyright 2017 Alexander Shaposhnikov (sanchaez@reactos.org)
 */

#include "rapps.h"
#include "misc.h"

static HANDLE hLog = NULL;

UINT
ErrorBox(HWND hOwner, UINT Error)
{
    if (!Error)
        Error = ERROR_INTERNAL_ERROR; // Note: geninst.cpp depends on this
    WCHAR buf[400];
    UINT fmf = FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM;
    FormatMessageW(fmf, NULL, Error, 0, buf, _countof(buf), NULL);
    MessageBoxW(hOwner, buf, 0, MB_OK | MB_ICONSTOP);
    return Error;
}

VOID
CopyTextToClipboard(LPCWSTR lpszText)
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

    Buffer = (PWCHAR)GlobalLock(ClipBuffer);
    hr = StringCchCopyW(Buffer, cchBuffer, lpszText);
    GlobalUnlock(ClipBuffer);

    if (SUCCEEDED(hr))
        SetClipboardData(CF_UNICODETEXT, ClipBuffer);

    CloseClipboard();
}

static INT_PTR CALLBACK
NothingDlgProc(HWND hDlg, UINT uMsg, WPARAM, LPARAM)
{
    return uMsg == WM_CLOSE ? DestroyWindow(hDlg) : FALSE;
}

VOID
EmulateDialogReposition(HWND hwnd)
{
    static const DWORD DlgTmpl[] = { WS_POPUP | WS_CAPTION | WS_SYSMENU, 0, 0, 0, 0, 0 };
    HWND hDlg = CreateDialogIndirectW(NULL, (LPDLGTEMPLATE)DlgTmpl, NULL, NothingDlgProc);
    if (hDlg)
    {
        RECT r;
        GetWindowRect(hwnd, &r);
        if (SetWindowPos(hDlg, hDlg, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_NOZORDER | SWP_NOACTIVATE))
        {
            SendMessage(hDlg, DM_REPOSITION, 0, 0);
            if (GetWindowRect(hDlg, &r))
                SetWindowPos(hwnd, hwnd, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        SendMessage(hDlg, WM_CLOSE, 0, 0);
    }
}

VOID
ShowPopupMenuEx(HWND hwnd, HWND hwndOwner, UINT MenuID, UINT DefaultItem, POINT *Point)
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

    if (!Point)
    {
        GetCursorPos(Point = &pt);
    }

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hPopupMenu, 0, Point->x, Point->y, 0, hwndOwner, NULL);

    if (hMenu)
    {
        DestroyMenu(hMenu);
    }
}

extern BOOL IsZipFile(PCWSTR Path);

UINT
ClassifyFile(PCWSTR Path)
{
    const UINT share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    HANDLE hFile = CreateFileW(Path, GENERIC_READ, share, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        BYTE buf[8];
        DWORD io;
        if (!ReadFile(hFile, buf, sizeof(buf), &io, NULL) || io != sizeof(buf))
            buf[0] = 0;
        CloseHandle(hFile);

        if (buf[0] == 0xD0 && buf[1] == 0xCF && buf[2] == 0x11 && buf[3] == 0xE0 &&
            buf[4] == 0xA1 && buf[5] == 0xB1 && buf[6] == 0x1A && buf[7] == 0xE1)
        {
            return MAKEWORD('M', PERCEIVED_TYPE_APPLICATION); // MSI
        }
        if (buf[0] == 'M' || buf[0] == 'Z')
        {
            SHFILEINFO shfi;
            if (SHGetFileInfoW(Path, 0, &shfi, sizeof(shfi), SHGFI_EXETYPE))
                return MAKEWORD('E', PERCEIVED_TYPE_APPLICATION);
        }
        if (buf[0] == 'M' && buf[1] == 'S' && buf[2] == 'C' && buf[3] == 'F')
        {
            return MAKEWORD('C', PERCEIVED_TYPE_COMPRESSED); // CAB
        }
    }

    if (IsZipFile(Path)) // .zip last because we want to return SFX.exe with higher priority
    {
        return MAKEWORD('Z', PERCEIVED_TYPE_COMPRESSED);
    }
    return PERCEIVED_TYPE_UNKNOWN;
}

BOOL
OpensWithExplorer(PCWSTR Path)
{
    WCHAR szCmd[MAX_PATH * 2];
    DWORD cch = _countof(szCmd);
    PCWSTR pszExt = PathFindExtensionW(Path);
    HRESULT hr = AssocQueryStringW(ASSOCF_INIT_IGNOREUNKNOWN | ASSOCF_NOTRUNCATE,
                                   ASSOCSTR_COMMAND, pszExt, NULL, szCmd, &cch);
    if (SUCCEEDED(hr) && StrStrIW(szCmd, L" zipfldr.dll,")) // .zip
        return TRUE;
    PathRemoveArgsW(szCmd);
    return SUCCEEDED(hr) && !StrCmpIW(PathFindFileNameW(szCmd), L"explorer.exe"); // .cab
}

BOOL
StartProcess(const CStringW &Path, BOOL Wait)
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
        // We got the real activation message during MsgWaitForMultipleObjects while
        // we were disabled, we need to set the focus again now.
        SetFocus(hMainWnd);
    }

    return TRUE;
}

BOOL
GetStorageDirectory(CStringW &Directory)
{
    static CStringW CachedDirectory;
    static BOOL CachedDirectoryInitialized = FALSE;

    if (!CachedDirectoryInitialized)
    {
        LPWSTR DirectoryStr = CachedDirectory.GetBuffer(MAX_PATH);
        BOOL bHasPath = SHGetSpecialFolderPathW(NULL, DirectoryStr, CSIDL_LOCAL_APPDATA, TRUE);
        if (bHasPath)
        {
            PathAppendW(DirectoryStr, RAPPS_NAME);
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

VOID
InitLogs()
{
    if (!SettingsInfo.bLogEnabled)
    {
        return;
    }

    WCHAR szPath[MAX_PATH];
    DWORD dwCategoryNum = 1;
    DWORD dwDisp, dwData;
    ATL::CRegKey key;

    if (key.Create(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\ReactOS Application Manager", REG_NONE,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &dwDisp) != ERROR_SUCCESS)
    {
        return;
    }

    if (!GetModuleFileNameW(NULL, szPath, _countof(szPath)))
    {
        return;
    }

    dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;

    if ((key.SetStringValue(L"EventMessageFile", szPath, REG_EXPAND_SZ) == ERROR_SUCCESS) &&
        (key.SetStringValue(L"CategoryMessageFile", szPath, REG_EXPAND_SZ) == ERROR_SUCCESS) &&
        (key.SetDWORDValue(L"TypesSupported", dwData) == ERROR_SUCCESS) &&
        (key.SetDWORDValue(L"CategoryCount", dwCategoryNum) == ERROR_SUCCESS))

    {
        hLog = RegisterEventSourceW(NULL, L"ReactOS Application Manager");
    }
}

VOID
FreeLogs()
{
    if (hLog)
    {
        DeregisterEventSource(hLog);
    }
}

BOOL
WriteLogMessage(WORD wType, DWORD dwEventID, LPCWSTR lpMsg)
{
    if (!SettingsInfo.bLogEnabled)
    {
        return TRUE;
    }

    if (!ReportEventW(hLog, wType, 0, dwEventID, NULL, 1, 0, &lpMsg, NULL))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL
GetInstalledVersion_WowUser(CStringW *szVersionResult, const CStringW &szRegName, BOOL IsUserKey, REGSAM keyWow)
{
    BOOL bHasSucceded = FALSE;
    ATL::CRegKey key;
    CStringW szVersion;
    CStringW szPath = CStringW(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\") + szRegName;

    if (key.Open(IsUserKey ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, szPath.GetString(), keyWow | KEY_READ) !=
        ERROR_SUCCESS)
    {
        return FALSE;
    }

    if (szVersionResult != NULL)
    {
        ULONG dwSize = MAX_PATH * sizeof(WCHAR);

        if (key.QueryStringValue(L"DisplayVersion", szVersion.GetBuffer(MAX_PATH), &dwSize) == ERROR_SUCCESS)
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
    }

    return bHasSucceded;
}

BOOL
GetInstalledVersion(CStringW *pszVersion, const CStringW &szRegName)
{
    return (
        !szRegName.IsEmpty() && (GetInstalledVersion_WowUser(pszVersion, szRegName, TRUE, KEY_WOW64_32KEY) ||
                                 GetInstalledVersion_WowUser(pszVersion, szRegName, FALSE, KEY_WOW64_32KEY) ||
                                 GetInstalledVersion_WowUser(pszVersion, szRegName, TRUE, KEY_WOW64_64KEY) ||
                                 GetInstalledVersion_WowUser(pszVersion, szRegName, FALSE, KEY_WOW64_64KEY)));
}

BOOL
IsSystem64Bit()
{
#ifdef _WIN64
    return TRUE;
#else
    static UINT cache = 0;
    if (!cache)
        cache = 1 + (IsOS(OS_WOW6432) != FALSE);
    return cache - 1;
#endif
}

UINT
GetOsBuildNumber()
{
    static UINT build = 0;
    if (!build)
    {
        OSVERSIONINFOW ovi;
        ovi.dwOSVersionInfoSize = sizeof(ovi);
        GetVersionExW(&ovi);
        build = ovi.dwBuildNumber;
    }
    return build;
}

INT
GetSystemColorDepth()
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
        case 32:
            ColorDepth = ILC_COLOR32;
            break;
        case 24:
            ColorDepth = ILC_COLOR24;
            break;
        case 16:
            ColorDepth = ILC_COLOR16;
            break;
        case 8:
            ColorDepth = ILC_COLOR8;
            break;
        case 4:
            ColorDepth = ILC_COLOR4;
            break;
        default:
            ColorDepth = ILC_COLOR;
            break;
    }

    return ColorDepth;
}

void
UnixTimeToFileTime(DWORD dwUnixTime, LPFILETIME pFileTime)
{
    // Note that LONGLONG is a 64-bit value
    LONGLONG ll;

    ll = Int32x32To64(dwUnixTime, 10000000) + 116444736000000000;
    pFileTime->dwLowDateTime = (DWORD)ll;
    pFileTime->dwHighDateTime = ll >> 32;
}

HRESULT
RegKeyHasValues(HKEY hKey, LPCWSTR Path, REGSAM wowsam)
{
    CRegKey key;
    LONG err = key.Open(hKey, Path, KEY_QUERY_VALUE | wowsam);
    if (err == ERROR_SUCCESS)
    {
        WCHAR name[1];
        DWORD cchname = _countof(name), cbsize = 0;
        err = RegEnumValueW(key, 0, name, &cchname, NULL, NULL, NULL, &cbsize);
        if (err == ERROR_NO_MORE_ITEMS)
            return S_FALSE;
        if (err == ERROR_MORE_DATA)
            err = ERROR_SUCCESS;
    }
    return HRESULT_FROM_WIN32(err);
}

LPCWSTR
GetRegString(CRegKey &Key, LPCWSTR Name, CStringW &Value)
{
    for (;;)
    {
        ULONG cb = 0, cch;
        ULONG err = Key.QueryValue(Name, NULL, NULL, &cb);
        if (err)
            break;
        cch = cb / sizeof(WCHAR);
        LPWSTR p = Value.GetBuffer(cch + 1);
        p[cch] = UNICODE_NULL;
        err = Key.QueryValue(Name, NULL, (BYTE*)p, &cb);
        if (err == ERROR_MORE_DATA)
            continue;
        if (err)
            break;
        Value.ReleaseBuffer();
        return Value.GetString();
    }
    return NULL;
}

bool
ExpandEnvStrings(CStringW &Str)
{
    CStringW buf;
    DWORD cch = ExpandEnvironmentStringsW(Str, NULL, 0);
    if (cch)
    {
        if (ExpandEnvironmentStringsW(Str, buf.GetBuffer(cch), cch) == cch)
        {
            buf.ReleaseBuffer(cch - 1);
            Str = buf;
            return true;
        }
    }
    return false;
}

BOOL
SearchPatternMatch(LPCWSTR szHaystack, LPCWSTR szNeedle)
{
    if (!*szNeedle)
        return TRUE;
    /* TODO: Improve pattern search beyond a simple case-insensitive substring search. */
    return StrStrIW(szHaystack, szNeedle) != NULL;
}

BOOL
DeleteDirectoryTree(LPCWSTR Dir, HWND hwnd)
{
    CStringW from(Dir);
    UINT cch = from.GetLength();
    from.Append(L"00");
    LPWSTR p = from.GetBuffer();
    p[cch] = p[cch + 1] = L'\0'; // Double null-terminate
    UINT fof = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
    SHFILEOPSTRUCT shfos = { hwnd, FO_DELETE, p, NULL, (FILEOP_FLAGS)fof };
    return SHFileOperationW(&shfos);
}

UINT
CreateDirectoryTree(LPCWSTR Dir)
{
    UINT err = SHCreateDirectory(NULL, Dir);
    return err == ERROR_ALREADY_EXISTS ? 0 : err;
}

CStringW
SplitFileAndDirectory(LPCWSTR FullPath, CStringW *pDir)
{
    CPathW dir = FullPath;
    //int win = dir.ReverseFind(L'\\'), nix = dir.ReverseFind(L'/'), sep = max(win, nix);
    int sep = dir.FindFileName();
    CStringW file = dir.m_strPath.Mid(sep);
    if (pDir)
        *pDir = sep == -1 ? L"" : dir.m_strPath.Left(sep - 1);
    return file;
}

HRESULT
GetSpecialPath(UINT csidl, CStringW &Path, HWND hwnd)
{
    if (!SHGetSpecialFolderPathW(hwnd, Path.GetBuffer(MAX_PATH), csidl, TRUE))
        return E_FAIL;
    Path.ReleaseBuffer();
    return S_OK;
}

HRESULT
GetKnownPath(REFKNOWNFOLDERID kfid, CStringW &Path, DWORD Flags)
{
    PWSTR p;
    FARPROC f = GetProcAddress(LoadLibraryW(L"SHELL32"), "SHGetKnownFolderPath");
    if (!f)
        return HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION);
    HRESULT hr = ((HRESULT(WINAPI*)(REFKNOWNFOLDERID,UINT,HANDLE,PWSTR*))f)(kfid, Flags, NULL, &p);
    if (FAILED(hr))
        return hr;
    Path = p;
    CoTaskMemFree(p);
    return hr;
}

HRESULT
GetProgramFilesPath(CStringW &Path, BOOL PerUser, HWND hwnd)
{
    if (!PerUser)
        return GetSpecialPath(CSIDL_PROGRAM_FILES, Path, hwnd);

    HRESULT hr = GetKnownPath(FOLDERID_UserProgramFiles, Path);
    if (FAILED(hr))
    {
        hr = GetSpecialPath(CSIDL_LOCAL_APPDATA, Path, hwnd);
        // Use the correct path on NT6 (on NT5 the path becomes a bit long)
        if (SUCCEEDED(hr) && GetOsBuildNumber() >= 6000)
        {
            Path = BuildPath(Path, L"Programs"); // Should not be localized
        }
    }
    return hr;
}

static BOOL
ReadAt(HANDLE Handle, UINT Offset, LPVOID Buffer, UINT Size)
{
    OVERLAPPED ol = {};
    ol.Offset = Offset;
    DWORD cb;
    return ReadFile(Handle, Buffer, Size, &cb, &ol) && cb == Size;
}

InstallerType
GuessInstallerType(LPCWSTR Installer, UINT &ExtraInfo)
{
    ExtraInfo = 0;
    InstallerType it = INSTALLER_UNKNOWN;

    HANDLE hFile = CreateFileW(Installer, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return it;

    BYTE buf[0x30 + 12];
    if (!ReadAt(hFile, 0, buf, sizeof(buf)))
        goto done;

    if (buf[0] == 0xD0 && buf[1] == 0xCF && buf[2] == 0x11 && buf[3] == 0xE0)
    {
        if (!StrCmpIW(L".msi", PathFindExtensionW(Installer)))
        {
            it = INSTALLER_MSI;
            goto done;
        }
    }

    if (buf[0] != 'M' || buf[1] != 'Z')
        goto done;

    // <= Inno 5.1.4
    {
        UINT sig[3];
        C_ASSERT(sizeof(buf) >= 0x30 + sizeof(sig));
        CopyMemory(sig, buf + 0x30, sizeof(sig));
        if (sig[0] == 0x6F6E6E49 && sig[1] == ~sig[2])
        {
            it = INSTALLER_INNO;
            ExtraInfo = 1;
            goto done;
        }
    }

    // NSIS 1.6+
    {
        UINT nsis[1+1+3+1+1], nsisskip = 512;
        for (UINT nsisoffset = nsisskip * 20; nsisoffset < 1024 * 1024 * 50; nsisoffset += nsisskip)
        {
            if (ReadAt(hFile, nsisoffset, nsis, sizeof(nsis)) && nsis[1] == 0xDEADBEEF)
            {
                if (nsis[2] == 0x6C6C754E && nsis[3] == 0x74666F73 && nsis[4] == 0x74736E49)
                {
                    it = INSTALLER_NSIS;
                    goto done;
                }
            }
        }
    }

    if (HMODULE hMod = LoadLibraryEx(Installer, NULL, LOAD_LIBRARY_AS_DATAFILE))
    {
        // Inno
        enum { INNORCSETUPLDR = 11111 };
        HGLOBAL hGlob;
        HRSRC hRsrc = FindResourceW(hMod, MAKEINTRESOURCE(INNORCSETUPLDR), RT_RCDATA);
        if (hRsrc && (hGlob = LoadResource(hMod, hRsrc)) != NULL)
        {
            if (BYTE *p = (BYTE*)LockResource(hGlob))
            {
                if (p[0] == 'r' && p[1] == 'D' && p[2] == 'l' && p[3] == 'P' && p[4] == 't' && p[5] == 'S')
                    it = INSTALLER_INNO;
                UnlockResource((void*)p);
            }
        }
        FreeLibrary(hMod);
    }
done:
    CloseHandle(hFile);
    return it;
}

BOOL
GetSilentInstallParameters(InstallerType InstallerType, UINT ExtraInfo, LPCWSTR Installer, CStringW &Parameters)
{
    switch (InstallerType)
    {
        case INSTALLER_MSI:
        {
            Parameters.Format(L"/i \"%s\" /qn", Installer);
            return TRUE;
        }

        case INSTALLER_INNO:
        {
            LPCWSTR pszInnoParams = L"/SILENT /VERYSILENT /SP-";
            if (ExtraInfo)
                Parameters.Format(L"%s", pszInnoParams);
            else
                Parameters.Format(L"%s /SUPPRESSMSGBOXES", pszInnoParams); // https://jrsoftware.org/files/is5.5-whatsnew.htm
            return TRUE;
        }

        case INSTALLER_NSIS:
        {
            Parameters = L"/S";
            return TRUE;
        }

        default:
        {
            return FALSE;
        }
    }
}
