/*
 * Unit test suite for gdiplus regions
 *
 * Copyright (C) 2008 Huw Davies
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "windows.h"
#include "gdiplus.h"
#include "wingdi.h"
#include "wine/test.h"

static inline void expect_dword(DWORD *value, DWORD expected)
{
    ok(*value == expected, "expected %08x got %08x\n", expected, *value);
}

static inline void expect_float(DWORD *value, FLOAT expected)
{
    FLOAT valuef = *(FLOAT*)value;
    ok(valuef == expected, "expected %f got %f\n", expected, valuef);
}

static void test_create_rgn(void)
{
    GpStatus status;
    GpRegion *region, *region2;
    UINT needed;
    DWORD buf[100];
    GpRect rect;

    status = GdipCreateRegion(&region);
todo_wine
    ok(status == Ok, "status %08x\n", status);

    if(status != Ok) return;

    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    ok(needed == 20, "got %d\n", needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    ok(needed == 20, "got %d\n", needed);
    expect_dword(buf, 12);
    trace("buf[1] = %08x\n", buf[1]);
    expect_dword(buf + 2, 0xdbc01001);
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, 0x10000003);

    status = GdipSetEmpty(region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    ok(needed == 20, "got %d\n", needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    ok(needed == 20, "got %d\n", needed);
    expect_dword(buf, 12);
    trace("buf[1] = %08x\n", buf[1]);
    expect_dword(buf + 2, 0xdbc01001);
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, 0x10000002);

    status = GdipSetInfinite(region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    ok(needed == 20, "got %d\n", needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    ok(needed == 20, "got %d\n", needed);
    expect_dword(buf, 12);
    trace("buf[1] = %08x\n", buf[1]);
    expect_dword(buf + 2, 0xdbc01001);
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, 0x10000003);

    status = GdipDeleteRegion(region);
    ok(status == Ok, "status %08x\n", status);

    rect.X = 10;
    rect.Y = 20;
    rect.Width = 100;
    rect.Height = 200;
    status = GdipCreateRegionRectI(&rect, &region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    ok(needed == 36, "got %d\n", needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    ok(needed == 36, "got %d\n", needed);
    expect_dword(buf, 28);
    trace("buf[1] = %08x\n", buf[1]);
    expect_dword(buf + 2, 0xdbc01001);
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, 0x10000000);
    expect_float(buf + 5, 10.0);
    expect_float(buf + 6, 20.0);
    expect_float(buf + 7, 100.0);
    expect_float(buf + 8, 200.0);

    rect.X = 50;
    rect.Y = 30;
    rect.Width = 10;
    rect.Height = 20;
    status = GdipCombineRegionRectI(region, &rect, CombineModeIntersect);
    ok(status == Ok, "status %08x\n", status);
    rect.X = 100;
    rect.Y = 300;
    rect.Width = 30;
    rect.Height = 50;
    status = GdipCombineRegionRectI(region, &rect, CombineModeXor);
    ok(status == Ok, "status %08x\n", status);

    rect.X = 200;
    rect.Y = 100;
    rect.Width = 133;
    rect.Height = 266;
    status = GdipCreateRegionRectI(&rect, &region2);
    ok(status == Ok, "status %08x\n", status);
    rect.X = 20;
    rect.Y = 10;
    rect.Width = 40;
    rect.Height = 66;
    status = GdipCombineRegionRectI(region2, &rect, CombineModeUnion);
    ok(status == Ok, "status %08x\n", status);

    status = GdipCombineRegionRegion(region, region2, CombineModeComplement);
    ok(status == Ok, "status %08x\n", status);

    rect.X = 400;
    rect.Y = 500;
    rect.Width = 22;
    rect.Height = 55;
    status = GdipCombineRegionRectI(region, &rect, CombineModeExclude);
    ok(status == Ok, "status %08x\n", status);

    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    ok(needed == 156, "got %d\n", needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    ok(needed == 156, "got %d\n", needed);
    expect_dword(buf, 148);
    trace("buf[1] = %08x\n", buf[1]);
    expect_dword(buf + 2, 0xdbc01001);
    expect_dword(buf + 3, 10);
    expect_dword(buf + 4, CombineModeExclude);
    expect_dword(buf + 5, CombineModeComplement);
    expect_dword(buf + 6, CombineModeXor);
    expect_dword(buf + 7, CombineModeIntersect);
    expect_dword(buf + 8, 0x10000000);
    expect_float(buf + 9, 10.0);
    expect_float(buf + 10, 20.0);
    expect_float(buf + 11, 100.0);
    expect_float(buf + 12, 200.0);
    expect_dword(buf + 13, 0x10000000);
    expect_float(buf + 14, 50.0);
    expect_float(buf + 15, 30.0);
    expect_float(buf + 16, 10.0);
    expect_float(buf + 17, 20.0);
    expect_dword(buf + 18, 0x10000000);
    expect_float(buf + 19, 100.0);
    expect_float(buf + 20, 300.0);
    expect_float(buf + 21, 30.0);
    expect_float(buf + 22, 50.0);
    expect_dword(buf + 23, CombineModeUnion);
    expect_dword(buf + 24, 0x10000000);
    expect_float(buf + 25, 200.0);
    expect_float(buf + 26, 100.0);
    expect_float(buf + 27, 133.0);
    expect_float(buf + 28, 266.0);
    expect_dword(buf + 29, 0x10000000);
    expect_float(buf + 30, 20.0);
    expect_float(buf + 31, 10.0);
    expect_float(buf + 32, 40.0);
    expect_float(buf + 33, 66.0);
    expect_dword(buf + 34, 0x10000000);
    expect_float(buf + 35, 400.0);
    expect_float(buf + 36, 500.0);
    expect_float(buf + 37, 22.0);
    expect_float(buf + 38, 55.0);


    status = GdipDeleteRegion(region2);
    ok(status == Ok, "status %08x\n", status);
    status = GdipDeleteRegion(region);
    ok(status == Ok, "status %08x\n", status);

}

START_TEST(region)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_create_rgn();

    GdiplusShutdown(gdiplusToken);

}
