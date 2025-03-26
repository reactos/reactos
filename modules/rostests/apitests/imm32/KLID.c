/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for Keyboard Layout ID (KLID), HKL, and registry
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

#include <stdlib.h>
#include <imm32_undoc.h>
#include <strsafe.h>

typedef enum tagHKL_TYPE
{
    HKL_TYPE_PURE    = 0,
    HKL_TYPE_SPECIAL = 1,
    HKL_TYPE_IME     = 2,
    HKL_TYPE_CHIMERA = 3,
} HKL_TYPE;

static HKL_TYPE GetHKLType(HKL hKL)
{
    /* 0xEXXXYYYY: An IME HKL. EXXX is an IME keyboard. YYYY is a language */
    if (IS_IME_HKL(hKL))
        return HKL_TYPE_IME;

    /* 0xFXXXYYYY: A special HKL. XXX is a special ID. YYYY is a language */
    if (IS_SPECIAL_HKL(hKL))
        return HKL_TYPE_SPECIAL;

    /* 0xXXXXXXXX: The keyboard layout and language is the same value */
    if (LOWORD(hKL) == HIWORD(hKL))
        return HKL_TYPE_PURE;

    /* 0xXXXXYYYY: XXXX is a keyboard. YYYY is a language */
    return HKL_TYPE_CHIMERA;
}

static HKEY OpenKeyboardLayouts(void)
{
    HKEY hKey = NULL;
    RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                  L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts",
                  0, KEY_READ, &hKey);
    return hKey;
}

static DWORD KLIDFromSpecialHKL(HKL hKL)
{
    WCHAR szName[16], szLayoutId[16];
    HKEY hkeyLayouts, hkeyKLID;
    LSTATUS error;
    DWORD dwSpecialId, dwLayoutId, cbValue, dwKLID = 0;

    hkeyLayouts = OpenKeyboardLayouts();
    ok(hkeyLayouts != NULL, "hkeyLayouts was NULL\n");

    dwSpecialId = SPECIALIDFROMHKL(hKL);

    /* Search from "Keyboard Layouts" registry key */
    for (DWORD dwIndex = 0; dwIndex < 1000; ++dwIndex)
    {
        error = RegEnumKeyW(hkeyLayouts, dwIndex, szName, _countof(szName));
        szName[_countof(szName) - 1] = UNICODE_NULL; /* Avoid buffer overrun */
        if (error != ERROR_SUCCESS)
            break;

        error = RegOpenKeyExW(hkeyLayouts, szName, 0, KEY_READ, &hkeyKLID);
        if (error != ERROR_SUCCESS)
            break;

        cbValue = sizeof(szLayoutId);
        error = RegQueryValueExW(hkeyKLID, L"Layout Id", NULL, NULL, (LPBYTE)szLayoutId, &cbValue);
        szLayoutId[_countof(szLayoutId) - 1] = UNICODE_NULL; /* Avoid buffer overrun */
        if (error != ERROR_SUCCESS)
        {
            RegCloseKey(hkeyKLID);
            continue;
        }

        dwLayoutId = wcstoul(szLayoutId, NULL, 16);
        RegCloseKey(hkeyKLID);
        if (dwLayoutId == dwSpecialId) /* Found */
        {
            dwKLID = wcstoul(szName, NULL, 16);
            break;
        }
    }

    RegCloseKey(hkeyLayouts);
    return dwKLID;
}

static DWORD KLIDFromHKL(HKL hKL)
{
    HKL_TYPE type = GetHKLType(hKL);

    trace("type: %d\n", type);
    switch (type)
    {
        case HKL_TYPE_PURE:
        case HKL_TYPE_CHIMERA:
            return HIWORD(hKL);

        case HKL_TYPE_SPECIAL:
            return KLIDFromSpecialHKL(hKL);

        case HKL_TYPE_IME:
            return HandleToUlong(hKL);
    }

    return 0;
}

static void Test_KLID(DWORD dwKLID, HKL hKL)
{
    WCHAR szKLID[16], szValue[MAX_PATH];
    LSTATUS error;
    DWORD dwValue, cbValue;
    HKEY hkeyKLID, hkeyLayouts;
    HKL_TYPE type;

    hkeyLayouts = OpenKeyboardLayouts();
    ok(hkeyLayouts != NULL, "hkeyLayouts was NULL\n");

    StringCchPrintfW(szKLID, _countof(szKLID), L"%08lX", dwKLID);
    RegOpenKeyExW(hkeyLayouts, szKLID, 0, KEY_READ, &hkeyKLID);
    ok(hkeyKLID != NULL, "hkeyKLID was NULL\n");

    error = RegQueryValueExW(hkeyKLID, L"Layout File", NULL, NULL, NULL, NULL);
    ok_long(error, ERROR_SUCCESS);

    type = GetHKLType(hKL);

    if (type == HKL_TYPE_IME)
    {
        ok_long(dwKLID, HandleToUlong(hKL));
        error = RegQueryValueExW(hkeyKLID, L"IME File", NULL, NULL, NULL, NULL);
        ok_long(error, ERROR_SUCCESS);
    }

    if (type == HKL_TYPE_SPECIAL)
    {
        cbValue = sizeof(szValue);
        error = RegQueryValueExW(hkeyKLID, L"Layout Id", NULL, NULL, (LPBYTE)&szValue, &cbValue);
        ok_long(error, ERROR_SUCCESS);

        dwValue = wcstoul(szValue, NULL, 16);
        ok_long(dwValue, SPECIALIDFROMHKL(hKL));
    }

    RegCloseKey(hkeyKLID);
    RegCloseKey(hkeyLayouts);
}

static void Test_HKL(HKL hKL)
{
    DWORD dwKLID;

    ok(hKL != NULL, "hKL was NULL\n");

    dwKLID = KLIDFromHKL(hKL);
    trace("dwKLID 0x%08lX, hKL %p\n", dwKLID, hKL);

    Test_KLID(dwKLID, hKL);
}

START_TEST(KLID)
{
    HKL *phKLs;
    INT iKL, cKLs;

    cKLs = GetKeyboardLayoutList(0, NULL);
    trace("cKLs: %d\n", cKLs);
    if (!cKLs)
    {
        skip("cKLs was zero\n");
        return;
    }

    phKLs = calloc(cKLs, sizeof(HKL));
    if (!phKLs)
    {
        skip("!phKLs\n");
        return;
    }

    ok_int(GetKeyboardLayoutList(cKLs, phKLs), cKLs);

    for (iKL = 0; iKL < cKLs; ++iKL)
    {
        trace("---\n");
        trace("phKLs[%d]: %p\n", iKL, phKLs[iKL]);
        Test_HKL(phKLs[iKL]);
    }

    free(phKLs);
}
