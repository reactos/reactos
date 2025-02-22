/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for BeginPath
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

void Test_BeginPath()
{
    HDC hdc;
    BOOL ret;

    SetLastError(0);
    ret = BeginPath(0);
    ok(ret == 0, "BeginPath(0) succeeded, ret == %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "GetLastError() == %ld\n", GetLastError());

    hdc = CreateCompatibleDC(NULL);

    SetLastError(0);
    ret = BeginPath(hdc);
    ok(ret == 1, "BeginPath(hdc) failed, ret == %d\n", ret);
    ok(GetLastError() == 0, "GetLastError() == %ld\n", GetLastError());

    DeleteDC(hdc);

}

START_TEST(BeginPath)
{
    Test_BeginPath();
}

