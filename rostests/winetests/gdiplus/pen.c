/*
 * Unit test suite for pens (and init)
 *
 * Copyright (C) 2007 Google (Evan Stade)
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

#include "windows.h"
#include "gdiplus.h"
#include "wine/test.h"

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)
#define expectf(expected, got) ok(fabs(got - expected) < 0.1, "Expected %.2f, got %.2f\n", expected, got)

static void test_startup(void)
{
    GpPen *pen = NULL;
    Status status;
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    status = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    expect(Ok, status);
    GdiplusShutdown(gdiplusToken);

    gdiplusStartupInput.GdiplusVersion = 2;

    status = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    expect(UnsupportedGdiplusVersion, status);
    GdiplusShutdown(gdiplusToken);

    status = GdipCreatePen1((ARGB)0xffff00ff, 10.0f, UnitPixel, &pen);

    todo_wine
        expect(GdiplusNotInitialized, status);

    GdipDeletePen(pen);
}

static void test_constructor_destructor(void)
{
    GpStatus status;
    GpPen *pen = NULL;

    status = GdipCreatePen1((ARGB)0xffff00ff, 10.0f, UnitPixel, NULL);
    expect(InvalidParameter, status);
    ok(pen == NULL, "Expected pen to be NULL\n");

    status = GdipCreatePen1((ARGB)0xffff00ff, 10.0f, UnitPixel, &pen);
    expect(Ok, status);
    ok(pen != NULL, "Expected pen to be initialized\n");

    status = GdipDeletePen(NULL);
    expect(InvalidParameter, status);

    status = GdipDeletePen(pen);
    expect(Ok, status);
}

static void test_constructor_destructor2(void)
{
    GpStatus status;
    GpPen *pen = NULL;
    GpBrush *brush = NULL;
    GpPointF points[2];

    status = GdipCreatePen2(NULL, 10.0f, UnitPixel, &pen);
    expect(InvalidParameter, status);
    ok(pen == NULL, "Expected pen to be NULL\n");

    points[0].X = 7.0;
    points[0].Y = 11.0;
    points[1].X = 13.0;
    points[1].Y = 17.0;

    status = GdipCreateLineBrush(&points[0], &points[1], (ARGB)0xffff00ff,
                    (ARGB)0xff0000ff, WrapModeTile, (GpLineGradient **)&brush);
    expect(Ok, status);
    ok(brush != NULL, "Expected brush to be initialized\n");

    status = GdipCreatePen2(brush, 10.0f, UnitPixel, &pen);
    expect(Ok, status);
    ok(pen != NULL, "Expected pen to be initialized\n");

    status = GdipDeletePen(pen);
    expect(Ok, status);

    status = GdipDeleteBrush(brush);
    expect(Ok, status);
}

static void test_brushfill(void)
{
    GpStatus status;
    GpPen *pen;
    GpBrush *brush, *brush2;
    GpBrushType type;
    ARGB color = 0;

    /* default solid */
    GdipCreatePen1(0xdeadbeef, 4.5, UnitWorld, &pen);
    status = GdipGetPenBrushFill(pen, &brush);
    expect(Ok, status);
    GdipGetBrushType(brush, &type);
    expect(BrushTypeSolidColor, type);
    GdipGetPenColor(pen, &color);
    expect(0xdeadbeef, color);
    GdipDeleteBrush(brush);

    /* color controlled by brush */
    GdipCreateSolidFill(0xabaddeed, (GpSolidFill**)&brush);
    status = GdipSetPenBrushFill(pen, brush);
    expect(Ok, status);
    GdipGetPenColor(pen, &color);
    expect(0xabaddeed, color);
    GdipDeleteBrush(brush);
    color = 0;

    /* get returns a clone, not a reference */
    GdipGetPenBrushFill(pen, &brush);
    GdipSetSolidFillColor((GpSolidFill*)brush, 0xbeadfeed);
    GdipGetPenBrushFill(pen, &brush2);
    ok(brush != brush2, "Expected to get a clone, not a copy of the reference\n");
    GdipGetSolidFillColor((GpSolidFill*)brush2, &color);
    expect(0xabaddeed, color);
    GdipDeleteBrush(brush);
    GdipDeleteBrush(brush2);

    /* brush cannot be NULL */
    status = GdipSetPenBrushFill(pen, NULL);
    expect(InvalidParameter, status);

    GdipDeletePen(pen);
}

static void test_dasharray(void)
{
    GpPen *pen;
    GpDashStyle style;
    GpStatus status;
    REAL dashes[12];

    GdipCreatePen1(0xdeadbeef, 10.0, UnitWorld, &pen);
    dashes[0] = 10.0;
    dashes[1] = 11.0;
    dashes[2] = 12.0;
    dashes[3] = 13.0;
    dashes[4] = 14.0;
    dashes[5] = -100.0;
    dashes[6] = -100.0;
    dashes[7] = dashes[8] = dashes[9] = dashes[10] = dashes[11] = 0.0;

    /* setting the array sets the type to custom */
    GdipGetPenDashStyle(pen, &style);
    expect(DashStyleSolid, style);
    status = GdipSetPenDashArray(pen, dashes, 2);
    expect(Ok, status);
    GdipGetPenDashStyle(pen, &style);
    expect(DashStyleCustom, style);

    /* Getting the array on a non-custom pen returns invalid parameter (unless
     * you are getting 0 elements).*/
    GdipSetPenDashStyle(pen, DashStyleSolid);
    status = GdipGetPenDashArray(pen, &dashes[5], 2);
    expect(InvalidParameter, status);
    status = GdipGetPenDashArray(pen, &dashes[5], 0);
    expect(Ok, status);

    /* What does setting DashStyleCustom do to the array length? */
    GdipSetPenDashArray(pen, dashes, 2);
    GdipSetPenDashStyle(pen, DashStyleCustom);
    status = GdipGetPenDashArray(pen, &dashes[5], 2);
    expect(Ok, status);
    expectf(10.0, dashes[5]);
    expectf(11.0, dashes[6]);

    /* Set the array, then get with different sized buffers. */
    status = GdipSetPenDashArray(pen, dashes, 5);
    expect(Ok, status);
    dashes[5] = -100.0;
    dashes[6] = -100.0;
    status = GdipGetPenDashArray(pen, &dashes[5], 1);
    expect(Ok, status); /* not InsufficientBuffer! */
    expectf(10.0, dashes[5]);
    expectf(-100.0, dashes[6]);
    dashes[5] = -100.0;
    status = GdipGetPenDashArray(pen, &dashes[5], 6);
    expect(InvalidParameter, status); /* not Ok! */
    expectf(-100.0, dashes[5]);
    expectf(-100.0, dashes[6]);

    /* Some invalid array values. */
    status = GdipSetPenDashArray(pen, &dashes[7], 5);
    expect(InvalidParameter, status);
    dashes[9] = -1.0;
    status = GdipSetPenDashArray(pen, &dashes[7], 5);
    expect(InvalidParameter, status);

    /* Try to set with count = 0. */
    GdipSetPenDashStyle(pen, DashStyleDot);
    status = GdipSetPenDashArray(pen, dashes, 0);
    expect(OutOfMemory, status);
    GdipGetPenDashStyle(pen, &style);
    expect(DashStyleDot, style);

    GdipDeletePen(pen);
}

static void test_customcap(void)
{
    GpPen *pen;
    GpStatus status;
    GpCustomLineCap *custom;

    status = GdipCreatePen1((ARGB)0xffff00ff, 10.0f, UnitPixel, &pen);
    expect(Ok, status);

    /* NULL args */
    status = GdipGetPenCustomStartCap(NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipGetPenCustomStartCap(pen, NULL);
    expect(InvalidParameter, status);
    status = GdipGetPenCustomStartCap(NULL, &custom);
    expect(InvalidParameter, status);

    status = GdipGetPenCustomEndCap(NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipGetPenCustomEndCap(pen, NULL);
    expect(InvalidParameter, status);
    status = GdipGetPenCustomEndCap(NULL, &custom);
    expect(InvalidParameter, status);

    /* native crashes on pen == NULL, custom != NULL */
    status = GdipSetPenCustomStartCap(NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipSetPenCustomStartCap(pen, NULL);
    expect(InvalidParameter, status);

    status = GdipSetPenCustomEndCap(NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipSetPenCustomEndCap(pen, NULL);
    expect(InvalidParameter, status);

    /* get without setting previously */
    custom = (GpCustomLineCap*)0xdeadbeef;
    status = GdipGetPenCustomEndCap(pen, &custom);
    expect(Ok, status);
    ok(custom == NULL,"Expect CustomCap == NULL\n");

    custom = (GpCustomLineCap*)0xdeadbeef;
    status = GdipGetPenCustomStartCap(pen, &custom);
    expect(Ok, status);
    ok(custom == NULL,"Expect CustomCap == NULL\n");

    GdipDeletePen(pen);
}

static void test_penfilltype(void)
{
    GpPen *pen;
    GpSolidFill *solid;
    GpLineGradient *line;
    GpPointF a, b;
    GpStatus status;
    GpPenType type;

    /* NULL */
    status = GdipGetPenFillType(NULL, NULL);
    expect(InvalidParameter, status);

    status = GdipCreatePen1((ARGB)0xffff00ff, 10.0f, UnitPixel, &pen);
    expect(Ok, status);
    status = GdipGetPenFillType(pen, NULL);
    expect(InvalidParameter, status);

    /* created with GdipCreatePen1() */
    status = GdipGetPenFillType(pen, &type);
    expect(Ok, status);
    expect(PenTypeSolidColor, type);
    GdipDeletePen(pen);

    /* based on SolidBrush */
    status = GdipCreateSolidFill((ARGB)0xffff00ff, &solid);
    expect(Ok, status);
    status = GdipCreatePen2((GpBrush*)solid, 10.0f, UnitPixel, &pen);
    expect(Ok, status);
    status = GdipGetPenFillType(pen, &type);
    expect(Ok, status);
    expect(PenTypeSolidColor, type);
    GdipDeletePen(pen);
    GdipDeleteBrush((GpBrush*)solid);

    /* based on LinearGradientBrush */
    a.X = a.Y = 0.0;
    b.X = b.Y = 10.0;
    status = GdipCreateLineBrush(&a, &b, (ARGB)0xffff00ff, (ARGB)0xffff0000,
                                 WrapModeTile, &line);
    expect(Ok, status);
    status = GdipCreatePen2((GpBrush*)line, 10.0f, UnitPixel, &pen);
    expect(Ok, status);
    status = GdipGetPenFillType(pen, &type);
    expect(Ok, status);
    expect(PenTypeLinearGradient, type);
    GdipDeletePen(pen);
    GdipDeleteBrush((GpBrush*)line);
}

static void test_compoundarray(void)
{
    GpStatus status;
    GpPen *pen;
    static const REAL testvalues[] = {0.2, 0.4, 0.6, 0.8};

    status = GdipSetPenCompoundArray(NULL, testvalues, 4);
    expect(InvalidParameter, status);

    status = GdipCreatePen1((ARGB)0xffff00ff, 10.0f, UnitPixel, &pen);
    expect(Ok, status);

    status = GdipSetPenCompoundArray(pen, NULL, 4);
    expect(InvalidParameter, status);
    status = GdipSetPenCompoundArray(pen, testvalues, 3);
    expect(InvalidParameter, status);
    status = GdipSetPenCompoundArray(pen, testvalues, 0);
    expect(InvalidParameter, status);
    status = GdipSetPenCompoundArray(pen, testvalues, -2);
    expect(InvalidParameter, status);

    status = GdipSetPenCompoundArray(pen, testvalues, 4);
    todo_wine expect(Ok, status);

    GdipDeletePen(pen);
}

START_TEST(pen)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    test_startup();

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_constructor_destructor();
    test_constructor_destructor2();
    test_brushfill();
    test_dasharray();
    test_customcap();
    test_penfilltype();
    test_compoundarray();

    GdiplusShutdown(gdiplusToken);
}
