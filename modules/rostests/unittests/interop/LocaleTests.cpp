/*
 * PROJECT:     ReactOS interoperability tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Formal locale verification tests
 * COPYRIGHT:   Copyright 2024 Stanislav Motylkov <x86corez@gmail.com>
 *              Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "interop.h"

#include <winnls.h>
#include <strsafe.h>
#include <shlwapi.h>

#include <set>
#include <map>

enum E_MODULE
{
    shell32,
    userenv,
    syssetup,
    mmsys,
    explorer_old,
};

enum E_STRING
{
    SH32_PROGRAMS,
    SH32_STARTUP,
    SH32_STARTMENU,
    SH32_PROGRAM_FILES,
    SH32_PROGRAM_FILES_COMMON,
    SH32_ADMINTOOLS,
    UENV_STARTMENU,
    UENV_PROGRAMS,
    UENV_STARTUP,
    SYSS_PROGRAMFILES,
    SYSS_COMMONFILES,
    MMSY_STARTMENU,
    EOLD_PROGRAMS,
};

typedef struct PART_TEST
{
    E_MODULE eModule;
    UINT id;
    SIZE_T nParts;
    SIZE_T gotParts;
} PART_TEST;

typedef struct PART
{
    E_STRING Num;
    UINT Idx;
} PART;

typedef struct PART_MATCH
{
    PART p1, p2;
} PART_MATCH;

DWORD dwVersion;
LCID curLcid = 0;
std::set<LANGID> langs;
std::map<E_MODULE, HMODULE> mod;
std::map<E_STRING, PART_TEST> parts;

struct PART_PAIR
{
    E_STRING eString;
    PART_TEST part_test;
};

static void InitParts(void)
{
    static const PART_PAIR s_pairs[] =
    {
        // { eString, { eModule, id, nParts } }
        { SH32_PROGRAMS, { shell32, 45 /* IDS_PROGRAMS "Start Menu\Programs" */, 2 } },
        { SH32_STARTUP, { shell32, 48 /* IDS_STARTUP "Start Menu\Programs\StartUp" */, 3 } },
        { SH32_STARTMENU, { shell32, 51 /* IDS_STARTMENU "Start Menu" */, 1 } },
        { SH32_PROGRAM_FILES, { shell32, 63 /* IDS_PROGRAM_FILES "Program Files" */, 1 } },
        { SH32_PROGRAM_FILES_COMMON, { shell32, 65 /* IDS_PROGRAM_FILES_COMMON "Program Files\Common Files" */, 2 } },
        { SH32_ADMINTOOLS, { shell32, 67 /* IDS_ADMINTOOLS "Start Menu\Programs\Administrative Tools" */, 3 } },
        { UENV_STARTMENU, { userenv, 11 /* IDS_STARTMENU "Start Menu" */, 1 } },
        { UENV_PROGRAMS, { userenv, 12 /* IDS_PROGRAMS "Start Menu\Programs" */, 2 } },
        { UENV_STARTUP, { userenv, 13 /* IDS_STARTUP "Start Menu\Programs\StartUp" */, 3 } },
        { SYSS_PROGRAMFILES, { syssetup, 3600 /* IDS_PROGRAMFILES "%SystemDrive%\Program Files" */, 2 } },
        { SYSS_COMMONFILES, { syssetup, 3601 /* IDS_COMMONFILES "Common Files" */, 1 } },
        { MMSY_STARTMENU, { mmsys, 5851 /* IDS_STARTMENU "Start Menu" */, 1 } },
        { EOLD_PROGRAMS, { explorer_old, 10 /* IDS_PROGRAMS "Programs" */, 1 } },
    };
    for (auto& pair : s_pairs)
    {
        parts.insert(std::make_pair(pair.eString, pair.part_test));
    }
}

static PART_MATCH PartMatches[] =
{
    // Start Menu
    { { SH32_PROGRAMS, 0 }, { SH32_STARTUP, 0 } },
    { { SH32_PROGRAMS, 0 }, { SH32_STARTMENU, 0 } },
    { { SH32_PROGRAMS, 0 }, { SH32_ADMINTOOLS, 0 } },
    { { SH32_PROGRAMS, 0 }, { UENV_STARTMENU, 0 } },
    { { SH32_PROGRAMS, 0 }, { UENV_PROGRAMS, 0 } },
    { { SH32_PROGRAMS, 0 }, { UENV_STARTUP, 0 } },
    { { SH32_PROGRAMS, 0 }, { MMSY_STARTMENU, 0 } },
    // Programs
    { { SH32_PROGRAMS, 1 }, { SH32_STARTUP, 1 } },
    { { SH32_PROGRAMS, 1 }, { SH32_ADMINTOOLS, 1 } },
    { { SH32_PROGRAMS, 1 }, { UENV_PROGRAMS, 1 } },
    { { SH32_PROGRAMS, 1 }, { UENV_STARTUP, 1 } },
    { { SH32_PROGRAMS, 1 }, { EOLD_PROGRAMS, 0 } },
    // StartUp
    { { SH32_STARTUP, 2 }, { UENV_STARTUP, 2 } },
    // Program Files
    { { SH32_PROGRAM_FILES, 0 }, { SH32_PROGRAM_FILES_COMMON, 0 } },
    { { SH32_PROGRAM_FILES, 0 }, { SYSS_PROGRAMFILES, 1 } },
    // Common Files
    { { SH32_PROGRAM_FILES_COMMON, 1 }, { SYSS_COMMONFILES, 0 } },
};

static int GetLocalisedText(_In_opt_ HINSTANCE hInstance, _In_ UINT uID, _Out_ LPWSTR lpBuffer, _In_ int cchBufferMax)
{
    HRSRC hRes = FindResourceExW(hInstance, (LPWSTR)RT_STRING,
                                 MAKEINTRESOURCEW((uID >> 4) + 1), curLcid);

    if (!hRes)
        hRes = FindResourceExW(hInstance, (LPWSTR)RT_STRING,
                               MAKEINTRESOURCEW((uID >> 4) + 1),
                               MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), SORT_DEFAULT));

    if (!hRes)
        return 0;

    HGLOBAL hMem = LoadResource(hInstance, hRes);
    if (!hMem)
        return 0;

    PWCHAR p = (PWCHAR)LockResource(hMem);
    for (UINT i = 0; i < (uID & 0x0F); i++) p += *p + 1;

    int len = (*p > cchBufferMax ? cchBufferMax : *p);
    memcpy(lpBuffer, p + 1, len * sizeof(WCHAR));
    lpBuffer[len] = UNICODE_NULL;
    return len;
}

static int LoadStringWrapW(_In_opt_ HINSTANCE hInstance, _In_ UINT uID, _Out_ LPWSTR lpBuffer, _In_ int cchBufferMax)
{
    if (dwVersion < _WIN32_WINNT_WS03)
        // Windows XP or lower: SetThreadLocale doesn't select user interface language
        return GetLocalisedText(hInstance, uID, lpBuffer, cchBufferMax);
    else
        return LoadStringW(hInstance, uID, lpBuffer, cchBufferMax);
}

static DWORD CountParts(_In_ LPWSTR str)
{
    DWORD count = 0;
    LPWSTR ptr = str;

    if (*ptr == UNICODE_NULL)
        return 0;

    while ((ptr = wcschr(ptr, L'\\')))
    {
        count++;
        ptr++;
    }

    return count + 1;
}

static LPWSTR GetPart(_In_ LPWSTR str, _In_ SIZE_T num, _Out_ SIZE_T* len)
{
    DWORD count = 0;
    LPWSTR ptr = str, next;

    while (count < num && (ptr = wcschr(ptr, L'\\')) != NULL)
    {
        count++;
        ptr++;
    }

    if (!ptr)
        ptr = str;

    next = wcschr(ptr, L'\\');
    *len = next ? next - ptr : wcslen(ptr);
    return ptr;
}

static BOOL CALLBACK find_locale_id_callback(
    _In_ HMODULE hModule, _In_ LPCWSTR type, _In_ LPCWSTR name, _In_ LANGID lang, _In_ LPARAM lParam)
{
    langs.insert(lang);
    return TRUE;
}

static void SetLocale(_In_ LCID lcid)
{
    SetThreadLocale(lcid);
    SetThreadUILanguage(lcid);
    curLcid = lcid;
}

static void TEST_NumParts(void)
{
    for (auto& p : parts)
    {
        E_MODULE m = p.second.eModule;

        if (!mod[m])
        {
            skip("No module for test %d\n", p.first);
            continue;
        }

        WCHAR szBuffer[MAX_PATH];

        LoadStringWrapW(mod[m], p.second.id, szBuffer, _countof(szBuffer));
        p.second.gotParts = CountParts(szBuffer);

        ok(p.second.nParts == p.second.gotParts, "Locale 0x%lX: Num parts mismatch %d - expected %lu, got %lu\n",
           curLcid, p.first, p.second.nParts, p.second.gotParts);
    }
}

static BOOL LoadPart(_In_ PART* p, _Out_ LPWSTR str, _In_ SIZE_T size)
{
    auto s = parts[p->Num];
    E_MODULE m = s.eModule;

    if (!mod[m])
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    if (s.nParts != s.gotParts)
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    WCHAR szBuffer[MAX_PATH];
    LPWSTR szPart;
    SIZE_T len;

    LoadStringWrapW(mod[m], s.id, szBuffer, _countof(szBuffer));
    szPart = GetPart(szBuffer, p->Idx, &len);
    StringCchCopyNW(str, size, szPart, len);

    return TRUE;
}

static void TEST_PartMatches(void)
{
    for (auto& match : PartMatches)
    {
        WCHAR szP1[MAX_PATH], szP2[MAX_PATH];

        if (!LoadPart(&match.p1, szP1, _countof(szP1)))
        {
            skip("%s for match test %d (pair 1)\n", GetLastError() == ERROR_FILE_NOT_FOUND
                ? "No module" : "Invalid data", match.p1.Num);
            continue;
        }

        if (!LoadPart(&match.p2, szP2, _countof(szP2)))
        {
            skip("%s for match test %d (pair 2)\n", GetLastError() == ERROR_FILE_NOT_FOUND
                ? "No module" : "Invalid data", match.p2.Num);
            continue;
        }

        ok(wcscmp(szP1, szP2) == 0, "Locale 0x%lX: Mismatching pairs %u:%u / %u:%u '%S' vs. '%S'\n",
           curLcid, match.p1.Num, match.p1.Idx, match.p2.Num, match.p2.Idx, szP1, szP2);
    }
}

static void TEST_LocaleTests(void)
{
    // Initialization
    InitParts();

    OSVERSIONINFOEXW osvi;
    memset(&osvi, 0, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    GetVersionExW((LPOSVERSIONINFOW)&osvi);
    dwVersion = (osvi.dwMajorVersion << 8) | osvi.dwMinorVersion;

    WCHAR szOldDir[MAX_PATH], szBuffer[MAX_PATH];
    GetCurrentDirectoryW(_countof(szOldDir), szOldDir);

    std::map<E_MODULE, LPCWSTR> lib;
#define ADD_LIB(eModule, pszPath) lib.insert(std::make_pair(eModule, pszPath))

    GetModuleFileNameW(NULL, szBuffer, _countof(szBuffer));
    LPCWSTR pszFind = StrStrW(szBuffer, L"modules\\rostests\\unittests");
    if (pszFind)
    {
        // We're running in ReactOS output folder
        WCHAR szNewDir[MAX_PATH];

        StringCchCopyNW(szNewDir, _countof(szNewDir), szBuffer, pszFind - szBuffer);
        SetCurrentDirectoryW(szNewDir);

        ADD_LIB(shell32, L"dll\\win32\\shell32\\shell32.dll");
        ADD_LIB(userenv, L"dll\\win32\\userenv\\userenv.dll");
        ADD_LIB(syssetup, L"dll\\win32\\syssetup\\syssetup.dll");
        ADD_LIB(mmsys, L"dll\\cpl\\mmsys\\mmsys.cpl");
        ADD_LIB(explorer_old, L"modules\\rosapps\\applications\\explorer-old\\explorer_old.exe");
    }
    else
    {
        ADD_LIB(shell32, L"shell32.dll");
        ADD_LIB(userenv, L"userenv.dll");
        ADD_LIB(syssetup, L"syssetup.dll");
        ADD_LIB(mmsys, L"mmsys.cpl");
        ADD_LIB(explorer_old, L"explorer_old.exe");
    }
#undef ADD_LIB

    for (auto& lb : lib)
    {
        E_MODULE m = lb.first;

        mod[m] = LoadLibraryExW(lib[m], NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (!mod[m])
        {
            trace("Failed to load '%S', error %lu\n", lib[m], GetLastError());
            continue;
        }

        EnumResourceLanguagesW(mod[m], (LPCWSTR)RT_STRING, (LPCWSTR)LOCALE_ILANGUAGE,
                               find_locale_id_callback, NULL);
    }

    // Actual tests
    for (auto& lang : langs)
    {
        SetLocale(MAKELCID(lang, SORT_DEFAULT));

        TEST_NumParts();
        TEST_PartMatches();
    }

    // Perform cleanup
    for (auto& m : mod)
    {
        if (!m.second)
            continue;

        FreeLibrary(m.second);
        m.second = NULL;
    }

    SetCurrentDirectoryW(szOldDir);
}

START_TEST(LocaleTests)
{
    TEST_LocaleTests();
}
