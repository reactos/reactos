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
        LPCTSTR name;
        LANGID wLangId;
    } HOH_SUFFIX_AND_LANG, *PHOH_SUFFIX_AND_LANG;

    // Add language suffix
    static inline void
    HOH_AddLangSuffix(LPTSTR pszPath, SIZE_T cchPathMax, LANGID wLangId)
    {
        static const HOH_SUFFIX_AND_LANG pairs[] =
        {
            // FIXME: Add more languages
            { TEXT("bg-BG"), MAKELANGID(LANG_BULGARIAN,     SUBLANG_DEFAULT) },
            { TEXT("ca-ES"), MAKELANGID(LANG_CATALAN,       SUBLANG_DEFAULT) },
            { TEXT("cs-CZ"), MAKELANGID(LANG_CZECH,         SUBLANG_DEFAULT) },
            { TEXT("da-DK"), MAKELANGID(LANG_DANISH,        SUBLANG_DEFAULT) },
            { TEXT("de-DE"), MAKELANGID(LANG_GERMAN,        SUBLANG_NEUTRAL) },
            { TEXT("el-GR"), MAKELANGID(LANG_GREEK,         SUBLANG_DEFAULT) },
            { TEXT("en-US"), MAKELANGID(LANG_ENGLISH,       SUBLANG_ENGLISH_US) },
            { TEXT("es-ES"), MAKELANGID(LANG_SPANISH,       SUBLANG_NEUTRAL) },
            { TEXT("et-EE"), MAKELANGID(LANG_ESTONIAN,      SUBLANG_DEFAULT) },
            { TEXT("fr-FR"), MAKELANGID(LANG_FRENCH,        SUBLANG_NEUTRAL) },
            { TEXT("he-IL"), MAKELANGID(LANG_HEBREW,        SUBLANG_DEFAULT) },
            { TEXT("hr-HR"), MAKELANGID(LANG_CROATIAN,      SUBLANG_CROATIAN_CROATIA) },
            { TEXT("hu-HU"), MAKELANGID(LANG_HUNGARIAN,     SUBLANG_DEFAULT) },
            { TEXT("id-ID"), MAKELANGID(LANG_INDONESIAN,    SUBLANG_DEFAULT) },
            { TEXT("it-IT"), MAKELANGID(LANG_ITALIAN,       SUBLANG_NEUTRAL) },
            { TEXT("ja-JP"), MAKELANGID(LANG_JAPANESE,      SUBLANG_DEFAULT) },
            { TEXT("ko-KR"), MAKELANGID(LANG_KOREAN,        SUBLANG_DEFAULT) },
            { TEXT("lt-LT"), MAKELANGID(LANG_LITHUANIAN,    SUBLANG_DEFAULT) },
            { TEXT("nl-NL"), MAKELANGID(LANG_DUTCH,         SUBLANG_NEUTRAL) },
            { TEXT("no-NO"), MAKELANGID(LANG_NORWEGIAN,     SUBLANG_NORWEGIAN_BOKMAL) },
            { TEXT("pl-PL"), MAKELANGID(LANG_POLISH,        SUBLANG_DEFAULT) },
            { TEXT("pt-BR"), MAKELANGID(LANG_PORTUGUESE,    SUBLANG_PORTUGUESE_BRAZILIAN) },
            { TEXT("pt-PT"), MAKELANGID(LANG_PORTUGUESE,    SUBLANG_PORTUGUESE) },
            { TEXT("ru-RU"), MAKELANGID(LANG_RUSSIAN,       SUBLANG_DEFAULT) },
            { TEXT("sk-SK"), MAKELANGID(LANG_SLOVAK,        SUBLANG_DEFAULT) },
            { TEXT("sq-AL"), MAKELANGID(LANG_ALBANIAN,      SUBLANG_NEUTRAL) },
            { TEXT("sv-SE"), MAKELANGID(LANG_SWEDISH,       SUBLANG_NEUTRAL) },
            { TEXT("th-TH"), MAKELANGID(LANG_THAI,          SUBLANG_DEFAULT) },
            { TEXT("tr-TR"), MAKELANGID(LANG_TURKISH,       SUBLANG_DEFAULT) },
            { TEXT("uk-UA"), MAKELANGID(LANG_UKRAINIAN,     SUBLANG_DEFAULT) },
            { TEXT("zh-CN"), MAKELANGID(LANG_CHINESE,       SUBLANG_CHINESE_SIMPLIFIED) },
            { TEXT("zh-HK"), MAKELANGID(LANG_CHINESE,       SUBLANG_CHINESE_HONGKONG) },
            { TEXT("zh-TW"), MAKELANGID(LANG_CHINESE,       SUBLANG_CHINESE_TRADITIONAL) },
        };
        const size_t count = _countof(pairs);

        // 1st try: Best match
        size_t iPair;
        for (iPair = 0; iPair < count; ++iPair)
        {
            if (pairs[iPair].wLangId == wLangId)
            {
                StringCchCat(pszPath, cchPathMax, TEXT("_"));
                StringCchCat(pszPath, cchPathMax, pairs[iPair].name);
                return;
            }
        }

        // 2nd try: PRIMARYLANGID match
        for (iPair = 0; iPair < count; ++iPair)
        {
            if (PRIMARYLANGID(pairs[iPair].wLangId) == PRIMARYLANGID(wLangId))
            {
                StringCchCat(pszPath, cchPathMax, TEXT("_"));
                StringCchCat(pszPath, cchPathMax, pairs[iPair].name);
                return;
            }
        }

        // 3rd try: default is English
        StringCchCat(pszPath, cchPathMax, TEXT("_"));
        StringCchCat(pszPath, cchPathMax, TEXT("en-US"));
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
            *pch = 0; // Cut off
        else
            pch = &szPath[_tcslen(szPath)];

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
            StringCchCat(szPath, _countof(szPath), L"_");
            StringCchCat(szPath, _countof(szPath), L"en-US");
        }

        // Open the file
        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.hwnd = hWndMain;
        sei.lpFile = szPath;
        sei.nShow = SW_SHOWNORMAL;
        return ShellExecuteEx(&sei);
    }
#else
    #define HelpOfHtml(hWndMain, lpszHelp, uCommand, dwData) /* empty */
#endif
