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

#include "objbase.h"
#include "gdiplus.h"
#include "wine/test.h"

#define expect(expected,got) expect_(__LINE__, expected, got)
static inline void expect_(unsigned line, DWORD expected, DWORD got)
{
    ok_(__FILE__, line)(expected == got, "Expected %.8ld, got %.8ld\n", expected, got);
}
#define expectf(expected, got) ok(fabs(got - expected) < 0.1, "Expected %.2f, got %.2f\n", expected, got)

static void test_startup(void)
{
    GpPen *pen = NULL;
    Status status;
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    int gpversion;

    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    for (gpversion=1; gpversion<256; gpversion++)
    {
        gdiplusStartupInput.GdiplusVersion = gpversion;
        status = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        ok(status == Ok || status == UnsupportedGdiplusVersion,
            "GdiplusStartup returned %x\n", status);
        GdiplusShutdown(gdiplusToken);
        if (status != Ok)
        {
            gpversion--;
            break;
        }
    }

    ok(gpversion > 0 && gpversion <= 2, "unexpected gdiplus version %i\n", gpversion);
    trace("gdiplus version is %i\n", gpversion);

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
    dashes[7] = dashes[8] = dashes[9] = dashes[10] = dashes[11] = 1.0;

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
    expect(Ok, status);
    dashes[9] = -1.0;
    status = GdipSetPenDashArray(pen, &dashes[7], 5);
    expect(InvalidParameter, status);
    dashes[9] = 0.0;
    status = GdipSetPenDashArray(pen, &dashes[7], 5);
    expect(InvalidParameter, status);

    /* Try to set with count = 0. */
    GdipSetPenDashStyle(pen, DashStyleDot);
    if (0)  /* corrupts stack on 64-bit Vista */
    {
    status = GdipSetPenDashArray(pen, dashes, 0);
    ok(status == OutOfMemory || status == InvalidParameter,
       "Expected OutOfMemory or InvalidParameter, got %.8x\n", status);
    }
    status = GdipSetPenDashArray(pen, dashes, -1);
    ok(status == OutOfMemory || status == InvalidParameter,
       "Expected OutOfMemory or InvalidParameter, got %.8x\n", status);
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
    REAL *returnvalues;
    static const REAL testvalues[] = {0.2, 0.4, 0.6, 0.8};
    static const REAL notSortedValues[] = {0.2, 0.6, 0.4, 0.8};
    static const REAL negativeValues[] = {-1.2, 0.4, 0.6, 0.8};
    static const REAL tooLargeValues[] = {0.2, 0.4, 0.6, 2.8};
    INT count;

    status = GdipSetPenCompoundArray(NULL, testvalues, 4);
    expect(InvalidParameter, status);

    status = GdipCreatePen1((ARGB)0xffff00ff, 10.0f, UnitPixel, &pen);
    expect(Ok, status);

    status = GdipGetPenCompoundCount(NULL, NULL);
    expect(InvalidParameter, status);

    status = GdipGetPenCompoundCount(pen, NULL);
    expect(InvalidParameter, status);

    count = 10;
    status = GdipGetPenCompoundCount(pen, &count);
    expect(Ok, status);
    ok(count == 0, "Unexpected compound count %d\n", count);

    status = GdipSetPenCompoundArray(pen, NULL, 0);
    expect(InvalidParameter, status);
    status = GdipSetPenCompoundArray(pen, NULL, 4);
    expect(InvalidParameter, status);
    status = GdipSetPenCompoundArray(pen, testvalues, 3);
    expect(InvalidParameter, status);
    status = GdipSetPenCompoundArray(pen, testvalues, 0);
    expect(InvalidParameter, status);
    status = GdipSetPenCompoundArray(pen, testvalues, -2);
    expect(InvalidParameter, status);

    status = GdipSetPenCompoundArray(pen, notSortedValues, 4);
    expect(InvalidParameter, status);
    status = GdipSetPenCompoundArray(pen, negativeValues, 4);
    expect(InvalidParameter, status);
    status = GdipSetPenCompoundArray(pen, tooLargeValues, 4);
    expect(InvalidParameter, status);

    status = GdipSetPenCompoundArray(pen, testvalues, 4);
    expect(Ok, status);
    status = GdipSetPenCompoundArray(pen, NULL, 0);
    expect(InvalidParameter, status);

    count = 0;
    status = GdipGetPenCompoundCount(pen, &count);
    expect(Ok, status);
    ok(count == 4, "Unexpected compound count %d\n", count);

    returnvalues = calloc(5, sizeof(REAL));
    /* When count larger than stored array return error */
    status = GdipGetPenCompoundArray(pen, returnvalues, 40);
    expect(InvalidParameter, status);
    status = GdipGetPenCompoundArray(NULL, returnvalues, 4);
    expect(InvalidParameter, status);
    /* When count is zero, it should do nothing */
    status = GdipGetPenCompoundArray(pen, returnvalues, 0);
    expect(Ok, status);
    ok(returnvalues[0] == 0.0, "Unexpected compound array %f\n", returnvalues[0]);

    status = GdipGetPenCompoundArray(pen, returnvalues, 4);
    expect(Ok, status);
    ok(memcmp(returnvalues, testvalues, 4 * sizeof(REAL)) == 0, "Unexpected compound array\n");

    status = GdipGetPenCompoundArray(pen, returnvalues, -10);
    expect(Ok, status);
    ok(memcmp(returnvalues, testvalues, 4 * sizeof(REAL)) == 0, "Unexpected compound array\n");

    free(returnvalues);
    GdipDeletePen(pen);
}

static void get_pen_transform(GpPen *pen, REAL *values)
{
    GpMatrix *matrix;
    GpStatus status;

    status = GdipCreateMatrix(&matrix);
    expect(Ok, status);

    status = GdipGetPenTransform(pen, matrix);
    expect(Ok, status);
    status = GdipGetMatrixElements(matrix, values);
    expect(Ok, status);

    GdipDeleteMatrix(matrix);
}

static void test_transform(void)
{
    GpStatus status;
    GpPen *pen;
    GpMatrix *matrix, *matrix2, *not_invertible_matrix;
    REAL values[6];

    /* Check default Pen Transformation */
    status = GdipCreatePen1((ARGB)0xffff00ff, 10.0f, UnitPixel, &pen);
    expect(Ok, status);

    status = GdipCreateMatrix(&matrix);
    expect(Ok, status);

    status = GdipGetPenTransform(pen, matrix);
    expect(Ok, status);

    status = GdipGetMatrixElements(matrix, values);
    expect(Ok, status);

    expectf(1.0, values[0]);
    expectf(0.0, values[1]);
    expectf(0.0, values[2]);
    expectf(1.0, values[3]);
    expectf(0.0, values[4]);
    expectf(0.0, values[5]);

    /* Setting Pen Tranformation to Not invertible matrix, should return InvalidParameter */
    GdipCreateMatrix2(3.0, 3.0, 2.0, 2.0, 6.0, 3.0, &not_invertible_matrix);

    status = GdipSetPenTransform(pen, not_invertible_matrix);
    expect(InvalidParameter, status);
    GdipDeleteMatrix(not_invertible_matrix);

    status = GdipGetPenTransform(pen, matrix);
    expect(Ok, status);
    status = GdipGetMatrixElements(matrix, values);
    expect(Ok, status);

    expectf(1.0, values[0]);
    expectf(0.0, values[1]);
    expectf(0.0, values[2]);
    expectf(1.0, values[3]);
    expectf(0.0, values[4]);
    expectf(0.0, values[5]);

    /* Setting Pen Tranformation to invertible matrix, should be successfull */
    GdipCreateMatrix2(3.0, -2.0, 5.0, 2.0, 6.0, 3.0, &matrix2);
    status = GdipSetPenTransform(pen, matrix2);
    expect(Ok, status);
    GdipDeleteMatrix(matrix2);

    status = GdipGetPenTransform(pen, matrix);
    expect(Ok, status);
    status = GdipGetMatrixElements(matrix, values);
    expect(Ok, status);
    expectf(3.0,  values[0]);
    expectf(-2.0,  values[1]);
    expectf(5.0,  values[2]);
    expectf(2.0, values[3]);
    expectf(6.0, values[4]);
    expectf(3.0,  values[5]);

    /* Translate */
    status = GdipTranslatePenTransform(NULL, 1.0, -2.0, MatrixOrderAppend);
    expect(InvalidParameter, status);

    status = GdipTranslatePenTransform(pen, 1.0, -2.0, MatrixOrderAppend);
    expect(Ok, status);

    status = GdipGetPenTransform(pen, matrix);
    expect(Ok, status);
    status = GdipGetMatrixElements(matrix, values);
    expect(Ok, status);
    expectf(3.0,  values[0]);
    expectf(-2.0,  values[1]);
    expectf(5.0,  values[2]);
    expectf(2.0, values[3]);
    expectf(7.0, values[4]);
    expectf(1.0,  values[5]);

    status = GdipTranslatePenTransform(pen, -3.0, 5.0, MatrixOrderPrepend);
    expect(Ok, status);

    status = GdipGetPenTransform(pen, matrix);
    expect(Ok, status);
    status = GdipGetMatrixElements(matrix, values);
    expect(Ok, status);
    expectf(3.0,  values[0]);
    expectf(-2.0,  values[1]);
    expectf(5.0,  values[2]);
    expectf(2.0, values[3]);
    expectf(23.0, values[4]);
    expectf(17.0,  values[5]);

    status = GdipResetPenTransform(pen);
    expect(Ok, status);

    status = GdipGetPenTransform(pen, matrix);
    expect(Ok, status);
    status = GdipGetMatrixElements(matrix, values);
    expect(Ok, status);

    expectf(1.0, values[0]);
    expectf(0.0, values[1]);
    expectf(0.0, values[2]);
    expectf(1.0, values[3]);
    expectf(0.0, values[4]);
    expectf(0.0, values[5]);

    /* Scale */
    status = GdipScalePenTransform(NULL, 1.0, 1.0, MatrixOrderPrepend);
    expect(InvalidParameter, status);

    status = GdipScalePenTransform(pen, 1.0, 1.0, MatrixOrderPrepend);
    expect(Ok, status);

    get_pen_transform(pen, values);
    expectf(1.0, values[0]);
    expectf(0.0, values[1]);
    expectf(0.0, values[2]);
    expectf(1.0, values[3]);
    expectf(0.0, values[4]);
    expectf(0.0, values[5]);

    status = GdipScalePenTransform(pen, 2.0, -10.0, MatrixOrderPrepend);
    expect(Ok, status);

    get_pen_transform(pen, values);
    expectf(2.0, values[0]);
    expectf(0.0, values[1]);
    expectf(0.0, values[2]);
    expectf(-10.0, values[3]);
    expectf(0.0, values[4]);
    expectf(0.0, values[5]);

    status = GdipScalePenTransform(pen, 2.0, -10.0, MatrixOrderAppend);
    expect(Ok, status);

    get_pen_transform(pen, values);
    expectf(4.0, values[0]);
    expectf(0.0, values[1]);
    expectf(0.0, values[2]);
    expectf(100.0, values[3]);
    expectf(0.0, values[4]);
    expectf(0.0, values[5]);

    status = GdipTranslatePenTransform(pen, 1.0, -2.0, MatrixOrderAppend);
    expect(Ok, status);

    get_pen_transform(pen, values);
    expectf(4.0, values[0]);
    expectf(0.0, values[1]);
    expectf(0.0, values[2]);
    expectf(100.0, values[3]);
    expectf(1.0, values[4]);
    expectf(-2.0, values[5]);

    status = GdipScalePenTransform(pen, 2.0, -10.0, MatrixOrderPrepend);
    expect(Ok, status);

    get_pen_transform(pen, values);
    expectf(8.0, values[0]);
    expectf(0.0, values[1]);
    expectf(0.0, values[2]);
    expectf(-1000.0, values[3]);
    expectf(1.0, values[4]);
    expectf(-2.0, values[5]);

    /* Multiply */
    status = GdipResetPenTransform(pen);
    expect(Ok, status);

    status = GdipSetMatrixElements(matrix, 2.0, 1.0, 1.0, 4.0, 1.0, 2.0);
    expect(Ok, status);

    status = GdipMultiplyPenTransform(NULL, matrix, MatrixOrderPrepend);
    expect(InvalidParameter, status);

    status = GdipMultiplyPenTransform(pen, matrix, MatrixOrderPrepend);
    expect(Ok, status);

    get_pen_transform(pen, values);
    expectf(2.0, values[0]);
    expectf(1.0, values[1]);
    expectf(1.0, values[2]);
    expectf(4.0, values[3]);
    expectf(1.0, values[4]);
    expectf(2.0, values[5]);

    status = GdipScalePenTransform(pen, 2.0, -10.0, MatrixOrderAppend);
    expect(Ok, status);

    status = GdipMultiplyPenTransform(pen, matrix, MatrixOrderAppend);
    expect(Ok, status);

    get_pen_transform(pen, values);
    expectf(-2.0, values[0]);
    expectf(-36.0, values[1]);
    expectf(-36.0, values[2]);
    expectf(-158.0, values[3]);
    expectf(-15.0, values[4]);
    expectf(-76.0, values[5]);

    /* Rotate */
    status = GdipResetPenTransform(pen);
    expect(Ok, status);

    status = GdipSetMatrixElements(matrix, 2.0, 1.0, 1.0, 4.0, 1.0, 2.0);
    expect(Ok, status);

    status = GdipSetPenTransform(pen, matrix);
    expect(Ok, status);

    status = GdipRotatePenTransform(NULL, 10.0, MatrixOrderPrepend);
    expect(InvalidParameter, status);

    status = GdipRotatePenTransform(pen, 45.0, MatrixOrderPrepend);
    expect(Ok, status);

    get_pen_transform(pen, values);
    expectf(2.12, values[0]);
    expectf(3.54, values[1]);
    expectf(-0.71, values[2]);
    expectf(2.12, values[3]);
    expectf(1.0, values[4]);
    expectf(2.0, values[5]);

    status = GdipScalePenTransform(pen, 2.0, -10.0, MatrixOrderAppend);
    expect(Ok, status);

    status = GdipRotatePenTransform(pen, 180.0, MatrixOrderAppend);
    expect(Ok, status);

    get_pen_transform(pen, values);
    expectf(-4.24, values[0]);
    expectf(35.36, values[1]);
    expectf(1.41, values[2]);
    expectf(21.21, values[3]);
    expectf(-2.0, values[4]);
    expectf(20.0, values[5]);

    GdipDeletePen(pen);

    GdipDeleteMatrix(matrix);
}

START_TEST(pen)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    HMODULE hmsvcrt;
    int (CDECL * _controlfp_s)(unsigned int *cur, unsigned int newval, unsigned int mask);

    /* Enable all FP exceptions except _EM_INEXACT, which gdi32 can trigger */
    hmsvcrt = LoadLibraryA("msvcrt");
    _controlfp_s = (void*)GetProcAddress(hmsvcrt, "_controlfp_s");
    if (_controlfp_s) _controlfp_s(0, 0, 0x0008001e);

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
    test_transform();

    GdiplusShutdown(gdiplusToken);
}
