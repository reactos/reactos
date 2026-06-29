/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for GetEnvironmentVariable(A/W)
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

static void Test_GetEnvironmentVariableA(void)
{
    CHAR Buffer[MAX_PATH];
    ULONG ActualLength, Length;

    /* Get the COMSPEC variable */
    memset(Buffer, 0xAB, sizeof(Buffer));
    Length = GetEnvironmentVariableA("COMSPEC", Buffer, ARRAYSIZE(Buffer));
    Buffer[MAX_PATH - 1] = '\0';
    ActualLength = (ULONG)strlen(Buffer);
    ok(ActualLength < ARRAYSIZE(Buffer) - 1, "ActualLength == ARRAYSIZE(Buffer) - 1. Not null-terminated?\n");
    ok_eq_ulong(Length, ActualLength);
    ok_eq_char(Buffer[ActualLength + 1], '\xAB');

    /* Get the length only (return value is including NULL char) */
    Length = GetEnvironmentVariableA("COMSPEC", NULL, 0);
    ok_eq_ulong(Length, ActualLength + 1);

    /* Test a buffer size that is too small */
    memset(Buffer, 0xAB, sizeof(Buffer));
    Length = GetEnvironmentVariableA("COMSPEC", Buffer, 5);
    ok_eq_ulong(Length, ActualLength + 1);
    ok_eq_char(Buffer[0], '\xAB');

    /* Test a buffer size that doesn't fit the terminating NULL char */
    memset(Buffer, 0xAB, sizeof(Buffer));
    Length = GetEnvironmentVariableA("COMSPEC", Buffer, ActualLength);
    ok_eq_ulong(Length, ActualLength + 1);
    ok_eq_char(Buffer[0], '\xAB');

    /* Test a buffer size that is just large enough */
    memset(Buffer, 0xAB, sizeof(Buffer));
    Length = GetEnvironmentVariableA("COMSPEC", Buffer, ActualLength + 1);
    ok_eq_ulong(Length, ActualLength);
    ok_eq_wchar(Buffer[ActualLength], '\0');
    ok_eq_wchar(Buffer[ActualLength + 1], '\xAB');

    /* Test non-existant variable name */
    memset(Buffer, 0xAB, sizeof(Buffer));
    Length = GetEnvironmentVariableA("ThisVariableDoesNotExit", Buffer, ARRAYSIZE(Buffer));
    ok_eq_ulong(Length, 0);
    ok_eq_wchar(Buffer[0], '\xAB');

    /* Test NULL variable name */
    Length = GetEnvironmentVariableA(NULL, Buffer, ARRAYSIZE(Buffer));
    ok_eq_ulong(Length, 0);

    /* Test NULL buffer with non-null size */
    StartSeh()
        Length = GetEnvironmentVariableA("COMSPEC", NULL, ARRAYSIZE(Buffer));
    EndSeh(STATUS_ACCESS_VIOLATION);
}

static void Test_GetEnvironmentVariableW(void)
{
    WCHAR Buffer[MAX_PATH];
    ULONG ActualLength, Length;

    /* Get the COMSPEC variable */
    memset(Buffer, 0xAB, sizeof(Buffer));
    Length = GetEnvironmentVariableW(L"COMSPEC", Buffer, ARRAYSIZE(Buffer));
    Buffer[MAX_PATH - 1] = L'\0';
    ActualLength = (ULONG)wcslen(Buffer);
    ok(ActualLength < ARRAYSIZE(Buffer) - 1, "ActualLength == ARRAYSIZE(Buffer) - 1. Not null-terminated?\n");
    ok_eq_ulong(Length, ActualLength);
    ok_eq_wchar(Buffer[ActualLength + 1], 0xABAB);

    /* Get the length only (return value is including NULL char) */
    Length = GetEnvironmentVariableW(L"COMSPEC", NULL, 0);
    ok_eq_ulong(Length, ActualLength + 1);

    /* Test a buffer size that is too small */
    memset(Buffer, 0xAB, sizeof(Buffer));
    Length = GetEnvironmentVariableW(L"COMSPEC", Buffer, 2);
    ok_eq_ulong(Length, ActualLength + 1);
    ok(Buffer[0] == UNICODE_NULL || broken(/* Windows 2003 */ Buffer[0] == 0xABAB), "Buffer[0] = 0x%x\n", Buffer[0]);
    ok_eq_wchar(Buffer[1], 0xABAB);

    /* Test a buffer size that doesn't fit the terminating NULL char */
    memset(Buffer, 0xAB, sizeof(Buffer));
    Length = GetEnvironmentVariableW(L"COMSPEC", Buffer, ActualLength);
    ok_eq_ulong(Length, ActualLength + 1);
    ok(Buffer[0] == UNICODE_NULL || broken(/* Windows 2003 */ Buffer[0] == 0xABAB), "Buffer[0] = 0x%x\n", Buffer[0]);
    ok_eq_wchar(Buffer[1], 0xABAB);

    /* Test a buffer size that is just large enough */
    memset(Buffer, 0xAB, sizeof(Buffer));
    Length = GetEnvironmentVariableW(L"COMSPEC", Buffer, ActualLength + 1);
    ok_eq_ulong(Length, ActualLength);
    ok_eq_wchar(Buffer[ActualLength], L'\0');
    ok_eq_wchar(Buffer[ActualLength + 1], 0xABAB);

    /* Test non-existant variable name */
    memset(Buffer, 0xAB, sizeof(Buffer));
    Length = GetEnvironmentVariableW(L"ThisVariableDoesNotExit", Buffer, ARRAYSIZE(Buffer));
    ok_eq_ulong(Length, 0);
    ok_eq_wchar(Buffer[0], 0xABAB);

    /* Test NULL variable name */
    StartSeh()
        Length = GetEnvironmentVariableW(NULL, Buffer, ARRAYSIZE(Buffer));
    EndSeh(STATUS_SUCCESS);
    ok_eq_ulong(Length, 0);

    /* Test NULL buffer with non-null size */
    StartSeh()
        Length = GetEnvironmentVariableW(L"COMSPEC", NULL, ARRAYSIZE(Buffer));
    EndSeh(STATUS_SUCCESS);
    ok((Length == ActualLength + 1) || broken(/* Windows 2003 */ Length == 0), "Length == %lu, ActualLength == %lu\n", Length, ActualLength);
}

START_TEST(GetEnvironmentVariable)
{
    Test_GetEnvironmentVariableA();
    Test_GetEnvironmentVariableW();
}
