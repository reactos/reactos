/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Test for ntstrsafe.h functions
 * COPYRIGHT:       Copyright 2018 Hernán Di Pietro <hernan.di.pietro@gmail.com>
 *                  Copyright 2019 Colin Finck <colin@reactos.org>
 */

#define KMT_EMULATE_KERNEL
#include <kmt_test.h>
#include <ntstrsafe.h>
#include <ntdef.h>
#include <ndk/rtlfuncs.h>

#define TESTAPI static void

TESTAPI
Test_RtlUnicodeStringPrintf()
{
    WCHAR Buffer[1024];
    WCHAR OvrBuffer[1024];
    WCHAR BufferSmall[16];
    UNICODE_STRING UsString;
    const WCHAR FormatStringInts[] = L"%d %d %d";
    const WCHAR FormatStringStrs[] = L"%s %s %s";
    const WCHAR Result[] = L"1 2 3";
    UNICODE_STRING UsStringNull;
    int i;
    NTSTATUS Status;

    /* No zeros (Don't assume UNICODE_STRINGS are NULL terminated) */

    RtlFillMemory(Buffer, sizeof(Buffer), 0xAA);
    RtlFillMemory(BufferSmall, sizeof(BufferSmall), 0xAA);

    /* STATUS_SUCCESS test */
    RtlInitEmptyUnicodeString(&UsString, Buffer, sizeof(Buffer));

    KmtStartSeh();
    Status = RtlUnicodeStringPrintf(&UsString, FormatStringInts, 1, 2, 3);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(UsString.Length, sizeof(Result) - sizeof(WCHAR));
    ok_eq_uint(UsString.MaximumLength, sizeof(Buffer));
    ok_eq_wchar(UsString.Buffer[0], L'1');
    ok_eq_wchar(UsString.Buffer[1], L' ');
    ok_eq_wchar(UsString.Buffer[2], L'2');
    ok_eq_wchar(UsString.Buffer[3], L' ');
    ok_eq_wchar(UsString.Buffer[4], L'3');
    ok_eq_wchar(UsString.Buffer[5], L'\0');

    /* STATUS_BUFFER_OVERFLOW tests */
    RtlInitEmptyUnicodeString(&UsString, BufferSmall, 2 * sizeof(WCHAR));

    Status = RtlUnicodeStringPrintf(&UsString, FormatStringStrs, L"AAA", L"BBB", L"CCC");
    ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_uint(UsString.Length, UsString.MaximumLength);
    ok_eq_wchar(UsString.Buffer[0], L'A');
    ok_eq_wchar(UsString.Buffer[1], L'A');
    for (i = 2; i < sizeof(BufferSmall) / sizeof(BufferSmall[0]); i++)
        ok(UsString.Buffer[i] == 0xAAAA, "UsString.Buffer[%d] is 0x%hx\n", i, (USHORT)UsString.Buffer[i]);

    RtlInitEmptyUnicodeString(&UsString, BufferSmall, 7 * sizeof(WCHAR));

    Status = RtlUnicodeStringPrintf(&UsString, FormatStringStrs, L"0123", L"4567", L"89AB");
    ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_uint(UsString.Length, UsString.MaximumLength);
    ok_eq_wchar(UsString.Buffer[0], L'0');
    ok_eq_wchar(UsString.Buffer[1], L'1');
    ok_eq_wchar(UsString.Buffer[2], L'2');
    ok_eq_wchar(UsString.Buffer[3], L'3');
    ok_eq_wchar(UsString.Buffer[4], L' ');
    ok_eq_wchar(UsString.Buffer[5], L'4');
    ok_eq_wchar(UsString.Buffer[6], L'5');
    for (i = 7; i < sizeof(BufferSmall) / sizeof(BufferSmall[0]); i++)
        ok(UsString.Buffer[i] == 0xAAAA, "UsString.Buffer[%d] is 0x%hx\n", i, (USHORT)UsString.Buffer[i]);

    // Note: RtlUnicodeStringPrintf returns STATUS_BUFFER_OVERFLOW here while RtlUnicodeStringPrintfEx returns STATUS_INVALID_PARAMETER!
    // Documented on MSDN and verified with the Win10 version of ntstrsafe.h
    RtlInitEmptyUnicodeString(&UsStringNull, NULL, 0);
    Status = RtlUnicodeStringPrintf(&UsStringNull, FormatStringStrs, L"AAA", L"BBB", L"CCC");
    ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);

    /* Test  for buffer overruns */

    RtlFillMemory(Buffer, sizeof(Buffer), 0xAA);
    RtlFillMemory(OvrBuffer, sizeof(OvrBuffer), 0xAA);
    RtlInitEmptyUnicodeString(&UsString, Buffer, 16 * sizeof(WCHAR));

    Status = RtlUnicodeStringPrintf(&UsString, FormatStringStrs, L"abc", L"def", L"ghi");
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(UsString.Length, sizeof(L"abc def ghi") - sizeof(WCHAR));
    ok_eq_wchar(UsString.Buffer[11], L'\0');
    ok_eq_uint(0, memcmp(OvrBuffer + 12, Buffer + 12, sizeof(Buffer) - (12 * sizeof(WCHAR))));

    // None of these functions should have crashed.
    KmtEndSeh(STATUS_SUCCESS);
}

TESTAPI
Test_RtlUnicodeStringPrintfEx()
{
    WCHAR Buffer[32];
    WCHAR BufferSmall[16];
    WCHAR OvrBuffer[1024];
    UNICODE_STRING UsString, RemString;
    UNICODE_STRING UsStringNull;
    const WCHAR FormatStringInts[] = L"%d %d %d";
    const WCHAR FormatStringStrs[] = L"%s %s %s";
    const WCHAR Result[] = L"1 2 3";
    int i;
    NTSTATUS Status;

    /* No zeros (Don't assume UNICODE_STRINGS are NULL terminated) */

    RtlFillMemory(Buffer, sizeof(Buffer), 0xAA);
    RtlFillMemory(BufferSmall, sizeof(BufferSmall), 0xAA);
    RtlInitEmptyUnicodeString(&UsString, Buffer, sizeof(Buffer));
    RtlInitEmptyUnicodeString(&RemString, NULL, 0);

    /* STATUS_SUCCESS test, fill behind flag: low-byte as fill character */

    KmtStartSeh();
    Status = RtlUnicodeStringPrintfEx(&UsString, &RemString, STRSAFE_FILL_BEHIND | 0xFF, FormatStringInts, 1, 2, 3);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(UsString.Length, sizeof(Result) - sizeof(WCHAR));
    ok_eq_uint(UsString.MaximumLength, sizeof(Buffer));
    ok_eq_uint(memcmp(UsString.Buffer, Result, sizeof(Result) - sizeof(WCHAR)), 0);
    for (i = sizeof(Result) - sizeof(WCHAR); i < sizeof(Buffer) / sizeof(Buffer[0]); i++)
        ok(UsString.Buffer[i] == 0xFFFF, "UsString.Buffer[%d] is 0x%hx\n", i, (USHORT)UsString.Buffer[i]);

    ok_eq_pointer(RemString.Buffer, &UsString.Buffer[UsString.Length / sizeof(WCHAR)]);
    ok_eq_uint(RemString.Length, 0);
    ok_eq_uint(RemString.MaximumLength, UsString.MaximumLength - UsString.Length);

    /* STATUS_BUFFER_OVERFLOW test  */
    RtlInitEmptyUnicodeString(&UsString, BufferSmall, 8 * sizeof(WCHAR));
    RtlInitEmptyUnicodeString(&RemString, NULL, 0);

    Status = RtlUnicodeStringPrintfEx(&UsString, &RemString, 0, FormatStringStrs, L"AAA", L"BBB", L"CCC");
    ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_uint(UsString.Length, UsString.MaximumLength);
    ok_eq_wchar(UsString.Buffer[0], L'A');
    ok_eq_wchar(UsString.Buffer[1], L'A');
    ok_eq_wchar(UsString.Buffer[2], L'A');
    ok_eq_wchar(UsString.Buffer[3], L' ');
    ok_eq_wchar(UsString.Buffer[4], L'B');
    ok_eq_wchar(UsString.Buffer[5], L'B');
    ok_eq_wchar(UsString.Buffer[6], L'B');
    ok_eq_wchar(UsString.Buffer[7], L' ');
    for (i = 8; i < sizeof(BufferSmall) / sizeof(WCHAR); i++)
        ok(UsString.Buffer[i] == 0xAAAA, "UsString.Buffer[%d] is 0x%hx\n", i, (USHORT)UsString.Buffer[i]);

    ok_eq_pointer(RemString.Buffer, &UsString.Buffer[UsString.Length / sizeof(WCHAR)]);
    ok_eq_uint(RemString.Length, 0);
    ok_eq_uint(RemString.MaximumLength, 0);

    /* Test  for buffer overruns */

    RtlFillMemory(Buffer, sizeof(Buffer), 0xAA);
    RtlFillMemory(OvrBuffer, sizeof(OvrBuffer), 0xAA);
    RtlInitEmptyUnicodeString(&UsString, Buffer, 16 * sizeof(WCHAR));

    Status = RtlUnicodeStringPrintfEx(&UsString, &RemString, 0, FormatStringStrs, L"abc", L"def", L"ghi");
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(UsString.Length, sizeof(L"abc def ghi") - sizeof(WCHAR));
    ok_eq_wchar(UsString.Buffer[11], L'\0');
    ok_eq_uint(0, memcmp(OvrBuffer + 12, Buffer + 12, sizeof(Buffer) - (12 * sizeof(WCHAR))));

    // Note: RtlUnicodeStringPrintf returns STATUS_BUFFER_OVERFLOW here while RtlUnicodeStringPrintfEx returns STATUS_INVALID_PARAMETER!
    // Documented on MSDN and verified with the Win10 version of ntstrsafe.h
    RtlInitEmptyUnicodeString(&UsStringNull, NULL, 0);
    Status = RtlUnicodeStringPrintfEx(&UsStringNull, NULL, 0, FormatStringStrs, L"AAA", L"BBB", L"CCC");
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    // None of these functions should have crashed.
    KmtEndSeh(STATUS_SUCCESS);
}

START_TEST(RtlStrSafe)
{
    Test_RtlUnicodeStringPrintf();
    Test_RtlUnicodeStringPrintfEx();
}
