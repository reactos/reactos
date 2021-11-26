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

static const WCHAR FormatStringInts[] = L"%d %d %d";
static const WCHAR FormatStringIntsResult[] = L"1 2 3";
static const WCHAR FormatStringStrs[] = L"%s %s %s";


TESTAPI
Test_RtlUnicodeStringPrintf()
{
    NTSTATUS Status;
    PWSTR pBuffer = NULL;
    USHORT BufferSize;
    size_t EqualBytes;
    UNICODE_STRING UsString;

    KmtStartSeh();

    /* STATUS_SUCCESS test */
    BufferSize = 6 * sizeof(WCHAR);
    pBuffer = KmtAllocateGuarded(BufferSize);
    if (!pBuffer)
        goto Cleanup;

    RtlFillMemory(pBuffer, BufferSize, 0xAA);
    RtlInitEmptyUnicodeString(&UsString, pBuffer, BufferSize);

    Status = RtlUnicodeStringPrintf(&UsString, FormatStringInts, 1, 2, 3);
    EqualBytes = RtlCompareMemory(UsString.Buffer, FormatStringIntsResult, sizeof(FormatStringIntsResult));
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_size(EqualBytes, sizeof(FormatStringIntsResult));
    ok_eq_uint(UsString.Length, sizeof(FormatStringIntsResult) - sizeof(WCHAR));
    ok_eq_uint(UsString.MaximumLength, BufferSize);

    KmtFreeGuarded(pBuffer);
    pBuffer = NULL;

    /* STATUS_BUFFER_OVERFLOW tests */
    BufferSize = 2 * sizeof(WCHAR);
    pBuffer = KmtAllocateGuarded(BufferSize);
    if (!pBuffer)
        goto Cleanup;

    RtlInitEmptyUnicodeString(&UsString, pBuffer, BufferSize);

    Status = RtlUnicodeStringPrintf(&UsString, FormatStringStrs, L"AAA", L"BBB", L"CCC");
    EqualBytes = RtlCompareMemory(UsString.Buffer, L"AA", BufferSize);
    ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_size(EqualBytes, BufferSize);
    ok_eq_uint(UsString.Length, UsString.MaximumLength);

    KmtFreeGuarded(pBuffer);
    pBuffer = NULL;


    BufferSize = 7 * sizeof(WCHAR);
    pBuffer = KmtAllocateGuarded(BufferSize);
    if (!pBuffer)
        goto Cleanup;

    RtlInitEmptyUnicodeString(&UsString, pBuffer, BufferSize);

    Status = RtlUnicodeStringPrintf(&UsString, FormatStringStrs, L"0123", L"4567", L"89AB");
    EqualBytes = RtlCompareMemory(UsString.Buffer, L"0123 45", BufferSize);
    ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_size(EqualBytes, BufferSize);
    ok_eq_uint(UsString.Length, UsString.MaximumLength);

    KmtFreeGuarded(pBuffer);
    pBuffer = NULL;

    // Note: RtlUnicodeStringPrintf returns STATUS_BUFFER_OVERFLOW here while RtlUnicodeStringPrintfEx returns STATUS_INVALID_PARAMETER!
    // Documented on MSDN and verified with the Win10 version of ntstrsafe.h
    RtlInitEmptyUnicodeString(&UsString, NULL, 0);
    Status = RtlUnicodeStringPrintf(&UsString, FormatStringStrs, L"AAA", L"BBB", L"CCC");
    ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);


Cleanup:
    if (pBuffer)
        KmtFreeGuarded(pBuffer);

    // None of these functions should have crashed.
    KmtEndSeh(STATUS_SUCCESS);
}

TESTAPI
Test_RtlUnicodeStringPrintfEx()
{
    NTSTATUS Status;
    PWSTR pBuffer = NULL;
    USHORT BufferSize;
    size_t EqualBytes;
    UNICODE_STRING RemString;
    UNICODE_STRING UsString;
    WCHAR FillResult[10];

    RtlFillMemory(FillResult, sizeof(FillResult), 0xAA);

    KmtStartSeh();

    /* STATUS_SUCCESS test, fill behind flag: low-byte as fill character */
    BufferSize = sizeof(FormatStringIntsResult) - sizeof(UNICODE_NULL) + sizeof(FillResult);
    pBuffer = KmtAllocateGuarded(BufferSize);
    if (!pBuffer)
        goto Cleanup;

    RtlInitEmptyUnicodeString(&UsString, pBuffer, BufferSize);
    RtlInitEmptyUnicodeString(&RemString, NULL, 0);

    Status = RtlUnicodeStringPrintfEx(&UsString, &RemString, STRSAFE_FILL_BEHIND | 0xAA, FormatStringInts, 1, 2, 3);
    EqualBytes = RtlCompareMemory(UsString.Buffer, FormatStringIntsResult, sizeof(FormatStringIntsResult) - sizeof(WCHAR));
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_size(EqualBytes, sizeof(FormatStringIntsResult) - sizeof(WCHAR));
    ok_eq_uint(UsString.Length, sizeof(FormatStringIntsResult) - sizeof(WCHAR));
    ok_eq_uint(UsString.MaximumLength, BufferSize);

    ok_eq_pointer(RemString.Buffer, &UsString.Buffer[UsString.Length / sizeof(WCHAR)]);
    ok_eq_uint(RemString.Length, 0);
    ok_eq_uint(RemString.MaximumLength, UsString.MaximumLength - UsString.Length);

    EqualBytes = RtlCompareMemory(RemString.Buffer, FillResult, RemString.MaximumLength);
    ok_eq_size(EqualBytes, sizeof(FillResult));

    KmtFreeGuarded(pBuffer);
    pBuffer = NULL;


    /* STATUS_BUFFER_OVERFLOW test  */
    BufferSize = 8 * sizeof(WCHAR);
    pBuffer = KmtAllocateGuarded(BufferSize);
    if (!pBuffer)
        goto Cleanup;

    RtlInitEmptyUnicodeString(&UsString, pBuffer, BufferSize);
    RtlInitEmptyUnicodeString(&RemString, NULL, 0);

    Status = RtlUnicodeStringPrintfEx(&UsString, &RemString, 0, FormatStringStrs, L"AAA", L"BBB", L"CCC");
    EqualBytes = RtlCompareMemory(UsString.Buffer, L"AAA BBB ", UsString.Length);
    ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_size(EqualBytes, UsString.Length);
    ok_eq_uint(UsString.Length, UsString.MaximumLength);

    ok_eq_pointer(RemString.Buffer, &UsString.Buffer[UsString.Length / sizeof(WCHAR)]);
    ok_eq_uint(RemString.Length, 0);
    ok_eq_uint(RemString.MaximumLength, 0);

    KmtFreeGuarded(pBuffer);
    pBuffer = NULL;


    // Note: RtlUnicodeStringPrintf returns STATUS_BUFFER_OVERFLOW here while RtlUnicodeStringPrintfEx returns STATUS_INVALID_PARAMETER!
    // Documented on MSDN and verified with the Win10 version of ntstrsafe.h
    RtlInitEmptyUnicodeString(&UsString, NULL, 0);
    Status = RtlUnicodeStringPrintfEx(&UsString, NULL, 0, FormatStringStrs, L"AAA", L"BBB", L"CCC");
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);


Cleanup:
    if (pBuffer)
        KmtFreeGuarded(pBuffer);

    // None of these functions should have crashed.
    KmtEndSeh(STATUS_SUCCESS);
}

START_TEST(RtlStrSafe)
{
    Test_RtlUnicodeStringPrintf();
    Test_RtlUnicodeStringPrintfEx();
}
