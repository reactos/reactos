/*
 * Unit test suite for customlinecap
 *
 * Copyright (C) 2008 Nikolay Sivov
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
#include <limits.h>

#include "objbase.h"
#include "gdiplus.h"
#include "wine/test.h"

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)
#define expectf(expected, got) ok(got == expected, "Expected %.2f, got %.2f\n", expected, got)

static BOOL compare_uint(unsigned int x, unsigned int y, unsigned int max_diff)
{
    unsigned int diff = x > y ? x - y : y - x;

    return diff <= max_diff;
}

static BOOL compare_float(float f, float g, unsigned int ulps)
{
    int x = *(int *)&f;
    int y = *(int *)&g;

    if (x < 0)
        x = INT_MIN - x;
    if (y < 0)
        y = INT_MIN - y;

    return compare_uint(x, y, ulps);
}

static void test_constructor_destructor(void)
{
    GpCustomLineCap *custom;
    GpPath *path, *path2, *pathFarAway;
    GpStatus stat;

    stat = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, stat);
    stat = GdipAddPathRectangle(path, -5.0, -4.0, 10.0, 8.0);
    expect(Ok, stat);

    stat = GdipCreatePath(FillModeAlternate, &path2);
    expect(Ok, stat);
    stat = GdipAddPathRectangle(path2, -5.0, -5.0, 10.0, 10.0);
    expect(Ok, stat);

    stat = GdipCreatePath(FillModeAlternate, &pathFarAway);
    expect(Ok, stat);
    stat = GdipAddPathRectangle(pathFarAway, 5.0, 5.0, 10.0, 10.0);
    expect(Ok, stat);

    /* NULL args */
    stat = GdipCreateCustomLineCap(NULL, NULL, LineCapFlat, 0.0, NULL);
    expect(InvalidParameter, stat);
    stat = GdipCreateCustomLineCap(path, NULL, LineCapFlat, 0.0, NULL);
    expect(InvalidParameter, stat);
    stat = GdipCreateCustomLineCap(NULL, path, LineCapFlat, 0.0, NULL);
    expect(InvalidParameter, stat);
    stat = GdipCreateCustomLineCap(NULL, NULL, LineCapFlat, 0.0, &custom);
    expect(InvalidParameter, stat);
    stat = GdipDeleteCustomLineCap(NULL);
    expect(InvalidParameter, stat);

    /* If both parameters are provided, then fillPath will be ignored. */
    custom = NULL;
    stat = GdipCreateCustomLineCap(path, path2, LineCapFlat, 0.0, &custom);
    expect(Ok, stat);
    ok(custom != NULL, "Custom line cap was not created\n");
    stat = GdipDeleteCustomLineCap(custom);
    expect(Ok, stat);

    /* valid args */
    custom = NULL;
    stat = GdipCreateCustomLineCap(NULL, path2, LineCapFlat, 0.0, &custom);
    expect(Ok, stat);
    ok(custom != NULL, "Custom line cap was not created\n");
    stat = GdipDeleteCustomLineCap(custom);
    expect(Ok, stat);

    custom = NULL;
    stat = GdipCreateCustomLineCap(path, NULL, LineCapFlat, 0.0, &custom);
    expect(Ok, stat);
    ok(custom != NULL, "Custom line cap was not created\n");
    stat = GdipDeleteCustomLineCap(custom);
    expect(Ok, stat);

    /* Custom line cap position (0, 0) is a place corresponding to the end of line.
    *  If Custom Line Cap is too big and too far from position (0, 0),
    *  then NotImplemented will be returned, due to floating point precision limitation. */
    custom = NULL;
    stat = GdipCreateCustomLineCap(pathFarAway, NULL, LineCapFlat, 10.0, &custom);
    todo_wine expect(NotImplemented, stat);
    todo_wine ok(custom == NULL, "Expected a failure on creation\n");
    if(stat == Ok) GdipDeleteCustomLineCap(custom);

    GdipDeletePath(pathFarAway);
    GdipDeletePath(path2);
    GdipDeletePath(path);
}

static void test_linejoin(void)
{
    GpCustomLineCap *custom;
    GpPath *path;
    GpLineJoin join;
    GpStatus stat;

    stat = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, stat);
    stat = GdipAddPathRectangle(path, 5.0, 5.0, 10.0, 10.0);
    expect(Ok, stat);

    stat = GdipCreateCustomLineCap(NULL, path, LineCapFlat, 0.0, &custom);
    expect(Ok, stat);

    /* NULL args */
    stat = GdipGetCustomLineCapStrokeJoin(NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipGetCustomLineCapStrokeJoin(custom, NULL);
    expect(InvalidParameter, stat);
    stat = GdipGetCustomLineCapStrokeJoin(NULL, &join);
    expect(InvalidParameter, stat);
    stat = GdipSetCustomLineCapStrokeJoin(NULL, LineJoinBevel);
    expect(InvalidParameter, stat);

    /* LineJoinMiter is default */
    stat = GdipGetCustomLineCapStrokeJoin(custom, &join);
    expect(Ok, stat);
    expect(LineJoinMiter, join);

    /* set/get */
    stat = GdipSetCustomLineCapStrokeJoin(custom, LineJoinBevel);
    expect(Ok, stat);
    stat = GdipGetCustomLineCapStrokeJoin(custom, &join);
    expect(Ok, stat);
    expect(LineJoinBevel, join);
    stat = GdipSetCustomLineCapStrokeJoin(custom, LineJoinRound);
    expect(Ok, stat);
    stat = GdipGetCustomLineCapStrokeJoin(custom, &join);
    expect(Ok, stat);
    expect(LineJoinRound, join);
    stat = GdipSetCustomLineCapStrokeJoin(custom, LineJoinMiterClipped);
    expect(Ok, stat);
    stat = GdipGetCustomLineCapStrokeJoin(custom, &join);
    expect(Ok, stat);
    expect(LineJoinMiterClipped, join);

    GdipDeleteCustomLineCap(custom);
    GdipDeletePath(path);
}

static void test_inset(void)
{
    GpCustomLineCap *custom;
    GpPath *path;
    REAL inset;
    GpStatus stat;

    stat = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, stat);
    stat = GdipAddPathRectangle(path, 5.0, 5.0, 10.0, 10.0);
    expect(Ok, stat);

    stat = GdipCreateCustomLineCap(NULL, path, LineCapFlat, 0.0, &custom);
    expect(Ok, stat);

    /* NULL args */
    stat = GdipGetCustomLineCapBaseInset(NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipGetCustomLineCapBaseInset(NULL, &inset);
    expect(InvalidParameter, stat);
    stat = GdipGetCustomLineCapBaseInset(custom, NULL);
    expect(InvalidParameter, stat);
    /* valid args */
    inset = (REAL)0xdeadbeef;
    stat = GdipGetCustomLineCapBaseInset(custom, &inset);
    expect(Ok, stat);
    expectf(0.0, inset);

    stat = GdipSetCustomLineCapBaseInset(custom, 2.0);
    expect(Ok, stat);

    inset = (REAL)0xdeadbeef;
    stat = GdipGetCustomLineCapBaseInset(custom, &inset);
    expect(Ok, stat);
    ok(inset == 2.0, "Unexpected inset value %f\n", inset);

    GdipDeleteCustomLineCap(custom);
    GdipDeletePath(path);
}

static void test_scale(void)
{
    GpCustomLineCap *custom;
    GpPath *path;
    REAL scale;
    GpStatus stat;

    stat = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, stat);
    stat = GdipAddPathRectangle(path, 5.0, 5.0, 10.0, 10.0);
    expect(Ok, stat);

    stat = GdipCreateCustomLineCap(NULL, path, LineCapFlat, 0.0, &custom);
    expect(Ok, stat);

    /* NULL args */
    stat = GdipGetCustomLineCapWidthScale(NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipGetCustomLineCapWidthScale(NULL, &scale);
    expect(InvalidParameter, stat);
    stat = GdipGetCustomLineCapWidthScale(custom, NULL);
    expect(InvalidParameter, stat);

    stat = GdipSetCustomLineCapWidthScale(NULL, 2.0);
    expect(InvalidParameter, stat);

    /* valid args: read default */
    scale = (REAL)0xdeadbeef;
    stat = GdipGetCustomLineCapWidthScale(custom, &scale);
    expect(Ok, stat);
    expectf(1.0, scale);

    /* set and read back some scale values: there is no limit for the scale */
    stat = GdipSetCustomLineCapWidthScale(custom, 2.5);
    expect(Ok, stat);
    scale = (REAL)0xdeadbeef;
    stat = GdipGetCustomLineCapWidthScale(custom, &scale);
    expect(Ok, stat);
    expectf(2.5, scale);

    stat = GdipSetCustomLineCapWidthScale(custom, 42.0);
    expect(Ok, stat);
    scale = (REAL)0xdeadbeef;
    stat = GdipGetCustomLineCapWidthScale(custom, &scale);
    expect(Ok, stat);
    expectf(42.0, scale);

    stat = GdipSetCustomLineCapWidthScale(custom, 3000.0);
    expect(Ok, stat);
    scale = (REAL)0xdeadbeef;
    stat = GdipGetCustomLineCapWidthScale(custom, &scale);
    expect(Ok, stat);
    expectf(3000.0, scale);

    stat = GdipSetCustomLineCapWidthScale(custom, 0.0);
    expect(Ok, stat);
    scale = (REAL)0xdeadbeef;
    stat = GdipGetCustomLineCapWidthScale(custom, &scale);
    expect(Ok, stat);
    expectf(0.0, scale);

    GdipDeleteCustomLineCap(custom);
    GdipDeletePath(path);
}

static void test_create_adjustable_cap(void)
{
    REAL inset, scale, height, width;
    GpAdjustableArrowCap *cap;
    GpLineJoin join;
    GpStatus stat;
    GpLineCap base;
    BOOL ret;

    stat = GdipCreateAdjustableArrowCap(10.0, 10.0, TRUE, NULL);
    ok(stat == InvalidParameter, "Unexpected return code, %d\n", stat);

    stat = GdipCreateAdjustableArrowCap(17.0, 15.0, TRUE, &cap);
    ok(stat == Ok, "Failed to create adjustable cap, %d\n", stat);

    stat = GdipGetAdjustableArrowCapFillState(cap, NULL);
    ok(stat == InvalidParameter, "Unexpected return code, %d\n", stat);

    ret = FALSE;
    stat = GdipGetAdjustableArrowCapFillState(cap, &ret);
    ok(stat == Ok, "Unexpected return code, %d\n", stat);
    ok(ret, "Unexpected fill state %d\n", ret);

    stat = GdipGetAdjustableArrowCapHeight(cap, NULL);
    ok(stat == InvalidParameter, "Unexpected return code, %d\n", stat);

    stat = GdipGetAdjustableArrowCapHeight(cap, &height);
    ok(stat == Ok, "Unexpected return code, %d\n", stat);
    ok(height == 17.0, "Unexpected cap height %f\n", height);

    stat = GdipGetAdjustableArrowCapWidth(cap, NULL);
    ok(stat == InvalidParameter, "Unexpected return code, %d\n", stat);

    stat = GdipGetAdjustableArrowCapWidth(cap, &width);
    ok(stat == Ok, "Unexpected return code, %d\n", stat);
    ok(width == 15.0, "Unexpected cap width %f\n", width);

    stat = GdipGetAdjustableArrowCapMiddleInset(cap, NULL);
    ok(stat == InvalidParameter, "Unexpected return code, %d\n", stat);

    stat = GdipGetAdjustableArrowCapMiddleInset(cap, &inset);
    ok(stat == Ok, "Unexpected return code, %d\n", stat);
    ok(inset == 0.0f, "Unexpected middle inset %f\n", inset);

    stat = GdipGetCustomLineCapBaseCap((GpCustomLineCap*)cap, &base);
    ok(stat == Ok, "Unexpected return code, %d\n", stat);
    ok(base == LineCapTriangle, "Unexpected base cap %d\n", base);

    stat = GdipSetCustomLineCapBaseCap((GpCustomLineCap*)cap, LineCapSquare);
    ok(stat == Ok, "Unexpected return code, %d\n", stat);

    stat = GdipGetCustomLineCapBaseCap((GpCustomLineCap*)cap, &base);
    ok(stat == Ok, "Unexpected return code, %d\n", stat);
    ok(base == LineCapSquare, "Unexpected base cap %d\n", base);

    stat = GdipSetCustomLineCapBaseCap((GpCustomLineCap*)cap, LineCapSquareAnchor);
    ok(stat == InvalidParameter, "Unexpected return code, %d\n", stat);

    /* Base inset */
    stat = GdipGetAdjustableArrowCapWidth(cap, &width);
    ok(stat == Ok, "Unexpected return code, %d\n", stat);

    stat = GdipGetAdjustableArrowCapHeight(cap, &height);
    ok(stat == Ok, "Unexpected return code, %d\n", stat);

    inset = 0.0;
    stat = GdipGetCustomLineCapBaseInset((GpCustomLineCap*)cap, &inset);
    ok(stat == Ok, "Unexpected return code, %d\n", stat);
    ok(compare_float(inset, height / width, 1), "Unexpected inset %f\n", inset);

    stat = GdipSetAdjustableArrowCapMiddleInset(cap, 1.0);
    ok(stat == Ok, "Unexpected return code, %d\n", stat);

    inset = 0.0;
    stat = GdipGetCustomLineCapBaseInset((GpCustomLineCap*)cap, &inset);
    ok(stat == Ok, "Unexpected return code, %d\n", stat);
    ok(compare_float(inset, height / width, 1), "Unexpected inset %f\n", inset);

    stat = GdipSetAdjustableArrowCapHeight(cap, 2.0 * height);
    ok(stat == Ok, "Unexpected return code, %d\n", stat);

    inset = 0.0;
    stat = GdipGetCustomLineCapBaseInset((GpCustomLineCap*)cap, &inset);
    ok(stat == Ok, "Unexpected return code, %d\n", stat);
    ok(compare_float(inset, 2.0 * height / width, 1), "Unexpected inset %f\n", inset);

    stat = GdipGetCustomLineCapWidthScale((GpCustomLineCap*)cap, &scale);
    ok(stat == Ok, "Unexpected return code, %d\n", stat);
    ok(scale == 1.0f, "Unexpected width scale %f\n", scale);

    stat = GdipGetCustomLineCapStrokeJoin((GpCustomLineCap*)cap, &join);
    ok(stat == Ok, "Unexpected return code, %d\n", stat);
    ok(join == LineJoinMiter, "Unexpected stroke join %d\n", join);

    GdipDeleteCustomLineCap((GpCustomLineCap*)cap);
}

static void test_captype(void)
{
    GpAdjustableArrowCap *arrowcap;
    GpCustomLineCap *custom;
    CustomLineCapType type;
    GpStatus stat;
    GpPath *path;

    stat = GdipGetCustomLineCapType(NULL, NULL);
    ok(stat == InvalidParameter, "Unexpected return code, %d\n", stat);

    type = 10;
    stat = GdipGetCustomLineCapType(NULL, &type);
    ok(stat == InvalidParameter, "Unexpected return code, %d\n", stat);
    ok(type == 10, "Unexpected cap type, %d\n", type);

    /* default cap */
    stat = GdipCreatePath(FillModeAlternate, &path);
    ok(stat == Ok, "Failed to create path, %d\n", stat);
    stat = GdipAddPathRectangle(path, 5.0, 5.0, 10.0, 10.0);
    ok(stat == Ok, "AddPathRectangle failed, %d\n", stat);

    stat = GdipCreateCustomLineCap(NULL, path, LineCapFlat, 0.0, &custom);
    ok(stat == Ok, "Failed to create cap, %d\n", stat);
    stat = GdipGetCustomLineCapType(custom, &type);
    ok(stat == Ok, "Failed to get cap type, %d\n", stat);
    ok(type == CustomLineCapTypeDefault, "Unexpected cap type %d\n", stat);
    GdipDeleteCustomLineCap(custom);
    GdipDeletePath(path);

    /* arrow cap */
    stat = GdipCreateAdjustableArrowCap(17.0, 15.0, TRUE, &arrowcap);
    ok(stat == Ok, "Failed to create adjustable cap, %d\n", stat);

    stat = GdipGetCustomLineCapType((GpCustomLineCap*)arrowcap, &type);
    ok(stat == Ok, "Failed to get cap type, %d\n", stat);
    ok(type == CustomLineCapTypeAdjustableArrow, "Unexpected cap type %d\n", stat);

    GdipDeleteCustomLineCap((GpCustomLineCap*)arrowcap);
}

static void test_strokecap(void)
{
    GpCustomLineCap *cap;
    GpStatus stat;
    GpPath *path;

    /* default cap */
    stat = GdipCreatePath(FillModeAlternate, &path);
    ok(stat == Ok, "Failed to create path, %d\n", stat);
    stat = GdipAddPathRectangle(path, 5.0, 5.0, 10.0, 10.0);
    ok(stat == Ok, "AddPathRectangle failed, %d\n", stat);

    stat = GdipCreateCustomLineCap(NULL, path, LineCapFlat, 0.0, &cap);
    ok(stat == Ok, "Failed to create cap, %d\n", stat);

    stat = GdipSetCustomLineCapStrokeCaps(cap, LineCapSquare, LineCapFlat);
    ok(stat == Ok, "Unexpected return code, %d\n", stat);

    stat = GdipSetCustomLineCapStrokeCaps(cap, LineCapSquareAnchor, LineCapFlat);
    ok(stat == InvalidParameter, "Unexpected return code, %d\n", stat);

    stat = GdipSetCustomLineCapStrokeCaps(cap, LineCapFlat, LineCapSquareAnchor);
    ok(stat == InvalidParameter, "Unexpected return code, %d\n", stat);
    GdipDeleteCustomLineCap(cap);
    GdipDeletePath(path);
}

START_TEST(customlinecap)
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

    test_constructor_destructor();
    test_linejoin();
    test_inset();
    test_scale();
    test_create_adjustable_cap();
    test_captype();
    test_strokecap();

    GdiplusShutdown(gdiplusToken);
}
