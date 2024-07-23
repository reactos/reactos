/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Formal locale verification tests
 * COPYRIGHT:   Copyright 2024 Stanislav Motylkov <x86corez@gmail.com>
 */

#include "shelltest.h"

#include <winnls.h>
#include <strsafe.h>

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

LCID curLcid = 0;
std::set<LANGID> langs;
std::map<E_MODULE, HMODULE> mod;
std::map<E_STRING, PART_TEST> parts;

static void InitParts(void)
{
    PART_TEST part_test;
#define DEFINE_PART(eString, eModule, id, nParts) do { \
        part_test = { eModule, id, nParts }; \
        parts.insert(std::make_pair(eString, part_test)); \
    } while (0);
#include "parts.h"
#undef DEFINE_PART
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

        LoadStringW(mod[m], p.second.id, szBuffer, _countof(szBuffer));
        ok(p.second.nParts == CountParts(szBuffer), "Locale 0x%lX: Num parts mismatch %d - expected %lu, got %lu\n",
           curLcid, p.first, p.second.nParts, CountParts(szBuffer));
    }
}

static BOOL LoadPart(_In_ PART* p, _Out_ LPWSTR str, _In_ SIZE_T size)
{
    auto s = parts[p->Num];
    E_MODULE m = s.eModule;

    if (!mod[m])
        return FALSE;

    WCHAR szBuffer[MAX_PATH];
    LPWSTR szPart;
    SIZE_T len;

    LoadStringW(mod[m], s.id, szBuffer, _countof(szBuffer));
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
            skip("No module for match test %d (pair 1)\n", match.p1.Num);
            continue;
        }

        if (!LoadPart(&match.p2, szP2, _countof(szP2)))
        {
            skip("No module for match test %d (pair 2)\n", match.p2.Num);
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

    WCHAR szOldDir[MAX_PATH], szBuffer[MAX_PATH];
    GetCurrentDirectoryW(_countof(szOldDir), szOldDir);

    std::map<E_MODULE, LPCWSTR> lib;

    GetModuleFileNameW(NULL, szBuffer, _countof(szBuffer));
    LPCWSTR pszFind = StrStrW(szBuffer, L"modules\\rostests\\apitests");
    if (pszFind)
    {
        // We're running in ReactOS output folder
        WCHAR szNewDir[MAX_PATH];

        StringCchCopyNW(szNewDir, _countof(szNewDir), szBuffer, pszFind - szBuffer);
        SetCurrentDirectoryW(szNewDir);

#define DEFINE_MODULE(eModule, pszPath) lib.insert(std::make_pair(eModule, pszPath));
#include "modules1.h"
#undef DEFINE_MODULE
    }
    else
    {
#define DEFINE_MODULE(eModule, pszPath) lib.insert(std::make_pair(eModule, pszPath));
#include "modules2.h"
#undef DEFINE_MODULE
    }

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
