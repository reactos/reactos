/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for RtlUnicodeStringToAnsiString
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include "precomp.h"

START_TEST(RtlUnicodeStringToAnsiString)
{
    WCHAR BufferU[10];
    CHAR BufferA[10];
    UNICODE_STRING StringU;
    ANSI_STRING StringA;
    NTSTATUS Status;
    DWORD i;

    memset(BufferU, 0xAA, sizeof(BufferU));
    memset(BufferA, 0xAA, sizeof(BufferA));

    BufferU[0] = L'A';
    BufferU[1] = UNICODE_NULL;

    StringU.Buffer = BufferU;
    StringU.MaximumLength = 10 * sizeof(WCHAR);

    RtlInitUnicodeString(&StringU, BufferU);
    ok(StringU.Length == 1 * sizeof(WCHAR), "Invalid size: %d\n", StringU.Length);
    ok(StringU.MaximumLength == 2 * sizeof(WCHAR), "Invalid size: %d\n", StringU.MaximumLength);
    ok(StringU.Buffer == BufferU, "Invalid buffer: %p\n", StringU.Buffer);

    StringA.Buffer = BufferA;
    StringA.MaximumLength = 10 * sizeof(CHAR);

    Status = RtlUnicodeStringToAnsiString(&StringA, &StringU, FALSE);
    ok(NT_SUCCESS(Status), "RtlUnicodeStringToAnsiString failed: %lx\n", Status);
    ok(StringA.Length == 1 * sizeof(CHAR), "Invalid size: %d\n", StringA.Length);
    ok(StringA.MaximumLength == 10 * sizeof(CHAR), "Invalid size: %d\n", StringA.MaximumLength);
    ok(StringA.Buffer == BufferA, "Invalid buffer: %p\n", StringA.Buffer);

    for (i = 0; i < 10; ++i)
    {
        if (BufferA[i] == 0)
        {
            break;
        }
    }

    ok(i != 10, "String was not null terminated!\n");
}
