/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for AddCommas
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "shelltest.h"

#include <winnls.h>
#include <bcrypt.h>
#include <strsafe.h>

extern "C" DECLSPEC_IMPORT LPWSTR WINAPI AddCommasW(DWORD lValue, LPWSTR lpNumber);

START_TEST(AddCommas)
{
    WCHAR Separator[4];
    WCHAR Grouping[11];
    WCHAR Number[32];
    WCHAR Expected[32];
    PWSTR Ptr;
    int Ret;

    StartSeh()
        AddCommasW(0, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);

    RtlFillMemory(Number, sizeof(Number), 0x55);
    Ptr = AddCommasW(0, Number);
    ok(Ptr == Number, "Ptr = %p, expected %p\n", Ptr, Number);
    ok(Number[0] == L'0', "Number[0] = 0x%x\n", Number[0]);
    ok(Number[1] == 0, "Number[1] = 0x%x\n", Number[1]);
    ok(Number[2] == 0x5555, "Number[2] = 0x%x\n", Number[2]);

    Ret = GetLocaleInfoW(LOCALE_USER_DEFAULT,
                         LOCALE_STHOUSAND,
                         Separator,
                         RTL_NUMBER_OF(Separator));
    if (!Ret)
    {
        skip("GetLocaleInfoW failed with %lu\n", GetLastError());
        return;
    }
    Ret = GetLocaleInfoW(LOCALE_USER_DEFAULT,
                         LOCALE_SGROUPING,
                         Grouping,
                         RTL_NUMBER_OF(Grouping));
    if (!Ret)
    {
        skip("GetLocaleInfoW failed with %lu\n", GetLastError());
        return;
    }

    if (wcscmp(Grouping, L"3;0"))
    {
        skip("Skipping remaining tests due to incompatible locale (separator '%ls', grouping '%ls')\n",
             Separator, Grouping);
        return;
    }

    RtlFillMemory(Number, sizeof(Number), 0x55);
    Ptr = AddCommasW(123456789, Number);
    ok(Ptr == Number, "Ptr = %p, expected %p\n", Ptr, Number);
    StringCbPrintfW(Expected, sizeof(Expected), L"123%ls456%ls789", Separator, Separator);
    ok(!wcscmp(Number, Expected), "Number = '%ls', expected %ls\n", Number, Expected);
    ok(Number[wcslen(Number) + 1] == 0x5555, "Number[N] = 0x%x\n", Number[wcslen(Number) + 1]);

    RtlFillMemory(Number, sizeof(Number), 0x55);
    Ptr = AddCommasW(4294967295U, Number);
    ok(Ptr == Number, "Ptr = %p, expected %p\n", Ptr, Number);
    StringCbPrintfW(Expected, sizeof(Expected), L"4%ls294%ls967%ls295", Separator, Separator, Separator);
    ok(!wcscmp(Number, Expected), "Number = '%ls', expected %ls\n", Number, Expected);
    ok(Number[wcslen(Number) + 1] == 0x5555, "Number[N] = 0x%x\n", Number[wcslen(Number) + 1]);
}
