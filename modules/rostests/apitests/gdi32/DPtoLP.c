/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for ...
 * PROGRAMMERS:     Timo Kreuzer
 *                  Katayama Hirofumi MZ
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
    ok_err(ERROR_SUCCESS);

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

static void GetExtent(HDC hdc, SIZE *psizWnd, SIZE *psizView)
{
    GetWindowExtEx(hdc, psizWnd);
    //trace("*psizWnd: (%ld, %ld)\n", psizWnd->cx, psizWnd->cy);

    GetViewportExtEx(hdc, psizView);
    //trace("*psizView: (%ld, %ld)\n", psizView->cx, psizView->cy);
}

void Test_DPtoLP()
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

    // MM_TEXT
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    SetMapMode(hdc, MM_TEXT);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(DPtoLP(hdc, apt, 2), 1);
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
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_long(apt[0].x, MulDiv(100, sizWnd.cx, sizView.cx));
    ok_long(apt[0].y, MulDiv(256, sizWnd.cy, sizView.cy));
    ok_long(apt[1].x, MulDiv(-1000, sizWnd.cx, sizView.cx));
    ok_long(apt[1].y, MulDiv(1000, sizWnd.cy, sizView.cy));
    xLow = apt[0].x;
    yLow = apt[0].y;

    // MM_HIMETRIC
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    SetMapMode(hdc, MM_HIMETRIC);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_long(apt[0].x, MulDiv(100, sizWnd.cx, sizView.cx));
    ok_long(apt[0].y, MulDiv(256, sizWnd.cy, sizView.cy));
    ok_long(apt[1].x, MulDiv(-1000, sizWnd.cx, sizView.cx));
    ok_long(apt[1].y, MulDiv(1000, sizWnd.cy, sizView.cy));
    xHigh = apt[0].x;
    yHigh = apt[0].y;
    ok(labs(xLow) * 9 < labs(xHigh) && labs(xHigh) < 11 * labs(xLow), "%ld, %ld\n", xLow, xHigh);
    ok(labs(yLow) * 9 < labs(yHigh) && labs(yHigh) < 11 * labs(yLow), "%ld, %ld\n", yLow, yHigh);

    // MM_LOENGLISH
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    SetMapMode(hdc, MM_LOENGLISH);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_long(apt[0].x, MulDiv(100, sizWnd.cx, sizView.cx));
    ok_long(apt[0].y, MulDiv(256, sizWnd.cy, sizView.cy));
    ok_long(apt[1].x, MulDiv(-1000, sizWnd.cx, sizView.cx));
    ok_long(apt[1].y, MulDiv(1000, sizWnd.cy, sizView.cy));
    xLow = apt[0].x;
    yLow = apt[0].y;

    // MM_HIENGLISH
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    SetMapMode(hdc, MM_HIENGLISH);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_long(apt[0].x, MulDiv(100, sizWnd.cx, sizView.cx));
    ok_long(apt[0].y, MulDiv(256, sizWnd.cy, sizView.cy));
    ok_long(apt[1].x, MulDiv(-1000, sizWnd.cx, sizView.cx));
    ok_long(apt[1].y, MulDiv(1000, sizWnd.cy, sizView.cy));
    xHigh = apt[0].x;
    yHigh = apt[0].y;
    ok(labs(xLow) * 9 < labs(xHigh) && labs(xHigh) < 11 * labs(xLow), "%ld, %ld\n", xLow, xHigh);
    ok(labs(yLow) * 9 < labs(yHigh) && labs(yHigh) < 11 * labs(yLow), "%ld, %ld\n", yLow, yHigh);

    // MM_TWIPS
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    SetMapMode(hdc, MM_TWIPS);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_long(apt[0].x, MulDiv(100, sizWnd.cx, sizView.cx));
    ok_long(apt[0].y, MulDiv(256, sizWnd.cy, sizView.cy));
    ok_long(apt[1].x, MulDiv(-1000, sizWnd.cx, sizView.cx));
    ok_long(apt[1].y, MulDiv(1000, sizWnd.cy, sizView.cy));

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
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_long(apt[0].x, MulDiv(100, sizWnd.cx, sizView.cx) - (LONG)xform.eDx);
    ok_long(apt[0].y, MulDiv(256, sizWnd.cy, sizView.cy) - (LONG)xform.eDy);
    ok_long(apt[1].x, MulDiv(-1000, sizWnd.cx, sizView.cx) - (LONG)xform.eDx);
    ok_long(apt[1].y, MulDiv(1000, sizWnd.cy, sizView.cy) - (LONG)xform.eDy);

    // eM11 == 10000000
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = 10000000.;
    xform.eDx = 0.;
    xform.eDy = 0.;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_long(apt[0].x, 0);
    ok_long(apt[0].y, MulDiv(256, sizWnd.cy, sizView.cy));
    ok_long(apt[1].x, 0);
    ok_long(apt[1].y, MulDiv(1000, sizWnd.cy, sizView.cy));

    // eM11 == 2
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = 2.;
    xform.eDx = 0.;
    xform.eDy = 0.;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_long(apt[0].x, MulDiv(100 / 2, sizWnd.cx, sizView.cx));
    ok_long(apt[0].y, MulDiv(256, sizWnd.cy, sizView.cy));
    ok_long(apt[1].x, MulDiv(-1000 / 2, sizWnd.cx, sizView.cx));
    ok_long(apt[1].y, MulDiv(1000, sizWnd.cy, sizView.cy));

    // eM11 == (FLOAT)0x1FFFFFFFF
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = (FLOAT)0x1FFFFFFFF;
    xform.eDx = 0.;
    xform.eDy = 0.;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_long(apt[0].x, 0);
    //ok_long(apt[0].y, MulDiv(256, sizWnd.cy, sizView.cy));
    ok_long(apt[1].x, 0);
    //ok_long(apt[1].y, MulDiv(1000, sizWnd.cy, sizView.cy));

    // eM11 == (FLOAT)0xFFFFFFFFU
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = (FLOAT)0xFFFFFFFFU;
    xform.eM22 = 1.;
    xform.eDx = 0.;
    xform.eDy = 0.;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_long(apt[0].x, 0);
    ok_long(apt[0].y, MulDiv(256, sizWnd.cy, sizView.cy));
    ok_long(apt[1].x, 0);
    ok_long(apt[1].y, MulDiv(1000, sizWnd.cy, sizView.cy));

    // eM22 == (FLOAT)0xFFFFFFFFU
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = 1.;
    xform.eM22 = (FLOAT)0xFFFFFFFFU;
    xform.eDx = 0.;
    xform.eDy = 0.;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(DPtoLP(hdc, apt, 2), 1);
    //ok_long(apt[0].x, MulDiv(100, sizWnd.cy, sizView.cy));
    ok_long(apt[0].y, 0);
    //ok_long(apt[1].x, MulDiv(-1000, sizWnd.cy, sizView.cy));
    ok_long(apt[1].y, 0);

    // eM22 == (FLOAT)0x1FFFFFFFFU
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = 1.;
    xform.eM22 = (FLOAT)0x1FFFFFFFFU;
    xform.eDx = 0.;
    xform.eDy = 0.;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(DPtoLP(hdc, apt, 2), 1);
    //ok_long(apt[0].x, MulDiv(100, sizWnd.cy, sizView.cy));
    ok_long(apt[0].y, 0);
    //ok_long(apt[1].x, MulDiv(-1000, sizWnd.cy, sizView.cy));
    ok_long(apt[1].y, 0);

    // eM11 == (FLOAT)0xFFFFFFFFU, eM22 == (FLOAT)0xFFFFFFFFU
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = (FLOAT)0xFFFFFFFFU;
    xform.eM22 = (FLOAT)0xFFFFFFFFU;
    xform.eDx = 0.;
    xform.eDy = 0.;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_long(apt[0].x, 0);
    ok_long(apt[0].y, 0);
    ok_long(apt[1].x, 0);
    ok_long(apt[1].y, 0);

    // eM11 == (FLOAT)0x1FFFFFFFFU, eM22 == (FLOAT)0x1FFFFFFFFU
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = (FLOAT)0x1FFFFFFFFU;
    xform.eM22 = (FLOAT)0x1FFFFFFFFU;
    xform.eDx = 0.;
    xform.eDy = 0.;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    GetExtent(hdc, &sizWnd, &sizView);
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_long(apt[0].x, 0);
    ok_long(apt[0].y, 0);
    ok_long(apt[1].x, 0);
    ok_long(apt[1].y, 0);

    // eM11 == 10000000
    apt[0].x = 100; apt[0].y = 256; apt[1].x = -1000; apt[1].y = 1000;
    xform.eM11 = 10000000.;
    xform.eM22 = 1.0;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_long(apt[0].x, 0);
    ok_long(apt[0].y, MulDiv(256, sizWnd.cy, sizView.cy));
    ok_long(apt[1].x, 0);
    ok_long(apt[1].y, MulDiv(1000, sizWnd.cy, sizView.cy));

    xform.eM11 = 1000000.;
    ok_int(SetWorldTransform(hdc, &xform), 1);
    ok_int(DPtoLP(hdc, apt, 2), 1);
    ok_long(apt[0].x, 0);
    //ok_long(apt[0].y, MulDiv(256, sizWnd.cy, sizView.cy));
    ok_long(apt[1].x, 0);
    //ok_long(apt[1].y, MulDiv(1000, sizWnd.cy, sizView.cy));

    DeleteDC(hdc);
}

START_TEST(DPtoLP)
{
    Test_DPtoLP_Params();
    Test_DPtoLP();
}

