/*
 * Unit test suite for gdiplus regions
 *
 * Copyright (C) 2008 Huw Davies
 * Copyright (C) 2013 Dmitry Timoshkov
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

#include <math.h>

#include "objbase.h"
#include "gdiplus.h"
#include "wine/test.h"

#define RGNDATA_RECT            0x10000000
#define RGNDATA_PATH            0x10000001
#define RGNDATA_EMPTY_RECT      0x10000002
#define RGNDATA_INFINITE_RECT   0x10000003

#define RGNDATA_MAGIC           0xdbc01001
#define RGNDATA_MAGIC2          0xdbc01002

#define expect(expected,got) expect_(__LINE__, expected, got)
static inline void expect_(unsigned line, DWORD expected, DWORD got)
{
    ok_(__FILE__, line)(expected == got, "Expected %ld, got %ld\n", expected, got);
}
#define expectf_(expected, got, precision) ok(fabs((expected) - (got)) < (precision), "Expected %f, got %f\n", (expected), (got))
#define expectf(expected, got) expectf_((expected), (got), 0.001)

#define expect_magic(value) ok(broken(*(value) == RGNDATA_MAGIC) || *(value) == RGNDATA_MAGIC2, "Expected a known magic value, got %8lx\n", *(value))
#define expect_dword(value, expected) expect((expected), *(value))
#define expect_float(value, expected) expectf((expected), *(FLOAT *)(value))

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
        ok(ret == sizeof(rgn.data.rdh), "expected sizeof(rdh), got %lu\n", ret);
    else
        ok(ret == sizeof(rgn.data.rdh) + sizeof(RECT), "expected sizeof(rgn), got %lu\n", ret);

    if (!ret) return;

    ret = GetRegionData(hrgn, sizeof(rgn), &rgn.data);
    if (IsRectEmpty(rc))
        ok(ret == sizeof(rgn.data.rdh), "expected sizeof(rdh), got %lu\n", ret);
    else
        ok(ret == sizeof(rgn.data.rdh) + sizeof(RECT), "expected sizeof(rgn), got %lu\n", ret);

    trace("size %lu, type %lu, count %lu, rgn size %lu, bound %s\n",
          rgn.data.rdh.dwSize, rgn.data.rdh.iType,
          rgn.data.rdh.nCount, rgn.data.rdh.nRgnSize,
          wine_dbgstr_rect(&rgn.data.rdh.rcBound));
    if (rgn.data.rdh.nCount != 0)
    {
        rect = (const RECT *)rgn.data.Buffer;
        trace("rect %s\n", wine_dbgstr_rect(rect));
        ok(EqualRect(rect, rc), "expected %s, got %s\n",
           wine_dbgstr_rect(rc), wine_dbgstr_rect(rect));
    }

    ok(rgn.data.rdh.dwSize == sizeof(rgn.data.rdh), "expected sizeof(rdh), got %lu\n", rgn.data.rdh.dwSize);
    ok(rgn.data.rdh.iType == RDH_RECTANGLES, "expected RDH_RECTANGLES, got %lu\n", rgn.data.rdh.iType);
    if (IsRectEmpty(rc))
    {
        ok(rgn.data.rdh.nCount == 0, "expected 0, got %lu\n", rgn.data.rdh.nCount);
        ok(rgn.data.rdh.nRgnSize == 0,  "expected 0, got %lu\n", rgn.data.rdh.nRgnSize);
    }
    else
    {
        ok(rgn.data.rdh.nCount == 1, "expected 1, got %lu\n", rgn.data.rdh.nCount);
        ok(rgn.data.rdh.nRgnSize == sizeof(RECT),  "expected sizeof(RECT), got %lu\n", rgn.data.rdh.nRgnSize);
    }
    ok(EqualRect(&rgn.data.rdh.rcBound, rc), "expected %s, got %s\n",
       wine_dbgstr_rect(rc), wine_dbgstr_rect(&rgn.data.rdh.rcBound));
}

static void test_region_data(DWORD *data, UINT size, INT line)
{
    GpStatus status;
    GpRegion *region;
    DWORD buf[256];
    UINT needed, i;

    status = GdipCreateRegionRgnData((BYTE *)data, size, &region);
    /* Windows always fails to create an empty path in a region */
    if (data[4] == RGNDATA_PATH)
    {
        struct path_header
        {
            DWORD size;
            DWORD magic;
            DWORD count;
            DWORD flags;
        } *path_header = (struct path_header *)(data + 5);
        if (!path_header->count)
        {
            ok_(__FILE__, line)(status == GenericError, "expected GenericError, got %d\n", status);
            return;
        }
    }

    ok_(__FILE__, line)(status == Ok, "GdipCreateRegionRgnData error %d\n", status);
    if (status != Ok) return;

    needed = 0;
    status = GdipGetRegionDataSize(region, &needed);
    ok_(__FILE__, line)(status == Ok, "status %d\n", status);
    ok_(__FILE__, line)(needed == size, "data size mismatch: %u != %u\n", needed, size);

    memset(buf, 0xee, sizeof(buf));
    needed = 0;
    status = GdipGetRegionData(region, (BYTE *)buf, sizeof(buf), &needed);
    ok_(__FILE__, line)(status == Ok, "status %08x\n", status);
    ok_(__FILE__, line)(needed == size, "data size mismatch: %u != %u\n", needed, size);

    size /= sizeof(DWORD);
    for (i = 0; i < size - 1; i++)
    {
        if (i == 1) continue; /* data[1] never matches */
        ok_(__FILE__, line)(data[i] == buf[i], "off %u: %#lx != %#lx\n", i, data[i], buf[i]);
    }
    /* some Windows versions fail to properly clear the aligned DWORD */
    ok_(__FILE__, line)(data[size - 1] == buf[size - 1] || broken(data[size - 1] != buf[size - 1]),
        "off %u: %#lx != %#lx\n", size - 1, data[size - 1], buf[size - 1]);

    GdipDeleteRegion(region);
}

static void test_getregiondata(void)
{
    GpStatus status;
    GpRegion *region, *region2;
    RegionDataPoint *point;
    UINT needed;
    DWORD buf[256];
    GpRect rect;
    GpPath *path;
    GpMatrix *matrix;

    status = GdipCreateRegion(&region);
    ok(status == Ok, "status %08x\n", status);

    needed = 0;
    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(20, needed);

    needed = 0;
    status = GdipGetRegionData(region, (BYTE*)buf, 0, &needed);
    ok(status == InvalidParameter, "status %08x\n", status);

    memset(buf, 0xee, sizeof(buf));
    needed = 0;
    status = GdipGetRegionData(region, (BYTE*)buf, 4, &needed);
    ok(status == InsufficientBuffer, "status %08x\n", status);
    expect(4, needed);
    expect_dword(buf, 0xeeeeeeee);

    memset(buf, 0xee, sizeof(buf));
    needed = 0;
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(20, needed);
    expect_dword(buf, 12);
    trace("buf[1] = %08lx\n", buf[1]);
    expect_magic(buf + 2);
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_INFINITE_RECT);
    expect_dword(buf + 6, 0xeeeeeeee);
    test_region_data(buf, needed, __LINE__);

    status = GdipSetEmpty(region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(20, needed);
    memset(buf, 0xee, sizeof(buf));
    needed = 0;
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(20, needed);
    expect_dword(buf, 12);
    trace("buf[1] = %08lx\n", buf[1]);
    expect_magic(buf + 2);
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_EMPTY_RECT);
    expect_dword(buf + 6, 0xeeeeeeee);
    test_region_data(buf, needed, __LINE__);

    status = GdipSetInfinite(region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(20, needed);
    memset(buf, 0xee, sizeof(buf));
    needed = 0;
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(20, needed);
    expect_dword(buf, 12);
    trace("buf[1] = %08lx\n", buf[1]);
    expect_magic(buf + 2);
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_INFINITE_RECT);
    expect_dword(buf + 6, 0xeeeeeeee);
    test_region_data(buf, needed, __LINE__);

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
    memset(buf, 0xee, sizeof(buf));
    needed = 0;
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(36, needed);
    expect_dword(buf, 28);
    trace("buf[1] = %08lx\n", buf[1]);
    expect_magic(buf + 2);
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_RECT);
    expect_float(buf + 5, 10.0);
    expect_float(buf + 6, 20.0);
    expect_float(buf + 7, 100.0);
    expect_float(buf + 8, 200.0);
    expect_dword(buf + 10, 0xeeeeeeee);
    test_region_data(buf, needed, __LINE__);

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
    memset(buf, 0xee, sizeof(buf));
    needed = 0;
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(156, needed);
    expect_dword(buf, 148);
    trace("buf[1] = %08lx\n", buf[1]);
    expect_magic(buf + 2);
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
    expect_dword(buf + 39, 0xeeeeeeee);
    test_region_data(buf, needed, __LINE__);

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
    memset(buf, 0xee, sizeof(buf));
    needed = 0;
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(72, needed);
    expect_dword(buf, 64);
    trace("buf[1] = %08lx\n", buf[1]);
    expect_magic(buf + 2);
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_PATH);
    expect_dword(buf + 5, 0x00000030);
    expect_magic(buf + 6);
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
    expect_dword(buf + 18, 0xeeeeeeee);
    test_region_data(buf, needed, __LINE__);

    rect.X = 50;
    rect.Y = 30;
    rect.Width = 10;
    rect.Height = 20;
    status = GdipCombineRegionRectI(region, &rect, CombineModeIntersect);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(96, needed);
    memset(buf, 0xee, sizeof(buf));
    needed = 0;
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(96, needed);
    expect_dword(buf, 88);
    trace("buf[1] = %08lx\n", buf[1]);
    expect_magic(buf + 2);
    expect_dword(buf + 3, 2);
    expect_dword(buf + 4, CombineModeIntersect);
    expect_dword(buf + 5, RGNDATA_PATH);
    expect_dword(buf + 6, 0x00000030);
    expect_magic(buf + 7);
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
    expect_dword(buf + 24, 0xeeeeeeee);
    test_region_data(buf, needed, __LINE__);

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
    memset(buf, 0xee, sizeof(buf));
    needed = 0;
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    expect(Ok, status);
    expect(36, needed);
    expect_dword(buf, 28);
    trace("buf[1] = %08lx\n", buf[1]);
    expect_magic(buf + 2);
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_PATH);
    /* Second signature for pathdata */
    expect_dword(buf + 5, 12);
    expect_magic(buf + 6);
    expect_dword(buf + 7, 0);
    /* flags 0 means that a path is an array of FLOATs */
    ok(*(buf + 8) == 0x4000 /* before win7 */ || *(buf + 8) == 0,
       "expected 0x4000 or 0, got %08lx\n", *(buf + 8));
    expect_dword(buf + 10, 0xeeeeeeee);
    test_region_data(buf, needed, __LINE__);

    /* Transform an empty region */
    status = GdipCreateMatrix(&matrix);
    expect(Ok, status);
    status = GdipTransformRegion(region, matrix);
    expect(Ok, status);
    GdipDeleteMatrix(matrix);

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
    memset(buf, 0xee, sizeof(buf));
    needed = 0;
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    expect(Ok, status);
    expect(56, needed);
    expect_dword(buf, 48);
    trace("buf[1] = %08lx\n", buf[1]);
    expect_magic(buf + 2);
    expect_dword(buf + 3 , 0);
    expect_dword(buf + 4 , RGNDATA_PATH);
    expect_dword(buf + 5, 32);
    expect_magic(buf + 6);
    expect_dword(buf + 7, 4);
    /* flags 0x4000 means that a path is an array of shorts instead of FLOATs */
    expect_dword(buf + 8, 0x4000);

    point = (RegionDataPoint*)(buf + 9);
    expect(5, point[0].X);
    expect(6, point[0].Y);
    expect(7, point[1].X); /* buf + 10 */
    expect(8, point[1].Y);
    expect(8, point[2].X); /* buf + 11 */
    expect(1, point[2].Y);
    expect(5, point[3].X); /* buf + 12 */
    expect(6, point[3].Y);
    expect_dword(buf + 13, 0x81010100); /* 0x01010100 if we don't close the path */
    expect_dword(buf + 14, 0xeeeeeeee);
    test_region_data(buf, needed, __LINE__);

    status = GdipTranslateRegion(region, 0.6, 0.8);
    expect(Ok, status);
    memset(buf, 0xee, sizeof(buf));
    needed = 0;
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    expect(Ok, status);
    expect(72, needed);
    expect_dword(buf, 64);
    expect_magic(buf + 2);
    expect_dword(buf + 3 , 0);
    expect_dword(buf + 4 , RGNDATA_PATH);
    expect_dword(buf + 5, 48);
    expect_magic(buf + 6);
    expect_dword(buf + 7, 4);
    /* flags 0 means that a path is an array of FLOATs */
    expect_dword(buf + 8, 0);
    expect_float(buf + 9, 5.6);
    expect_float(buf + 10, 6.8);
    expect_float(buf + 11, 7.6);
    expect_float(buf + 12, 8.8);
    expect_float(buf + 13, 8.6);
    expect_float(buf + 14, 1.8);
    expect_float(buf + 15, 5.6);
    expect_float(buf + 16, 6.8);
    expect_dword(buf + 17, 0x81010100); /* 0x01010100 if we don't close the path */
    expect_dword(buf + 18, 0xeeeeeeee);
    test_region_data(buf, needed, __LINE__);

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
    memset(buf, 0xee, sizeof(buf));
    needed = 0;
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    expect(Ok, status);
    expect(72, needed);
    expect_dword(buf, 64);
    trace("buf[1] = %08lx\n", buf[1]);
    expect_magic(buf + 2);
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_PATH);
    expect_dword(buf + 5, 48);
    expect_magic(buf + 6);
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
    expect_dword(buf + 17, 0x01010100);
    expect_dword(buf + 18, 0xeeeeeeee);
    test_region_data(buf, needed, __LINE__);

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
    memset(buf, 0xee, sizeof(buf));
    needed = 0;
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    expect(Ok, status);
    expect(116, needed);
    expect_dword(buf, 108);
    trace("buf[1] = %08lx\n", buf[1]);
    expect_magic(buf + 2);
    expect_dword(buf + 3, 2);
    expect_dword(buf + 4, CombineModeUnion);
    expect_dword(buf + 5, RGNDATA_RECT);
    expect_float(buf + 6, 20.0);
    expect_float(buf + 7, 25.0);
    expect_float(buf + 8, 60.0);
    expect_float(buf + 9, 120.0);
    expect_dword(buf + 10, RGNDATA_PATH);
    expect_dword(buf + 11, 68);
    expect_magic(buf + 12);
    expect_dword(buf + 13, 6);
    expect_float(buf + 14, 0.0);
    expect_float(buf + 15, 50.0);
    expect_float(buf + 16, 70.2);
    expect_float(buf + 17, 60.0);
    expect_float(buf + 18, 102.8);
    expect_float(buf + 19, 55.4);
    expect_float(buf + 20, 122.4);
    expect_float(buf + 21, 40.4);
    expect_float(buf + 22, 60.2);
    expect_float(buf + 23, 45.6);
    expect_float(buf + 24, 20.2);
    expect_float(buf + 25, 50.0);
    expect_float(buf + 26, 70.2);
    expect_dword(buf + 27, 0x01010100);
    ok((*(buf + 28) & 0xffff) == 0x0101,
       "expected ????0101 got %08lx\n", *(buf + 28));
    expect_dword(buf + 29, 0xeeeeeeee);
    test_region_data(buf, needed, __LINE__);

    status = GdipDeletePath(path);
    expect(Ok, status);
    status = GdipDeleteRegion(region);
    expect(Ok, status);

    /* Test how shorts are stored in the region path data */
    status = GdipCreatePath(FillModeAlternate, &path);
    ok(status == Ok, "status %08x\n", status);
    GdipAddPathRectangleI(path, -1969, -1974, 1995, 1997);

    status = GdipCreateRegionPath(path, &region);
    ok(status == Ok, "status %08x\n", status);
    needed = 0;
    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(56, needed);
    memset(buf, 0xee, sizeof(buf));
    needed = 0;
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(56, needed);
    expect_dword(buf, 48);
    trace("buf[1] = %08lx\n", buf[1]);
    expect_magic(buf + 2);
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_PATH);
    expect_dword(buf + 5, 32);
    expect_magic(buf + 6);
    expect_dword(buf + 7, 4);
    /* flags 0x4000 means that a path is an array of shorts instead of FLOATs */
    expect_dword(buf + 8, 0x4000);
    point = (RegionDataPoint*)(buf + 9);
    expect(-1969, point[0].X);
    expect(-1974, point[0].Y);
    expect(26, point[1].X); /* buf + 10 */
    expect(-1974, point[1].Y);
    expect(26, point[2].X); /* buf + 11 */
    expect(23, point[2].Y);
    expect(-1969, point[3].X); /* buf + 12 */
    expect(23, point[3].Y);
    expect_dword(buf + 13, 0x81010100); /* 0x01010100 if we don't close the path */
    expect_dword(buf + 14, 0xeeeeeeee);
    test_region_data(buf, needed, __LINE__);

    status = GdipDeletePath(path);
    expect(Ok, status);
    status = GdipDeleteRegion(region);
    expect(Ok, status);

    /* Test with integers that can't be stored as shorts */
    status = GdipCreatePath(FillModeAlternate, &path);
    ok(status == Ok, "status %08x\n", status);
    GdipAddPathRectangleI(path, -196900, -197400, 199500, 199700);

    status = GdipCreateRegionPath(path, &region);
    ok(status == Ok, "status %08x\n", status);
    needed = 0;
    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(72, needed);
    memset(buf, 0xee, sizeof(buf));
    needed = 0;
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(72, needed);
    expect_dword(buf, 64);
    trace("buf[1] = %08lx\n", buf[1]);
    expect_magic(buf + 2);
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_PATH);
    expect_dword(buf + 5, 48);
    expect_magic(buf + 6);
    expect_dword(buf + 7, 4);
    /* flags 0 means that a path is an array of FLOATs */
    expect_dword(buf + 8, 0);
    expect_float(buf + 9, -196900.0);
    expect_float(buf + 10, -197400.0);
    expect_float(buf + 11, 2600.0);
    expect_float(buf + 12, -197400.0);
    expect_float(buf + 13, 2600.0);
    expect_float(buf + 14, 2300.0);
    expect_float(buf + 15, -196900.0);
    expect_float(buf + 16, 2300.0);
    expect_dword(buf + 17, 0x81010100); /* 0x01010100 if we don't close the path */
    expect_dword(buf + 18, 0xeeeeeeee);
    test_region_data(buf, needed, __LINE__);

    status = GdipDeletePath(path);
    expect(Ok, status);
    status = GdipDeleteRegion(region);
    expect(Ok, status);

    /* Test beziers */
    GdipCreatePath(FillModeAlternate, &path);
      /* Exactly 90 degrees */
    status = GdipAddPathArc(path, 100.0, 100.0, 500.0, 700.0, 0.0, 90.0);
    expect(Ok, status);
    /* Over 90 degrees */
    status = GdipAddPathArc(path, 100.0, 100.0, 500.0, 700.0, 0.0, 100.0);
    expect(Ok, status);
    status = GdipCreateRegionPath(path, &region);
    ok(status == Ok, "status %08x\n", status);
    needed = 0;
    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(136, needed);
    memset(buf, 0xee, sizeof(buf));
    needed = 0;
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(136, needed);
    expect_dword(buf, 128);
    trace("buf[1] = %08lx\n", buf[1]);
    expect_magic(buf + 2);
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_PATH);
    expect_dword(buf + 5, 112);
    expect_magic(buf + 6);
    expect_dword(buf + 7, 11);
    /* flags 0 means that a path is an array of FLOATs */
    expect_dword(buf + 8, 0);
    expect_float(buf + 9, 600.0);
    expect_float(buf + 10, 450.0);
    expect_float(buf + 11, 600.0);
    expect_float(buf + 12, 643.299561);
    expect_float(buf + 13, 488.071198);
    expect_float(buf + 14, 800.0);
    expect_float(buf + 15, 350.0);
    expect_float(buf + 16, 800.0);
    expect_float(buf + 17, 600.0);
    expect_float(buf + 18, 450.0);
    expect_float(buf + 19, 600.0);
    expect_float(buf + 20, 643.299622);
    expect_float(buf + 21, 488.071167);
    expect_float(buf + 22, 800.0);
    expect_float(buf + 23, 350.0);
    expect_float(buf + 24, 800.0);
    expect_float(buf + 25, 329.807129);
    expect_float(buf + 26, 800.0);
    expect_float(buf + 27, 309.688568);
    expect_float(buf + 28, 796.574890);
    expect_float(buf + 29, 290.084167);
    expect_float(buf + 30, 789.799561);
    expect_dword(buf + 31, 0x03030300);
    expect_dword(buf + 32, 0x03030301);
    ok((*(buf + 33) & 0xffffff) == 0x030303,
       "expected 0x??030303 got %08lx\n", *(buf + 33));
    expect_dword(buf + 34, 0xeeeeeeee);
    test_region_data(buf, needed, __LINE__);

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
    status = GdipCreateRegion(&region);
    expect(Ok, status);

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
    status = GdipCreateRegion(&region);
    expect(Ok, status);

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
    trace("buf[1] = %08lx\n", buf[1]);
    expect_magic(buf + 2);
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
    trace("buf[1] = %08lx\n", buf[1]);
    expect_magic(buf + 2);
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
    trace("buf[1] = %08lx\n", buf[1]);
    expect_magic(buf + 2);
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
    trace("buf[1] = %08lx\n", buf[1]);
    expect_magic(buf + 2);
    expect_dword(buf + 3, 2);
    expect_dword(buf + 4, CombineModeUnion);

    GdipDeleteRegion(region);
}

static void test_fromhrgn(void)
{
    GpStatus status;
    GpRegion *region = (GpRegion*)0xabcdef01;
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
    ok(region == (GpRegion*)0xabcdef01, "Expected region not to be created\n");

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
    expect_magic(buf + 2);
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_PATH);
    expect_dword(buf + 5, 0x00000020);
    expect_magic(buf + 6);
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
    expect(Ok, status);

    status = GdipGetRegionDataSize(region, &needed);
    expect(Ok, status);
    ok(needed == 216 ||
       needed == 196, /* win98 */
       "Got %.8x\n", needed);

    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    expect(Ok, status);

    if(status == Ok && needed == 216) /* Don't try to test win98 layout */
    {
    expect(Ok, status);
    expect(216, needed);
    expect_dword(buf, 208);
    expect_magic(buf + 2);
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_PATH);
    expect_dword(buf + 5, 0x000000C0);
    expect_magic(buf + 6);
    expect_dword(buf + 7, 0x00000024);
    todo_wine expect_dword(buf + 8, 0x00006000); /* ?? */
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
    INT rgntype;
    RECT rgnbox;
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

    status = GdipGetRegionHRgn(region, graphics, &hrgn);
    ok(status == Ok, "status %08x\n", status);
    ok(hrgn == NULL, "hrgn=%p\n", hrgn);

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

    /* test with gdi32 transform */
    SetViewportOrgEx(hdc, 10, 10, NULL);

    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);

    status = GdipCreateRegionRect(&test_rectF, &region);
    expect(Ok, status);

    status = GdipGetRegionHRgn(region, graphics, &hrgn);
    expect(Ok, status);

    rgntype = GetRgnBox(hrgn, &rgnbox);
    DeleteObject(hrgn);

    expect(SIMPLEREGION, rgntype);
    expect(20, rgnbox.left);
    expect(21, rgnbox.top);
    expect(30, rgnbox.right);
    expect(31, rgnbox.bottom);

    status = GdipDeleteRegion(region);
    expect(Ok, status);
    status = GdipDeleteGraphics(graphics);
    expect(Ok, status);

    SetViewportOrgEx(hdc, 0, 0, NULL);

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

static DWORD get_region_type(GpRegion *region)
{
    DWORD *data;
    UINT size;
    DWORD result;
    DWORD status;
    status = GdipGetRegionDataSize(region, &size);
    expect(Ok, status);
    data = GdipAlloc(size);
    status = GdipGetRegionData(region, (BYTE*)data, size, NULL);
    ok(status == Ok || status == InsufficientBuffer, "unexpected status 0x%lx\n", status);
    result = data[4];
    GdipFree(data);
    return result;
}

static void test_transform(void)
{
    GpRegion *region, *region2;
    GpMatrix *matrix;
    GpGraphics *graphics;
    GpPath *path;
    GpRectF rectf;
    GpStatus status;
    HDC hdc = GetDC(0);
    BOOL res;
    DWORD type;

    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);

    status = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, status);

    status = GdipCreateRegion(&region);
    expect(Ok, status);
    status = GdipCreateRegion(&region2);
    expect(Ok, status);

    status = GdipCreateMatrix(&matrix);
    expect(Ok, status);
    status = GdipScaleMatrix(matrix, 2.0, 3.0, MatrixOrderAppend);
    expect(Ok, status);

    /* NULL */
    status = GdipTransformRegion(NULL, matrix);
    expect(InvalidParameter, status);

    status = GdipTransformRegion(region, NULL);
    expect(InvalidParameter, status);

    /* infinite */
    status = GdipTransformRegion(region, matrix);
    expect(Ok, status);

    res = FALSE;
    status = GdipIsEqualRegion(region, region2, graphics, &res);
    expect(Ok, status);
    ok(res, "Expected to be equal.\n");
    type = get_region_type(region);
    expect(0x10000003 /* RegionDataInfiniteRect */, type);

    /* empty */
    status = GdipSetEmpty(region);
    expect(Ok, status);
    status = GdipTransformRegion(region, matrix);
    expect(Ok, status);

    status = GdipSetEmpty(region2);
    expect(Ok, status);

    res = FALSE;
    status = GdipIsEqualRegion(region, region2, graphics, &res);
    expect(Ok, status);
    ok(res, "Expected to be equal.\n");
    type = get_region_type(region);
    expect(0x10000002 /* RegionDataEmptyRect */, type);

    /* rect */
    rectf.X = 10.0;
    rectf.Y = 0.0;
    rectf.Width = rectf.Height = 100.0;
    status = GdipCombineRegionRect(region, &rectf, CombineModeReplace);
    expect(Ok, status);
    rectf.X = 20.0;
    rectf.Y = 0.0;
    rectf.Width = 200.0;
    rectf.Height = 300.0;
    status = GdipCombineRegionRect(region2, &rectf, CombineModeReplace);
    expect(Ok, status);
    status = GdipTransformRegion(region, matrix);
    expect(Ok, status);
    res = FALSE;
    status = GdipIsEqualRegion(region, region2, graphics, &res);
    expect(Ok, status);
    ok(res, "Expected to be equal.\n");
    type = get_region_type(region);
    expect(0x10000000 /* RegionDataRect */, type);

    /* path */
    status = GdipAddPathEllipse(path, 0.0, 10.0, 100.0, 150.0);
    expect(Ok, status);
    status = GdipCombineRegionPath(region, path, CombineModeReplace);
    expect(Ok, status);
    status = GdipResetPath(path);
    expect(Ok, status);
    status = GdipAddPathEllipse(path, 0.0, 30.0, 200.0, 450.0);
    expect(Ok, status);
    status = GdipCombineRegionPath(region2, path, CombineModeReplace);
    expect(Ok, status);
    status = GdipTransformRegion(region, matrix);
    expect(Ok, status);
    res = FALSE;
    status = GdipIsEqualRegion(region, region2, graphics, &res);
    expect(Ok, status);
    ok(res, "Expected to be equal.\n");
    type = get_region_type(region);
    expect(0x10000001 /* RegionDataPath */, type);

    /* rotated rect -> path */
    rectf.X = 10.0;
    rectf.Y = 0.0;
    rectf.Width = rectf.Height = 100.0;
    status = GdipCombineRegionRect(region, &rectf, CombineModeReplace);
    expect(Ok, status);
    status = GdipRotateMatrix(matrix, 45.0, MatrixOrderAppend);
    expect(Ok, status);
    status = GdipTransformRegion(region, matrix);
    expect(Ok, status);
    type = get_region_type(region);
    expect(0x10000001 /* RegionDataPath */, type);

    status = GdipDeleteRegion(region);
    expect(Ok, status);
    status = GdipDeleteRegion(region2);
    expect(Ok, status);
    status = GdipDeleteGraphics(graphics);
    expect(Ok, status);
    status = GdipDeletePath(path);
    expect(Ok, status);
    status = GdipDeleteMatrix(matrix);
    expect(Ok, status);
    ReleaseDC(0, hdc);
}

static void test_scans(void)
{
    GpRegion *region;
    GpMatrix *matrix;
    GpRectF rectf;
    GpStatus status;
    UINT count=80085;
    INT icount;
    GpRectF scans[2];
    GpRect scansi[2];

    status = GdipCreateRegion(&region);
    expect(Ok, status);

    status = GdipCreateMatrix(&matrix);
    expect(Ok, status);

    /* test NULL values */
    status = GdipGetRegionScansCount(NULL, &count, matrix);
    expect(InvalidParameter, status);

    status = GdipGetRegionScansCount(region, NULL, matrix);
    expect(InvalidParameter, status);

    status = GdipGetRegionScansCount(region, &count, NULL);
    expect(InvalidParameter, status);

    status = GdipGetRegionScans(NULL, scans, &icount, matrix);
    expect(InvalidParameter, status);

    status = GdipGetRegionScans(region, scans, NULL, matrix);
    expect(InvalidParameter, status);

    status = GdipGetRegionScans(region, scans, &icount, NULL);
    expect(InvalidParameter, status);

    /* infinite */
    status = GdipGetRegionScansCount(region, &count, matrix);
    expect(Ok, status);
    expect(1, count);

    status = GdipGetRegionScans(region, NULL, &icount, matrix);
    expect(Ok, status);
    expect(1, icount);

    status = GdipGetRegionScans(region, scans, &icount, matrix);
    expect(Ok, status);
    expect(1, icount);

    status = GdipGetRegionScansI(region, scansi, &icount, matrix);
    expect(Ok, status);
    expect(1, icount);
    expect(-0x400000, scansi[0].X);
    expect(-0x400000, scansi[0].Y);
    expect(0x800000, scansi[0].Width);
    expect(0x800000, scansi[0].Height);

    status = GdipGetRegionScans(region, scans, &icount, matrix);
    expect(Ok, status);
    expect(1, icount);
    expectf((double)-0x400000, scans[0].X);
    expectf((double)-0x400000, scans[0].Y);
    expectf((double)0x800000, scans[0].Width);
    expectf((double)0x800000, scans[0].Height);

    /* empty */
    status = GdipSetEmpty(region);
    expect(Ok, status);

    status = GdipGetRegionScansCount(region, &count, matrix);
    expect(Ok, status);
    expect(0, count);

    status = GdipGetRegionScans(region, scans, &icount, matrix);
    expect(Ok, status);
    expect(0, icount);

    /* single rectangle */
    rectf.X = rectf.Y = 0.0;
    rectf.Width = rectf.Height = 5.0;
    status = GdipCombineRegionRect(region, &rectf, CombineModeReplace);
    expect(Ok, status);

    status = GdipGetRegionScansCount(region, &count, matrix);
    expect(Ok, status);
    expect(1, count);

    status = GdipGetRegionScans(region, scans, &icount, matrix);
    expect(Ok, status);
    expect(1, icount);
    expectf(0.0, scans[0].X);
    expectf(0.0, scans[0].Y);
    expectf(5.0, scans[0].Width);
    expectf(5.0, scans[0].Height);

    /* two rectangles */
    rectf.X = rectf.Y = 5.0;
    rectf.Width = rectf.Height = 5.0;
    status = GdipCombineRegionRect(region, &rectf, CombineModeUnion);
    expect(Ok, status);

    status = GdipGetRegionScansCount(region, &count, matrix);
    expect(Ok, status);
    expect(2, count);

    /* Native ignores the initial value of count */
    scans[1].X = scans[1].Y = scans[1].Width = scans[1].Height = 8.0;
    icount = 1;
    status = GdipGetRegionScans(region, scans, &icount, matrix);
    expect(Ok, status);
    expect(2, icount);
    expectf(0.0, scans[0].X);
    expectf(0.0, scans[0].Y);
    expectf(5.0, scans[0].Width);
    expectf(5.0, scans[0].Height);
    expectf(5.0, scans[1].X);
    expectf(5.0, scans[1].Y);
    expectf(5.0, scans[1].Width);
    expectf(5.0, scans[1].Height);

    status = GdipGetRegionScansI(region, scansi, &icount, matrix);
    expect(Ok, status);
    expect(2, icount);
    expect(0, scansi[0].X);
    expect(0, scansi[0].Y);
    expect(5, scansi[0].Width);
    expect(5, scansi[0].Height);
    expect(5, scansi[1].X);
    expect(5, scansi[1].Y);
    expect(5, scansi[1].Width);
    expect(5, scansi[1].Height);

    status = GdipDeleteRegion(region);
    expect(Ok, status);
    status = GdipDeleteMatrix(matrix);
    expect(Ok, status);
}

static void test_getbounds(void)
{
    GpRegion *region;
    GpPath *path;
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

    /* the world and page transforms are ignored */
    status = GdipScaleWorldTransform(graphics, 2.0, 2.0, MatrixOrderPrepend);
    ok(status == Ok, "status %08x\n", status);
    GdipSetPageUnit(graphics, UnitInch);
    GdipSetPageScale(graphics, 2.0);
    status = GdipGetRegionBounds(region, graphics, &rectf);
    ok(status == Ok, "status %08x\n", status);
    ok(rectf.X == 10.0, "Expected X = 0.0, got %.2f\n", rectf.X);
    ok(rectf.Y == 0.0, "Expected Y = 0.0, got %.2f\n", rectf.Y);
    ok(rectf.Width  == 100.0, "Expected width = 0.0, got %.2f\n", rectf.Width);

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

    /* coordinates are not rounded */
    status = GdipResetWorldTransform(graphics);
    ok(status == Ok, "status %08x\n", status);
    status = GdipResetPageTransform(graphics);
    ok(status == Ok, "status %08x\n", status);
    rectf.X = 0.125;
    rectf.Y = 1.125;
    rectf.Width = 2.125;
    rectf.Height = 3.125;
    status = GdipCombineRegionRect(region, &rectf, CombineModeReplace);
    ok(status == Ok, "status %08x\n", status);
    rectf.X = rectf.Y = 0.0;
    rectf.Height = rectf.Width = 0.0;
    status = GdipGetRegionBounds(region, graphics, &rectf);
    ok(status == Ok, "status %08x\n", status);
    ok(rectf.X == 0.125, "Expected X = 0.0, got %.2f\n", rectf.X);
    ok(rectf.Y == 1.125, "Expected Y = 0.0, got %.2f\n", rectf.Y);
    ok(rectf.Width  == 2.125, "Expected width = 0.0, got %.2f\n", rectf.Width);
    ok(rectf.Height == 3.125, "Expected height = 0.0, got %.2f\n", rectf.Height);

    /* test path */
    status = GdipCreatePath(FillModeAlternate, &path);
    ok(status == Ok, "status %08x\n", status);
    status = GdipAddPathRectangle(path, 0.125, 1.125, 2.125, 3.125);
    ok(status == Ok, "status %08x\n", status);
    status = GdipCombineRegionPath(region, path, CombineModeReplace);
    ok(status == Ok, "status %08x\n", status);
    status = GdipDeletePath(path);
    ok(status == Ok, "status %08x\n", status);
    rectf.X = rectf.Y = 0.0;
    rectf.Height = rectf.Width = 0.0;
    status = GdipGetRegionBounds(region, graphics, &rectf);
    ok(status == Ok, "status %08x\n", status);
    ok(rectf.X == 0.125, "Expected X = 0.0, got %.2f\n", rectf.X);
    ok(rectf.Y == 1.125, "Expected Y = 0.0, got %.2f\n", rectf.Y);
    ok(rectf.Width  == 2.125, "Expected width = 0.0, got %.2f\n", rectf.Width);
    ok(rectf.Height == 3.125, "Expected height = 0.0, got %.2f\n", rectf.Height);

    status = GdipDeleteRegion(region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipDeleteGraphics(graphics);
    ok(status == Ok, "status %08x\n", status);
    ReleaseDC(0, hdc);
}

static void test_isvisiblepoint(void)
{
    HDC hdc = GetDC(0);
    GpGraphics* graphics;
    GpRegion* region;
    GpPath* path;
    GpRectF rectf;
    GpStatus status;
    BOOL res;
    REAL x, y;

    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);

    status = GdipCreateRegion(&region);
    expect(Ok, status);

    /* null parameters */
    status = GdipIsVisibleRegionPoint(NULL, 0, 0, graphics, &res);
    expect(InvalidParameter, status);
    status = GdipIsVisibleRegionPointI(NULL, 0, 0, graphics, &res);
    expect(InvalidParameter, status);

    status = GdipIsVisibleRegionPoint(region, 0, 0, NULL, &res);
    expect(Ok, status);
    status = GdipIsVisibleRegionPointI(region, 0, 0, NULL, &res);
    expect(Ok, status);

    status = GdipIsVisibleRegionPoint(region, 0, 0, graphics, NULL);
    expect(InvalidParameter, status);
    status = GdipIsVisibleRegionPointI(region, 0, 0, graphics, NULL);
    expect(InvalidParameter, status);

    /* infinite region */
    status = GdipIsInfiniteRegion(region, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Region should be infinite\n");

    x = 10;
    y = 10;
    status = GdipIsVisibleRegionPoint(region, x, y, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%.2f, %.2f) to be visible\n", x, y);
    status = GdipIsVisibleRegionPointI(region, (INT)x, (INT)y, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%d, %d) to be visible\n", (INT)x, (INT)y);

    x = -10;
    y = -10;
    status = GdipIsVisibleRegionPoint(region, x, y, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%.2f, %.2f) to be visible\n", x, y);
    status = GdipIsVisibleRegionPointI(region, (INT)x, (INT)y, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%d, %d) to be visible\n", (INT)x, (INT)y);

    /* rectangular region */
    rectf.X = 10;
    rectf.Y = 20;
    rectf.Width = 30;
    rectf.Height = 40;

    status = GdipCombineRegionRect(region, &rectf, CombineModeReplace);
    expect(Ok, status);

    x = 0;
    y = 0;
    status = GdipIsVisibleRegionPoint(region, x, y, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%.2f, %.2f) not to be visible\n", x, y);
    status = GdipIsVisibleRegionPointI(region, (INT)x, (INT)y, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%d, %d) not to be visible\n", (INT)x, (INT)y);

    x = 9;
    y = 19;
    status = GdipIsVisibleRegionPoint(region, x, y, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%.2f, %.2f) to be visible\n", x, y);

    x = 9.25;
    y = 19.25;
    status = GdipIsVisibleRegionPoint(region, x, y, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%.2f, %.2f) to be visible\n", x, y);

    x = 9.5;
    y = 19.5;
    status = GdipIsVisibleRegionPoint(region, x, y, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%.2f, %.2f) to be visible\n", x, y);

    x = 9.75;
    y = 19.75;
    status = GdipIsVisibleRegionPoint(region, x, y, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%.2f, %.2f) to be visible\n", x, y);

    x = 10;
    y = 20;
    status = GdipIsVisibleRegionPoint(region, x, y, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%.2f, %.2f) to be visible\n", x, y);

    x = 25;
    y = 40;
    status = GdipIsVisibleRegionPoint(region, x, y, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%.2f, %.2f) to be visible\n", x, y);
    status = GdipIsVisibleRegionPointI(region, (INT)x, (INT)y, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%d, %d) to be visible\n", (INT)x, (INT)y);

    x = 40;
    y = 60;
    status = GdipIsVisibleRegionPoint(region, x, y, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%.2f, %.2f) not to be visible\n", x, y);
    status = GdipIsVisibleRegionPointI(region, (INT)x, (INT)y, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%d, %d) not to be visible\n", (INT)x, (INT)y);

    /* translate into the center of the rectangle */
    status = GdipTranslateWorldTransform(graphics, 25, 40, MatrixOrderAppend);
    expect(Ok, status);

    /* native ignores the world transform, so treat these as if
     * no transform exists */
    x = -20;
    y = -30;
    status = GdipIsVisibleRegionPoint(region, x, y, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%.2f, %.2f) not to be visible\n", x, y);
    status = GdipIsVisibleRegionPointI(region, (INT)x, (INT)y, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%d, %d) not to be visible\n", (INT)x, (INT)y);

    x = 0;
    y = 0;
    status = GdipIsVisibleRegionPoint(region, x, y, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%.2f, %.2f) not to be visible\n", x, y);
    status = GdipIsVisibleRegionPointI(region, (INT)x, (INT)y, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%d, %d) not to be visible\n", (INT)x, (INT)y);

    x = 25;
    y = 40;
    status = GdipIsVisibleRegionPoint(region, x, y, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%.2f, %.2f) to be visible\n", x, y);
    status = GdipIsVisibleRegionPointI(region, (INT)x, (INT)y, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%d, %d) to be visible\n", (INT)x, (INT)y);

    /* translate back to origin */
    status = GdipTranslateWorldTransform(graphics, -25, -40, MatrixOrderAppend);
    expect(Ok, status);

    /* region from path */
    status = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, status);

    status = GdipAddPathEllipse(path, 10, 20, 30, 40);
    expect(Ok, status);

    status = GdipCombineRegionPath(region, path, CombineModeReplace);
    expect(Ok, status);

    x = 11;
    y = 21;
    status = GdipIsVisibleRegionPoint(region, x, y, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%.2f, %.2f) not to be visible\n", x, y);
    status = GdipIsVisibleRegionPointI(region, (INT)x, (INT)y, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%d, %d) not to be visible\n", (INT)x, (INT)y);

    x = 25;
    y = 40;
    status = GdipIsVisibleRegionPoint(region, x, y, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%.2f, %.2f) to be visible\n", x, y);
    status = GdipIsVisibleRegionPointI(region, (INT)x, (INT)y, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%d, %d) to be visible\n", (INT)x, (INT)y);

    x = 40;
    y = 60;
    status = GdipIsVisibleRegionPoint(region, x, y, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%.2f, %.2f) not to be visible\n", x, y);
    status = GdipIsVisibleRegionPointI(region, (INT)x, (INT)y, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%d, %d) not to be visible\n", (INT)x, (INT)y);

    GdipDeletePath(path);

    GdipDeleteRegion(region);
    GdipDeleteGraphics(graphics);
    ReleaseDC(0, hdc);
}

static void test_isvisiblerect(void)
{
    HDC hdc = GetDC(0);
    GpGraphics* graphics;
    GpRegion* region;
    GpPath* path;
    GpRectF rectf;
    GpStatus status;
    BOOL res;
    REAL x, y, w, h;

    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);

    status = GdipCreateRegion(&region);
    expect(Ok, status);

    /* null parameters */
    status = GdipIsVisibleRegionRect(NULL, 0, 0, 0, 0, graphics, &res);
    expect(InvalidParameter, status);
    status = GdipIsVisibleRegionRectI(NULL, 0, 0, 0, 0, graphics, &res);
    expect(InvalidParameter, status);

    status = GdipIsVisibleRegionRect(region, 0, 0, 0, 0, NULL, &res);
    expect(Ok, status);
    status = GdipIsVisibleRegionRectI(region, 0, 0, 0, 0, NULL, &res);
    expect(Ok, status);

    status = GdipIsVisibleRegionRect(region, 0, 0, 0, 0, graphics, NULL);
    expect(InvalidParameter, status);
    status = GdipIsVisibleRegionRectI(region, 0, 0, 0, 0, graphics, NULL);
    expect(InvalidParameter, status);

    /* infinite region */
    status = GdipIsInfiniteRegion(region, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Region should be infinite\n");

    x = 10; w = 10;
    y = 10; h = 10;
    status = GdipIsVisibleRegionRect(region, x, y, w, h, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%.2f, %.2f, %.2f, %.2f) to be visible\n", x, y, w, h);

    x = -10; w = 5;
    y = -10; h = 5;
    status = GdipIsVisibleRegionRect(region, x, y, w, h, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%.2f, %.2f, %.2f, %.2f) to be visible\n", x, y, w, h);

    /* rectangular region */
    rectf.X = 10;
    rectf.Y = 20;
    rectf.Width = 30;
    rectf.Height = 40;

    status = GdipCombineRegionRect(region, &rectf, CombineModeIntersect);
    expect(Ok, status);

    /* entirely within the region */
    x = 11; w = 10;
    y = 12; h = 10;
    status = GdipIsVisibleRegionRect(region, x, y, w, h, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%.2f, %.2f, %.2f, %.2f) to be visible\n", x, y, w, h);
    status = GdipIsVisibleRegionRectI(region, (INT)x, (INT)y, (INT)w, (INT)h, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%d, %d, %d, %d) to be visible\n", (INT)x, (INT)y, (INT)w, (INT)h);

    /* entirely outside of the region */
    x = 0; w = 5;
    y = 0; h = 5;
    status = GdipIsVisibleRegionRect(region, x, y, w, h, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%.2f, %.2f, %.2f, %.2f) not to be visible\n", x, y, w, h);
    status = GdipIsVisibleRegionRectI(region, (INT)x, (INT)y, (INT)w, (INT)h, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%d, %d, %d, %d) not to be visible\n", (INT)x, (INT)y, (INT)w, (INT)h);

    /* corner cases */
    x = 0; w = 10;
    y = 0; h = 20;
    status = GdipIsVisibleRegionRect(region, x, y, w, h, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%.2f, %.2f, %.2f, %.2f) not to be visible\n", x, y, w, h);

    x = 0; w = 10.25;
    y = 0; h = 20.25;
    status = GdipIsVisibleRegionRect(region, x, y, w, h, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%.2f, %.2f, %.2f, %.2f) to be visible\n", x, y, w, h);

    x = 39; w = 10;
    y = 59; h = 10;
    status = GdipIsVisibleRegionRect(region, x, y, w, h, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%.2f, %.2f, %.2f, %.2f) to be visible\n", x, y, w, h);

    x = 39.25; w = 10;
    y = 59.25; h = 10;
    status = GdipIsVisibleRegionRect(region, x, y, w, h, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%.2f, %.2f, %.2f, %.2f) not to be visible\n", x, y, w, h);

    /* corners outside, but some intersection */
    x = 0; w = 100;
    y = 0; h = 100;
    status = GdipIsVisibleRegionRect(region, x, y, w, h, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%.2f, %.2f, %.2f, %.2f) to be visible\n", x, y, w, h);

    x = 0; w = 100;
    y = 0; h = 40;
    status = GdipIsVisibleRegionRect(region, x, y, w, h, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%.2f, %.2f, %.2f, %.2f) to be visible\n", x, y, w, h);

    x = 0; w = 25;
    y = 0; h = 100;
    status = GdipIsVisibleRegionRect(region, x, y, w, h, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%.2f, %.2f, %.2f, %.2f) to be visible\n", x, y, w, h);

    /* translate into the center of the rectangle */
    status = GdipTranslateWorldTransform(graphics, 25, 40, MatrixOrderAppend);
    expect(Ok, status);

    /* native ignores the world transform, so treat these as if
     * no transform exists */
    x = 0; w = 5;
    y = 0; h = 5;
    status = GdipIsVisibleRegionRect(region, x, y, w, h, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%.2f, %.2f, %.2f, %.2f) not to be visible\n", x, y, w, h);
    status = GdipIsVisibleRegionRectI(region, (INT)x, (INT)y, (INT)w, (INT)h, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%d, %d, %d, %d) not to be visible\n", (INT)x, (INT)y, (INT)w, (INT)h);

    x = 11; w = 10;
    y = 12; h = 10;
    status = GdipIsVisibleRegionRect(region, x, y, w, h, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%.2f, %.2f, %.2f, %.2f) to be visible\n", x, y, w, h);
    status = GdipIsVisibleRegionRectI(region, (INT)x, (INT)y, (INT)w, (INT)h, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%d, %d, %d, %d) to be visible\n", (INT)x, (INT)y, (INT)w, (INT)h);

    /* translate back to origin */
    status = GdipTranslateWorldTransform(graphics, -25, -40, MatrixOrderAppend);
    expect(Ok, status);

    /* region from path */
    status = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, status);

    status = GdipAddPathEllipse(path, 10, 20, 30, 40);
    expect(Ok, status);

    status = GdipCombineRegionPath(region, path, CombineModeReplace);
    expect(Ok, status);

    x = 0; w = 12;
    y = 0; h = 22;
    status = GdipIsVisibleRegionRect(region, x, y, w, h, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%.2f, %.2f, %.2f, %.2f) not to be visible\n", x, y, w, h);
    status = GdipIsVisibleRegionRectI(region, (INT)x, (INT)y, (INT)w, (INT)h, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%d, %d, %d, %d) not to be visible\n", (INT)x, (INT)y, (INT)w, (INT)h);

    x = 0; w = 25;
    y = 0; h = 40;
    status = GdipIsVisibleRegionRect(region, x, y, w, h, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%.2f, %.2f, %.2f, %.2f) to be visible\n", x, y, w, h);
    status = GdipIsVisibleRegionRectI(region, (INT)x, (INT)y, (INT)w, (INT)h, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%d, %d, %d, %d) to be visible\n", (INT)x, (INT)y, (INT)w, (INT)h);

    x = 38; w = 10;
    y = 55; h = 10;
    status = GdipIsVisibleRegionRect(region, x, y, w, h, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%.2f, %.2f, %.2f, %.2f) not to be visible\n", x, y, w, h);
    status = GdipIsVisibleRegionRectI(region, (INT)x, (INT)y, (INT)w, (INT)h, graphics, &res);
    expect(Ok, status);
    ok(res == FALSE, "Expected (%d, %d, %d, %d) not to be visible\n", (INT)x, (INT)y, (INT)w, (INT)h);

    x = 0; w = 100;
    y = 0; h = 100;
    status = GdipIsVisibleRegionRect(region, x, y, w, h, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%.2f, %.2f, %.2f, %.2f) to be visible\n", x, y, w, h);
    status = GdipIsVisibleRegionRectI(region, (INT)x, (INT)y, (INT)w, (INT)h, graphics, &res);
    expect(Ok, status);
    ok(res == TRUE, "Expected (%d, %d, %d, %d) to be visible\n", (INT)x, (INT)y, (INT)w, (INT)h);

    GdipDeletePath(path);

    GdipDeleteRegion(region);
    GdipDeleteGraphics(graphics);
    ReleaseDC(0, hdc);
}

static void test_excludeinfinite(void)
{
    GpStatus status;
    GpRegion *region;
    UINT count=0xdeadbeef;
    GpRectF scans[4];
    GpMatrix *identity;
    static const RectF rect_exclude = {0.0, 0.0, 1.0, 1.0};

    status = GdipCreateMatrix(&identity);
    expect(Ok, status);

    status = GdipCreateRegion(&region);
    expect(Ok, status);

    status = GdipCombineRegionRect(region, &rect_exclude, CombineModeExclude);
    expect(Ok, status);

    status = GdipGetRegionScansCount(region, &count, identity);
    expect(Ok, status);
    expect(4, count);

    count = 4;
    status = GdipGetRegionScans(region, scans, (INT*)&count, identity);
    expect(Ok, status);

    expectf(-4194304.0, scans[0].X);
    expectf(-4194304.0, scans[0].Y);
    expectf(8388608.0, scans[0].Width);
    expectf(4194304.0, scans[0].Height);

    expectf(-4194304.0, scans[1].X);
    expectf(0.0, scans[1].Y);
    expectf(4194304.0, scans[1].Width);
    expectf(1.0, scans[1].Height);

    expectf(1.0, scans[2].X);
    expectf(0.0, scans[2].Y);
    expectf(4194303.0, scans[2].Width);
    expectf(1.0, scans[2].Height);

    expectf(-4194304.0, scans[3].X);
    expectf(1.0, scans[3].Y);
    expectf(8388608.0, scans[3].Width);
    expectf(4194303.0, scans[3].Height);

    GdipDeleteRegion(region);
    GdipDeleteMatrix(identity);
}

static void test_GdipCreateRegionRgnData(void)
{
    GpGraphics *graphics = NULL;
    GpRegion *region, *region2;
    HDC hdc = GetDC(0);
    GpStatus status;
    BYTE buf[512];
    UINT needed;
    BOOL ret;

    status = GdipCreateRegionRgnData(NULL, 0, NULL);
    ok(status == InvalidParameter, "status %d\n", status);

    status = GdipCreateFromHDC(hdc, &graphics);
    ok(status == Ok, "status %d\n", status);

    status = GdipCreateRegion(&region);
    ok(status == Ok, "status %d\n", status);

    /* infinite region */
    memset(buf, 0xee, sizeof(buf));
    needed = 0;
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %d\n", status);
    expect(20, needed);

    status = GdipCreateRegionRgnData(buf, needed, NULL);
    ok(status == InvalidParameter, "status %d\n", status);

    status = GdipCreateRegionRgnData(buf, needed, &region2);
    ok(status == Ok, "status %d\n", status);

    ret = FALSE;
    status = GdipIsInfiniteRegion(region2, graphics, &ret);
    ok(status == Ok, "status %d\n", status);
    ok(ret, "got %d\n", ret);
    GdipDeleteRegion(region2);

    /* empty region */
    status = GdipSetEmpty(region);
    ok(status == Ok, "status %d\n", status);

    memset(buf, 0xee, sizeof(buf));
    needed = 0;
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %d\n", status);
    expect(20, needed);

    status = GdipCreateRegionRgnData(buf, needed, &region2);
    ok(status == Ok, "status %d\n", status);

    ret = FALSE;
    status = GdipIsEmptyRegion(region2, graphics, &ret);
    ok(status == Ok, "status %d\n", status);
    ok(ret, "got %d\n", ret);
    GdipDeleteRegion(region2);

    GdipDeleteGraphics(graphics);
    GdipDeleteRegion(region);
    ReleaseDC(0, hdc);
}

static void test_incombinedregion(void)
{
    struct testrgn
    {
        const char* desc;
        BOOL origin_in_region;
        GpRegion *region;
    };

    struct testrgn test_regions[] = {
        { "infinite region", TRUE },
        { "infinite region inverted", FALSE },
        { "empty region", FALSE },
        { "empty region inverted", TRUE },
        { "inside rectangle", TRUE },
        { "inside rectangle inverted", FALSE },
        { "outside rectangle", FALSE },
        { "outside rectangle inverted", TRUE },
        { "inside path", TRUE },
        { "inside path inverted", FALSE },
        { "outside path but in bounding rect", FALSE },
        { "outside path but in bounding rect inverted", TRUE },
        { "outside path", FALSE },
        { "outside path inverted", TRUE },
    };

    GpStatus stat;
    GpRectF rect;
    const GpPointF inside_path_points[] = { { -1, -2 }, { 2, 1 }, { -1, 1 } };
    const GpPointF outside_path_bounding_points[] = { { -2, -1 }, { 1, 2 }, { -2, 2 } };
    const GpPointF outside_path_points[] = { { 5, 5 }, { 5, 10 }, { 10, 10 } };
    GpPath *path;
    int i, j;
    BOOL in_region;

    /* Prepare test regions: */

    /* infinite */
    stat = GdipCreateRegion(&test_regions[0].region);
    expect(Ok, stat);

    /* empty */
    stat = GdipCreateRegion(&test_regions[2].region);
    expect(Ok, stat);
    stat = GdipSetEmpty(test_regions[2].region);
    expect(Ok, stat);

    /* inside rectangle */
    rect.X = -5;
    rect.Y = -2;
    rect.Width = 10;
    rect.Height = 4;
    stat = GdipCreateRegionRect(&rect, &test_regions[4].region);
    expect(Ok, stat);

    /* outside rectangle */
    rect.X = -10;
    rect.Y = -10;
    rect.Width = 7;
    rect.Height = 7;
    stat = GdipCreateRegionRect(&rect, &test_regions[6].region);
    expect(Ok, stat);

    /* inside path */
    stat = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, stat);
    stat = GdipAddPathPolygon(path, inside_path_points, ARRAY_SIZE(inside_path_points));
    expect(Ok, stat);
    stat = GdipCreateRegion(&test_regions[8].region);
    expect(Ok, stat);
    stat = GdipCombineRegionPath(test_regions[8].region, path, CombineModeReplace);
    expect(Ok, stat);
    stat = GdipDeletePath(path);
    expect(Ok, stat);

    /* outside path but in bounding rect */
    stat = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, stat);
    stat = GdipAddPathPolygon(path, outside_path_bounding_points, ARRAY_SIZE(outside_path_bounding_points));
    expect(Ok, stat);
    stat = GdipCreateRegion(&test_regions[10].region);
    expect(Ok, stat);
    stat = GdipCombineRegionPath(test_regions[10].region, path, CombineModeReplace);
    expect(Ok, stat);
    stat = GdipDeletePath(path);
    expect(Ok, stat);

    /* outside path */
    stat = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, stat);
    stat = GdipAddPathPolygon(path, outside_path_points, ARRAY_SIZE(outside_path_points));
    expect(Ok, stat);
    stat = GdipCreateRegion(&test_regions[12].region);
    expect(Ok, stat);
    stat = GdipCombineRegionPath(test_regions[12].region, path, CombineModeReplace);
    expect(Ok, stat);
    stat = GdipDeletePath(path);
    expect(Ok, stat);

    for (i = 1; i < ARRAY_SIZE(test_regions); i += 2)
    {
        winetest_push_context("%s", test_regions[i].desc);
        stat = GdipCreateRegion(&test_regions[i].region);
        expect(Ok, stat);
        stat = GdipCombineRegionRegion(test_regions[i].region, test_regions[i-1].region, CombineModeExclude);
        expect(Ok, stat);
        winetest_pop_context();
    }

    /* Check regions individually */
    for (i = 0; i < ARRAY_SIZE(test_regions); i++)
    {
        winetest_push_context("%s", test_regions[i].desc);
        stat = GdipIsVisibleRegionPoint(test_regions[i].region, 0, 0, NULL, &in_region);
        expect(Ok, stat);
        expect(test_regions[i].origin_in_region, in_region);
        winetest_pop_context();
    }

    /* Check combined regions */
    for (i = 0; i < ARRAY_SIZE(test_regions); i++)
    {
        for (j = 0; j < ARRAY_SIZE(test_regions); j++)
        {
            GpRegion *region;
            BOOL expected_result;

            winetest_push_context("%s + %s", test_regions[i].desc, test_regions[j].desc);

            stat = GdipCreateRegion(&region);
            expect(Ok, stat);

            /* CombineModeIntersect */
            stat = GdipCombineRegionRegion(region, test_regions[i].region, CombineModeReplace);
            expect(Ok, stat);
            stat = GdipCombineRegionRegion(region, test_regions[j].region, CombineModeIntersect);
            expect(Ok, stat);

            expected_result = test_regions[i].origin_in_region & test_regions[j].origin_in_region;
            stat = GdipIsVisibleRegionPoint(region, 0, 0, NULL, &in_region);
            expect(Ok, stat);
            ok(expected_result == in_region, "CombineModeIntersect: expected %i, got %i\n", expected_result, in_region);

            /* CombineModeUnion */
            stat = GdipCombineRegionRegion(region, test_regions[i].region, CombineModeReplace);
            expect(Ok, stat);
            stat = GdipCombineRegionRegion(region, test_regions[j].region, CombineModeUnion);
            expect(Ok, stat);

            expected_result = test_regions[i].origin_in_region | test_regions[j].origin_in_region;
            stat = GdipIsVisibleRegionPoint(region, 0, 0, NULL, &in_region);
            expect(Ok, stat);
            ok(expected_result == in_region, "CombineModeUnion: expected %i, got %i\n", expected_result, in_region);

            /* CombineModeXor */
            stat = GdipCombineRegionRegion(region, test_regions[i].region, CombineModeReplace);
            expect(Ok, stat);
            stat = GdipCombineRegionRegion(region, test_regions[j].region, CombineModeXor);
            expect(Ok, stat);

            expected_result = test_regions[i].origin_in_region ^ test_regions[j].origin_in_region;
            stat = GdipIsVisibleRegionPoint(region, 0, 0, NULL, &in_region);
            expect(Ok, stat);
            ok(expected_result == in_region, "CombineModeXor: expected %i, got %i\n", expected_result, in_region);

            /* CombineModeExclude */
            stat = GdipCombineRegionRegion(region, test_regions[i].region, CombineModeReplace);
            expect(Ok, stat);
            stat = GdipCombineRegionRegion(region, test_regions[j].region, CombineModeExclude);
            expect(Ok, stat);

            expected_result = test_regions[i].origin_in_region & !test_regions[j].origin_in_region;
            stat = GdipIsVisibleRegionPoint(region, 0, 0, NULL, &in_region);
            expect(Ok, stat);
            ok(expected_result == in_region, "CombineModeExclude: expected %i, got %i\n", expected_result, in_region);

            /* CombineModeComplement */
            stat = GdipCombineRegionRegion(region, test_regions[i].region, CombineModeReplace);
            expect(Ok, stat);
            stat = GdipCombineRegionRegion(region, test_regions[j].region, CombineModeComplement);
            expect(Ok, stat);

            expected_result = (!test_regions[i].origin_in_region) & test_regions[j].origin_in_region;
            stat = GdipIsVisibleRegionPoint(region, 0, 0, NULL, &in_region);
            expect(Ok, stat);
            ok(expected_result == in_region, "CombineModeComplement: expected %i, got %i\n", expected_result, in_region);

            winetest_pop_context();
        }
    }

    for (i = 0; i < ARRAY_SIZE(test_regions); i++)
    {
        stat = GdipDeleteRegion(test_regions[i].region);
        expect(Ok, stat);
    }
}

START_TEST(region)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    HMODULE hmsvcrt;
    int (CDECL * _controlfp_s)(unsigned int *cur, unsigned int newval, unsigned int mask);

    /* Enable all FP exceptions except _EM_INEXACT, which gdi32 can trigger */
    hmsvcrt = LoadLibraryA("msvcrt");
    _controlfp_s = (void*)GetProcAddress(hmsvcrt, "_controlfp_s");
    if (_controlfp_s) _controlfp_s(0, 0, 0x0008001e);

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
    test_transform();
    test_scans();
    test_getbounds();
    test_isvisiblepoint();
    test_isvisiblerect();
    test_excludeinfinite();
    test_GdipCreateRegionRgnData();
    test_incombinedregion();

    GdiplusShutdown(gdiplusToken);
}
