/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for FrameRgn
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <apitest.h>
#include <windows.h>
#include <stdio.h>
#include "init.h"


void Test_PaintRgn()
{
    RECT rc = { 0, 0, 100, 100 };
    HRGN hrgn1, hrgn2;
    BOOL bRet;
    XFORM xform;
    PULONG pulDIB = gpvDIB1;

    FillRect(ghdcDIB1, &rc, GetStockObject(BLACK_BRUSH));

    hrgn1 = CreateRectRgn(0, 0, 8, 3);
    ok(hrgn1 != NULL, "failed to create region\n");

    hrgn2 = CreateRectRgn(2, 3, 5, 8);
    ok(hrgn1 != NULL, "failed to create region\n");

    CombineRgn(hrgn1, hrgn1, hrgn2, RGN_OR);

    xform.eM11 = 1.0;
    xform.eM12 = 0.5f;
    xform.eM21 = 0.0;
    xform.eM22 = 1.0;
    xform.eDx = 0.0;
    xform.eDy = 0.0;

    SetGraphicsMode(ghdcDIB1, GM_ADVANCED);
    ok(SetWorldTransform(ghdcDIB1, &xform) == TRUE, "SetWorldTransform failed\n");

    SelectObject(ghdcDIB1, GetStockObject(WHITE_BRUSH));

    bRet = PaintRgn(ghdcDIB1, hrgn1);
    ok(bRet == TRUE, "PaintRgn failed\n");

    ok_long(pulDIB[0], 0x00000000); // 000000000
    ok_long(pulDIB[1], 0x000000C0); // 110000000
    ok_long(pulDIB[2], 0x000000F0); // 111110000
    ok_long(pulDIB[3], 0x000000FC); // 111111000
    ok_long(pulDIB[4], 0x0000003F); // 001111110
    ok_long(pulDIB[5], 0x0000003F); // 001111110
    ok_long(pulDIB[6], 0x0000003B); // 001110110
    ok_long(pulDIB[7], 0x00000038); // 001110000
    ok_long(pulDIB[8], 0x00000038); // 001110000
}

START_TEST(PaintRgn)
{
    InitStuff();
    Test_PaintRgn();
}

