/*
 * Copyright 2017 Doug Lyons
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* Documentation: https://learn.microsoft.com/en-us/windows/win32/api/shlwapi/nf-shlwapi-strformatbytesizew */

#include <apitest.h>
#include <shlwapi.h>
#include <strsafe.h>

#define DO_TEST(exp, str) \
do { \
     StrFormatByteSizeW(exp, lpszDest, cchMax);\
     if (lpszDest[1] == L',') lpszDest[1] = L'.';\
     ok(_wcsicmp(lpszDest, (str)) == 0, "Expected %s got %s\n",\
           wine_dbgstr_w((str)), wine_dbgstr_w((lpszDest)));\
} while (0)

WCHAR lpszDest[260];
UINT cchMax=260;

/* Returns true if the user interface is in English. Note that this does not
 * presume of the formatting of dates, numbers, etc.
 * Taken from WINE.
 */
static BOOL is_lang_english(void)
{
    static HMODULE hkernel32 = NULL;
    static LANGID (WINAPI *pGetThreadUILanguage)(void) = NULL;
    static LANGID (WINAPI *pGetUserDefaultUILanguage)(void) = NULL;

    if (!hkernel32)
    {
        hkernel32 = GetModuleHandleA("kernel32.dll");
        pGetThreadUILanguage = (void*)GetProcAddress(hkernel32, "GetThreadUILanguage");
        pGetUserDefaultUILanguage = (void*)GetProcAddress(hkernel32, "GetUserDefaultUILanguage");
    }

    if (pGetThreadUILanguage)
        return PRIMARYLANGID(pGetThreadUILanguage()) == LANG_ENGLISH;
    if (pGetUserDefaultUILanguage)
        return PRIMARYLANGID(pGetUserDefaultUILanguage()) == LANG_ENGLISH;

    return PRIMARYLANGID(GetUserDefaultLangID()) == LANG_ENGLISH;
}

/* Returns true if the dates, numbers, etc. are formatted using English
 * conventions.
 * Taken from WINE.
 */
static BOOL is_locale_english(void)
{
    /* Surprisingly GetThreadLocale() is irrelevant here */
    LANGID langid = PRIMARYLANGID(GetUserDefaultLangID());
    /* With the checks in DO_TEST, DUTCH can be used here as well.
    TODO: Add other combinations that should work. */
    return langid == LANG_ENGLISH || langid == LANG_DUTCH;
}

START_TEST(StrFormatByteSizeW)
{
    /* language-dependent test */
    if (!is_lang_english() || !is_locale_english())
    {
        skip("An English UI and locale is required for the StrFormat*Size tests\n");
        return;
    }
    DO_TEST(0, L"0 bytes");                            // 0x0
    DO_TEST(1, L"1 bytes");                            // 0x1
    DO_TEST(1024, L"1.00 KB");                         // 0x400
    DO_TEST(1048576, L"1.00 MB");                      // 0x100000
    DO_TEST(1073741824, L"1.00 GB");                   // 0x40000000
    DO_TEST(0x70000000, L"1.75 GB");                   // 0x70000000
    DO_TEST(0x80000000, L"2.00 GB");                   // 0x80000000
    DO_TEST(0x100000000, L"4.00 GB");                  // 0x100000000
    DO_TEST(1099511627776, L"1.00 TB");                // 0x10000000000
    DO_TEST(1125899906842624, L"1.00 PB");             // 0x4000000000000
    DO_TEST(1152921504606846976, L"1.00 EB");          // 0x1000000000000000
    DO_TEST(2305843009213693952, L"2.00 EB");          // 0x2000000000000000
    DO_TEST(4611686018427387904, L"4.00 EB");          // 0x4000000000000000
    DO_TEST(0x7fffffffffffffff, L"7.99 EB");           // 0x7fffffffffffffff
    DO_TEST(0x8000000000000000, L"0 bytes");           // 0x8000000000000000  High Bit Set Here
    DO_TEST(0xffffffff00000000, L"0 bytes");           // 0xffffffff00000000
    DO_TEST(0xffffffff00000001, L"1 bytes");           // 0xffffffff00000001
    DO_TEST(0xffffffff70000000, L"1879048192 bytes");  // 0xffffffff70000000
    DO_TEST(0xffffffff7fffffff, L"2147483647 bytes");  // 0xffffffff7fffffff
    DO_TEST(0xffffffff80000000, L"-2147483648 bytes"); // 0xffffffff80000000 // Maximum Negative Number -2.00 GB
    DO_TEST(0xffffffff80000001, L"-2147483647 bytes"); // 0xffffffff80000001
    DO_TEST(0xffffffff90000000, L"-1879048192 bytes"); // 0xffffffff90000000
    DO_TEST(-1073741824, L"-1073741824 bytes");        // 0xffffffffc0000000  -1.00 GB
    DO_TEST(-1048576, L"-1048576 bytes");              // 0xfffffffffff00000  -1.00 MB
    DO_TEST(-1024, L"-1024 bytes");                    // 0xfffffffffffffc00  -1.00 KB
    DO_TEST(0xffffffffffffffff, L"-1 bytes");          // 0xffffffffffffffff

    // Here are some large negative tests and they all return zero bytes

    DO_TEST(-4294967296, L"0 bytes");           // 0xffffffff00000000
    DO_TEST(-8589934592, L"0 bytes");           // 0xfffffffe00000000
    DO_TEST(-17179869184, L"0 bytes");          // 0xfffffffc00000000
    DO_TEST(-34359738368, L"0 bytes");          // 0xfffffff800000000
    DO_TEST(-68719476736, L"0 bytes");          // 0xfffffff000000000
    DO_TEST(-137438953472, L"0 bytes");         // 0xffffffe000000000
    DO_TEST(-274877906944, L"0 bytes");         // 0xffffffc000000000
    DO_TEST(-549755813888, L"0 bytes");         // 0xffffff8000000000
    DO_TEST(-1099511627776, L"0 bytes");        // 0xffffff0000000000
    DO_TEST(-1152921504606846976, L"0 bytes");  // 0xf000000000000000

    // These statements create compile errors and seem to be compiler conversion related
    //DO_TEST(2147483648, L"2.00 GB");         //  2.00 GB & This gives a compile error
    //DO_TEST(-2147483648, L"0 bytes");        // -2.00 GB & This gives a compile error
}
