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
    trace("%ld vs %ld\n", value1, value2);
    ok_long(value1, value2);
}

static void
TEST_SHGetRestriction_Stage(
    INT iStage,
    FN_SHSettingsChanged fnSHSettingsChanged,
    FN_SHGetRestriction fnSHGetRestriction)
{
    size_t iItem;
    DWORD dwValue;

    trace("Stage #%d\n", iStage);

    switch (iStage)
    {
        case 0:
            SHDeleteValueW(HKEY_CURRENT_USER, REGKEY_POLICIES_EXPLORER, L"NoRun");
            SHDeleteValueW(HKEY_LOCAL_MACHINE, REGKEY_POLICIES_EXPLORER, L"NoRun");
            break;
        case 1:
            dwValue = 0;
            SHSetValueW(HKEY_CURRENT_USER, REGKEY_POLICIES_EXPLORER, L"NoRun",
                        REG_DWORD, &dwValue, sizeof(dwValue));
            SHDeleteValueW(HKEY_LOCAL_MACHINE, REGKEY_POLICIES_EXPLORER, L"NoRun");
            break;
        case 2:
            dwValue = 1;
            SHSetValueW(HKEY_CURRENT_USER, REGKEY_POLICIES_EXPLORER, L"NoRun",
                        REG_DWORD, &dwValue, sizeof(dwValue));
            SHDeleteValueW(HKEY_LOCAL_MACHINE, REGKEY_POLICIES_EXPLORER, L"NoRun");
            break;
        case 3:
            dwValue = 0;
            SHDeleteValueW(HKEY_CURRENT_USER, REGKEY_POLICIES_EXPLORER, L"NoRun");
            SHSetValueW(HKEY_LOCAL_MACHINE, REGKEY_POLICIES_EXPLORER, L"NoRun",
                        REG_DWORD, &dwValue, sizeof(dwValue));
            break;
        case 4:
            dwValue = 1;
            SHDeleteValueW(HKEY_CURRENT_USER, REGKEY_POLICIES_EXPLORER, L"NoRun");
            SHSetValueW(HKEY_LOCAL_MACHINE, REGKEY_POLICIES_EXPLORER, L"NoRun",
                        REG_DWORD, &dwValue, sizeof(dwValue));
            break;
        case 5:
            dwValue = 0;
            SHSetValueW(HKEY_LOCAL_MACHINE, REGKEY_POLICIES_EXPLORER, L"NoRun",
                        REG_DWORD, &dwValue, sizeof(dwValue));
            dwValue = 1;
            SHSetValueW(HKEY_LOCAL_MACHINE, REGKEY_POLICIES_EXPLORER, L"NoRun",
                        REG_DWORD, &dwValue, sizeof(dwValue));
            break;
        case 6:
            dwValue = 1;
            SHSetValueW(HKEY_LOCAL_MACHINE, REGKEY_POLICIES_EXPLORER, L"NoRun",
                        REG_DWORD, &dwValue, sizeof(dwValue));
            dwValue = 0;
            SHSetValueW(HKEY_LOCAL_MACHINE, REGKEY_POLICIES_EXPLORER, L"NoRun",
                        REG_DWORD, &dwValue, sizeof(dwValue));
            break;
    }

    fnSHSettingsChanged(NULL, NULL);

    for (iItem = 0; iItem < _countof(s_Entries); ++iItem)
    {
        TEST_DoEntry(&s_Entries[iItem], fnSHGetRestriction);
    }
}

START_TEST(SHGetRestriction)
{
    HMODULE hSHELL32 = LoadLibraryW(L"shell32.dll");
    HMODULE hSHLWAPI = LoadLibraryW(L"shlwapi.dll");
    FN_SHSettingsChanged fn1;
    FN_SHGetRestriction fn2;

    fn1 = (FN_SHSettingsChanged)GetProcAddress(hSHELL32, MAKEINTRESOURCEA(244));
    fn2 = (FN_SHGetRestriction)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(271));

    if (fn1 && fn2)
    {
        INT iStage;
        for (iStage = 0; iStage < 7; ++iStage)
            TEST_SHGetRestriction_Stage(iStage, fn1, fn2);

        SHDeleteValueW(HKEY_CURRENT_USER, REGKEY_POLICIES_EXPLORER, L"NoRun");
        SHDeleteValueW(HKEY_LOCAL_MACHINE, REGKEY_POLICIES_EXPLORER, L"NoRun");
    }
    else
    {
        if (!fn1)
            skip("SHSetingsChanged not found\n");
        if (!fn2)
            skip("SHGetRestriction not found\n");
    }

    FreeLibrary(hSHELL32);
    FreeLibrary(hSHLWAPI);
}
