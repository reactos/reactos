/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for GetCurrentDirectory
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

static
BOOLEAN
CheckBuffer(
    const VOID *Buffer,
    SIZE_T Size,
    UCHAR Value)
{
    const UCHAR *Array = Buffer;
    SIZE_T i;

    for (i = 0; i < Size; i++)
        if (Array[i] != Value)
        {
            trace("Expected %x, found %x at offset %lu\n", Value, Array[i], (ULONG)i);
            return FALSE;
        }
    return TRUE;
}

static
BOOLEAN
CheckStringBufferA(
    const VOID *Buffer,
    SIZE_T Size,
    PCSTR Expected,
    UCHAR Fill)
{
    const CHAR *Chars = Buffer;
    const UCHAR *UChars = Buffer;
    SIZE_T Length = strlen(Expected);
    SIZE_T i;

    if (Size < Length)
    {
        ok(0, "Size = %lu, Length = %lu\n", (ULONG)Size, (ULONG)Length);
        return FALSE;
    }

    for (i = 0; i < Length; i++)
        if (Chars[i] != Expected[i])
        {
            trace("Expected %x, found %x at offset %lu\n", Expected[i], Chars[i], (ULONG)i);
            return FALSE;
        }

    ok(Chars[i] == 0, "Expected null terminator, found %x at offset %lu\n", Chars[i], (ULONG)i);
    i++;

    for (; i < Size; i++)
        if (UChars[i] != Fill)
        {
            trace("Expected %x, found %x at offset %lu\n", Fill, UChars[i], (ULONG)i);
            return FALSE;
        }
    return TRUE;
}

static
BOOLEAN
CheckStringBufferW(
    const VOID *Buffer,
    SIZE_T Size,
    PCWSTR Expected,
    UCHAR Fill)
{
    const WCHAR *Chars = Buffer;
    const UCHAR *UChars = Buffer;
    SIZE_T Length = wcslen(Expected);
    SIZE_T i;

    if (Size < Length)
    {
        ok(0, "Size = %lu, Length = %lu\n", (ULONG)Size, (ULONG)Length);
        return FALSE;
    }

    for (i = 0; i < Length; i++)
        if (Chars[i] != Expected[i])
        {
            trace("Expected %x, found %x at offset %lu\n", Expected[i], Chars[i], (ULONG)i);
            return FALSE;
        }

    ok(Chars[i] == 0, "Expected null terminator, found %x at offset %lu\n", Chars[i], (ULONG)i);
    i++;

    i *= sizeof(WCHAR);

    for (; i < Size; i++)
        if (UChars[i] != Fill)
        {
            trace("Expected %x, found %x at offset %lu\n", Fill, UChars[i], (ULONG)i);
            return FALSE;
        }
    return TRUE;
}

static
VOID
TestGetCurrentDirectoryA(VOID)
{
    CHAR Buffer[MAX_PATH];
    DWORD Length;
    BOOL Ret;
    BOOLEAN Okay;

    Ret = SetCurrentDirectoryA("C:\\");
    ok(Ret == TRUE, "SetCurrentDirectory failed with %lu\n", GetLastError());

    Length = GetCurrentDirectoryA(0, NULL);
    ok(Length == sizeof("C:\\"), "Length = %lu\n", Length);

    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Length = GetCurrentDirectoryA(sizeof(Buffer), Buffer);
    ok(Length == sizeof("C:\\") - 1, "Length = %lu\n", Length);
    Okay = CheckStringBufferA(Buffer, sizeof(Buffer), "C:\\", 0x55);
    ok(Okay, "CheckStringBufferA failed\n");

    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Length = GetCurrentDirectoryA(0, Buffer);
    ok(Length == sizeof("C:\\"), "Length = %lu\n", Length);
    Okay = CheckBuffer(Buffer, sizeof(Buffer), 0x55);
    ok(Okay, "CheckBuffer failed\n");

    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Length = GetCurrentDirectoryA(1, Buffer);
    ok(Length == sizeof("C:\\"), "Length = %lu\n", Length);
    Okay = CheckBuffer(Buffer, sizeof(Buffer), 0x55);
    ok(Okay, "CheckBuffer failed\n");

    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Length = GetCurrentDirectoryA(2, Buffer);
    ok(Length == sizeof("C:\\"), "Length = %lu\n", Length);
    Okay = CheckBuffer(Buffer, sizeof(Buffer), 0x55);
    ok(Okay, "CheckBuffer failed\n");

    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Length = GetCurrentDirectoryA(3, Buffer);
    ok(Length == sizeof("C:\\"), "Length = %lu\n", Length);
    Okay = CheckBuffer(Buffer, sizeof(Buffer), 0x55);
    ok(Okay, "CheckBuffer failed\n");

    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Length = GetCurrentDirectoryA(4, Buffer);
    ok(Length == sizeof("C:\\") - 1, "Length = %lu\n", Length);
    Okay = CheckStringBufferA(Buffer, sizeof(Buffer), "C:\\", 0x55);
    ok(Okay, "CheckStringBufferA failed\n");
}

static
VOID
TestGetCurrentDirectoryW(VOID)
{
    WCHAR Buffer[MAX_PATH];
    DWORD Length;
    BOOL Ret;
    BOOLEAN Okay;

    Ret = SetCurrentDirectoryW(L"C:\\");
    ok(Ret == TRUE, "SetCurrentDirectory failed with %lu\n", GetLastError());

    Length = GetCurrentDirectoryW(0, NULL);
    ok(Length == sizeof("C:\\"), "Length = %lu\n", Length);

    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Length = GetCurrentDirectoryW(sizeof(Buffer) / sizeof(WCHAR), Buffer);
    ok(Length == sizeof("C:\\") - 1, "Length = %lu\n", Length);
    Okay = CheckStringBufferW(Buffer, sizeof(Buffer), L"C:\\", 0x55);
    ok(Okay, "CheckStringBufferW failed\n");

    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Length = GetCurrentDirectoryW(0, Buffer);
    ok(Length == sizeof("C:\\"), "Length = %lu\n", Length);
    Okay = CheckBuffer(Buffer, sizeof(Buffer), 0x55);
    ok(Okay, "CheckBuffer failed\n");

    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Length = GetCurrentDirectoryW(1, Buffer);
    ok(Length == sizeof("C:\\"), "Length = %lu\n", Length);
    Okay = CheckBuffer(Buffer, sizeof(Buffer), 0x55);
    ok(Okay, "CheckBuffer failed\n");

    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Length = GetCurrentDirectoryW(2, Buffer);
    ok(Length == sizeof("C:\\"), "Length = %lu\n", Length);
    Okay = CheckBuffer(Buffer, sizeof(Buffer), 0x55);
    ok(Okay, "CheckBuffer failed\n");

    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Length = GetCurrentDirectoryW(3, Buffer);
    ok(Length == sizeof("C:\\"), "Length = %lu\n", Length);
    Okay = CheckBuffer(Buffer, sizeof(Buffer), 0x55);
    ok(Okay, "CheckBuffer failed\n");

    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Length = GetCurrentDirectoryW(4, Buffer);
    ok(Length == sizeof("C:\\") - 1, "Length = %lu\n", Length);
    Okay = CheckStringBufferW(Buffer, sizeof(Buffer), L"C:\\", 0x55);
    ok(Okay, "CheckStringBufferW failed\n");
}

START_TEST(GetCurrentDirectory)
{
    TestGetCurrentDirectoryA();
    TestGetCurrentDirectoryW();
}
