/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for ...
 * PROGRAMMERS:     Katayama Hirofumi MZ
 */

#include "precomp.h"

#define INVALID_POINTER ((PVOID)(ULONG_PTR)0xdeadbeefdeadbeefULL)

void Test_LPtoDP_Params()
{
    HDC hdc;
    POINT apt[2];

    apt[0].x = 0;
    apt[0].y = 0;
    apt[1].x = -1000;
    apt[1].y = 1000;

    SetLastError(ERROR_SUCCESS);
    ok_int(LPtoDP(NULL, NULL, 0), 1);
    ok_err(ERROR_SUCCESS);

    ok_int(LPtoDP(NULL, NULL, -1), 1);
    ok_err(ERROR_SUCCESS);

    ok_int(LPtoDP(NULL, INVALID_POINTER, -1), 1);
    ok_err(ERROR_SUCCESS);

    ok_int(LPtoDP(NULL, NULL, 2), 0);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(ERROR_SUCCESS);
    ok_int(LPtoDP(NULL, apt, 2), 0);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(ERROR_SUCCESS);
    ok_int(LPtoDP(NULL, apt, 0), 1);
    ok_err(ERROR_SUCCESS);

    SetLastError(ERROR_SUCCESS);
    ok_int(LPtoDP(NULL, apt, -2), 1);
    ok_err(ERROR_SUCCESS);

    SetLastError(ERROR_SUCCESS);
    ok_int(LPtoDP((HDC)-4, apt, -2), 1);
    ok_err(ERROR_SUCCESS);

    hdc = GetDC(0);
    SetLastError(ERROR_SUCCESS);
    ok_int(LPtoDP(hdc, NULL, 2), 1);
    ok_err(ERROR_SUCCESS);

    hdc = GetDC(0);
    SetLastError(ERROR_SUCCESS);
    ok_int(LPtoDP(hdc, INVALID_POINTER, 2), 1);
    ok_err(ERROR_SUCCESS);


    ReleaseDC(0, hdc);
}

static void GetExtent(HDC hdc, SIZE *psizWnd, SIZE *psizView)
{
    GetWindowExtEx(hdc, psizWnd);
    //trace("*psizWnd: (%ld, %ld)\n", psizWnd->cx, psizWnd->cy);

    GetViewportExtEx(hdc, psizView);
    //trace("*psizView: (%ld, %ld)\n", psizView->cx, psizView->cy);
}

void Test_LPtoDP()
{
    HDC hdc;
    POINT apt[2];
    XFORM xform;
    LONG lLogPixelsX, lLogPixelsY;
    SIZE sizWnd, sizView;
    LONG xLow, yLow, xHigh, yHigh;

    hdc = CreateCompatibleDC(NULL);
    lLogPixelsX = GetDeviceCaps(hdc, LOGPIXELSX);
    lLogPixelsY = GetDeviceCaps(hdc, LOGPIXELSY);
    trace("lLogPixelsX: %ld\n", lLogPixelsX);
    trace("lLogPixelsY: %ld\n", lLogPixelsY);

//#define MULDIV(a, b, c) (((a) * (b)) / (c))
#define MULDIV(a, b, c) MulDiv((a), (b), (c))

    // MM_TEXT
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    SetMapMode(hdc, MM_TEXT);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(LPtoDP(hdc, apt, 2), 1);
    ok_long(sizWnd.cx, 1);
    ok_long(sizWnd.cy, 1);
    ok_long(sizView.cx, 1);
    ok_long(sizView.cy, 1);
    ok_long(apt[0].x, 100);
    ok_long(apt[0].y, 256);
    ok_long(apt[1].x, -1000);
    ok_long(apt[1].y, 1000);

    // MM_LOMETRIC
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    SetMapMode(hdc, MM_LOMETRIC);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(LPtoDP(hdc, apt, 2), 1);
    ok_long(apt[0].x, MULDIV(100, sizView.cx, sizWnd.cx));
    ok_long(apt[0].y, MULDIV(256, sizView.cy, sizWnd.cy));
    ok_long(apt[1].x, MULDIV(-1000, sizView.cx, sizWnd.cx));
    ok_long(apt[1].y, MULDIV(1000, sizView.cy, sizWnd.cy));
    xLow = apt[0].x;
    yLow = apt[0].y;

    // MM_HIMETRIC
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    SetMapMode(hdc, MM_HIMETRIC);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(LPtoDP(hdc, apt, 2), 1);
    //ok_long(apt[0].x, MULDIV(100, sizView.cx, sizWnd.cx));
    ok_long(apt[0].y, MULDIV(256, sizView.cy, sizWnd.cy));
    //ok_long(apt[1].x, MULDIV(-1000, sizView.cx, sizWnd.cx));
    ok_long(apt[1].y, MULDIV(1000, sizView.cy, sizWnd.cy));
    xHigh = apt[0].x;
    yHigh = apt[0].y;
    ok(labs(xHigh) <= labs(xLow) / 9 && labs(xLow) / 11 <= labs(xHigh), "%ld, %ld\n", xLow, xHigh);
    ok(labs(yHigh) <= labs(yLow) / 9 && labs(yLow) / 11 <= labs(yHigh), "%ld, %ld\n", yLow, yHigh);

    // MM_LOENGLISH
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    SetMapMode(hdc, MM_LOENGLISH);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(LPtoDP(hdc, apt, 2), 1);
    ok_long(apt[0].x, MULDIV(100, sizView.cx, sizWnd.cx));
    ok_long(apt[0].y, MULDIV(256, sizView.cy, sizWnd.cy));
    ok_long(apt[1].x, MULDIV(-1000, sizView.cx, sizWnd.cx));
    ok_long(apt[1].y, MULDIV(1000, sizView.cy, sizWnd.cy));
    xLow = apt[0].x;
    yLow = apt[0].y;

    // MM_HIENGLISH
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    SetMapMode(hdc, MM_HIENGLISH);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(LPtoDP(hdc, apt, 2), 1);
    ok_long(apt[0].x, MULDIV(100, sizView.cx, sizWnd.cx));
    ok_long(apt[0].y, MULDIV(256, sizView.cy, sizWnd.cy));
    ok_long(apt[1].x, MULDIV(-1000, sizView.cx, sizWnd.cx));
    ok_long(apt[1].y, MULDIV(1000, sizView.cy, sizWnd.cy));
    xHigh = apt[0].x;
    yHigh = apt[0].y;
    ok(labs(xHigh) <= labs(xLow) / 9 && labs(xLow) / 11 <= labs(xHigh), "%ld, %ld\n", xLow, xHigh);
    ok(labs(yHigh) <= labs(yLow) / 9 && labs(yLow) / 11 <= labs(yHigh), "%ld, %ld\n", yLow, yHigh);

    // MM_TWIPS
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    SetMapMode(hdc, MM_TWIPS);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(LPtoDP(hdc, apt, 2), 1);
    ok_long(apt[0].x, MULDIV(100, sizView.cx, sizWnd.cx));
    ok_long(apt[0].y, MULDIV(256, sizView.cy, sizWnd.cy));
    ok_long(apt[1].x, MULDIV(-1000, sizView.cx, sizWnd.cx));
    ok_long(apt[1].y, MULDIV(1000, sizView.cy, sizWnd.cy));

    SetGraphicsMode(hdc, GM_ADVANCED);
    SetMapMode(hdc, MM_ANISOTROPIC);

    xform.eM11 = 1.;
    xform.eM12 = 0.;
    xform.eM21 = 0.;
    xform.eM22 = 1.;
    xform.eDx = 2.;
    xform.eDy = 1.;
    ok_int(SetWorldTransform(hdc, &xform), 1);

    // eDx == 2, eDy == 1
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(LPtoDP(hdc, apt, 2), 1);
    ok_long(apt[0].x, MULDIV(100 + (LONG)xform.eDx, sizView.cx, sizWnd.cx));
    ok_long(apt[0].y, MULDIV(256 + (LONG)xform.eDy, sizView.cy, sizWnd.cy));
    ok_long(apt[1].x, MULDIV(-1000 - (LONG)xform.eDx, sizView.cx, sizWnd.cx));
    ok_long(apt[1].y, MULDIV(1000 + (LONG)xform.eDy, sizView.cy, sizWnd.cy));

    // eM11 == 0.0000001
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = 0.0000001;
    xform.eDx = 0.;
    xform.eDy = 0.;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(LPtoDP(hdc, apt, 2), 1);
    ok_long(apt[0].x, 0);
    ok_long(apt[0].y, MULDIV(256, sizView.cy, sizWnd.cy));
    ok_long(apt[1].x, 0);
    ok_long(apt[1].y, MULDIV(1000, sizView.cy, sizWnd.cy));

    // eM11 == 0.5
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = 0.5;
    xform.eDx = 0.;
    xform.eDy = 0.;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(LPtoDP(hdc, apt, 2), 1);
    ok_long(apt[0].x, MULDIV(100, sizView.cx, sizWnd.cx * 2));
    ok_long(apt[0].y, MULDIV(256, sizView.cy, sizWnd.cy));
    ok_long(apt[1].x, MULDIV(-1000, sizView.cx, sizWnd.cx * 2));
    ok_long(apt[1].y, MULDIV(1000, sizView.cy, sizWnd.cy));

    // eM11 == 1.164153218404873e-10
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = 1.164153218404873e-10;
    xform.eDx = 0.;
    xform.eDy = 0.;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(LPtoDP(hdc, apt, 2), 1);
    ok_long(apt[0].x, 0);
    ok_long(apt[0].y, MULDIV(256, sizView.cy, sizWnd.cy));
    ok_long(apt[1].x, 0);
    ok_long(apt[1].y, MULDIV(1000, sizView.cy, sizWnd.cy));

    // eM11 == 2.328306437080797e-10
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = 2.328306437080797e-10;
    xform.eM22 = 1.;
    xform.eDx = 0.;
    xform.eDy = 0.;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(LPtoDP(hdc, apt, 2), 1);
    ok_long(apt[0].x, 0);
    ok_long(apt[0].y, MULDIV(256, sizView.cy, sizWnd.cy));
    ok_long(apt[1].x, 0);
    ok_long(apt[1].y, MULDIV(1000, sizView.cy, sizWnd.cy));

    // eM22 == 2.328306437080797e-10
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = 1.;
    xform.eM22 = 2.328306437080797e-10;
    xform.eDx = 0.;
    xform.eDy = 0.;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(LPtoDP(hdc, apt, 2), 1);
    //ok_long(apt[0].x, MULDIV(100, sizView.cy, sizWnd.cy));
    ok_long(apt[0].y, 0);
    //ok_long(apt[1].x, MULDIV(-1000, sizView.cy, sizWnd.cy));
    ok_long(apt[1].y, 0);

    // eM22 == 1.164153218404873e-10
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = 1.;
    xform.eM22 = 1.164153218404873e-10;
    xform.eDx = 0.;
    xform.eDy = 0.;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(LPtoDP(hdc, apt, 2), 1);
    //ok_long(apt[0].x, MULDIV(100, sizView.cy, sizWnd.cy));
    ok_long(apt[0].y, 0);
    //ok_long(apt[1].x, MULDIV(-1000, sizView.cy, sizWnd.cy));
    ok_long(apt[1].y, 0);

    // eM11 == 2.328306437080797e-10, eM22 == 2.328306437080797e-10
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = 2.328306437080797e-10;
    xform.eM22 = 2.328306437080797e-10;
    xform.eDx = 0.;
    xform.eDy = 0.;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(LPtoDP(hdc, apt, 2), 1);
    ok_long(apt[0].x, 0);
    ok_long(apt[0].y, 0);
    ok_long(apt[1].x, 0);
    ok_long(apt[1].y, 0);

    // eM11 == 1.164153218404873e-10, eM22 == 1.164153218404873e-10
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = 1.164153218404873e-10;
    xform.eM22 = 1.164153218404873e-10;
    xform.eDx = 0.;
    xform.eDy = 0.;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(LPtoDP(hdc, apt, 2), 1);
    ok_long(apt[0].x, 0);
    ok_long(apt[0].y, 0);
    ok_long(apt[1].x, 0);
    ok_long(apt[1].y, 0);

    // eM11 == 0.0000001
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = 0.0000001;
    xform.eM22 = 1.0;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(LPtoDP(hdc, apt, 2), 1);
    ok_long(apt[0].x, 0);
    ok_long(apt[0].y, MULDIV(256, sizView.cy, sizWnd.cy));
    ok_long(apt[1].x, 0);
    ok_long(apt[1].y, MULDIV(1000, sizView.cy, sizWnd.cy));

    DeleteDC(hdc);
}

START_TEST(LPtoDP)
{
    Test_LPtoDP_Params();
    Test_LPtoDP();
}
