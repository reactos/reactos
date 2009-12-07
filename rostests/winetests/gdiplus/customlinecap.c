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

#include "windows.h"
#include "gdiplus.h"
#include "wine/test.h"

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)
#define expectf(expected, got) ok(got == expected, "Expected %.2f, got %.2f\n", expected, got)

static void test_constructor_destructor(void)
{
    GpCustomLineCap *custom;
    GpPath *path, *path2;
    GpStatus stat;

    stat = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, stat);
    stat = GdipAddPathRectangle(path, 5.0, 5.0, 10.0, 10.0);
    expect(Ok, stat);

    stat = GdipCreatePath(FillModeAlternate, &path2);
    expect(Ok, stat);
    stat = GdipAddPathRectangle(path2, 5.0, 5.0, 10.0, 10.0);
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

    /* valid args */
    stat = GdipCreateCustomLineCap(NULL, path2, LineCapFlat, 0.0, &custom);
    expect(Ok, stat);
    stat = GdipDeleteCustomLineCap(custom);
    expect(Ok, stat);
    /* it's strange but native returns NotImplemented on stroke == NULL */
    stat = GdipCreateCustomLineCap(path, NULL, LineCapFlat, 10.0, &custom);
    todo_wine expect(NotImplemented, stat);

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
    /* valid args */
    scale = (REAL)0xdeadbeef;
    stat = GdipGetCustomLineCapWidthScale(custom, &scale);
    expect(Ok, stat);
    expectf(1.0, scale);

    GdipDeleteCustomLineCap(custom);
    GdipDeletePath(path);
}

START_TEST(customlinecap)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_constructor_destructor();
    test_linejoin();
    test_inset();
    test_scale();

    GdiplusShutdown(gdiplusToken);
}
