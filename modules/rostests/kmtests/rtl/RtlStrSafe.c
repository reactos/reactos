/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Test for ntstrsafe.h functions
 * COPYRIGHT:       Copyright 2018 Hernán Di Pietro <hernan.di.pietro@gmail.com>
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
    WCHAR BufferSmall[2];
    WCHAR BufferSmall2[7];
    UNICODE_STRING UsString;
    WCHAR FormatStringInts[] = L"%d %d %d";
    WCHAR FormatStringStrs[] = L"%s %s %s";

    /* No zeros (Don't assume UNICODE_STRINGS are NULL terminated) */

    RtlFillMemory(Buffer, sizeof(Buffer), 0xAA);
    RtlFillMemory(BufferSmall, sizeof(BufferSmall), 0xAA);
    RtlFillMemory(BufferSmall2, sizeof(BufferSmall2), 0xAA);

    /* STATUS_SUCCESS test */

    UsString.Buffer = Buffer;
    UsString.Length = 0;
    UsString.MaximumLength = sizeof(Buffer);

    const WCHAR Result[] = L"1 2 3";
    ok_eq_hex(RtlUnicodeStringPrintf(&UsString, FormatStringInts, 1, 2, 3), STATUS_SUCCESS);
    ok_eq_uint(UsString.Length, sizeof(Result) - sizeof(WCHAR));
    ok_eq_uint(UsString.MaximumLength, sizeof(Buffer));
    ok_eq_wchar(UsString.Buffer[0], L'1');
    ok_eq_wchar(UsString.Buffer[1], L' ');
    ok_eq_wchar(UsString.Buffer[2], L'2');
    ok_eq_wchar(UsString.Buffer[3], L' ');
    ok_eq_wchar(UsString.Buffer[4], L'3');
    ok_eq_wchar(UsString.Buffer[5], (WCHAR) 0);
    
    /* STATUS_BUFFER_OVERFLOW tests */

    UsString.Buffer = BufferSmall;
    UsString.Length = 0;
    UsString.MaximumLength = sizeof(BufferSmall);
    
    ok_eq_hex(RtlUnicodeStringPrintf(&UsString, FormatStringStrs, L"AAA", L"BBB", L"CCC"), STATUS_BUFFER_OVERFLOW);
    ok_eq_uint(UsString.Length, UsString.MaximumLength);
    ok_eq_char(UsString.Buffer[0], L'A');
    ok_eq_char(UsString.Buffer[1], (WCHAR)0);

    UsString.Buffer = BufferSmall2;
    UsString.Length = 0;
    UsString.MaximumLength = sizeof(BufferSmall2);

    ok_eq_hex(RtlUnicodeStringPrintf(&UsString, FormatStringStrs, L"0123", L"4567", L"89AB"), STATUS_BUFFER_OVERFLOW);
    ok_eq_uint(UsString.Length, UsString.MaximumLength);
    ok_eq_char(UsString.Buffer[0], L'0');
    ok_eq_char(UsString.Buffer[1], L'1');
    ok_eq_char(UsString.Buffer[2], L'2');
    ok_eq_char(UsString.Buffer[3], L'3');
    ok_eq_char(UsString.Buffer[4], L' ');
    ok_eq_char(UsString.Buffer[5], L'4');
    ok_eq_char(UsString.Buffer[6], (WCHAR) 0);

    ///* STATUS_INVALID_PARAMETER tests */

    ok_eq_hex(RtlUnicodeStringPrintf(NULL, FormatStringStrs, L"AAA", L"BBB", L"CCC"), STATUS_INVALID_PARAMETER);
    ok_eq_hex(RtlUnicodeStringPrintf(&UsString, NULL, L"AAA", L"BBB", L"CCC"), STATUS_INVALID_PARAMETER);

    UNICODE_STRING UsStringNull;
    UsStringNull.Buffer = (PWCH)NULL;
    UsStringNull.Length = 0;
    UsStringNull.MaximumLength = 0;
    ok_eq_bool(RtlUnicodeStringPrintf(&UsStringNull, FormatStringStrs, L"AAA", L"BBB", L"CCC"), STATUS_INVALID_PARAMETER);

    /* Test  for buffer overruns */

    RtlFillMemory(Buffer, sizeof(Buffer), 0xAA);
    RtlFillMemory(OvrBuffer, sizeof(OvrBuffer), 0xAA);
    UsString.Buffer = Buffer;
    UsString.Length = 0;
    UsString.MaximumLength = 16 * sizeof(WCHAR);

    ok_eq_hex(RtlUnicodeStringPrintf(&UsString, FormatStringStrs, L"abc", L"def", L"ghi"), STATUS_SUCCESS);
    ok_eq_uint(UsString.Length, sizeof(L"abc def ghi") -sizeof(WCHAR));
    ok_eq_char(UsString.Buffer[11], (WCHAR)0);
    ok_eq_uint(0, memcmp(OvrBuffer + 12, Buffer + 12, sizeof(Buffer) - (12 * sizeof(WCHAR))));
}

TESTAPI
Test_RtlUnicodeStringPrintfEx()
{
    WCHAR Buffer[32];
    WCHAR BufferSmall[8] = { 0 };
    WCHAR OvrBuffer[1024];
    UNICODE_STRING UsString, RemString;
    WCHAR FormatStringInts[] = L"%d %d %d";
    WCHAR FormatStringStrs[] = L"%s %s %s";

    /* No zeros (Don't assume UNICODE_STRINGS are NULL terminated) */

    RtlFillMemory(Buffer, sizeof(Buffer), 0xAA);

    UsString.Buffer = Buffer;
    UsString.Length = 0;
    UsString.MaximumLength = sizeof(Buffer);

    RemString.Buffer = (PWCH)NULL;
    RemString.Length = 0;
    RemString.MaximumLength = 0;

    /* STATUS_SUCCESS test, fill behind flag: low-byte as fill character */

    const WCHAR Result[] = L"1 2 3";
    ok_eq_hex(RtlUnicodeStringPrintfEx(&UsString, &RemString, STRSAFE_FILL_BEHIND | 0xFF, FormatStringInts, 1, 2, 3), STATUS_SUCCESS);
    
    ok_eq_uint(UsString.Length, sizeof(Result) - sizeof(WCHAR));
    ok_eq_uint(UsString.MaximumLength, sizeof(Buffer));
    ok_eq_uint(memcmp(UsString.Buffer, Result, sizeof(Result) - sizeof(WCHAR)), 0);

    ok_eq_pointer(RemString.Buffer, UsString.Buffer + (UsString.Length / sizeof(WCHAR)));
    ok_eq_uint(RemString.Length, 0);
    ok_eq_uint(RemString.MaximumLength, UsString.MaximumLength - UsString.Length);

    /* STATUS_BUFFER_OVERFLOW test  */

    UsString.Buffer = BufferSmall;
    UsString.Length = 0;
    UsString.MaximumLength = sizeof(BufferSmall);

    RemString.Buffer = (PWCH)NULL;
    RemString.Length = 0;
    RemString.MaximumLength = 0;
    
    ok_eq_hex(RtlUnicodeStringPrintfEx(&UsString, &RemString, 0, FormatStringStrs, L"AAA", L"BBB", L"CCC"), STATUS_BUFFER_OVERFLOW);
    ok_eq_uint(UsString.Length, UsString.MaximumLength);
    ok_eq_char(UsString.Buffer[0], L'A');
    ok_eq_char(UsString.Buffer[1], L'A');
    ok_eq_char(UsString.Buffer[2], L'A');
    ok_eq_char(UsString.Buffer[3], L' ');
    ok_eq_char(UsString.Buffer[4], L'B');
    ok_eq_char(UsString.Buffer[5], L'B');
    ok_eq_char(UsString.Buffer[6], L'B');
    ok_eq_char(UsString.Buffer[7], (WCHAR)0);

    // Takes \0 into account
    ok_eq_pointer(RemString.Buffer, UsString.Buffer + (UsString.Length - 1) / sizeof(WCHAR));
    ok_eq_uint(RemString.Length, 0); 
    ok_eq_uint(RemString.MaximumLength, 2);

    /* Test  for buffer overruns */

    RtlFillMemory(Buffer, sizeof(Buffer), 0xAA);
    RtlFillMemory(OvrBuffer, sizeof(OvrBuffer), 0xAA);
    UsString.Buffer = Buffer;
    UsString.Length = 0;
    UsString.MaximumLength = 16 * sizeof(WCHAR);

    ok_eq_hex(RtlUnicodeStringPrintfEx(&UsString, &RemString, 0, FormatStringStrs, L"abc", L"def", L"ghi"), STATUS_SUCCESS);
    ok_eq_uint(UsString.Length, sizeof(L"abc def ghi") - sizeof(WCHAR));
    ok_eq_char(UsString.Buffer[11], (WCHAR)0);
    ok_eq_uint(0, memcmp(OvrBuffer + 12, Buffer + 12, sizeof(Buffer) - (12 * sizeof(WCHAR))));
}

START_TEST(RtlStrSafe)
{
    Test_RtlUnicodeStringPrintf();
    Test_RtlUnicodeStringPrintfEx();
}
