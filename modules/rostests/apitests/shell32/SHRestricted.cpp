/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for SHRestricted
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <versionhelpers.h>

#define REGKEY_POLICIES L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies"
#define REGKEY_POLICIES_EXPLORER REGKEY_POLICIES L"\\Explorer"

typedef DWORD (WINAPI *FN_SHRestricted)(RESTRICTIONS rest);
typedef BOOL (WINAPI *FN_SHSettingsChanged)(LPCVOID unused, LPCVOID inpRegKey);

#define DELETE_VALUE(hBaseKey) \
    SHDeleteValueW((hBaseKey), REGKEY_POLICIES_EXPLORER, L"NoRun")

#define SET_VALUE(hBaseKey, value) do { \
    dwValue = (value); \
    SHSetValueW((hBaseKey), REGKEY_POLICIES_EXPLORER, L"NoRun", \
                REG_DWORD, &dwValue, sizeof(dwValue)); \
} while (0)

static VOID
TEST_SHRestricted(FN_SHRestricted fn1, FN_SHSettingsChanged fn2)
{
    DWORD dwValue;

    DELETE_VALUE(HKEY_CURRENT_USER);
    DELETE_VALUE(HKEY_LOCAL_MACHINE);

    fn2(NULL, NULL);
    ok_long(fn1(REST_NORUN), 0);

    SET_VALUE(HKEY_CURRENT_USER, 0);
    DELETE_VALUE(HKEY_LOCAL_MACHINE);

    fn2(NULL, NULL);
    ok_long(fn1(REST_NORUN), 0);

    SET_VALUE(HKEY_CURRENT_USER, 1);
    DELETE_VALUE(HKEY_LOCAL_MACHINE);

    fn2(NULL, NULL);
    ok_long(fn1(REST_NORUN), 1);

    DELETE_VALUE(HKEY_CURRENT_USER);
    SET_VALUE(HKEY_LOCAL_MACHINE, 0);

    fn2(NULL, NULL);
    ok_long(fn1(REST_NORUN), 0);

    DELETE_VALUE(HKEY_CURRENT_USER);
    SET_VALUE(HKEY_LOCAL_MACHINE, 1);

    fn2(NULL, NULL);
    ok_long(fn1(REST_NORUN), 1);

    SET_VALUE(HKEY_CURRENT_USER, 2);
    SET_VALUE(HKEY_LOCAL_MACHINE, 1);

    fn2(NULL, NULL);
    ok_long(fn1(REST_NORUN), 1);

    DELETE_VALUE(HKEY_CURRENT_USER);
    DELETE_VALUE(HKEY_LOCAL_MACHINE);

    fn2(NULL, NULL);
    ok_long(fn1(REST_NORUN), 0);
}

START_TEST(SHRestricted)
{
    if (IsWindowsVistaOrGreater())
    {
        skip("Vista+");
        return;
    }

    HMODULE hSHELL32 = LoadLibraryW(L"shell32.dll");
    FN_SHRestricted fn1;
    FN_SHSettingsChanged fn2;

    fn1 = (FN_SHRestricted)GetProcAddress(hSHELL32, MAKEINTRESOURCEA(100));
    fn2 = (FN_SHSettingsChanged)GetProcAddress(hSHELL32, MAKEINTRESOURCEA(244));

    if (fn1 && fn2)
    {
        TEST_SHRestricted(fn1, fn2);
    }
    else
    {
        if (!fn1)
            skip("SHRestricted not found\n");
        if (!fn2)
            skip("SHSettingsChanged not found\n");
    }

    FreeLibrary(hSHELL32);
}
