/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for SHGetRestriction
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <shlwapi.h>

#define REGKEY_POLICIES L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies"
#define REGKEY_POLICIES_EXPLORER  REGKEY_POLICIES L"\\Explorer"

typedef DWORD (WINAPI *FN_SHGetRestriction)(LPCWSTR lpSubKey, LPCWSTR lpSubName, LPCWSTR lpValue);
typedef BOOL (WINAPI *FN_SHSettingsChanged)(LPCVOID unused, LPCVOID inpRegKey);

static DWORD
Candidate_SHGetRestriction(LPCWSTR lpSubKey, LPCWSTR lpSubName, LPCWSTR lpValue)
{
    WCHAR szPath[MAX_PATH];
    DWORD cbValue, dwValue = 0;

    if (!lpSubKey)
        lpSubKey = REGKEY_POLICIES;

    PathCombineW(szPath, lpSubKey, lpSubName);

    cbValue = sizeof(dwValue);
    if (SHGetValueW(HKEY_LOCAL_MACHINE, szPath, lpValue, NULL, &dwValue, &cbValue) == ERROR_SUCCESS)
        return dwValue;

    cbValue = sizeof(dwValue);
    SHGetValueW(HKEY_CURRENT_USER, szPath, lpValue, NULL, &dwValue, &cbValue);
    return dwValue;
}

typedef struct tagTEST_ENTRY
{
    LPCWSTR lpSubName;
    LPCWSTR lpValue;
} TEST_ENTRY, *PTEST_ENTRY;

static const TEST_ENTRY s_Entries[] =
{
    { L"Explorer", L"NoRun" },
    { L"Explorer", L"ForceActiveDesktopOn" },
    { L"Explorer", L"NoActiveDesktop" },
    { L"Explorer", L"NoDisconnect" },
    { L"Explorer", L"NoRecentDocsHistory" },
    { L"Explorer", L"NoDriveTypeAutoRun" },
    { L"Explorer", L"NoSimpleStartMenu" },
    { L"System", L"DontDisplayLastUserName" },
    { L"System", L"ShutdownWithoutLogon" },
    { L"System", L"UndockWithoutLogon" },
};

static void
TEST_DoEntry(const TEST_ENTRY *entry, FN_SHGetRestriction fnSHGetRestriction)
{
    DWORD value1 = fnSHGetRestriction(NULL, entry->lpSubName, entry->lpValue);
    DWORD value2 = Candidate_SHGetRestriction(NULL, entry->lpSubName, entry->lpValue);
    //trace("%ld vs %ld\n", value1, value2);
    ok_long(value1, value2);
}

#define DELETE_VALUE(hBaseKey) \
    SHDeleteValueW((hBaseKey), REGKEY_POLICIES_EXPLORER, L"NoRun")

#define SET_VALUE(hBaseKey, value) do { \
    dwValue = (value); \
    SHSetValueW((hBaseKey), REGKEY_POLICIES_EXPLORER, L"NoRun", \
                REG_DWORD, &dwValue, sizeof(dwValue)); \
} while (0)

static void
TEST_SHGetRestriction_Stage(
    INT iStage,
    FN_SHGetRestriction fnSHGetRestriction)
{
    size_t iItem;
    DWORD dwValue;

    trace("Stage #%d\n", iStage);

    switch (iStage)
    {
        case 0:
            DELETE_VALUE(HKEY_CURRENT_USER);
            DELETE_VALUE(HKEY_LOCAL_MACHINE);
            break;
        case 1:
            SET_VALUE(HKEY_CURRENT_USER, 0);
            DELETE_VALUE(HKEY_LOCAL_MACHINE);
            break;
        case 2:
            SET_VALUE(HKEY_CURRENT_USER, 1);
            DELETE_VALUE(HKEY_LOCAL_MACHINE);
            break;
        case 3:
            DELETE_VALUE(HKEY_CURRENT_USER);
            SET_VALUE(HKEY_LOCAL_MACHINE, 0);
            break;
        case 4:
            DELETE_VALUE(HKEY_CURRENT_USER);
            SET_VALUE(HKEY_LOCAL_MACHINE, 1);
            break;
        case 5:
            SET_VALUE(HKEY_CURRENT_USER, 0);
            SET_VALUE(HKEY_LOCAL_MACHINE, 1);
            break;
        case 6:
            SET_VALUE(HKEY_CURRENT_USER, 1);
            SET_VALUE(HKEY_LOCAL_MACHINE, 0);
            break;
    }

    for (iItem = 0; iItem < _countof(s_Entries); ++iItem)
    {
        TEST_DoEntry(&s_Entries[iItem], fnSHGetRestriction);
    }
}

START_TEST(SHGetRestriction)
{
    HMODULE hSHLWAPI = LoadLibraryW(L"shlwapi.dll");
    FN_SHGetRestriction fn = (FN_SHGetRestriction)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(271));
    INT iStage;

    if (fn)
    {
        for (iStage = 0; iStage < 7; ++iStage)
            TEST_SHGetRestriction_Stage(iStage, fn);

        DELETE_VALUE(HKEY_CURRENT_USER);
        DELETE_VALUE(HKEY_LOCAL_MACHINE);
    }
    else
    {
        skip("SHGetRestriction not found\n");
    }

    FreeLibrary(hSHLWAPI);
}
