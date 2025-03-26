/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for RtlGetLengthWithoutLastFullDosOrNtPathElement
 * COPYRIGHT:   Copyright 2021 Mark Jansen <mark.jansen@reactos.org>
 */

#include "precomp.h"


NTSTATUS
NTAPI
RtlGetLengthWithoutLastFullDosOrNtPathElement(
    IN ULONG Flags,
    IN PCUNICODE_STRING Path,
    OUT PULONG LengthOut);


typedef struct _rtl_test_data
{
    LPCWSTR Path;
    ULONG Length;
    NTSTATUS Status;
} rtl_test_data;

// Based on http://undoc.airesoft.co.uk/ntdll.dll/RtlGetLengthWithoutLastFullDosOrNtPathElement.php
rtl_test_data tests[] = {
    { L"", 0, STATUS_SUCCESS, },
    { L"C", 0, STATUS_INVALID_PARAMETER, },
    { L"C:", 0, STATUS_INVALID_PARAMETER, },
    { L"C:\\", 0, STATUS_SUCCESS, },
    { L"C:\\test", 3, STATUS_SUCCESS, },
    { L"C:\\test\\", 3, STATUS_SUCCESS, },
    { L"C:\\test\\a", 8, STATUS_SUCCESS, },
    { L"C:\\test\\a\\", 8, STATUS_SUCCESS, },
    { L"C://test", 3, STATUS_SUCCESS, },
    { L"C://test\\", 3, STATUS_SUCCESS, },
    { L"C://test\\\\", 3, STATUS_SUCCESS, },
    { L"C://test/", 3, STATUS_SUCCESS, },
    { L"C://test//", 3, STATUS_SUCCESS, },
    { L"C://test\\a", 9, STATUS_SUCCESS, },
    { L"C://test\\\\a", 9, STATUS_SUCCESS, },
    { L"C://test/a", 9, STATUS_SUCCESS, },
    { L"C://test//a", 9, STATUS_SUCCESS, },
    { L"C://test\\a\\", 9, STATUS_SUCCESS, },
    { L"C://test//a//", 9, STATUS_SUCCESS, },
    { L"C://test//a/", 9, STATUS_SUCCESS, },
    { L"X", 0, STATUS_INVALID_PARAMETER, },
    { L"X:", 0, STATUS_INVALID_PARAMETER, },
    { L"X:\\", 0, STATUS_SUCCESS, },
    { L"D:\\Test\\hello.ext", 8, STATUS_SUCCESS, },
    { L"\\\\?\\C", 0, STATUS_INVALID_PARAMETER, },
    { L"\\\\?\\C:", 0, STATUS_INVALID_PARAMETER, },
    { L"\\\\?\\CC", 0, STATUS_INVALID_PARAMETER, },
    { L"\\\\?\\C:\\", 4, STATUS_SUCCESS, },
    { L"\\\\?\\::\\", 4, STATUS_SUCCESS, },
    { L"\\\\?\\CCC", 0, STATUS_INVALID_PARAMETER, },
    { L"\\\\?\\CCC\\", 0, STATUS_INVALID_PARAMETER, },
    { L"\\??\\UNC\\Mytest", 8, STATUS_SUCCESS, },
    { L"\\SystemRoot", 0, STATUS_SUCCESS, },
    { L"\\SystemRoot\\", 0, STATUS_SUCCESS, },
    { L"\\SystemRoot\\ntdll.dll", 12, STATUS_SUCCESS, },
    { L"\\Device\\HarddiskVolume9000", 8, STATUS_SUCCESS, },
    { L"\\Stuff\\doesnt\\really\\matter", 21, STATUS_SUCCESS, },
    { L"this\\doesnt\\really\\work", 0, STATUS_INVALID_PARAMETER, },
    { L"multi(0)disk(0)rdisk(0)partition(1)", 0, STATUS_INVALID_PARAMETER, },
    { L"multi(0)disk(0)rdisk(0)partition(1)\\test", 0, STATUS_INVALID_PARAMETER, },
    { L"xyz", 0, STATUS_INVALID_PARAMETER, },
    { L"CON", 0, STATUS_INVALID_PARAMETER, },
    { L":", 0, STATUS_INVALID_PARAMETER, },
    { L"\\\\", 0, STATUS_SUCCESS, },
};


START_TEST(RtlGetLengthWithoutLastFullDosOrNtPathElement)
{
    UNICODE_STRING Dum;
    NTSTATUS Status;
    ULONG Length;
    RtlInitUnicodeString(&Dum, L"c:\\test\\");

    Length = 333;
    Status = RtlGetLengthWithoutLastFullDosOrNtPathElement(0, NULL, &Length);
    ok_int(Length, 0);
    ok_hex(Status, STATUS_INVALID_PARAMETER);

    Status = RtlGetLengthWithoutLastFullDosOrNtPathElement(0, &Dum, NULL);
    ok_hex(Status, STATUS_INVALID_PARAMETER);

    for (ULONG n = 0; n < 32; ++n)
    {
        Length = 333;
        Status = RtlGetLengthWithoutLastFullDosOrNtPathElement((1 << n), &Dum, &Length);
        ok_int(Length, 0);
        ok_hex(Status, STATUS_INVALID_PARAMETER);
    }

    for (ULONG n = 0; n < ARRAYSIZE(tests); ++n)
    {
        UNICODE_STRING Str;
        Length = 333;

        RtlInitUnicodeString(&Str, tests[n].Path);

        Status = RtlGetLengthWithoutLastFullDosOrNtPathElement(0, &Str, &Length);
        ok(Status == tests[n].Status, "Got Status=0x%lx, expected 0x%lx (%S)\n", Status, tests[n].Status, Str.Buffer);
        ok(Length == tests[n].Length, "Got Length=0x%lx, expected 0x%lx (%S)\n", Length, tests[n].Length, Str.Buffer);
    }
}
