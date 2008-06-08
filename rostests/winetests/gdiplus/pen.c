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

    GdiplusShutdown(gdiplusToken);
}
