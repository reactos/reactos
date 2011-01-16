/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for ...
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>

#define ok_int(x, y) ok(x == y, "Wrong value for " #x ", expected " #y ", got %ld\n", (long)x);

void Test_DPtoLP_Params()
{
    BOOL ret;
    HDC hdc;
    POINT apt[2];

    apt[0].x = 0;
    apt[0].y = 0;
    apt[1].x = -1000;
    apt[1].y = 1000;

    SetLastError(ERROR_SUCCESS);
    ret = DPtoLP(NULL, NULL, 0);
    ok(ret == 1, "Expected ret == 1, got %d\n", ret);
	ok(GetLastError() == ERROR_SUCCESS, "Exected ERROR_SUCCESS, got %ld\n", GetLastError());

    ret = DPtoLP(NULL, NULL, -1);
    ok(ret == 1, "Expected ret == 1, got %d\n", ret);
	ok(GetLastError() == ERROR_SUCCESS, "Exected ERROR_SUCCESS, got %ld\n", GetLastError());

    ret = DPtoLP(NULL, (PVOID)0x80000000, -1);
    ok(ret == 1, "Expected ret == 1, got %d\n", ret);
	ok(GetLastError() == ERROR_SUCCESS, "Exected ERROR_SUCCESS, got %ld\n", GetLastError());

    ret = DPtoLP(NULL, NULL, 2);
    ok(ret == 0, "Expected ret == 0, got %d\n", ret);
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "Exected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(ERROR_SUCCESS);
    ret = DPtoLP(NULL, apt, 2);
    ok(ret == 0, "Expected ret == 0, got %d\n", ret);
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "Exected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(ERROR_SUCCESS);
    ret = DPtoLP(NULL, apt, 0);
    ok(ret == 1, "Expected ret == 1, got %d\n", ret);
	ok(GetLastError() == ERROR_SUCCESS, "Exected ERROR_SUCCESS, got %ld\n", GetLastError());

    SetLastError(ERROR_SUCCESS);
    ret = DPtoLP(NULL, apt, -2);
    ok(ret == 1, "Expected ret == 1, got %d\n", ret);
	ok(GetLastError() == ERROR_SUCCESS, "Exected ERROR_SUCCESS, got %ld\n", GetLastError());

    SetLastError(ERROR_SUCCESS);
    ret = DPtoLP((HDC)-4, apt, -2);
    ok(ret == 1, "Expected ret == 1, got %d\n", ret);
	ok(GetLastError() == ERROR_SUCCESS, "Exected ERROR_SUCCESS, got %ld\n", GetLastError());

    hdc = GetDC(0);
    SetLastError(ERROR_SUCCESS);
    ret = DPtoLP(hdc, NULL, 2);
    ok(ret == 1, "Expected ret == 1, got %d\n", ret);
	ok(GetLastError() == ERROR_SUCCESS, "Exected ERROR_SUCCESS, got %ld\n", GetLastError());

    hdc = GetDC(0);
    SetLastError(ERROR_SUCCESS);
    ret = DPtoLP(hdc, (PVOID)0x80000000, 2);
    ok(ret == 1, "Expected ret == 1, got %d\n", ret);
	ok(GetLastError() == ERROR_SUCCESS, "Exected ERROR_SUCCESS, got %ld\n", GetLastError());


    ReleaseDC(0, hdc);
}

void Test_DPtoLP()
{
    BOOL ret;
    HDC hdc;
    POINT apt[2];
    XFORM xform;

    apt[0].x = 1;
    apt[0].y = 1;
    apt[1].x = -1000;
    apt[1].y = 1000;

    hdc = GetDC(0);

    SetMapMode(hdc, MM_TEXT);
    ret = DPtoLP(hdc, apt, 2);
    ok(ret == 1, "Expected ret == 1, got %d\n", ret);
    ok_int(apt[0].x, 1);
    ok_int(apt[0].y, 1);
    ok_int(apt[1].x, -1000);
    ok_int(apt[1].y, 1000);

    SetMapMode(hdc, MM_LOMETRIC);
    ret = DPtoLP(hdc, apt, 2);
    ok(ret == 1, "Expected ret == 1, got %d\n", ret);
    ok_int(apt[0].x, 4);
    ok_int(apt[0].y, -4);
    ok_int(apt[1].x, -3527);
    ok_int(apt[1].y, -3527);

    SetGraphicsMode(hdc, GM_ADVANCED);
    SetMapMode(hdc, MM_ANISOTROPIC);

    xform.eM11 = 1.;
    xform.eM12 = 0.;
    xform.eM21 = 0.;
    xform.eM22 = 1.;
    xform.eDx = 4294967167.999999761;
    xform.eDy = 1.;
    ret = SetWorldTransform(hdc, &xform);
    ok(ret == 1, "ret\n");

    ret = DPtoLP(hdc, apt, 2);
    ok(ret == 1, "Expected ret == 0, got %d\n", ret);
    ok_int(apt[0].x, 526);
    ok_int(apt[0].y, 13);
    ok_int(apt[1].x, -11929);
    ok_int(apt[1].y, 12440);

    xform.eM11 = 10000000.;
    ret = SetWorldTransform(hdc, &xform);
    ret = DPtoLP(hdc, apt, 2);
    ok(ret == 1, "Expected ret == 0, got %d\n", ret);
    ok_int(apt[0].x, -429);
    ok_int(apt[0].y, -47);
    ok_int(apt[1].x, -429);
    ok_int(apt[1].y, -43881);

    xform.eM11 = 1.;
    xform.eDx = 4294967167.999999762; // this is too big
    ret = SetWorldTransform(hdc, &xform);
    ok(ret == 1, "ret\n");
    ret = DPtoLP(hdc, apt, 2);
    ok(ret == 0, "Expected ret == 0, got %d\n", ret);
    ok_int(apt[0].x, -429);
    ok_int(apt[0].y, -47);
    ok_int(apt[1].x, -429);
    ok_int(apt[1].y, -43881);

    xform.eM11 = 2.;
    xform.eDx = 4294967167.999999762;
    ret = SetWorldTransform(hdc, &xform);
    ok(ret == 1, "ret\n");
    ret = DPtoLP(hdc, apt, 2);
    ok(ret == 1, "Expected ret == 1, got %d\n", ret);
    ok_int(apt[0].x, 2147482891);
    ok_int(apt[0].y, 165);
    ok_int(apt[1].x, 2147482891);
    ok_int(apt[1].y, 154782);

    apt[0].x = 1;
    apt[0].y = 1;
    apt[1].x = -1000;
    apt[1].y = 1000;

    xform.eM11 = 10000000.;
    ret = SetWorldTransform(hdc, &xform);
    ok(ret == 1, "ret\n");
    ret = DPtoLP(hdc, apt, 2);
    ok(ret == 1, "Expected ret == 1, got %d\n", ret);
    ok_int(apt[0].x, -429);
    ok_int(apt[0].y, -5);
    ok_int(apt[1].x, -429);
    ok_int(apt[1].y, -3528);

    xform.eM11 = 1000000.;
    ret = SetWorldTransform(hdc, &xform);
    ok(ret == 1, "ret\n");
    ret = DPtoLP(hdc, apt, 2);
    ok(ret == 1, "Expected ret == 1, got %d\n", ret);
    ok_int(apt[0].x, -4295);
    ok_int(apt[0].y, 17);
    ok_int(apt[1].x, -4295);
    ok_int(apt[1].y, 12443);

    ReleaseDC(0, hdc);
}

START_TEST(DPtoLP)
{
    Test_DPtoLP_Params();
    Test_DPtoLP();
}

