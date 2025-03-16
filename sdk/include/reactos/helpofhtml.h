/*
 * PROJECT:     ReactOS HelpOfHtml (HOH)
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS-specific help system
 * COPYRIGHT:   Copyright (c) 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

#ifdef ENABLE_HELP

#include <winreg.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <strsafe.h>

static inline BOOL
HOH_IsRegKeyAvailable(LPCWSTR lpSubKey)
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
HOH_IsWineGeckoAvailable(void)
{
    return HOH_IsRegKeyAvailable(L"SOFTWARE\\Wine\\MSHTML");
}

typedef struct tagHOH_LANG_NAME_AND_ID
{
    PCWSTR name;
    LANGID wLangId;
} HOH_LANG_NAME_AND_ID, *PHOH_LANG_NAME_AND_ID;

// Add language suffix
static inline void
HOH_AddLangSuffix(LPWSTR pszPath, SIZE_T cchPathMax, LANGID wLangId)
{
    static const HOH_LANG_NAME_AND_ID pairs[] =
    {
        // FIXME: Add more languages
        { L"bg-BG", MAKELANGID(LANG_BULGARIAN,     SUBLANG_DEFAULT) },
        { L"ca-ES", MAKELANGID(LANG_CATALAN,       SUBLANG_DEFAULT) },
        { L"cs-CZ", MAKELANGID(LANG_CZECH,         SUBLANG_DEFAULT) },
        { L"da-DK", MAKELANGID(LANG_DANISH,        SUBLANG_DEFAULT) },
        { L"de-DE", MAKELANGID(LANG_GERMAN,        SUBLANG_NEUTRAL) },
        { L"el-GR", MAKELANGID(LANG_GREEK,         SUBLANG_DEFAULT) },
        { L"en-US", MAKELANGID(LANG_ENGLISH,       SUBLANG_ENGLISH_US) },
        { L"es-ES", MAKELANGID(LANG_SPANISH,       SUBLANG_NEUTRAL) },
        { L"et-EE", MAKELANGID(LANG_ESTONIAN,      SUBLANG_DEFAULT) },
        { L"fr-FR", MAKELANGID(LANG_FRENCH,        SUBLANG_NEUTRAL) },
        { L"he-IL", MAKELANGID(LANG_HEBREW,        SUBLANG_DEFAULT) },
        { L"hr-HR", MAKELANGID(LANG_CROATIAN,      SUBLANG_CROATIAN_CROATIA) },
        { L"hu-HU", MAKELANGID(LANG_HUNGARIAN,     SUBLANG_DEFAULT) },
        { L"id-ID", MAKELANGID(LANG_INDONESIAN,    SUBLANG_DEFAULT) },
        { L"it-IT", MAKELANGID(LANG_ITALIAN,       SUBLANG_NEUTRAL) },
        { L"ja-JP", MAKELANGID(LANG_JAPANESE,      SUBLANG_DEFAULT) },
        { L"ko-KR", MAKELANGID(LANG_KOREAN,        SUBLANG_DEFAULT) },
        { L"lt-LT", MAKELANGID(LANG_LITHUANIAN,    SUBLANG_DEFAULT) },
        { L"nl-NL", MAKELANGID(LANG_DUTCH,         SUBLANG_NEUTRAL) },
        { L"no-NO", MAKELANGID(LANG_NORWEGIAN,     SUBLANG_NORWEGIAN_BOKMAL) },
        { L"pl-PL", MAKELANGID(LANG_POLISH,        SUBLANG_DEFAULT) },
        { L"pt-BR", MAKELANGID(LANG_PORTUGUESE,    SUBLANG_PORTUGUESE_BRAZILIAN) },
        { L"pt-PT", MAKELANGID(LANG_PORTUGUESE,    SUBLANG_PORTUGUESE) },
        { L"ru-RU", MAKELANGID(LANG_RUSSIAN,       SUBLANG_DEFAULT) },
        { L"sk-SK", MAKELANGID(LANG_SLOVAK,        SUBLANG_DEFAULT) },
        { L"sq-AL", MAKELANGID(LANG_ALBANIAN,      SUBLANG_NEUTRAL) },
        { L"sv-SE", MAKELANGID(LANG_SWEDISH,       SUBLANG_NEUTRAL) },
        { L"th-TH", MAKELANGID(LANG_THAI,          SUBLANG_DEFAULT) },
        { L"tr-TR", MAKELANGID(LANG_TURKISH,       SUBLANG_DEFAULT) },
        { L"uk-UA", MAKELANGID(LANG_UKRAINIAN,     SUBLANG_DEFAULT) },
        { L"zh-CN", MAKELANGID(LANG_CHINESE,       SUBLANG_CHINESE_SIMPLIFIED) },
        { L"zh-HK", MAKELANGID(LANG_CHINESE,       SUBLANG_CHINESE_HONGKONG) },
        { L"zh-TW", MAKELANGID(LANG_CHINESE,       SUBLANG_CHINESE_TRADITIONAL) },
    };
    const SIZE_T nPairs = _countof(pairs);

    // 1st try: Best match
    SIZE_T iPair;
    for (iPair = 0; iPair < nPairs; ++iPair)
    {
        if (pairs[iPair].wLangId == wLangId)
        {
            StringCchCatW(pszPath, cchPathMax, L"_");
            StringCchCatW(pszPath, cchPathMax, pairs[iPair].name);
            return;
        }
    }

    // 2nd try: PRIMARYLANGID match
    for (iPair = 0; iPair < nPairs; ++iPair)
    {
        if (PRIMARYLANGID(pairs[iPair].wLangId) == PRIMARYLANGID(wLangId))
        {
            StringCchCatW(pszPath, cchPathMax, L"_");
            StringCchCatW(pszPath, cchPathMax, pairs[iPair].name);
            return;
        }
    }

    // default is English
    StringCchCatW(pszPath, cchPathMax, L"_en-US");
}

static inline BOOL
HOH_IsBrowserAvailable(void)
{
    // Check file association of *.html
    ASSOCF af = ASSOCF_INIT_IGNOREUNKNOWN | ASSOCF_NOTRUNCATE;
    WCHAR szFile[MAX_PATH];
    DWORD cchFile = _countof(szFile);
    HRESULT hr = AssocQueryStringW(af, ASSOCSTR_EXECUTABLE, L".html", NULL, szFile, &cchFile);
    return SUCCEEDED(hr) && PathFileExistsW(szFile) &&
           (lstrcmpiW(PathFindFileNameW(szFile), L"iexplore.exe") != 0 ||
            HOH_IsWineGeckoAvailable());
}

static inline BOOL
HelpOfHtml(
    HWND hWndMain,
    LPCWSTR lpszHelp,
    UINT uCommand,
    DWORD_PTR dwData)
{
    UNREFERENCED_PARAMETER(uCommand);
    UNREFERENCED_PARAMETER(dwData);

    // Build help file pathname
    WCHAR szPath[MAX_PATH];
    GetWindowsDirectoryW(szPath, _countof(szPath));
    StringCchCatW(szPath, _countof(szPath), _T("\\Help\\"));
    StringCchCatW(szPath, _countof(szPath), lpszHelp);

    // Delete .extension
    LPWSTR pch = PathFindExtensionW(szPath);
    if (pch)
        PathRemoveExtensionW(szPath);

    HOH_AddLangSuffix(szPath, _countof(szPath), GetUserDefaultLangID());

    INT iTry;
    BOOL bBrowserAvailable = HOH_IsBrowserAvailable();
    for (iTry = 0; iTry < 3; ++iTry)
    {
        // Add .html if the browser and .html are available; otherwise .txt
        if (iTry < 2 && bBrowserAvailable)
            StringCchCatW(szPath, _countof(szPath), L".html");
        else
            StringCchCatW(szPath, _countof(szPath), L".txt");

        if (PathFileExistsW(szPath))
            break; // File found

        // English is default
        *pch = UNICODE_NULL; // Truncate
        StringCchCatW(szPath, _countof(szPath), L"_en-US");
    }

    // Open the file
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.hwnd = hWndMain;
    sei.lpFile = szPath;
    sei.nShow = SW_SHOWNORMAL;
    return ShellExecuteExW(&sei);
}

#else // !def ENABLE_HELP

#define HelpOfHtml(hWndMain, lpszHelp, uCommand, dwData) /* empty */

#endif // !def ENABLE_HELP
