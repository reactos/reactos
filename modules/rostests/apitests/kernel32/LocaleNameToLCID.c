/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for LocaleNameToLCID
 * PROGRAMMER:      Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

typedef
LCID
WINAPI
FN_LocaleNameToLCID(
    _In_ LPCWSTR lpName,
    _In_ DWORD dwFlags);

FN_LocaleNameToLCID* pLocaleNameToLCID = NULL;

START_TEST(LocaleNameToLCID)
{
    HMODULE hmodKernel32;
    LCID lcid;

    hmodKernel32 = GetModuleHandleW(L"kernel32.dll");
    pLocaleNameToLCID = (FN_LocaleNameToLCID*)GetProcAddress(hmodKernel32, "LocaleNameToLCID");
    if (pLocaleNameToLCID == NULL)
    {
        hmodKernel32 = LoadLibraryW(L"kernel32_vista.dll");
        pLocaleNameToLCID = (FN_LocaleNameToLCID*)GetProcAddress(hmodKernel32, "LocaleNameToLCID");
        if (pLocaleNameToLCID == NULL)
        {
            skip("LocaleNameToLCID not found in kernel32.dll\n");
            return;
        }
    }

    lcid = pLocaleNameToLCID(L"en-US", 0);
    ok_eq_hex(lcid, MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT));

    lcid = pLocaleNameToLCID(L"en", 0);
    ok_eq_hex(lcid, MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), SORT_DEFAULT));

    lcid = pLocaleNameToLCID(L"en", LOCALE_ALLOW_NEUTRAL_NAMES);
    ok_eq_hex(lcid, MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL), SORT_DEFAULT));

    // SUBLANG_SPANISH_US is 0x15
    lcid = pLocaleNameToLCID(L"es-US", LOCALE_ALLOW_NEUTRAL_NAMES);
    ok_eq_hex(lcid, MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_US), SORT_DEFAULT));
    lcid = pLocaleNameToLCID(L"es-US", 0);
    ok_eq_hex(lcid, MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_US), SORT_DEFAULT));

    lcid = pLocaleNameToLCID(L"es-419", LOCALE_ALLOW_NEUTRAL_NAMES);
    ok_eq_hex(lcid, MAKELCID(MAKELANGID(LANG_SPANISH, 0x16), SORT_DEFAULT));
    lcid = pLocaleNameToLCID(L"es-419", 0);
    ok_eq_hex(lcid, MAKELCID(MAKELANGID(LANG_SPANISH, 0x16), SORT_DEFAULT));

    lcid = pLocaleNameToLCID(L"es-CU", LOCALE_ALLOW_NEUTRAL_NAMES);
    ok_eq_hex(lcid, MAKELCID(MAKELANGID(LANG_SPANISH, 0x17), SORT_DEFAULT));
    lcid = pLocaleNameToLCID(L"es-CU", 0);
    ok_eq_hex(lcid, MAKELCID(MAKELANGID(LANG_SPANISH, 0x17), SORT_DEFAULT));

    // Special neutral languages:
    static const struct
    {
        const wchar_t* Name;
        LCID Neutral;
        LCID Default;
    } SpecialCases[] =
    {
        { L"az-Cyrl",  0x0742C, 0x0082C }, // -> "az-Cyrl-AZ"
        { L"az-Latn",  0x0782C, 0x0042C }, // -> "az-Latn-AZ"
        { L"ff-Latn",  0x07C67, 0x00867 }, // -> "ff-Latn-SN"
        { L"bs",       0x0781A, 0x0141A }, // -> "bs-Latn-BA"
        { L"bs-Cyrl",  0x0641A, 0x0201A }, // -> "bs-Cyrl-BA"
        { L"bs-Latn",  0x0681A, 0x0141A }, // -> "bs-Latn-BA"
        { L"chr-Cher", 0x07C5C, 0x0045C }, // -> "chr-Cher-US"
        { L"dsb",      0x07C2E, 0x0082E }, // -> "dsb-DE"
        { L"ha-Latn",  0x07C68, 0x00468 }, // -> "ha-Latn-NG"
        { L"iu-Cans",  0x0785D, 0x0045D }, // -> "iu-Cans-CA"
        { L"iu-Latn",  0x07C5D, 0x0085D }, // -> "iu-Latn-CA"
        { L"ku-Arab",  0x07C92, 0x00492 }, // -> "ku-Arab-IQ"
        { L"mn-Cyrl",  0x07850, 0x00450 }, // -> "mn-MN"
        { L"mn-Mong",  0x07C50, 0x00850 }, // -> "mn-Mong-CN"
        { L"nb",       0x07C14, 0x00414 }, // -> "nb-NO"
        { L"nn",       0x07814, 0x00814 }, // -> "nn-NO"
        { L"pa-Arab",  0x07C46, 0x00846 }, // -> "pa-Arab-PK"
        { L"smn",      0x0703B, 0x0243B }, // -> "smn-FI"
        { L"sd-Arab",  0x07C59, 0x00859 }, // -> "sd-Arab-PK"
        { L"sma",      0x0783B, 0x01C3B }, // -> "sma-SE"
        { L"smj",      0x07C3B, 0x0143B }, // -> "smj-SE"
        { L"sms",      0x0743B, 0x0203B }, // -> "sms-FI"
        { L"sr-Cyrl",  0x06C1A, 0x0281A }, // -> "sr-Cyrl-RS"
        { L"sr-Latn",  0x0701A, 0x0241A }, // -> "sr-Latn-RS"
        { L"tg-Cyrl",  0x07C28, 0x00428 }, // -> "tg-Cyrl-TJ"
        { L"tzm-Latn", 0x07C5F, 0x0085F }, // -> "tzm-Latn-DZ"
        { L"tzm-Tfng", 0x0785F, 0x0105F }, // -> "tzm-Tfng-MA"
        { L"uz-Cyrl",  0x07843, 0x00843 }, // -> "uz-Cyrl-UZ"
        { L"uz-Latn",  0x07C43, 0x00443 }, // -> "uz-Latn-UZ"
        { L"zh",       0x07804, 0x00804 }, // -> "zh-CN"
        { L"zh-Hant",  0x07C04, 0x00C04 }, // -> "zh-HK"
    };

    for (ULONG i = 0; i < ARRAYSIZE(SpecialCases); i++)
    {
        lcid = pLocaleNameToLCID(SpecialCases[i].Name, LOCALE_ALLOW_NEUTRAL_NAMES);
        ok(lcid == SpecialCases[i].Neutral,
           "Wrong neutral lcid for '%S': expected 0x%lx, got 0x%lx\n",
           SpecialCases[i].Name, SpecialCases[i].Neutral, lcid);
        lcid = pLocaleNameToLCID(SpecialCases[i].Name, 0);
        ok(lcid == SpecialCases[i].Default,
           "Wrong default lcid for '%S': expected 0x%lx, got 0x%lx\n",
           SpecialCases[i].Name, SpecialCases[i].Default, lcid);
    }

    SetLastError(0xdeadbeef);
    lcid = pLocaleNameToLCID(L"en-", 0);
    ok_eq_hex(lcid, 0);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xdeadbeef);
    lcid = pLocaleNameToLCID(L"american", 0);
    ok_eq_hex(lcid, 0);
    ok_err(ERROR_INVALID_PARAMETER);

    // Test NULL aka LOCALE_NAME_USER_DEFAULT
    lcid = pLocaleNameToLCID(NULL, 0);
    ok_eq_hex(lcid, GetUserDefaultLCID());

    // Test empty string aka LOCALE_NAME_INVARIANT
    lcid = pLocaleNameToLCID(L"", 0);
    ok_eq_hex(lcid, LOCALE_INVARIANT);

    // Test LOCALE_NAME_SYSTEM_DEFAULT
    SetLastError(0xdeadbeef);
    lcid = pLocaleNameToLCID(L"!sys-default-locale", 0);
    ok_eq_hex(lcid, 0);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xdeadbeef);
    lcid = pLocaleNameToLCID(L"en-US", 1);
    ok_eq_hex(lcid, 0);
    ok_err(ERROR_INVALID_PARAMETER);

}
