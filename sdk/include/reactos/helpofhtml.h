/*
 * PROJECT:     ReactOS HelpOfHtml (HOH)
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS-specific help system
 * COPYRIGHT:   Copyright (c) 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

#ifdef ENABLE_HELP
    #include <winreg.h>
    #include <string.h>
    #include <tchar.h>
    #include <shellapi.h>
    #include <strsafe.h>

    // Does registry key exist?
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
    HOH_IsFirefoxAvailable(void)
    {
        return HOH_IsRegKeyAvailable(L"SOFTWARE\\Mozilla\\Mozilla Firefox") ||
               HOH_IsRegKeyAvailable(L"SOFTWARE\\Wow6432Node\\Mozilla\\Mozilla Firefox");
    }

    static inline BOOL
    HOH_IsChromeAvailable(void)
    {
        return HOH_IsRegKeyAvailable(L"SOFTWARE\\Google\\Chrome") ||
               HOH_IsRegKeyAvailable(L"SOFTWARE\\Wow6432Node\\Google\\Chrome");
    }

    static inline BOOL
    HOH_IsWineGeckoAvailable(void)
    {
        return HOH_IsRegKeyAvailable(L"SOFTWARE\\Wine\\MSHTML");
    }

    typedef struct tagHOH_SUFFIX_AND_LANG
    {
        LPCTSTR suffix;
        LANGID wLangId;
    } HOH_SUFFIX_AND_LANG, *PHOH_SUFFIX_AND_LANG;

    // Add language suffix
    static inline void
    HOH_AddLangSuffix(LPTSTR pszPath, SIZE_T cchPathMax, LANGID wLangId)
    {
        static const HOH_SUFFIX_AND_LANG pairs[] =
        {
            // FIXME: Add more languages
            { TEXT("_de"), MAKELANGID(LANG_GERMAN,   SUBLANG_NEUTRAL) },
            { TEXT("_en"), MAKELANGID(LANG_ENGLISH,  SUBLANG_ENGLISH_US) },
            { TEXT("_fr"), MAKELANGID(LANG_FRENCH,   SUBLANG_NEUTRAL) },
            { TEXT("_it"), MAKELANGID(LANG_ITALIAN,  SUBLANG_NEUTRAL) },
            { TEXT("_ja"), MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT) },
            { TEXT("_zh"), MAKELANGID(LANG_CHINESE,  SUBLANG_CHINESE_SIMPLIFIED) },
        };
        const size_t count = _countof(pairs);

        // 1st try: Best match
        size_t iPair;
        for (iPair = 0; iPair < count; ++iPair)
        {
            if (pairs[iPair].wLangId == wLangId)
            {
                StringCchCat(pszPath, cchPathMax, pairs[iPair].suffix);
                return;
            }
        }

        // 2nd try: PRIMARYLANGID match
        for (iPair = 0; iPair < count; ++iPair)
        {
            if (PRIMARYLANGID(pairs[iPair].wLangId) == PRIMARYLANGID(wLangId))
            {
                StringCchCat(pszPath, cchPathMax, pairs[iPair].suffix);
                return;
            }
        }

        // 3rd try: default is English
        StringCchCat(pszPath, cchPathMax, TEXT("_en"));
    }

    static inline BOOL
    HOH_IsBrowserAvailable(void)
    {
        return (HOH_IsFirefoxAvailable() ||
                HOH_IsChromeAvailable() ||
                HOH_IsWineGeckoAvailable());
    }

    static inline BOOL
    HelpOfHtml(
        HWND hWndMain,
        LPCTSTR lpszHelp,
        UINT uCommand,
        DWORD_PTR dwData)
    {
        UNREFERENCED_PARAMETER(uCommand);
        UNREFERENCED_PARAMETER(dwData);

        // Build help file pathname
        TCHAR szPath[MAX_PATH];
        GetWindowsDirectory(szPath, _countof(szPath));
        StringCchCat(szPath, _countof(szPath), _T("\\Help\\"));
        StringCchCat(szPath, _countof(szPath), lpszHelp);

        // Delete .extension
        LPTSTR pch = _tcsrchr(szPath, _T('\\'));
        if (pch)
            pch = _tcsrchr(pch, _T('.'));
        if (pch)
            *pch = 0;
        else
            pch = szPath + _tcslen(szPath);

        LANGID wLangId = GetUserDefaultLangID();
        HOH_AddLangSuffix(szPath, _countof(szPath), wLangId);

        INT iTry;
        BOOL bBrowserAvailable = HOH_IsBrowserAvailable();
        for (iTry = 0; iTry < 3; ++iTry)
        {
            // Add .html if the browser and .html are available; otherwise .txt
            if (iTry < 2 && bBrowserAvailable)
                StringCchCat(szPath, _countof(szPath), _T(".html"));
            else
                StringCchCat(szPath, _countof(szPath), _T(".txt"));

            if (GetFileAttributes(szPath) != INVALID_FILE_ATTRIBUTES)
                break; // File found

            // English is default
            *pch = 0; // Cut off
            StringCchCat(szPath, _countof(szPath), L"_en");
        }

        // Open the file
        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.hwnd = hWndMain;
        sei.lpFile = szPath;
        sei.nShow = SW_SHOWNORMAL;
        return ShellExecuteEx(&sei);
    }
#else
    #define HelpOfHtml WinHelp
#endif
