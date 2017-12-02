/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for CreateBitmapIndirect
 * PROGRAMMERS:     Magnus Olsen
 */

#include "precomp.h"

void Test_CreateBitmapIndirect()
{
    HBITMAP win_hBmp;
    BITMAP win_bitmap;

    win_bitmap.bmBits = 0;
    win_bitmap.bmBitsPixel = 1;
    win_bitmap.bmHeight = 0;
    win_bitmap.bmPlanes = 1;
    win_bitmap.bmType = 1;
    win_bitmap.bmWidth = 0;
    win_bitmap.bmWidthBytes = 2;
    win_hBmp = CreateBitmapIndirect(&win_bitmap);
    ok(win_hBmp != 0, "CreateBitmapIndirect failed\n");

    DeleteObject(win_hBmp);

    RtlZeroMemory(&win_bitmap,sizeof(BITMAP));
    win_bitmap.bmBits = 0;
    win_bitmap.bmBitsPixel = 1;
    win_bitmap.bmHeight = 0;
    win_bitmap.bmPlanes = 1;
    win_bitmap.bmType = 1;
    win_bitmap.bmWidth = 0;
    win_bitmap.bmWidthBytes = 1;
    win_hBmp = CreateBitmapIndirect(&win_bitmap);
    ok(win_hBmp == 0, "CreateBitmapIndirect succeeded\n");

    RtlZeroMemory(&win_bitmap,sizeof(BITMAP));
    win_bitmap.bmBits = 0;
    win_bitmap.bmBitsPixel = 1;
    win_bitmap.bmHeight = 0;
    win_bitmap.bmPlanes = 1;
    win_bitmap.bmType = 1;
    win_bitmap.bmWidth = 0;
    win_bitmap.bmWidthBytes = 3;
    win_hBmp = CreateBitmapIndirect(&win_bitmap);
    ok(win_hBmp == 0, "CreateBitmapIndirect succeeded\n");

    RtlZeroMemory(&win_bitmap,sizeof(BITMAP));
    win_bitmap.bmBits = 0;
    win_bitmap.bmBitsPixel = 1;
    win_bitmap.bmHeight = 0;
    win_bitmap.bmPlanes = 1;
    win_bitmap.bmType = 1;
    win_bitmap.bmWidth = 0;
    win_bitmap.bmWidthBytes = 4;
    win_hBmp = CreateBitmapIndirect(&win_bitmap);
    ok(win_hBmp != 0, "CreateBitmapIndirect failed\n");

    RtlZeroMemory(&win_bitmap,sizeof(BITMAP));
    win_bitmap.bmBits = 0;
    win_bitmap.bmBitsPixel = 8;
    win_bitmap.bmHeight = 0xF000;
    win_bitmap.bmPlanes = 1;
    win_bitmap.bmType = 1;
    win_bitmap.bmWidth = 0x8000;
    win_bitmap.bmWidthBytes = win_bitmap.bmWidth;
    win_hBmp = CreateBitmapIndirect(&win_bitmap);
    //ok(win_hBmp != 0, "CreateBitmapIndirect failed\n"); // fails on win 2003

    DeleteObject(win_hBmp);
}

START_TEST(CreateBitmapIndirect)
{
    Test_CreateBitmapIndirect();
}

