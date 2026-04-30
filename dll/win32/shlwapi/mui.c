/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     MUI (multilingual user interface)
 * COPYRIGHT:   Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"
#include <winreg.h>
#include <winver.h>
#include <htmlhelp.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <strsafe.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(shell);

/* See https://www.geoffchappell.com/studies/windows/shell/shlwapi/api/mlui/index.htm */

typedef struct PUIITEM
{
    HMODULE hModule;
    DWORD wLangId;
} PUIITEM, *PPUIITEM, *LPPUIITEM;

CRITICAL_SECTION g_csMuiLock;
static HDPA g_hdpaPUI = NULL;
static WCHAR g_szMuiDest[MAX_PATH] = L"";
static LANGID g_wGotLangId = 0;
static BOOL g_bIECheckVersion = FALSE;
static BOOL g_bIEVersionChecked = FALSE;
static UINT g_nACP = CP_ACP;
static BOOL g_bGotACP = FALSE;
HINSTANCE g_mluiInfo = NULL;

// Initialize the global DPA list used to track MUI-loaded module instances.
static BOOL InitPUI_NoLock(VOID)
{
    BOOL ret;

    if (!g_hdpaPUI)
        g_hdpaPUI = DPA_Create(4);
    ret = !!g_hdpaPUI;

    return ret;
}

static BOOL InitPUI(VOID)
{
    BOOL ret;

    EnterCriticalSection(&g_csMuiLock);
    ret = InitPUI_NoLock();
    LeaveCriticalSection(&g_csMuiLock);

    return ret;
}

static VOID DeinitPUI_NoLock(HDPA hDPA)
{
    INT iItem, cItems;

    if (!hDPA)
        return;

    cItems = DPA_GetPtrCount(hDPA);
    for (iItem = 0; iItem < cItems; ++iItem)
        LocalFree(DPA_GetPtr(hDPA, iItem));

    DPA_Destroy(hDPA);
}

VOID DeinitPUI(VOID)
{
    EnterCriticalSection(&g_csMuiLock);
    DeinitPUI_NoLock(g_hdpaPUI);
    g_hdpaPUI = NULL;
    LeaveCriticalSection(&g_csMuiLock);
}

// Search the PUI list for an entry matching the given instance handle and return its index.
static INT GetPUIITEM_NoLock(_In_ HMODULE hModule)
{
    INT cItems, iItem;
    PPUIITEM pItem;

    if (!InitPUI_NoLock())
        return -1;

    cItems = DPA_GetPtrCount(g_hdpaPUI);
    if (cItems <= 0)
        return -1;

    for (iItem = 0; iItem < cItems; ++iItem)
    {
        pItem = DPA_GetPtr(g_hdpaPUI, iItem);
        if (pItem && pItem->hModule == hModule)
            return iItem;
    }

    return -1;
}

// Return TRUE if the given language ID requires munging (i.e. its ANSI code
// page differs from the system ACP).
static BOOL ShouldMungeLangId(_In_ LANGID wLangId)
{
    CHAR szText[8];
    LANGID wSysUILangId;

    wSysUILangId = GetSystemDefaultUILanguage();
    if (wLangId == MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US) || wLangId == wSysUILangId)
        return FALSE;

    if (!g_bGotACP)
    {
        g_nACP = GetACP();
        g_bGotACP = TRUE;
    }

    if (!GetLocaleInfoA(wLangId, LOCALE_IDEFAULTANSICODEPAGE, szText, _countof(szText)))
        return FALSE;

    return g_nACP != StrToIntA(szText);
}

// Resolve a normalized language ID from dwCrossCodePage, falling back to the system
// UI language if munging is required.
static LANGID GetNormalizedLangId(_In_ DWORD dwCrossCodePage)
{
    LANGID wLangId = GetUserDefaultUILanguage();
    if (!(dwCrossCodePage & ML_CROSSCODEPAGE_MASK) && ShouldMungeLangId(wLangId))
        return GetSystemDefaultUILanguage();
    return wLangId;
}

// Build the MUI-localized path for an IE file by looking up the IE install
// directory in the registry.
static HRESULT GetMUIPath(
    _Out_writes_(cchDest) PWSTR pszDest,
    _In_ INT cchDest,
    _In_ PCWSTR pszFileName,
    _In_ LANGID wLangId)
{
    WCHAR szIEDir[MAX_PATH];
    DWORD cbData;
    PCWSTR pszFileNameOnly;
    PWSTR pszLastSep;
    LSTATUS error;
    static const PCWSTR szIExploreAppPath =
        L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\iexplore.exe";

    pszFileNameOnly = pszFileName;
    pszLastSep = StrRChrW(pszFileName, NULL, L'\\');
    if (pszLastSep)
        pszFileNameOnly = pszLastSep + 1;

    EnterCriticalSection(&g_csMuiLock);

    if (!g_szMuiDest[0] || g_wGotLangId != wLangId)
    {
        g_wGotLangId = wLangId;

        szIEDir[0] = UNICODE_NULL;
        cbData = sizeof(szIEDir);
        error = SHGetValueW(HKEY_LOCAL_MACHINE, szIExploreAppPath,
                            NULL, NULL, szIEDir, &cbData);
        if (error == ERROR_SUCCESS)
        {
            PathRemoveFileSpecW(szIEDir);
        }
        else
        {
            szIEDir[0] = L'0';
            szIEDir[1] = UNICODE_NULL;
        }

        StringCchPrintfW(g_szMuiDest, _countof(g_szMuiDest), L"%s\\mui\\%04x\\", szIEDir, wLangId);
    }

    StrCpyNW(pszDest, g_szMuiDest, cchDest);
    StrCatBuffW(pszDest, pszFileNameOnly, cchDest);

    LeaveCriticalSection(&g_csMuiLock);
    return S_OK;
}

// Parse a "major.minor.build.revision" version string into two DWORDs compatible
// with VS_FIXEDFILEINFO.
static void
ConvertVersionStrToDwords(
    _In_ PCSTR pszSrc,
    _Out_ PDWORD pdwVersionMS,
    _Out_ PDWORD pdwVersionLS)
{
    WORD parts[4];
    PCSTR dot, pch;
    INT i;

    *pdwVersionMS = *pdwVersionLS = 0;

    parts[0] = parts[1] = parts[2] = parts[3] = 0;
    pch = pszSrc;

    for (i = 0; i < 4 && pch; ++i)
    {
        parts[i] = (WORD)StrToIntA(pch);
        dot = StrChrA(pch, '.');
        pch = dot ? dot + 1 : NULL;
    }

    *pdwVersionMS = MAKELONG(parts[1], parts[0]);
    *pdwVersionLS = MAKELONG(parts[3], parts[2]);
}

// Check whether a MUI DLL's version falls within the compatible range recorded in
// the IE registry settings.
static BOOL IsMUICompatible(_In_ DWORD versionMS, _In_ DWORD versionLS)
{
    CHAR szModulePath[MAX_PATH], szVerRange[MAX_PATH];
    PCSTR pszFileName, minStr, maxStr;
    DWORD type, cbData;
    PSTR dash;
    DWORD minMS, minLS, maxMS, maxLS;
    ULONGLONG version, minVer, maxVer;

    if (!GetModuleFileNameA(NULL, szModulePath, ARRAYSIZE(szModulePath)))
        return FALSE;

    pszFileName = PathFindFileNameA(szModulePath);

    type = REG_NONE;
    cbData = sizeof(szVerRange);
    if (SHRegGetUSValueA("Software\\Microsoft\\Internet Explorer\\International",
                         pszFileName,
                         &type,
                         szVerRange,
                         &cbData,
                         TRUE,   // ignore HKCU/HKLM priority
                         NULL,
                         0) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    dash = StrChrA(szVerRange, '-');
    if (!dash || !dash[1])
        return FALSE;

    *dash = ANSI_NULL;
    minStr = szVerRange;
    maxStr = dash + 1;

    ConvertVersionStrToDwords(minStr, &minMS, &minLS);
    ConvertVersionStrToDwords(maxStr, &maxMS, &maxLS);

    version = ((ULONGLONG)versionMS << 32) | versionLS;
    minVer  = ((ULONGLONG)minMS << 32) | minLS;
    maxVer  = ((ULONGLONG)maxMS << 32) | maxLS;
    return (minVer <= version && version <= maxVer);
}

static BOOL CheckFileVersion(_In_ PCWSTR pszFileName, _In_ PCWSTR pszMuiPath)
{
    DWORD handle1, handle2;
    DWORD size1, size2;
    PVOID ver1, ver2;
    VS_FIXEDFILEINFO *ffi1, *ffi2;
    UINT len1, len2;
    DWORD fileVerMS1, fileVerLS1, fileVerMS2, fileVerLS2;
    BOOL ret = FALSE;

    if (!pszFileName || !pszMuiPath)
        return FALSE;

    handle1 = handle2 = 0;
    size1 = GetFileVersionInfoSizeW(pszFileName, &handle1);
    size2 = GetFileVersionInfoSizeW(pszMuiPath, &handle2);
    if (!size1 || !size2)
        return FALSE;

    ver1 = LocalAlloc(LPTR, size1);
    ver2 = LocalAlloc(LPTR, size2);
    if (!ver1 || !ver2)
        goto Cleanup;

    if (!GetFileVersionInfoW(pszFileName, handle1, size1, ver1) ||
        !GetFileVersionInfoW(pszMuiPath, handle2, size2, ver2))
    {
        goto Cleanup;
    }

    ffi1 = ffi2 = NULL;
    len1 = len2 = 0;
    if (!VerQueryValueW(ver1, L"\\", (LPVOID*)&ffi1, &len1) ||
        !VerQueryValueW(ver2, L"\\", (LPVOID*)&ffi2, &len2))
    {
        goto Cleanup;
    }

    fileVerMS1 = ffi1->dwFileVersionMS;
    fileVerLS1 = ffi1->dwFileVersionLS;
    fileVerMS2 = ffi2->dwFileVersionMS;
    fileVerLS2 = ffi2->dwFileVersionLS;

    if ((fileVerMS1 == fileVerMS2 && fileVerLS1 == fileVerLS2) ||
        IsMUICompatible(fileVerMS2, fileVerLS2))
    {
        ret = TRUE;
    }

Cleanup:
    LocalFree(ver2);
    LocalFree(ver1);
    return ret;
}

static HRESULT
GetFilePathFromLangId(
    PCWSTR lpszSourcePath,
    PWSTR lpszOutPath,
    INT cchOutPath,
    LCID lcid)
{
    HRESULT hr = S_OK;
    LPCWSTR lpszResolved = lpszSourcePath;
    INT cchCopy = MAX_PATH;
    WCHAR szMUIPath[MAX_PATH];
    LANGID wLangId;

    if (!lpszSourcePath || lpszSourcePath[0] == L'>')
        return E_FAIL;

    wLangId = GetNormalizedLangId(lcid);
    if (wLangId != 0 && GetSystemDefaultUILanguage() != wLangId)
    {
        GetMUIPath(szMUIPath, _countof(szMUIPath), lpszSourcePath, wLangId);
        lpszResolved = szMUIPath;
    }

    if (cchOutPath < MAX_PATH)
        cchCopy = cchOutPath;

    StringCchCopyW(lpszOutPath, cchCopy, lpszResolved);
    return hr;
}

/*************************************************************************
 * MLLoadLibraryA [SHLWAPI.377]
 *
 * ANSI wrapper for MLLoadLibraryW: converts the file name to Unicode before delegating.
 */
HMODULE WINAPI
MLLoadLibraryA(
    _In_ LPCSTR lpszLibFileName,
    _In_ HMODULE hModule,
    _In_ DWORD dwCrossCodePage)
{
    WCHAR szBuff[MAX_PATH];
    SHAnsiToUnicode(lpszLibFileName, szBuff, _countof(szBuff));
    return MLLoadLibraryW(szBuff, hModule, dwCrossCodePage);
}

/*************************************************************************
 * MLLoadLibraryW [SHLWAPI.378]
 *
 * Load the best available MUI-localized DLL for the given file name, with
 * multi-level language fallback.
 * @see https://www.geoffchappell.com/studies/windows/shell/shlwapi/api/mlui/load.htm
 */
HMODULE WINAPI
MLLoadLibraryW(
    _In_ LPCWSTR lpszLibFileName,
    _In_ HMODULE hModule,
    _In_ DWORD dwCrossCodePage)
{
    LANGID wLangId, wSysLangId;
    WCHAR szModPath[MAX_PATH], szMuiPath[MAX_PATH];
    PCWSTR pszLoadPath;
    HMODULE hinstLoaded;

    if (!lpszLibFileName)
        return NULL;

    EnterCriticalSection(&g_csMuiLock);
    if (!g_bIEVersionChecked)
    {
        g_bIEVersionChecked = TRUE;
        g_bIECheckVersion = SHRegGetBoolUSValueA(
            "Software\\Microsoft\\Internet Explorer\\International",
            "CheckVersion", TRUE, TRUE);
    }
    LeaveCriticalSection(&g_csMuiLock);

    wLangId = GetNormalizedLangId(dwCrossCodePage);
    wSysLangId = GetSystemDefaultUILanguage();

    szModPath[0] = szMuiPath[0] = UNICODE_NULL;
    pszLoadPath = NULL;

    if (hModule && GetModuleFileNameW(hModule, szModPath, _countof(szModPath)))
    {
        PathRemoveFileSpecW(szModPath);
        PathAppendW(szModPath, lpszLibFileName);

        if (wLangId == wSysLangId)
            pszLoadPath = szModPath;
    }

    if (!pszLoadPath)
    {
        GetMUIPath(szMuiPath, _countof(szMuiPath), lpszLibFileName, wLangId);
        pszLoadPath = szMuiPath;
    }

    if (g_bIECheckVersion && szModPath[0] && szMuiPath[0] &&
        !CheckFileVersion(szModPath, szMuiPath))
    {
        pszLoadPath = szModPath;
        wLangId = wSysLangId;
    }

    hinstLoaded = LoadLibraryW(pszLoadPath);
    if (!hinstLoaded)
    {
        if (pszLoadPath != szModPath)
        {
            wLangId = wSysLangId;
            hinstLoaded = LoadLibraryW(szModPath);
        }

        if (!hinstLoaded)
            hinstLoaded = LoadLibraryW(lpszLibFileName);
    }

    MLSetMLHInstance(hinstLoaded, wLangId);
    return hinstLoaded;
}

/*************************************************************************
 * MLFreeLibrary [SHLWAPI.418]
 *
 * Unregister the instance from the MUI tracking list and free the underlying library.
 * @see https://www.geoffchappell.com/studies/windows/shell/shlwapi/api/mlui/free.htm
 */
BOOL WINAPI MLFreeLibrary(_In_ HMODULE hModule)
{
    MLClearMLHInstance(hModule);
    return FreeLibrary(hModule);
}

/*************************************************************************
 * MLBuildResURLA [SHLWAPI.405]
 *
 * ANSI wrapper for MLBuildResURLW: converts all string arguments to Unicode before delegating.
 */
HRESULT WINAPI
MLBuildResURLA(
    _In_ PCSTR pszLibName,
    _In_ HMODULE hModule,
    _In_ DWORD dwCrossCodePage,
    _In_ PCSTR pszRes,
    _Out_writes_(dwDestLen) PSTR pszDest,
    _In_ INT cchDest)
{
    HRESULT hr;
    WCHAR szLibNameW[MAX_PATH], szResW[MAX_PATH], szDestW[MAX_PATH];

    SHAnsiToUnicode(pszLibName, szLibNameW, _countof(szLibNameW));
    SHAnsiToUnicode(pszRes, szResW, _countof(szResW));

    hr = MLBuildResURLW(szLibNameW, hModule, dwCrossCodePage, szResW, szDestW, _countof(szDestW));

    SHUnicodeToAnsi(szDestW, pszDest, cchDest);
    return hr;
}

/*************************************************************************
 * MLBuildResURLW [SHLWAPI.406]
 *
 * Construct a "res://<DLLPath>/<resource>" URL for the MUI-localized version of the given library.
 */
HRESULT WINAPI
MLBuildResURLW(
    _In_ PCWSTR pszLibName,
    _In_ HMODULE hModule,
    _In_ DWORD dwCrossCodePage,
    _In_ PCWSTR pszRes,
    _Out_writes_(dwDestLen) PWSTR pszDest,
    _In_ size_t cchDest)
{
    HRESULT hr;
    static const WCHAR k_szResPrefix[] = L"res://";
    INT cchPrefix;
    PWSTR pszCursor;
    size_t cchRemain, cchDllPath, cchResName;
    HMODULE hMui;
    WCHAR szDllPath[MAX_PATH];
    BOOL bGotPath;

    if (!pszLibName || !hModule || hModule == INVALID_HANDLE_VALUE ||
        (dwCrossCodePage != ML_NO_CROSSCODEPAGE && dwCrossCodePage != ML_CROSSCODEPAGE) ||
        !pszRes || !pszDest)
    {
        return E_INVALIDARG;
    }

    hr = E_FAIL;

    cchPrefix = lstrlenW(k_szResPrefix);
    if (cchDest < cchPrefix + 1)
        goto cleanup;

    StrCpyNW(pszDest, k_szResPrefix, cchDest);

    pszCursor = pszDest + cchPrefix;
    cchRemain = cchDest - cchPrefix;

    hMui = MLLoadLibraryW(pszLibName, hModule, dwCrossCodePage);
    if (!hMui)
        goto cleanup;

    szDllPath[0] = UNICODE_NULL;
    bGotPath = GetModuleFileNameW(hMui, szDllPath, _countof(szDllPath));
    MLFreeLibrary(hMui);

    if (!bGotPath)
        goto cleanup;

    cchDllPath = wcslen(szDllPath);
    cchResName = wcslen(pszRes);
    if (cchRemain < cchDllPath + 1 + cchResName + 1)
        goto cleanup;

    StrCpyNW(pszCursor, szDllPath, cchRemain);
    pszCursor += cchDllPath;
    cchRemain -= cchDllPath;

    *pszCursor = L'/';
    StrCpyNW(pszCursor + 1, pszRes, cchRemain - 1);

    return S_OK;

cleanup:
    *pszDest = UNICODE_NULL;
    return hr;
}

/*************************************************************************
 * MLIsMLHInstance [SHLWAPI.429]
 *
 * Return TRUE if the given instance handle is registered in the MUI tracking list.
 * @see https://www.geoffchappell.com/studies/windows/shell/shlwapi/api/mlui/is.htm?ts=0,100
 */
BOOL WINAPI MLIsMLHInstance(_In_ HINSTANCE hInstance)
{
    INT iItem;
    EnterCriticalSection(&g_csMuiLock);
    iItem = GetPUIITEM_NoLock(hInstance);
    LeaveCriticalSection(&g_csMuiLock);
    return iItem >= 0;
}

/*************************************************************************
 * MLSetMLHInstance [SHLWAPI.430]
 *
 * Register a loaded MUI instance and its language ID in the global tracking list.
 * @see https://www.geoffchappell.com/studies/windows/shell/shlwapi/api/mlui/set.htm
 */
HRESULT WINAPI MLSetMLHInstance(_In_ HINSTANCE hInstance, _In_ LANGID wLangId)
{
    INT iInserted;
    PPUIITEM pItem;

    if (!hInstance)
        return E_INVALIDARG;

    if (!InitPUI())
        return E_OUTOFMEMORY;

    pItem = LocalAlloc(LPTR, sizeof(PUIITEM));
    if (!pItem)
        return E_OUTOFMEMORY;

    pItem->hModule = hInstance;
    pItem->wLangId = wLangId;

    EnterCriticalSection(&g_csMuiLock);
    iInserted = DPA_AppendPtr(g_hdpaPUI, pItem);
    LeaveCriticalSection(&g_csMuiLock);

    if (iInserted == DPA_ERR)
    {
        LocalFree(pItem);
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

/*************************************************************************
 * MLClearMLHInstance [SHLWAPI.431]
 *
 * Remove a MUI instance entry from the tracking list and free its associated data.
 * @see https://www.geoffchappell.com/studies/windows/shell/shlwapi/api/mlui/clear.htm
 */
HRESULT WINAPI MLClearMLHInstance(_In_ HINSTANCE hInstance)
{
    INT iItem;
    EnterCriticalSection(&g_csMuiLock);
    iItem = GetPUIITEM_NoLock(hInstance);
    if (iItem >= 0)
    {
        PPUIITEM pItem = DPA_GetPtr(g_hdpaPUI, iItem);
        LocalFree(pItem);
        DPA_DeletePtr(g_hdpaPUI, iItem);
    }
    LeaveCriticalSection(&g_csMuiLock);
    return S_OK;
}

/*************************************************************************
 * MLWinHelpA [SHLWAPI.395]
 */
BOOL WINAPI
MLWinHelpA(
    _In_opt_ HWND hWndMain,
    _In_ LPCSTR lpszHelp,
    _In_ UINT uCommand,
    _In_ ULONG_PTR dwData)
{
    LPWSTR pszHelp = NULL;
    WCHAR szHelp[MAX_PATH];
    if (lpszHelp && SHAnsiToUnicode(lpszHelp, szHelp, _countof(szHelp)))
        pszHelp = szHelp;
    return MLWinHelpW(hWndMain, pszHelp, uCommand, dwData);
}

/*************************************************************************
 * MLHtmlHelpA [SHLWAPI.396]
 */
HWND WINAPI
MLHtmlHelpA(
    _In_opt_ HWND hwndCaller,
    _In_ LPCSTR pszFile,
    _In_ UINT uCommand,
    _In_ DWORD_PTR dwData,
    _In_ UINT wLangId)
{
    WCHAR szPathW[MAX_PATH];
    CHAR szPathA[MAX_PATH];
    PWSTR lpszPathW = NULL;

    if (uCommand != HH_DISPLAY_TOPIC && uCommand != HH_DISPLAY_TEXT_POPUP)
        return HtmlHelpA(hwndCaller, pszFile, uCommand, dwData);

    lpszPathW = NULL;
    if (pszFile)
    {
        SHAnsiToUnicode(pszFile, szPathW, MAX_PATH);
        lpszPathW = szPathW;
    }

    if (FAILED(GetFilePathFromLangId(lpszPathW, szPathW, MAX_PATH, wLangId)))
        return HtmlHelpA(hwndCaller, pszFile, uCommand, dwData);

    SHUnicodeToAnsi(szPathW, szPathA, MAX_PATH);
    return HtmlHelpA(hwndCaller, szPathA, uCommand, dwData);
}

/*************************************************************************
 * MLWinHelpW [SHLWAPI.397]
 */
BOOL WINAPI
MLWinHelpW(
    _In_opt_ HWND hWndMain,
    _In_ LPCWSTR lpszHelp,
    _In_ UINT uCommand,
    _In_ ULONG_PTR dwData)
{
    WCHAR szPath[MAX_PATH];
    if (GetFilePathFromLangId(lpszHelp, szPath, _countof(szPath), 0))
        return WinHelpW(hWndMain, lpszHelp, uCommand, dwData);
    else
        return WinHelpW(hWndMain, szPath, uCommand, dwData);
}

/*************************************************************************
 * MLHtmlHelpW [SHLWAPI.398]
 */
HWND WINAPI
MLHtmlHelpW(
    _In_opt_ HWND hwndCaller,
    _In_ LPCWSTR pszFile,
    _In_ UINT uCommand,
    _In_ DWORD_PTR dwData,
    _In_ UINT wLangId)
{
    WCHAR szPathW[MAX_PATH];

    if (uCommand != HH_DISPLAY_TOPIC && uCommand != HH_DISPLAY_TEXT_POPUP)
        return HtmlHelpW(hwndCaller, pszFile, uCommand, dwData);

    if (FAILED(GetFilePathFromLangId(pszFile, szPathW, MAX_PATH, wLangId)))
        return HtmlHelpW(hwndCaller, pszFile, uCommand, dwData);

    return HtmlHelpW(hwndCaller, szPathW, uCommand, dwData);
}
