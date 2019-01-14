/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for ...
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

#define INVALID_POINTER ((PVOID)(ULONG_PTR)0xdeadbeefdeadbeefULL)

void Test_DPtoLP_Params()
{
    HDC hdc;
    POINT apt[2];

    apt[0].x = 0;
    apt[0].y = 0;
    apt[1].x = -1000;
    apt[1].y = 1000;

    SetLastError(ERROR_SUCCESS);
    ok_int(DPtoLP(NULL, NULL, 0), 1);
    ok_err(ERROR_SUCCESS);

    ok_int(DPtoLP(NULL, NULL, -1), 1);
    ok_err(ERROR_SUCCESS);

    ok_int(DPtoLP(NULL, INVALID_POINTER, -1), 1);
    ok_err(ERROR_SUCCESS);

    ok_int(DPtoLP(NULL, NULL, 2), 0);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(ERROR_SUCCESS);
    ok_int(DPtoLP(NULL, apt, 2), 0);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(ERROR_SUCCESS);
    ok_int(DPtoLP(NULL, apt, 0), 1);
    ok(GetLastError() == ERROR_SUCCESS, "Exected ERROR_SUCCESS, got %ld\n", GetLastError());

    SetLastError(ERROR_SUCCESS);
    ok_int(DPtoLP(NULL, apt, -2), 1);
    ok_err(ERROR_SUCCESS);

    SetLastError(ERROR_SUCCESS);
    ok_int(DPtoLP((HDC)-4, apt, -2), 1);
    ok_err(ERROR_SUCCESS);

    hdc = GetDC(0);
    SetLastError(ERROR_SUCCESS);
    ok_int(DPtoLP(hdc, NULL, 2), 1);
    ok_err(ERROR_SUCCESS);

    hdc = GetDC(0);
    SetLastError(ERROR_SUCCESS);
    ok_int(DPtoLP(hdc, INVALID_POINTER, 2), 1);
    ok_err(ERROR_SUCCESS);


    ReleaseDC(0, hdc);
}

void Test_DPtoLP()
{
    HDC hdc;
    POINT apt[2];
    XFORM xform;
    LONG lLogPixelsX, lLogPixelsY;

    apt[0].x = 1;
    apt[0].y = 1;
    apt[1].x = -1000;
    apt[1].y = 1000;

    hdc = GetDC(0);
    lLogPixelsX = GetDeviceCaps(hdc, LOGPIXELSX);
    lLogPixelsY = GetDeviceCaps(hdc, LOGPIXELSY);

    SetMapMode(hdc, MM_TEXT);
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_int(apt[0].x, 1);
    ok_int(apt[0].y, 1);
    ok_int(apt[1].x, -1000);
    ok_int(apt[1].y, 1000);

    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    SetMapMode(hdc, MM_LOMETRIC);
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_int(apt[0].x * lLogPixelsX, 33888 );
    ok_int(apt[0].y * lLogPixelsY, -86688 );
    ok_int(apt[1].x * lLogPixelsX, -338688 );
    ok_int(apt[1].y * lLogPixelsY, -338688 );
    SetGraphicsMode(hdc, GM_ADVANCED);
    SetMapMode(hdc, MM_ANISOTROPIC);

    xform.eM11 = 1.;
    xform.eM12 = 0.;
    xform.eM21 = 0.;
    xform.eM22 = 1.;
    xform.eDx = (FLOAT)4294967167.999999761;
    xform.eDy = 1.;
    ok_int(SetWorldTransform(hdc, &xform), 1);

    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_int(apt[0].x * lLogPixelsX, 58464 );
    ok_int(apt[0].y * lLogPixelsY, -86784 );
    ok_int(apt[1].x * lLogPixelsX, -314112 );
    ok_int(apt[1].y * lLogPixelsY, -338784 );

    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = 10000000.;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_int(apt[0].x * lLogPixelsX, -41184 );
    ok_int(apt[0].y * lLogPixelsY, -86784 );
    ok_int(apt[1].x * lLogPixelsX, -41184 );
    ok_int(apt[1].y * lLogPixelsY, -338784 );

    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = 1.;
    xform.eDx = (FLOAT)4294967167.999999762; // this is too big
    ok_int(SetWorldTransform(hdc, &xform), 1);
    ok_int(DPtoLP(hdc, apt, 2), 0);
    ok_int(apt[0].x, 100 );
    ok_int(apt[0].y, 256 );
    ok_int(apt[1].x, -1000 );
    ok_int(apt[1].y, 1000 );

    xform.eM11 = 2.;
    xform.eDx = (FLOAT)4294967167.999999762;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_int(apt[0].x * lLogPixelsX, 16896 );
    ok_int(apt[0].y * lLogPixelsY, -86784 );
    ok_int(apt[1].x * lLogPixelsX, -169344 );
    ok_int(apt[1].y * lLogPixelsY, -338784 );

    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = 10000000.;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_int(apt[0].x * lLogPixelsX, -41184 );
    ok_int(apt[0].y * lLogPixelsY, -86784 );
    ok_int(apt[1].x * lLogPixelsX, -41184 );
    ok_int(apt[1].y * lLogPixelsY, -338784 );

    xform.eM11 = 1000000.;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_int(apt[0].x * lLogPixelsX, -412320 );
    ok_int(apt[0].y * lLogPixelsY, 306048 );
    ok_int(apt[1].x * lLogPixelsX, -412320 );
    ok_int(apt[1].y * lLogPixelsY, 1195104 );

    ReleaseDC(0, hdc);
}

START_TEST(DPtoLP)
{
    Test_DPtoLP_Params();
    Test_DPtoLP();
}

