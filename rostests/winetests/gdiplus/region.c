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

#define RGNDATA_RECT            0x10000000
#define RGNDATA_PATH            0x10000001
#define RGNDATA_EMPTY_RECT      0x10000002
#define RGNDATA_INFINITE_RECT   0x10000003

#define RGNDATA_MAGIC           0xdbc01001
#define RGNDATA_MAGIC2          0xdbc01002

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)

#define expect_magic(value) ok(*value == RGNDATA_MAGIC || *value == RGNDATA_MAGIC2, "Expected a known magic value, got %8x\n", *value)

#define expect_dword(value, expected) ok(*(value) == expected, "expected %08x got %08x\n", expected, *(value))

static inline void expect_float(DWORD *value, FLOAT expected)
{
    FLOAT valuef = *(FLOAT*)value;
    ok(valuef == expected, "expected %f got %f\n", expected, valuef);
}

/* We get shorts back, not INTs like a GpPoint */
typedef struct RegionDataPoint
{
    short X, Y;
} RegionDataPoint;

static void verify_region(HRGN hrgn, const RECT *rc)
{
    union
    {
        RGNDATA data;
        char buf[sizeof(RGNDATAHEADER) + sizeof(RECT)];
    } rgn;
    const RECT *rect;
    DWORD ret;

    ret = GetRegionData(hrgn, 0, NULL);
    if (IsRectEmpty(rc))
        ok(ret == sizeof(rgn.data.rdh), "expected sizeof(rdh), got %u\n", ret);
    else
        ok(ret == sizeof(rgn.data.rdh) + sizeof(RECT), "expected sizeof(rgn), got %u\n", ret);

    if (!ret) return;

    ret = GetRegionData(hrgn, sizeof(rgn), &rgn.data);
    if (IsRectEmpty(rc))
        ok(ret == sizeof(rgn.data.rdh), "expected sizeof(rdh), got %u\n", ret);
    else
        ok(ret == sizeof(rgn.data.rdh) + sizeof(RECT), "expected sizeof(rgn), got %u\n", ret);

    trace("size %u, type %u, count %u, rgn size %u, bound (%d,%d-%d,%d)\n",
          rgn.data.rdh.dwSize, rgn.data.rdh.iType,
          rgn.data.rdh.nCount, rgn.data.rdh.nRgnSize,
          rgn.data.rdh.rcBound.left, rgn.data.rdh.rcBound.top,
          rgn.data.rdh.rcBound.right, rgn.data.rdh.rcBound.bottom);
    if (rgn.data.rdh.nCount != 0)
    {
        rect = (const RECT *)rgn.data.Buffer;
        trace("rect (%d,%d-%d,%d)\n", rect->left, rect->top, rect->right, rect->bottom);
        ok(EqualRect(rect, rc), "rects don't match\n");
    }

    ok(rgn.data.rdh.dwSize == sizeof(rgn.data.rdh), "expected sizeof(rdh), got %u\n", rgn.data.rdh.dwSize);
    ok(rgn.data.rdh.iType == RDH_RECTANGLES, "expected RDH_RECTANGLES, got %u\n", rgn.data.rdh.iType);
    if (IsRectEmpty(rc))
    {
        ok(rgn.data.rdh.nCount == 0, "expected 0, got %u\n", rgn.data.rdh.nCount);
        ok(rgn.data.rdh.nRgnSize == 0,  "expected 0, got %u\n", rgn.data.rdh.nRgnSize);
    }
    else
    {
        ok(rgn.data.rdh.nCount == 1, "expected 1, got %u\n", rgn.data.rdh.nCount);
        ok(rgn.data.rdh.nRgnSize == sizeof(RECT),  "expected sizeof(RECT), got %u\n", rgn.data.rdh.nRgnSize);
    }
    ok(EqualRect(&rgn.data.rdh.rcBound, rc), "rects don't match\n");
}

static void test_getregiondata(void)
{
    GpStatus status;
    GpRegion *region, *region2;
    RegionDataPoint *point;
    UINT needed;
    DWORD buf[100];
    GpRect rect;
    GpPath *path;

    memset(buf, 0xee, sizeof(buf));

    status = GdipCreateRegion(&region);
    ok(status == Ok, "status %08x\n", status);

    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(20, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(20, needed);
    expect_dword(buf, 12);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_INFINITE_RECT);

    status = GdipSetEmpty(region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(20, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(20, needed);
    expect_dword(buf, 12);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_EMPTY_RECT);

    status = GdipSetInfinite(region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(20, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(20, needed);
    expect_dword(buf, 12);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_INFINITE_RECT);

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
    expect(36, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(36, needed);
    expect_dword(buf, 28);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_RECT);
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
    expect(156, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(156, needed);
    expect_dword(buf, 148);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 10);
    expect_dword(buf + 4, CombineModeExclude);
    expect_dword(buf + 5, CombineModeComplement);
    expect_dword(buf + 6, CombineModeXor);
    expect_dword(buf + 7, CombineModeIntersect);
    expect_dword(buf + 8, RGNDATA_RECT);
    expect_float(buf + 9, 10.0);
    expect_float(buf + 10, 20.0);
    expect_float(buf + 11, 100.0);
    expect_float(buf + 12, 200.0);
    expect_dword(buf + 13, RGNDATA_RECT);
    expect_float(buf + 14, 50.0);
    expect_float(buf + 15, 30.0);
    expect_float(buf + 16, 10.0);
    expect_float(buf + 17, 20.0);
    expect_dword(buf + 18, RGNDATA_RECT);
    expect_float(buf + 19, 100.0);
    expect_float(buf + 20, 300.0);
    expect_float(buf + 21, 30.0);
    expect_float(buf + 22, 50.0);
    expect_dword(buf + 23, CombineModeUnion);
    expect_dword(buf + 24, RGNDATA_RECT);
    expect_float(buf + 25, 200.0);
    expect_float(buf + 26, 100.0);
    expect_float(buf + 27, 133.0);
    expect_float(buf + 28, 266.0);
    expect_dword(buf + 29, RGNDATA_RECT);
    expect_float(buf + 30, 20.0);
    expect_float(buf + 31, 10.0);
    expect_float(buf + 32, 40.0);
    expect_float(buf + 33, 66.0);
    expect_dword(buf + 34, RGNDATA_RECT);
    expect_float(buf + 35, 400.0);
    expect_float(buf + 36, 500.0);
    expect_float(buf + 37, 22.0);
    expect_float(buf + 38, 55.0);

    status = GdipDeleteRegion(region2);
    ok(status == Ok, "status %08x\n", status);
    status = GdipDeleteRegion(region);
    ok(status == Ok, "status %08x\n", status);

    /* Try some paths */

    status = GdipCreatePath(FillModeAlternate, &path);
    ok(status == Ok, "status %08x\n", status);
    GdipAddPathRectangle(path, 12.5, 13.0, 14.0, 15.0);

    status = GdipCreateRegionPath(path, &region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(72, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(72, needed);
    expect_dword(buf, 64);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_PATH);
    expect_dword(buf + 5, 0x00000030);
    expect_magic((DWORD*)(buf + 6));
    expect_dword(buf + 7, 0x00000004);
    expect_dword(buf + 8, 0x00000000);
    expect_float(buf + 9, 12.5);
    expect_float(buf + 10, 13.0);
    expect_float(buf + 11, 26.5);
    expect_float(buf + 12, 13.0);
    expect_float(buf + 13, 26.5);
    expect_float(buf + 14, 28.0);
    expect_float(buf + 15, 12.5);
    expect_float(buf + 16, 28.0);
    expect_dword(buf + 17, 0x81010100);


    rect.X = 50;
    rect.Y = 30;
    rect.Width = 10;
    rect.Height = 20;
    status = GdipCombineRegionRectI(region, &rect, CombineModeIntersect);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(96, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(96, needed);
    expect_dword(buf, 88);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 2);
    expect_dword(buf + 4, CombineModeIntersect);
    expect_dword(buf + 5, RGNDATA_PATH);
    expect_dword(buf + 6, 0x00000030);
    expect_magic((DWORD*)(buf + 7));
    expect_dword(buf + 8, 0x00000004);
    expect_dword(buf + 9, 0x00000000);
    expect_float(buf + 10, 12.5);
    expect_float(buf + 11, 13.0);
    expect_float(buf + 12, 26.5);
    expect_float(buf + 13, 13.0);
    expect_float(buf + 14, 26.5);
    expect_float(buf + 15, 28.0);
    expect_float(buf + 16, 12.5);
    expect_float(buf + 17, 28.0);
    expect_dword(buf + 18, 0x81010100);
    expect_dword(buf + 19, RGNDATA_RECT);
    expect_float(buf + 20, 50.0);
    expect_float(buf + 21, 30.0);
    expect_float(buf + 22, 10.0);
    expect_float(buf + 23, 20.0);

    status = GdipDeleteRegion(region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipDeletePath(path);
    ok(status == Ok, "status %08x\n", status);

    /* Test an empty path */
    status = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, status);
    status = GdipCreateRegionPath(path, &region);
    expect(Ok, status);
    status = GdipGetRegionDataSize(region, &needed);
    expect(Ok, status);
    expect(36, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    expect(Ok, status);
    expect(36, needed);
    expect_dword(buf, 28);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_PATH);

    /* Second signature for pathdata */
    expect_dword(buf + 5, 12);
    expect_magic((DWORD*)(buf + 6));
    expect_dword(buf + 7, 0);
    expect_dword(buf + 8, 0x00004000);

    status = GdipDeleteRegion(region);
    expect(Ok, status);

    /* Test a simple triangle of INTs */
    status = GdipAddPathLine(path, 5, 6, 7, 8);
    expect(Ok, status);
    status = GdipAddPathLine(path, 8, 1, 5, 6);
    expect(Ok, status);
    status = GdipClosePathFigure(path);
    expect(Ok, status);
    status = GdipCreateRegionPath(path, &region);
    expect(Ok, status);
    status = GdipGetRegionDataSize(region, &needed);
    expect(Ok, status);
    expect(56, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    expect(Ok, status);
    expect(56, needed);
    expect_dword(buf, 48);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3 , 0);
    expect_dword(buf + 4 , RGNDATA_PATH);

    expect_dword(buf + 5, 32);
    expect_magic((DWORD*)(buf + 6));
    expect_dword(buf + 7, 4);
    expect_dword(buf + 8, 0x00004000); /* ?? */

    point = (RegionDataPoint*)buf + 9;
    expect(5, point[0].X);
    expect(6, point[0].Y);
    expect(7, point[1].X); /* buf + 10 */
    expect(8, point[1].Y);
    expect(8, point[2].X); /* buf + 11 */
    expect(1, point[2].Y);
    expect(5, point[3].X); /* buf + 12 */
    expect(6, point[3].Y);
    expect_dword(buf + 13, 0x81010100); /* 0x01010100 if we don't close the path */

    status = GdipDeletePath(path);
    expect(Ok, status);
    status = GdipDeleteRegion(region);
    expect(Ok, status);

    /* Test a floating-point triangle */
    status = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, status);
    status = GdipAddPathLine(path, 5.6, 6.2, 7.2, 8.9);
    expect(Ok, status);
    status = GdipAddPathLine(path, 8.1, 1.6, 5.6, 6.2);
    expect(Ok, status);
    status = GdipCreateRegionPath(path, &region);
    expect(Ok, status);
    status = GdipGetRegionDataSize(region, &needed);
    expect(Ok, status);
    expect(72, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    expect(Ok, status);
    expect(72, needed);
    expect_dword(buf, 64);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_PATH);

    expect_dword(buf + 5, 48);
    expect_magic((DWORD*)(buf + 6));
    expect_dword(buf + 7, 4);
    expect_dword(buf + 8, 0);
    expect_float(buf + 9, 5.6);
    expect_float(buf + 10, 6.2);
    expect_float(buf + 11, 7.2);
    expect_float(buf + 12, 8.9);
    expect_float(buf + 13, 8.1);
    expect_float(buf + 14, 1.6);
    expect_float(buf + 15, 5.6);
    expect_float(buf + 16, 6.2);

    status = GdipDeletePath(path);
    expect(Ok, status);
    status = GdipDeleteRegion(region);
    expect(Ok, status);

    /* Test for a path with > 4 points, and CombineRegionPath */
    GdipCreatePath(FillModeAlternate, &path);
    status = GdipAddPathLine(path, 50, 70.2, 60, 102.8);
    expect(Ok, status);
    status = GdipAddPathLine(path, 55.4, 122.4, 40.4, 60.2);
    expect(Ok, status);
    status = GdipAddPathLine(path, 45.6, 20.2, 50, 70.2);
    expect(Ok, status);
    rect.X = 20;
    rect.Y = 25;
    rect.Width = 60;
    rect.Height = 120;
    status = GdipCreateRegionRectI(&rect, &region);
    expect(Ok, status);
    status = GdipCombineRegionPath(region, path, CombineModeUnion);
    expect(Ok, status);

    status = GdipGetRegionDataSize(region, &needed);
    expect(Ok, status);
    expect(116, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    expect(Ok, status);
    expect(116, needed);
    expect_dword(buf, 108);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 2);
    expect_dword(buf + 4, CombineModeUnion);
    expect_dword(buf + 5, RGNDATA_RECT);
    expect_float(buf + 6, 20);
    expect_float(buf + 7, 25);
    expect_float(buf + 8, 60);
    expect_float(buf + 9, 120);
    expect_dword(buf + 10, RGNDATA_PATH);

    expect_dword(buf + 11, 68);
    expect_magic((DWORD*)(buf + 12));
    expect_dword(buf + 13, 6);
    expect_float(buf + 14, 0x0);

    expect_float(buf + 15, 50);
    expect_float(buf + 16, 70.2);
    expect_float(buf + 17, 60);
    expect_float(buf + 18, 102.8);
    expect_float(buf + 19, 55.4);
    expect_float(buf + 20, 122.4);
    expect_float(buf + 21, 40.4);
    expect_float(buf + 22, 60.2);
    expect_float(buf + 23, 45.6);
    expect_float(buf + 24, 20.2);
    expect_float(buf + 25, 50);
    expect_float(buf + 26, 70.2);
    expect_dword(buf + 27, 0x01010100);
    expect_dword(buf + 28, 0x00000101);

    status = GdipDeletePath(path);
    expect(Ok, status);
    status = GdipDeleteRegion(region);
    expect(Ok, status);
}

static void test_isinfinite(void)
{
    GpStatus status;
    GpRegion *region;
    GpGraphics *graphics = NULL;
    GpMatrix *m;
    HDC hdc = GetDC(0);
    BOOL res;

    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);
    GdipCreateRegion(&region);

    GdipCreateMatrix2(3.0, 0.0, 0.0, 1.0, 20.0, 30.0, &m);

    /* NULL arguments */
    status = GdipIsInfiniteRegion(NULL, NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipIsInfiniteRegion(region, NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipIsInfiniteRegion(NULL, graphics, NULL);
    expect(InvalidParameter, status);
    status = GdipIsInfiniteRegion(NULL, NULL, &res);
    expect(InvalidParameter, status);
    status = GdipIsInfiniteRegion(region, NULL, &res);
    expect(InvalidParameter, status);

    res = FALSE;
    status = GdipIsInfiniteRegion(region, graphics, &res);
    expect(Ok, status);
    expect(TRUE, res);

    /* after world transform */
    status = GdipSetWorldTransform(graphics, m);
    expect(Ok, status);

    res = FALSE;
    status = GdipIsInfiniteRegion(region, graphics, &res);
    expect(Ok, status);
    expect(TRUE, res);

    GdipDeleteMatrix(m);
    GdipDeleteRegion(region);
    GdipDeleteGraphics(graphics);
    ReleaseDC(0, hdc);
}

static void test_isempty(void)
{
    GpStatus status;
    GpRegion *region;
    GpGraphics *graphics = NULL;
    HDC hdc = GetDC(0);
    BOOL res;

    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);
    GdipCreateRegion(&region);

    /* NULL arguments */
    status = GdipIsEmptyRegion(NULL, NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipIsEmptyRegion(region, NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipIsEmptyRegion(NULL, graphics, NULL);
    expect(InvalidParameter, status);
    status = GdipIsEmptyRegion(NULL, NULL, &res);
    expect(InvalidParameter, status);
    status = GdipIsEmptyRegion(region, NULL, &res);
    expect(InvalidParameter, status);

    /* default is infinite */
    res = TRUE;
    status = GdipIsEmptyRegion(region, graphics, &res);
    expect(Ok, status);
    expect(FALSE, res);

    status = GdipSetEmpty(region);
    expect(Ok, status);

    res = FALSE;
    status = GdipIsEmptyRegion(region, graphics, &res);
    expect(Ok, status);
    expect(TRUE, res);

    GdipDeleteRegion(region);
    GdipDeleteGraphics(graphics);
    ReleaseDC(0, hdc);
}

static void test_combinereplace(void)
{
    GpStatus status;
    GpRegion *region, *region2;
    GpPath *path;
    GpRectF rectf;
    UINT needed;
    DWORD buf[50];

    rectf.X = rectf.Y = 0.0;
    rectf.Width = rectf.Height = 100.0;

    status = GdipCreateRegionRect(&rectf, &region);
    expect(Ok, status);

    /* replace with the same rectangle */
    status = GdipCombineRegionRect(region, &rectf,CombineModeReplace);
    expect(Ok, status);

    status = GdipGetRegionDataSize(region, &needed);
    expect(Ok, status);
    expect(36, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    expect(Ok, status);
    expect(36, needed);
    expect_dword(buf, 28);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_RECT);

    /* replace with path */
    status = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, status);
    status = GdipAddPathEllipse(path, 0.0, 0.0, 100.0, 250.0);
    expect(Ok, status);
    status = GdipCombineRegionPath(region, path, CombineModeReplace);
    expect(Ok, status);

    status = GdipGetRegionDataSize(region, &needed);
    expect(Ok, status);
    expect(156, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    expect(Ok, status);
    expect(156, needed);
    expect_dword(buf, 148);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_PATH);
    GdipDeletePath(path);

    /* replace with infinite rect */
    status = GdipCreateRegion(&region2);
    expect(Ok, status);
    status = GdipCombineRegionRegion(region, region2, CombineModeReplace);
    expect(Ok, status);

    status = GdipGetRegionDataSize(region, &needed);
    expect(Ok, status);
    expect(20, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    expect(Ok, status);
    expect(20, needed);
    expect_dword(buf, 12);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_INFINITE_RECT);
    GdipDeleteRegion(region2);

    /* more complex case : replace with a combined region */
    status = GdipCreateRegionRect(&rectf, &region2);
    expect(Ok, status);
    status = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, status);
    status = GdipAddPathEllipse(path, 0.0, 0.0, 100.0, 250.0);
    expect(Ok, status);
    status = GdipCombineRegionPath(region2, path, CombineModeUnion);
    expect(Ok, status);
    GdipDeletePath(path);
    status = GdipCombineRegionRegion(region, region2, CombineModeReplace);
    expect(Ok, status);
    GdipDeleteRegion(region2);

    status = GdipGetRegionDataSize(region, &needed);
    expect(Ok, status);
    expect(180, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    expect(Ok, status);
    expect(180, needed);
    expect_dword(buf, 172);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 2);
    expect_dword(buf + 4, CombineModeUnion);

    GdipDeleteRegion(region);
}

static void test_fromhrgn(void)
{
    GpStatus status;
    GpRegion *region;
    HRGN hrgn;
    UINT needed;
    DWORD buf[220];
    RegionDataPoint *point;
    GpGraphics *graphics = NULL;
    HDC hdc;
    BOOL res;

    /* NULL */
    status = GdipCreateRegionHrgn(NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipCreateRegionHrgn(NULL, &region);
    expect(InvalidParameter, status);
    status = GdipCreateRegionHrgn((HRGN)0xdeadbeef, &region);
    expect(InvalidParameter, status);

    /* empty rectangle */
    hrgn = CreateRectRgn(0, 0, 0, 0);
    status = GdipCreateRegionHrgn(hrgn, &region);
    expect(Ok, status);
    if(status == Ok) {

    hdc = GetDC(0);
    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);
    res = FALSE;
    status = GdipIsEmptyRegion(region, graphics, &res);
    expect(Ok, status);
    expect(TRUE, res);
    GdipDeleteGraphics(graphics);
    ReleaseDC(0, hdc);
    GdipDeleteRegion(region);

    }
    DeleteObject(hrgn);

    /* rectangle */
    hrgn = CreateRectRgn(0, 0, 100, 10);
    status = GdipCreateRegionHrgn(hrgn, &region);
    expect(Ok, status);

    status = GdipGetRegionDataSize(region, &needed);
    expect(Ok, status);
    expect(56, needed);

    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    expect(Ok, status);

    if(status == Ok){

    expect(56, needed);
    expect_dword(buf, 48);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_PATH);
    expect_dword(buf + 5, 0x00000020);
    expect_magic((DWORD*)(buf + 6));
    expect_dword(buf + 7, 0x00000004);
    todo_wine expect_dword(buf + 8, 0x00006000); /* ?? */

    point = (RegionDataPoint*)buf + 9;

    expect(0,  point[0].X);
    expect(0,  point[0].Y);

    expect(100,point[1].X); /* buf + 10 */
    expect(0,  point[1].Y);
    expect(100,point[2].X); /* buf + 11 */
    expect(10, point[2].Y);

    expect(0,  point[3].X); /* buf + 12 */

    expect(10, point[3].Y);
    expect_dword(buf + 13, 0x81010100); /* closed */

    }

    GdipDeleteRegion(region);
    DeleteObject(hrgn);

    /* ellipse */
    hrgn = CreateEllipticRgn(0, 0, 100, 10);
    status = GdipCreateRegionHrgn(hrgn, &region);
    todo_wine expect(Ok, status);

    status = GdipGetRegionDataSize(region, &needed);
todo_wine{
    expect(Ok, status);
    ok(needed == 216 ||
       needed == 196, /* win98 */
       "Got %.8x\n", needed);
}
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    todo_wine expect(Ok, status);

    if(status == Ok && needed == 216) /* Don't try to test win98 layout */
    {
todo_wine{
    expect(Ok, status);
    expect(216, needed);
    expect_dword(buf, 208);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_PATH);
    expect_dword(buf + 5, 0x000000C0);
    expect_magic((DWORD*)(buf + 6));
    expect_dword(buf + 7, 0x00000024);
    expect_dword(buf + 8, 0x00006000); /* ?? */
}
    }

    GdipDeleteRegion(region);
    DeleteObject(hrgn);
}

static void test_gethrgn(void)
{
    GpStatus status;
    GpRegion *region, *region2;
    GpPath *path;
    GpGraphics *graphics;
    HRGN hrgn;
    HDC hdc=GetDC(0);
    static const RECT empty_rect = {0,0,0,0};
    static const RECT test_rect = {10, 11, 20, 21};
    static const GpRectF test_rectF = {10.0, 11.0, 10.0, 10.0};
    static const RECT scaled_rect = {20, 22, 40, 42};
    static const RECT test_rect2 = {10, 21, 20, 31};
    static const GpRectF test_rect2F = {10.0, 21.0, 10.0, 10.0};
    static const RECT test_rect3 = {10, 11, 20, 31};
    static const GpRectF test_rect3F = {10.0, 11.0, 10.0, 20.0};

    status = GdipCreateFromHDC(hdc, &graphics);
    ok(status == Ok, "status %08x\n", status);

    status = GdipCreateRegion(&region);
    ok(status == Ok, "status %08x\n", status);

    status = GdipGetRegionHRgn(NULL, graphics, &hrgn);
    ok(status == InvalidParameter, "status %08x\n", status);
    status = GdipGetRegionHRgn(region, graphics, NULL);
    ok(status == InvalidParameter, "status %08x\n", status);

    status = GdipGetRegionHRgn(region, NULL, &hrgn);
    ok(status == Ok, "status %08x\n", status);
    ok(hrgn == NULL, "hrgn=%p\n", hrgn);
    DeleteObject(hrgn);

    status = GdipGetRegionHRgn(region, graphics, &hrgn);
    ok(status == Ok, "status %08x\n", status);
    ok(hrgn == NULL, "hrgn=%p\n", hrgn);
    DeleteObject(hrgn);

    status = GdipSetEmpty(region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionHRgn(region, NULL, &hrgn);
    ok(status == Ok, "status %08x\n", status);
    verify_region(hrgn, &empty_rect);
    DeleteObject(hrgn);

    status = GdipCreatePath(FillModeAlternate, &path);
    ok(status == Ok, "status %08x\n", status);
    status = GdipAddPathRectangle(path, 10.0, 11.0, 10.0, 10.0);
    ok(status == Ok, "status %08x\n", status);

    status = GdipCreateRegionPath(path, &region2);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionHRgn(region2, NULL, &hrgn);
    ok(status == Ok, "status %08x\n", status);
    verify_region(hrgn, &test_rect);
    DeleteObject(hrgn);

    /* resulting HRGN is in device coordinates */
    status = GdipScaleWorldTransform(graphics, 2.0, 2.0, MatrixOrderPrepend);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionHRgn(region2, graphics, &hrgn);
    ok(status == Ok, "status %08x\n", status);
    verify_region(hrgn, &scaled_rect);
    DeleteObject(hrgn);

    status = GdipCombineRegionRect(region2, &test_rectF, CombineModeReplace);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionHRgn(region2, NULL, &hrgn);
    ok(status == Ok, "status %08x\n", status);
    verify_region(hrgn, &test_rect);
    DeleteObject(hrgn);

    status = GdipGetRegionHRgn(region2, graphics, &hrgn);
    ok(status == Ok, "status %08x\n", status);
    verify_region(hrgn, &scaled_rect);
    DeleteObject(hrgn);

    status = GdipSetInfinite(region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipCombineRegionRect(region, &test_rectF, CombineModeIntersect);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionHRgn(region, NULL, &hrgn);
    ok(status == Ok, "status %08x\n", status);
    verify_region(hrgn, &test_rect);
    DeleteObject(hrgn);

    status = GdipCombineRegionRect(region, &test_rectF, CombineModeReplace);
    ok(status == Ok, "status %08x\n", status);
    status = GdipCombineRegionRect(region, &test_rect2F, CombineModeUnion);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionHRgn(region, NULL, &hrgn);
    ok(status == Ok, "status %08x\n", status);
    verify_region(hrgn, &test_rect3);
    DeleteObject(hrgn);

    status = GdipCombineRegionRect(region, &test_rect3F, CombineModeReplace);
    ok(status == Ok, "status %08x\n", status);
    status = GdipCombineRegionRect(region, &test_rect2F, CombineModeXor);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionHRgn(region, NULL, &hrgn);
    ok(status == Ok, "status %08x\n", status);
    verify_region(hrgn, &test_rect);
    DeleteObject(hrgn);

    status = GdipCombineRegionRect(region, &test_rect3F, CombineModeReplace);
    ok(status == Ok, "status %08x\n", status);
    status = GdipCombineRegionRect(region, &test_rectF, CombineModeExclude);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionHRgn(region, NULL, &hrgn);
    ok(status == Ok, "status %08x\n", status);
    verify_region(hrgn, &test_rect2);
    DeleteObject(hrgn);

    status = GdipCombineRegionRect(region, &test_rectF, CombineModeReplace);
    ok(status == Ok, "status %08x\n", status);
    status = GdipCombineRegionRect(region, &test_rect3F, CombineModeComplement);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionHRgn(region, NULL, &hrgn);
    ok(status == Ok, "status %08x\n", status);
    verify_region(hrgn, &test_rect2);
    DeleteObject(hrgn);

    status = GdipDeletePath(path);
    ok(status == Ok, "status %08x\n", status);
    status = GdipDeleteRegion(region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipDeleteRegion(region2);
    ok(status == Ok, "status %08x\n", status);
    status = GdipDeleteGraphics(graphics);
    ok(status == Ok, "status %08x\n", status);
    ReleaseDC(0, hdc);
}

static void test_isequal(void)
{
    GpRegion *region1, *region2;
    GpGraphics *graphics;
    GpRectF rectf;
    GpStatus status;
    HDC hdc = GetDC(0);
    BOOL res;

    status = GdipCreateFromHDC(hdc, &graphics);
    ok(status == Ok, "status %08x\n", status);

    status = GdipCreateRegion(&region1);
    ok(status == Ok, "status %08x\n", status);
    status = GdipCreateRegion(&region2);
    ok(status == Ok, "status %08x\n", status);

    /* NULL */
    status = GdipIsEqualRegion(NULL, NULL, NULL, NULL);
    ok(status == InvalidParameter, "status %08x\n", status);
    status = GdipIsEqualRegion(region1, region2, NULL, NULL);
    ok(status == InvalidParameter, "status %08x\n", status);
    status = GdipIsEqualRegion(region1, region2, graphics, NULL);
    ok(status == InvalidParameter, "status %08x\n", status);
    status = GdipIsEqualRegion(region1, region2, NULL, &res);
    ok(status == InvalidParameter, "status %08x\n", status);

    /* infinite regions */
    res = FALSE;
    status = GdipIsEqualRegion(region1, region2, graphics, &res);
    ok(status == Ok, "status %08x\n", status);
    ok(res, "Expected to be equal.\n");
    /* empty regions */
    status = GdipSetEmpty(region1);
    ok(status == Ok, "status %08x\n", status);
    status = GdipSetEmpty(region2);
    ok(status == Ok, "status %08x\n", status);
    res = FALSE;
    status = GdipIsEqualRegion(region1, region2, graphics, &res);
    ok(status == Ok, "status %08x\n", status);
    ok(res, "Expected to be equal.\n");
    /* empty & infinite */
    status = GdipSetInfinite(region1);
    ok(status == Ok, "status %08x\n", status);
    res = TRUE;
    status = GdipIsEqualRegion(region1, region2, graphics, &res);
    ok(status == Ok, "status %08x\n", status);
    ok(!res, "Expected to be unequal.\n");
    /* rect & (inf/empty) */
    rectf.X = rectf.Y = 0.0;
    rectf.Width = rectf.Height = 100.0;
    status = GdipCombineRegionRect(region1, &rectf, CombineModeReplace);
    ok(status == Ok, "status %08x\n", status);
    res = TRUE;
    status = GdipIsEqualRegion(region1, region2, graphics, &res);
    ok(status == Ok, "status %08x\n", status);
    ok(!res, "Expected to be unequal.\n");
    status = GdipSetInfinite(region2);
    ok(status == Ok, "status %08x\n", status);
    res = TRUE;
    status = GdipIsEqualRegion(region1, region2, graphics, &res);
    ok(status == Ok, "status %08x\n", status);
    ok(!res, "Expected to be unequal.\n");
    /* roughly equal rectangles */
    rectf.X = rectf.Y = 0.0;
    rectf.Width = rectf.Height = 100.001;
    status = GdipCombineRegionRect(region2, &rectf, CombineModeReplace);
    ok(status == Ok, "status %08x\n", status);
    res = FALSE;
    status = GdipIsEqualRegion(region1, region2, graphics, &res);
    ok(status == Ok, "status %08x\n", status);
    ok(res, "Expected to be equal.\n");
    /* equal rectangles */
    rectf.X = rectf.Y = 0.0;
    rectf.Width = rectf.Height = 100.0;
    status = GdipCombineRegionRect(region2, &rectf, CombineModeReplace);
    ok(status == Ok, "status %08x\n", status);
    res = FALSE;
    status = GdipIsEqualRegion(region1, region2, graphics, &res);
    ok(status == Ok, "status %08x\n", status);
    ok(res, "Expected to be equal.\n");

    /* cleanup */
    status = GdipDeleteRegion(region1);
    ok(status == Ok, "status %08x\n", status);
    status = GdipDeleteRegion(region2);
    ok(status == Ok, "status %08x\n", status);
    status = GdipDeleteGraphics(graphics);
    ok(status == Ok, "status %08x\n", status);
    ReleaseDC(0, hdc);
}

static void test_translate(void)
{
    GpRegion *region, *region2;
    GpGraphics *graphics;
    GpPath *path;
    GpRectF rectf;
    GpStatus status;
    HDC hdc = GetDC(0);
    BOOL res;

    status = GdipCreateFromHDC(hdc, &graphics);
    ok(status == Ok, "status %08x\n", status);

    status = GdipCreatePath(FillModeAlternate, &path);
    ok(status == Ok, "status %08x\n", status);

    status = GdipCreateRegion(&region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipCreateRegion(&region2);
    ok(status == Ok, "status %08x\n", status);

    /* NULL */
    status = GdipTranslateRegion(NULL, 0.0, 0.0);
    ok(status == InvalidParameter, "status %08x\n", status);

    /* infinite */
    status = GdipTranslateRegion(region, 10.0, 10.0);
    ok(status == Ok, "status %08x\n", status);
    /* empty */
    status = GdipSetEmpty(region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipTranslateRegion(region, 10.0, 10.0);
    ok(status == Ok, "status %08x\n", status);
    /* rect */
    rectf.X = 10.0; rectf.Y = 0.0;
    rectf.Width = rectf.Height = 100.0;
    status = GdipCombineRegionRect(region, &rectf, CombineModeReplace);
    ok(status == Ok, "status %08x\n", status);
    rectf.X = 15.0; rectf.Y = -2.0;
    rectf.Width = rectf.Height = 100.0;
    status = GdipCombineRegionRect(region2, &rectf, CombineModeReplace);
    ok(status == Ok, "status %08x\n", status);
    status = GdipTranslateRegion(region, 5.0, -2.0);
    ok(status == Ok, "status %08x\n", status);
    res = FALSE;
    status = GdipIsEqualRegion(region, region2, graphics, &res);
    ok(status == Ok, "status %08x\n", status);
    ok(res, "Expected to be equal.\n");
    /* path */
    status = GdipAddPathEllipse(path, 0.0, 10.0, 100.0, 150.0);
    ok(status == Ok, "status %08x\n", status);
    status = GdipCombineRegionPath(region, path, CombineModeReplace);
    ok(status == Ok, "status %08x\n", status);
    status = GdipResetPath(path);
    ok(status == Ok, "status %08x\n", status);
    status = GdipAddPathEllipse(path, 10.0, 21.0, 100.0, 150.0);
    ok(status == Ok, "status %08x\n", status);
    status = GdipCombineRegionPath(region2, path, CombineModeReplace);
    ok(status == Ok, "status %08x\n", status);
    status = GdipTranslateRegion(region, 10.0, 11.0);
    ok(status == Ok, "status %08x\n", status);
    res = FALSE;
    status = GdipIsEqualRegion(region, region2, graphics, &res);
    ok(status == Ok, "status %08x\n", status);
    ok(res, "Expected to be equal.\n");

    status = GdipDeleteRegion(region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipDeleteRegion(region2);
    ok(status == Ok, "status %08x\n", status);
    status = GdipDeleteGraphics(graphics);
    ok(status == Ok, "status %08x\n", status);
    status = GdipDeletePath(path);
    ok(status == Ok, "status %08x\n", status);
    ReleaseDC(0, hdc);
}

static void test_getbounds(void)
{
    GpRegion *region;
    GpGraphics *graphics;
    GpStatus status;
    GpRectF rectf;
    HDC hdc = GetDC(0);

    status = GdipCreateFromHDC(hdc, &graphics);
    ok(status == Ok, "status %08x\n", status);
    status = GdipCreateRegion(&region);
    ok(status == Ok, "status %08x\n", status);

    /* NULL */
    status = GdipGetRegionBounds(NULL, NULL, NULL);
    ok(status == InvalidParameter, "status %08x\n", status);
    status = GdipGetRegionBounds(region, NULL, NULL);
    ok(status == InvalidParameter, "status %08x\n", status);
    status = GdipGetRegionBounds(region, graphics, NULL);
    ok(status == InvalidParameter, "status %08x\n", status);
    /* infinite */
    rectf.X = rectf.Y = 0.0;
    rectf.Height = rectf.Width = 100.0;
    status = GdipGetRegionBounds(region, graphics, &rectf);
    ok(status == Ok, "status %08x\n", status);
    ok(rectf.X == -(REAL)(1 << 22), "Expected X = %.2f, got %.2f\n", -(REAL)(1 << 22), rectf.X);
    ok(rectf.Y == -(REAL)(1 << 22), "Expected Y = %.2f, got %.2f\n", -(REAL)(1 << 22), rectf.Y);
    ok(rectf.Width  == (REAL)(1 << 23), "Expected width = %.2f, got %.2f\n", (REAL)(1 << 23), rectf.Width);
    ok(rectf.Height == (REAL)(1 << 23), "Expected height = %.2f, got %.2f\n",(REAL)(1 << 23), rectf.Height);
    /* empty */
    rectf.X = rectf.Y = 0.0;
    rectf.Height = rectf.Width = 100.0;
    status = GdipSetEmpty(region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionBounds(region, graphics, &rectf);
    ok(status == Ok, "status %08x\n", status);
    ok(rectf.X == 0.0, "Expected X = 0.0, got %.2f\n", rectf.X);
    ok(rectf.Y == 0.0, "Expected Y = 0.0, got %.2f\n", rectf.Y);
    ok(rectf.Width  == 0.0, "Expected width = 0.0, got %.2f\n", rectf.Width);
    ok(rectf.Height == 0.0, "Expected height = 0.0, got %.2f\n", rectf.Height);
    /* rect */
    rectf.X = 10.0; rectf.Y = 0.0;
    rectf.Width = rectf.Height = 100.0;
    status = GdipCombineRegionRect(region, &rectf, CombineModeReplace);
    ok(status == Ok, "status %08x\n", status);
    rectf.X = rectf.Y = 0.0;
    rectf.Height = rectf.Width = 0.0;
    status = GdipGetRegionBounds(region, graphics, &rectf);
    ok(status == Ok, "status %08x\n", status);
    ok(rectf.X == 10.0, "Expected X = 0.0, got %.2f\n", rectf.X);
    ok(rectf.Y == 0.0, "Expected Y = 0.0, got %.2f\n", rectf.Y);
    ok(rectf.Width  == 100.0, "Expected width = 0.0, got %.2f\n", rectf.Width);
    ok(rectf.Height == 100.0, "Expected height = 0.0, got %.2f\n", rectf.Height);

    status = GdipDeleteRegion(region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipDeleteGraphics(graphics);
    ok(status == Ok, "status %08x\n", status);
    ReleaseDC(0, hdc);
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

    test_getregiondata();
    test_isinfinite();
    test_isempty();
    test_combinereplace();
    test_fromhrgn();
    test_gethrgn();
    test_isequal();
    test_translate();
    test_getbounds();

    GdiplusShutdown(gdiplusToken);
}
