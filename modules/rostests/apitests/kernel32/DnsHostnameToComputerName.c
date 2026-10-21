/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for DnsHostnameToComputerNameA/W
 * COPYRIGHT:   Copyright 2026 Mark Severson <gr3yf0x87@skyn3tlab.com>
 */

#include "precomp.h"

#define FILL_W L'#'
#define FILL_A '#'

static
VOID
TestW(
    _In_opt_ PCWSTR Hostname,
    _In_ DWORD InSize,
    _In_ BOOL ExpectRet,
    _In_ DWORD ExpectError,
    _In_ DWORD ExpectSize,
    _In_opt_ PCWSTR ExpectName)
{
    WCHAR Buffer[64];
    DWORD Size = InSize;
    BOOL Ret;
    ULONG i;

    for (i = 0; i < RTL_NUMBER_OF(Buffer); i++)
        Buffer[i] = FILL_W;
    SetLastError(0xdeadbeef);
    Ret = DnsHostnameToComputerNameW(Hostname, Buffer, &Size);
    ok(Ret == ExpectRet, "'%ls' %lu: ret %d, expected %d\n", Hostname, InSize, Ret, ExpectRet);
    ok(GetLastError() == ExpectError, "'%ls' %lu: error %lu, expected %lu\n",
       Hostname, InSize, GetLastError(), ExpectError);
    ok(Size == ExpectSize, "'%ls' %lu: size %lu, expected %lu\n", Hostname, InSize, Size, ExpectSize);
    if (ExpectName)
    {
        ok(!wcscmp(Buffer, ExpectName), "'%ls' %lu: name '%ls', expected '%ls'\n",
           Hostname, InSize, Buffer, ExpectName);
    }
    else
    {
        ok(Buffer[0] == FILL_W, "'%ls' %lu: buffer was written\n", Hostname, InSize);
    }
}

static
VOID
TestA(
    _In_ PCSTR Hostname,
    _In_ DWORD InSize,
    _In_ BOOL ExpectRet,
    _In_ DWORD ExpectError,
    _In_ DWORD ExpectSize,
    _In_opt_ PCSTR ExpectName)
{
    CHAR Buffer[64];
    DWORD Size = InSize;
    BOOL Ret;

    memset(Buffer, FILL_A, sizeof(Buffer));
    SetLastError(0xdeadbeef);
    Ret = DnsHostnameToComputerNameA(Hostname, Buffer, &Size);
    ok(Ret == ExpectRet, "'%s' %lu: ret %d, expected %d\n", Hostname, InSize, Ret, ExpectRet);
    ok(GetLastError() == ExpectError, "'%s' %lu: error %lu, expected %lu\n",
       Hostname, InSize, GetLastError(), ExpectError);
    ok(Size == ExpectSize, "'%s' %lu: size %lu, expected %lu\n", Hostname, InSize, Size, ExpectSize);
    if (ExpectName)
    {
        ok(!strcmp(Buffer, ExpectName), "'%s' %lu: name '%s', expected '%s'\n",
           Hostname, InSize, Buffer, ExpectName);
    }
}

START_TEST(DnsHostnameToComputerName)
{
    /* Upper-cased, and the size excludes the terminating null */
    TestW(L"ROShost", 16, TRUE, 0xdeadbeef, 7, L"ROSHOST");
    TestW(L"roshost", 16, TRUE, 0xdeadbeef, 7, L"ROSHOST");
    TestW(L"ROShost", 8, TRUE, 0xdeadbeef, 7, L"ROSHOST");

    /* No room for the terminating null: ERROR_MORE_DATA and the needed size */
    TestW(L"ROShost", 7, FALSE, ERROR_MORE_DATA, 7, NULL);
    TestW(L"ROShost", 1, FALSE, ERROR_MORE_DATA, 7, NULL);
    TestW(L"ROShost", 0, FALSE, ERROR_MORE_DATA, 7, NULL);

    /* Only the leftmost label, at most MAX_COMPUTERNAME_LENGTH characters */
    TestW(L"roshost.example.com", 16, TRUE, 0xdeadbeef, 7, L"ROSHOST");
    TestW(L"abcdefghijklmnopqrstuvwxyz", 16, TRUE, 0xdeadbeef, 15, L"ABCDEFGHIJKLMNO");
    TestW(L"abcdefghijklmnopqrstuvwxyz", 64, TRUE, 0xdeadbeef, 15, L"ABCDEFGHIJKLMNO");
    TestW(L"abcdefghijklmno", 16, TRUE, 0xdeadbeef, 15, L"ABCDEFGHIJKLMNO");

    /* Other characters are kept as they are */
    TestW(L"bad*name", 16, TRUE, 0xdeadbeef, 8, L"BAD*NAME");
    TestW(L"a b", 16, TRUE, 0xdeadbeef, 3, L"A B");

    /* No name */
    TestW(L"", 16, FALSE, ERROR_INVALID_COMPUTERNAME, 16, NULL);
    TestW(NULL, 16, FALSE, ERROR_INVALID_COMPUTERNAME, 16, NULL);

    TestA("ROShost", 16, TRUE, 0xdeadbeef, 7, "ROSHOST");
    TestA("ROShost", 7, FALSE, ERROR_MORE_DATA, 7, NULL);
    TestA("roshost.example.com", 16, TRUE, 0xdeadbeef, 7, "ROSHOST");
    TestA("abcdefghijklmnopqrstuvwxyz", 16, TRUE, 0xdeadbeef, 15, "ABCDEFGHIJKLMNO");
}
