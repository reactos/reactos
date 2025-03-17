/*
 * PROJECT:     ReactOS Help View
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS-specific HTML-based help system
 * COPYRIGHT:   Copyright (c) 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

#include <winreg.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <assert.h>

static inline BOOL
HV_IsRegKeyAvailable(PCWSTR lpSubKey)
{
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, lpSubKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return TRUE;
    }
    return FALSE;
}

static inline BOOL
HV_IsWineGeckoAvailable(void)
{
    return HV_IsRegKeyAvailable(L"SOFTWARE\\Wine\\MSHTML");
}

/////////////////////////////////////////////////////////////////////////////////////////
// SHLWAPI loader

static inline HINSTANCE
HV_LoadSHLWAPI(void)
{
    static HINSTANCE s_hSHLWAPI = NULL;
    if (!s_hSHLWAPI)
        s_hSHLWAPI = LoadLibraryW(L"shlwapi.dll");
    return s_hSHLWAPI;
}

// function types
typedef BOOL (WINAPI *FN_PathFileExistsW)(PCWSTR);
typedef PWSTR (WINAPI *FN_PathFindFileNameW)(PCWSTR);
typedef void (WINAPI *FN_PathRemoveExtensionW)(PWSTR);
typedef HRESULT (WINAPI *FN_AssocQueryStringW)(ASSOCF, ASSOCSTR, PCWSTR, PCWSTR, PWSTR, PDWORD);

static inline BOOL
HV_PathFileExistsW(PCWSTR pszPath)
{
    static FN_PathFileExistsW s_fn = NULL;
    if (!s_fn)
        s_fn = (FN_PathFileExistsW)GetProcAddress(HV_LoadSHLWAPI(), "PathFileExistsW");
    if (s_fn)
        return s_fn(pszPath);
    assert(FALSE);
    return FALSE;
}

static inline PWSTR
HV_PathFindFileNameW(PCWSTR pszPath)
{
    static FN_PathFindFileNameW s_fn = NULL;
    if (!s_fn)
        s_fn = (FN_PathFindFileNameW)GetProcAddress(HV_LoadSHLWAPI(), "PathFindFileNameW");
    if (s_fn)
        return s_fn(pszPath);
    assert(FALSE);
    return (PWSTR)pszPath;
}

static inline void
HV_PathRemoveExtensionW(PWSTR pszPath)
{
    static FN_PathRemoveExtensionW s_fn = NULL;
    if (!s_fn)
        s_fn = (FN_PathRemoveExtensionW)GetProcAddress(HV_LoadSHLWAPI(), "PathRemoveExtensionW");
    if (s_fn)
        s_fn(pszPath);
    else
        assert(FALSE);
}

static inline HRESULT WINAPI
HV_AssocQueryStringW(
    ASSOCF cfFlags,
    ASSOCSTR str,
    PCWSTR pszAssoc,
    PCWSTR pszExtra,
    PWSTR pszOut,
    DWORD *pcchOut)
{
    static FN_AssocQueryStringW s_fn = NULL;
    if (!s_fn)
        s_fn = (FN_AssocQueryStringW)GetProcAddress(HV_LoadSHLWAPI(), "AssocQueryStringW");
    if (s_fn)
        return s_fn(cfFlags, str, pszAssoc, pszExtra, pszOut, pcchOut);
    assert(FALSE);
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////

static inline BOOL
HV_IsBrowserAvailable(void)
{
    // Check file association of *.htm
    ASSOCF af = ASSOCF_INIT_IGNOREUNKNOWN | ASSOCF_NOTRUNCATE;
    WCHAR szFile[MAX_PATH];
    DWORD cchFile = _countof(szFile);
    HRESULT hr = HV_AssocQueryStringW(af, ASSOCSTR_EXECUTABLE, L".htm", NULL, szFile, &cchFile);
    return SUCCEEDED(hr) && HV_PathFileExistsW(szFile) &&
           (lstrcmpiW(HV_PathFindFileNameW(szFile), L"iexplore.exe") != 0 ||
            HV_IsWineGeckoAvailable());
}

// Get string (such as "en-US") from locale ID
static inline BOOL
HV_GetIsoLangAndCountryName(PWSTR pszName, SIZE_T cchNameMax, LANGID LangID)
{
    WCHAR szLang[16], szCtry[16];
    if (!GetLocaleInfoW(LangID, LOCALE_SISO639LANGNAME, szLang, _countof(szLang)))
        return FALSE;
    if (!GetLocaleInfoW(LangID, LOCALE_SISO3166CTRYNAME, szCtry, _countof(szCtry)))
        return FALSE;
    StringCchPrintfW(pszName, cchNameMax, L"%s-%s", szLang, szCtry);
    return TRUE;
}

// Helper structure to find help file
typedef struct tagHV_FIND_HELP_FILE
{
    BOOL bFound;
    BOOL bHTML;
    WCHAR szHelpPath[MAX_PATH];
    WCHAR szFound[MAX_PATH];
    LANGID LangID;
} HV_FIND_HELP_FILE, *PHV_FIND_HELP_FILE;

// data storage inline function
static inline PHV_FIND_HELP_FILE
HV_GetFindHelpFileData(void)
{
    static HV_FIND_HELP_FILE s_data;
    return &s_data;
}

static inline BOOL
HV_FindHelpFile(PHV_FIND_HELP_FILE pFind)
{
    if (pFind->bFound)
        return TRUE; // Found

    // Copy path
    WCHAR szPath[MAX_PATH];
    StringCchCopyW(szPath, _countof(szPath), pFind->szHelpPath);

    // Add language suffix
    WCHAR szName[64];
    HV_GetIsoLangAndCountryName(szName, _countof(szName), pFind->LangID); // "en-US" etc.
    StringCchCatW(szPath, _countof(szPath), L"_");
    StringCchCatW(szPath, _countof(szPath), szName);

    // Add .htm or .txt
    StringCchCatW(szPath, _countof(szPath), (pFind->bHTML ? L".htm" : L".txt"));

    if (!HV_PathFileExistsW(szPath))
        return FALSE; // Not found

    pFind->bFound = TRUE;
    StringCchCopyW(pFind->szFound, _countof(pFind->szFound), szPath);
    return TRUE; // Found
}

// "Enumerate locales" procedure to find near help file
static inline BOOL CALLBACK
HV_EnumLocalesFindNearHelpFileProc(PWSTR lpLocaleString)
{
    PHV_FIND_HELP_FILE pFind = HV_GetFindHelpFileData();
    if (pFind->bFound)
        return FALSE; // Cancel since found

    // lpLocaleString is hexadecimal
    LCID lcid = wcstol(lpLocaleString, NULL, 16);
    LANGID targetLangID = LANGIDFROMLCID(lcid);
    if (PRIMARYLANGID(targetLangID) != PRIMARYLANGID(pFind->LangID))
        return TRUE; // Continue

    LANGID savedLangID = pFind->LangID;
    pFind->LangID = targetLangID;
    HV_FindHelpFile(pFind);
    pFind->LangID = savedLangID;

    return !pFind->bFound; // Continue if not found
}

static inline BOOL
HelpView(
    HWND hWndMain,
    PCWSTR lpszHelp,
    UINT uCommand,
    DWORD_PTR dwData)
{
    // Build help file pathname
    WCHAR szPath[MAX_PATH];
    GetWindowsDirectoryW(szPath, _countof(szPath));
    StringCchCatW(szPath, _countof(szPath), _T("\\Help\\"));
    StringCchCatW(szPath, _countof(szPath), lpszHelp);
    HV_PathRemoveExtensionW(szPath); // Delete .extension

    // Initialize pFind
    PHV_FIND_HELP_FILE pFind = HV_GetFindHelpFileData();
    pFind->bFound = FALSE;
    pFind->bHTML = HV_IsBrowserAvailable();
    StringCchCopyW(pFind->szHelpPath, _countof(pFind->szHelpPath), szPath);
    pFind->szFound[0] = UNICODE_NULL;
    pFind->LangID = GetUserDefaultLangID();

    // Try to find help file flexibly
    INT iTry;
    const LANGID EnglishUS = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    for (iTry = 0; iTry < 4; ++iTry)
    {
        if (!HV_FindHelpFile(pFind))
            EnumSystemLocalesW(HV_EnumLocalesFindNearHelpFileProc, LCID_SUPPORTED);

        if (pFind->bFound)
            break;

        if (iTry == 0 && pFind->bHTML)
        {
            pFind->bHTML = FALSE;
            continue;
        }

        if (pFind->LangID != EnglishUS)
        {
            pFind->LangID = EnglishUS;
            pFind->bHTML = TRUE;
            continue;
        }

        if (pFind->bHTML)
        {
            pFind->bHTML = FALSE;
            continue;
        }

        break;
    }

    if (pFind->bFound) // Found?
    {
        // Open the file
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.hwnd = hWndMain;
        sei.lpFile = pFind->szFound;
        sei.nShow = SW_SHOWNORMAL;
        return ShellExecuteExW(&sei);
    }

    // Fallback
    return WinHelpW(hWndMain, lpszHelp, uCommand, dwData);
}
