/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     0BSD (https://spdx.org/licenses/0BSD.html)
 * PURPOSE:     Test for RtlxUnicodeStringToAnsiSize
 * COPYRIGHT:   Copyright 2021 Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include "precomp.h"

static const struct
{
    ULONG AnsiCp;
    ULONG OemCp;
    PCWSTR Str;
    ULONG OemLength;
} TestData[] =
{
    { 1252, 932, L"\u30c7\u30b9\u30af\u30c8\u30c3\u30d7", 7 }, /* "Desktop" in Japanese */
    { 932, 1252, L"\u30c7\u30b9\u30af\u30c8\u30c3\u30d7", 13 }, /* "Desktop" in Japanese */
};

START_TEST(RtlxUnicodeStringToAnsiSize)
{
    for (int i = 0; i < _countof(TestData); i++)
    {
        SetupLocale(TestData[i].AnsiCp, TestData[i].OemCp, -1);
        UNICODE_STRING Ustr;
        RtlInitUnicodeString(&Ustr, TestData[i].Str);
        ULONG Length = RtlxUnicodeStringToAnsiSize(&Ustr);
        ok(Length == TestData[i].OemLength, "Wrong OEM length for test %u, expected %u, got %u\n",
           i, (UINT)TestData[i].OemLength, (UINT)Length);
    }
}
