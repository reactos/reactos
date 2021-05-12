/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later.html)
 * PURPOSE:     Test for RtlUnicodeStringToCountedOemString
 * COPYRIGHT:   Copyright 2021 Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include "precomp.h"

static const struct
{
    ULONG AnsiCp;
    ULONG OemCp;
    const UNICODE_STRING StrU;
    NTSTATUS Status;
    const OEM_STRING StrOem;
} TestData[] =
{
    {
        1252, 932, /* Western SBCS - Modified SJIS */
        RTL_CONSTANT_STRING(L"\u30c7\u30b9\u30af\u30c8\u30c3\u30d7"),
        STATUS_BUFFER_OVERFLOW,
        RTL_CONSTANT_STRING("\x83\x66\x83\x58\x83\x4e")
    },
    {
        932, 1252, /* Modified SJIS - Western SBCS */
        RTL_CONSTANT_STRING(L"\u30c7\u30b9\u30af\u30c8\u30c3\u30d7"),
        STATUS_UNMAPPABLE_CHARACTER,
        RTL_CONSTANT_STRING("??????")
    },
    {
        932, 932, /* Modified SJIS - Modified SJIS */
        RTL_CONSTANT_STRING(L"\u30c7\u30b9\u30af\u30c8\u30c3\u30d7"),
        STATUS_SUCCESS,
        RTL_CONSTANT_STRING("\x83\x66\x83\x58\x83\x4e\x83\x67\x83\x62\x83\x76")
    },
    {
        1252, 1252, /* Western SBCS */
        RTL_CONSTANT_STRING(L"\u30c7\u30b9\u30af\u30c8\u30c3\u30d7"), /* Some japanese characters */
        STATUS_UNMAPPABLE_CHARACTER,
        RTL_CONSTANT_STRING("??????")
    },
};

START_TEST(RtlUnicodeStringToCountedOemString)
{
    WCHAR BufferU[10];
    CHAR BufferOem[10];
    UNICODE_STRING StringU;
    OEM_STRING StringOem;
    NTSTATUS Status;
    int i;

    memset(BufferU, 0xAA, sizeof(BufferU));
    memset(BufferOem, 0xAA, sizeof(BufferOem));

    BufferU[0] = L'A';
    BufferU[1] = UNICODE_NULL;

    StringU.Buffer = BufferU;
    StringU.MaximumLength = 10 * sizeof(WCHAR);

    RtlInitUnicodeString(&StringU, BufferU);
    ok(StringU.Length == 1 * sizeof(WCHAR), "Invalid size: %d\n", StringU.Length);
    ok(StringU.MaximumLength == 2 * sizeof(WCHAR), "Invalid size: %d\n", StringU.MaximumLength);
    ok(StringU.Buffer == BufferU, "Invalid buffer: %p\n", StringU.Buffer);

    StringOem.Buffer = BufferOem;
    StringOem.Length = 0;
    StringOem.MaximumLength = 10 * sizeof(CHAR);

    Status = RtlUnicodeStringToCountedOemString(&StringOem, &StringU, FALSE);
    ok(NT_SUCCESS(Status), "RtlUnicodeStringToCountedOemString failed: %lx\n", Status);
    ok(StringOem.Length == 1 * sizeof(CHAR), "Invalid size: %d\n", StringOem.Length);
    ok(StringOem.MaximumLength == 10 * sizeof(CHAR), "Invalid size: %d\n", StringOem.MaximumLength);
    ok(StringOem.Buffer == BufferOem, "Invalid buffer: %p\n", StringOem.Buffer);

    ok(BufferOem[0] == 'A', "Unexpected first char 0x%02x for OEM string.\n", BufferOem[0]);
    for (i = 1; i < 10; ++i)
    {
        ok(BufferOem[i] == (CHAR)0xAA, "Unexpected char 0x%02x at position %d.\n", BufferOem[i], i);
    }

    ok(i == 10, "String was not null terminated!\n");

    /* Test buffer overflow */
    wcsncpy(BufferU, L"Test", _countof(BufferU));
    RtlInitUnicodeString(&StringU, BufferU);
    StringU.MaximumLength = sizeof(BufferU);
    StringOem.Buffer = BufferOem;
    StringOem.MaximumLength = 1 * sizeof(CHAR);
    StringOem.Length = 0;
    memset(BufferOem, 0xAA, sizeof(BufferOem));

    Status = RtlUnicodeStringToCountedOemString(&StringOem, &StringU, FALSE);
    ok_ntstatus(Status, STATUS_BUFFER_OVERFLOW);
    ok(StringOem.Length == 4 * sizeof(CHAR), "Invalid size: %d\n", StringOem.Length);
    ok(StringOem.MaximumLength == 1 * sizeof(CHAR), "Invalid size: %d\n", StringOem.MaximumLength);
    ok(StringOem.Buffer == BufferOem, "Invalid buffer: %p\n", StringOem.Buffer);

    for (i = 0; i < 10; ++i)
    {
        ok(BufferOem[i] == (CHAR)0xAA, "Unexpected char 0x%02x at position %d.\n", BufferOem[i], i);
    }

    for (i = 0; i < _countof(TestData); i++)
    {
        SetupLocale(TestData[i].AnsiCp, TestData[i].OemCp, -1);

        trace("Testing locale %u. ANSI: %u, OEM %u\n", i, (UINT)TestData[i].AnsiCp, (UINT)TestData[i].OemCp);

        /* Get the right length */
        StringOem.Buffer = NULL;
        StringOem.Length = 0;
        StringOem.MaximumLength = 0;

        Status = RtlUnicodeStringToCountedOemString(&StringOem, &TestData[i].StrU, FALSE);
        ok_ntstatus(Status, STATUS_BUFFER_OVERFLOW);
        ok_long(StringOem.Length, TestData[i].StrOem.Length);
        ok_long(StringOem.MaximumLength, 0);

        StringOem.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, StringOem.Length);
        StringOem.MaximumLength = StringOem.Length;
        StringOem.Length = 0;

        Status = RtlUnicodeStringToCountedOemString(&StringOem, &TestData[i].StrU, FALSE);
        ok_ntstatus(Status, TestData[i].Status);

        ok_long(StringOem.Length, TestData[i].StrOem.Length);
        ok_long(StringOem.MaximumLength, TestData[i].StrOem.Length); /* Untouched */
        ok_long(memcmp(StringOem.Buffer, TestData[i].StrOem.Buffer, min(StringOem.Length, TestData[i].StrOem.Length)), 0);

        RtlFreeHeap(RtlGetProcessHeap(), 0, StringOem.Buffer);
    }
}

