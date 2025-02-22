/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for GetVolumeInformation
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include "precomp.h"

static VOID
TestGetVolumeInformationA(VOID)
{
    BOOL Ret;
    CHAR Outbuf[MAX_PATH];
    DWORD i, MCL, Flags, Len;

    memset(Outbuf, 0xAA, MAX_PATH);
    Ret = GetVolumeInformationA("C:\\", NULL, 0, NULL, &MCL, &Flags, Outbuf, MAX_PATH);
    ok(Ret != FALSE, "GetVolumeInformationA failed: %ld\n", GetLastError());
    for (i = 0; i < MAX_PATH; ++i)
    {
        if (Outbuf[i] == 0)
        {
            break;
        }
    }
    ok(i != MAX_PATH, "String was not null terminated!\n");

    memset(Outbuf, 0xAA, MAX_PATH);
    Len = i;
    Ret = GetVolumeInformationA("C:\\", NULL, 0, NULL, &MCL, &Flags, Outbuf, Len + 1);
    ok(Ret != FALSE, "GetVolumeInformationA failed: %ld\n", GetLastError());
    for (i = 0; i < MAX_PATH; ++i)
    {
        if (Outbuf[i] == 0)
        {
            break;
        }
    }
    ok(i != MAX_PATH, "String was not null terminated!\n");
    ok(i == Len, "String was truncated\n");

    memset(Outbuf, 0xAA, MAX_PATH);
    Len = i;
    SetLastError(0xdeadbeef);
    Ret = GetVolumeInformationA("C:\\", NULL, 0, NULL, &MCL, &Flags, Outbuf, Len);
    ok(Ret != TRUE, "GetVolumeInformationA succeed\n");
    ok(GetLastError() == ERROR_BAD_LENGTH, "Expected ERROR_BAD_LENGTH error, got %ld\n", GetLastError());
    ok(Outbuf[0] != 0xAA, "Output buffer was not written to\n");
    for (i = 0; i < MAX_PATH; ++i)
    {
        if (Outbuf[i] == 0)
        {
            break;
        }
    }
    ok(i == MAX_PATH, "String was null terminated!\n");
    for (i = 0; i < MAX_PATH; ++i)
    {
        if (Outbuf[i] != 0xAA)
        {
            break;
        }
    }
    ok(i != MAX_PATH, "String was not written to!\n");
    ok(i < Len, "Buffer has been overruned\n");
}

static VOID
TestGetVolumeInformationW(VOID)
{
    BOOL Ret;
    WCHAR Outbuf[MAX_PATH];
    DWORD i, MCL, Flags, Len;

    memset(Outbuf, 0xAA, sizeof(Outbuf));
    Ret = GetVolumeInformationW(L"C:\\", NULL, 0, NULL, &MCL, &Flags, Outbuf, MAX_PATH);
    ok(Ret != FALSE, "GetVolumeInformationW failed: %ld\n", GetLastError());
    for (i = 0; i < MAX_PATH; ++i)
    {
        if (Outbuf[i] == 0)
        {
            break;
        }
    }
    ok(i != MAX_PATH, "String was not null terminated!\n");

    memset(Outbuf, 0xAA, sizeof(Outbuf));
    Len = i;
    Ret = GetVolumeInformationW(L"C:\\", NULL, 0, NULL, &MCL, &Flags, Outbuf, Len + 1);
    ok(Ret != FALSE, "GetVolumeInformationW failed: %ld\n", GetLastError());
    for (i = 0; i < MAX_PATH; ++i)
    {
        if (Outbuf[i] == 0)
        {
            break;
        }
    }
    ok(i != MAX_PATH, "String was not null terminated!\n");
    ok(i == Len, "String was truncated\n");

    memset(Outbuf, 0xAA, sizeof(Outbuf));
    Len = i;
    SetLastError(0xdeadbeef);
    Ret = GetVolumeInformationW(L"C:\\", NULL, 0, NULL, &MCL, &Flags, Outbuf, Len);
    ok(Ret != TRUE, "GetVolumeInformationW succeed\n");
    ok(GetLastError() == ERROR_BAD_LENGTH, "Expected ERROR_BAD_LENGTH error, got %ld\n", GetLastError());
    ok(Outbuf[0] != 0xAA, "Output buffer was not written to\n");
    for (i = 0; i < MAX_PATH; ++i)
    {
        if (Outbuf[i] == 0)
        {
            break;
        }
    }
    ok(i == MAX_PATH, "String was null terminated!\n");
    ok(i >= Len, "Buffer has not been overrun\n");
}

START_TEST(GetVolumeInformation)
{
    TestGetVolumeInformationW();
    TestGetVolumeInformationA();
}
