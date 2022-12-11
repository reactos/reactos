/*
 * PROJECT:     ReactOS Win32k tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for NtUserGetAsyncKeyState
 * COPYRIGHT:   Copyright 2022 Thomas Faber (thomas.faber@reactos.org)
 */

#include <win32nt.h>

START_TEST(NtUserGetAsyncKeyState)
{
    SHORT Ret;
    DWORD Error;

    SetLastError(0xdeadbeef);
    Ret = NtUserGetAsyncKeyState(0);
    Error = GetLastError();
    ok(Ret == 0, "Ret = %d\n", Ret);
    ok(Error == 0xdeadbeef, "Error = %lu\n", Error);

    SetLastError(0xdeadbeef);
    Ret = NtUserGetAsyncKeyState(1); // VK_LBUTTON
    Error = GetLastError();
    ok(Ret == 0 || Ret == 1, "Ret = %d\n", Ret);
    ok(Error == 0xdeadbeef, "Error = %lu\n", Error);

    SetLastError(0xdeadbeef);
    Ret = NtUserGetAsyncKeyState(0xfe);
    Error = GetLastError();
    ok(Ret == 0, "Ret = %d\n", Ret);
    ok(Error == 0xdeadbeef, "Error = %lu\n", Error);

    SetLastError(0xdeadbeef);
    Ret = NtUserGetAsyncKeyState(0xff);
    Error = GetLastError();
    ok(Ret == 0, "Ret = %d\n", Ret);
    ok(Error == 0xdeadbeef, "Error = %lu\n", Error);

    SetLastError(0xdeadbeef);
    Ret = NtUserGetAsyncKeyState(0x100);
    Error = GetLastError();
    ok(Ret == 0, "Ret = %d\n", Ret);
    ok(Error == ERROR_INVALID_PARAMETER, "Error = %lu\n", Error);

    SetLastError(0xdeadbeef);
    Ret = NtUserGetAsyncKeyState(0x101);
    Error = GetLastError();
    ok(Ret == 0, "Ret = %d\n", Ret);
    ok(Error == ERROR_INVALID_PARAMETER, "Error = %lu\n", Error);

    SetLastError(0xdeadbeef);
    Ret = NtUserGetAsyncKeyState(0x10000000);
    Error = GetLastError();
    ok(Ret == 0, "Ret = %d\n", Ret);
    ok(Error == ERROR_INVALID_PARAMETER, "Error = %lu\n", Error);

    SetLastError(0xdeadbeef);
    Ret = NtUserGetAsyncKeyState(0x7fffffff);
    Error = GetLastError();
    ok(Ret == 0, "Ret = %d\n", Ret);
    ok(Error == ERROR_INVALID_PARAMETER, "Error = %lu\n", Error);

    SetLastError(0xdeadbeef);
    Ret = NtUserGetAsyncKeyState(0x80000000);
    Error = GetLastError();
    ok(Ret == 0, "Ret = %d\n", Ret);
    ok(Error == ERROR_INVALID_PARAMETER, "Error = %lu\n", Error);

    SetLastError(0xdeadbeef);
    Ret = NtUserGetAsyncKeyState(-2);
    Error = GetLastError();
    ok(Ret == 0, "Ret = %d\n", Ret);
    ok(Error == ERROR_INVALID_PARAMETER, "Error = %lu\n", Error);

    SetLastError(0xdeadbeef);
    Ret = NtUserGetAsyncKeyState(-1);
    Error = GetLastError();
    ok(Ret == 0, "Ret = %d\n", Ret);
    ok(Error == ERROR_INVALID_PARAMETER, "Error = %lu\n", Error);
}
