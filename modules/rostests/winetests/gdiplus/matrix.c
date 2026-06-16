/*
 * Unit test suite for matrices
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
#include <limits.h>

#include "objbase.h"
#include "gdiplus.h"
#include "wine/test.h"

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)
#define expectf(expected, got) ok(fabs(expected - got) < 0.0001, "Expected %.2f, got %.2f\n", expected, got)

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
    GpStatus status;
    GpMatrix *matrix = NULL;

    status = GdipCreateMatrix2(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, &matrix);
    expect(Ok, status);
    ok(matrix != NULL, "Expected matrix to be initialized\n");

    status = GdipDeleteMatrix(NULL);
    expect(InvalidParameter, status);

    status = GdipDeleteMatrix(matrix);
    expect(Ok, status);
}

typedef struct{
    REAL X;
    REAL Y;
} real_point;

static real_point transform_points[] = {
    {1000.00, 2600.00}, /*0*/
    {855.00, 2390.00}, /*1*/
    {700.00, 2200.00}, /*2*/
    {565.00, 1970.00}, /*3*/
    {400.00, 1800.00}, /*4*/
    {275.00, 1550.00}, /*5*/
    {100.00, 1400.00}, /*6*/
    {-15.00, 1130.00}, /*7*/
    {-200.00, 1000.00}, /*8*/
    {-305.00, 710.00} /*9*/
    };

static void test_transform(void)
{
    GpStatus status;
    GpMatrix *matrix = NULL;
    GpPointF pts[10];
    INT i;
    BOOL match;

    for(i = 0; i < 10; i ++){
        pts[i].X = i * 5.0 * (REAL)(i % 2);
        pts[i].Y = 50.0 - i * 5.0;
    }

    GdipCreateMatrix2(1.0, -2.0, 30.0, 40.0, -500.0, 600.0, &matrix);

    status = GdipTransformMatrixPoints(matrix, pts, 0);
    expect(InvalidParameter, status);

    status = GdipTransformMatrixPoints(matrix, pts, 10);
    expect(Ok, status);

    for(i = 0; i < 10; i ++){
        match = fabs(transform_points[i].X - pts[i].X) < 2.0
            && fabs(transform_points[i].Y -  pts[i].Y) < 2.0;

        ok(match, "Expected #%d to be (%.2f, %.2f) but got (%.2f, %.2f)\n", i,
           transform_points[i].X, transform_points[i].Y, pts[i].X, pts[i].Y);
    }

    GdipDeleteMatrix(matrix);
}

static void test_translate(void)
{
    GpStatus status;
    GpMatrix *matrix = NULL;
    REAL elems[6];

    static const REAL expected_elem_append[] = {1.0, -2.0, 30.0, 40.0, -470.0, 620.0};
    static const REAL expected_elem_prepend[] = {1.0, -2.0, 30.0, 40.0, 130.0, 1340.0};

    GdipCreateMatrix2(1.0, -2.0, 30.0, 40.0, -500.0, 600.0, &matrix);

    status = GdipTranslateMatrix(NULL, 30.0, 20.0, MatrixOrderAppend);
    expect(InvalidParameter, status);
    status = GdipTranslateMatrix(matrix, 30.0, 20.0, MatrixOrderAppend);
    expect(Ok, status);

    status = GdipGetMatrixElements(matrix, elems);
    expect(Ok, status);
    GdipDeleteMatrix(matrix);

    for(INT i = 0; i < 6; i++)
        ok(expected_elem_append[i] == elems[i], "Expected #%d to be (%.1f) but got (%.1f)\n", i,
           expected_elem_append[i], elems[i]);

    GdipCreateMatrix2(1.0, -2.0, 30.0, 40.0, -500.0, 600.0, &matrix);

    status = GdipTranslateMatrix(matrix, 30.0, 20.0, MatrixOrderPrepend);
    expect(Ok, status);

    status = GdipGetMatrixElements(matrix, elems);
    expect(Ok, status);
    GdipDeleteMatrix(matrix);

    for(INT i = 0; i < 6; i++)
        ok(expected_elem_prepend[i] == elems[i], "Expected #%d to be (%.1f) but got (%.1f)\n", i,
           expected_elem_prepend[i], elems[i]);

}

static void test_scale(void)
{
    GpStatus status;
    GpMatrix *matrix = NULL;
    REAL elems[6];
    int i;

    static const REAL expected_elem[] = {3.0, -4.0, 90.0, 80.0, -1500.0, 1200.0};
    static const REAL expected_elem2[] = {3.0, -6.0, 60.0, 80.0, -500.0, 600.0};

    GdipCreateMatrix2(1.0, -2.0, 30.0, 40.0, -500.0, 600.0, &matrix);

    status = GdipScaleMatrix(NULL, 3, 2, MatrixOrderAppend);
    expect(InvalidParameter, status);
    status = GdipScaleMatrix(matrix, 3, 2, MatrixOrderAppend);
    expect(Ok, status);

    status = GdipGetMatrixElements(matrix, elems);
    expect(Ok, status);
    GdipDeleteMatrix(matrix);

    for(i = 0; i < 6; i++)
        ok(expected_elem[i] == elems[i], "Expected #%d to be (%.1f) but got (%.1f)\n", i,
           expected_elem[i], elems[i]);

    GdipCreateMatrix2(1.0, -2.0, 30.0, 40.0, -500.0, 600.0, &matrix);

    status = GdipScaleMatrix(matrix, 3, 2, MatrixOrderPrepend);
    expect(Ok, status);

    status = GdipGetMatrixElements(matrix, elems);
    expect(Ok, status);
    GdipDeleteMatrix(matrix);

    for(i = 0; i < 6; i++)
        ok(expected_elem2[i] == elems[i], "Expected #%d to be (%.1f) but got (%.1f)\n", i,
           expected_elem2[i], elems[i]);

}

static void test_isinvertible(void)
{
    GpStatus status;
    GpMatrix *matrix = NULL;
    BOOL result;

    /* NULL arguments */
    status = GdipIsMatrixInvertible(NULL, &result);
    expect(InvalidParameter, status);
    status = GdipIsMatrixInvertible((GpMatrix*)0xdeadbeef, NULL);
    expect(InvalidParameter, status);
    status = GdipIsMatrixInvertible(NULL, NULL);
    expect(InvalidParameter, status);

    /* invertible */
    GdipCreateMatrix2(1.0, 1.2, 2.3, -1.0, 2.0, 3.0, &matrix);
    status = GdipIsMatrixInvertible(matrix, &result);
    expect(Ok, status);
    expect(TRUE, result);
    GdipDeleteMatrix(matrix);

    /* noninvertible */
    GdipCreateMatrix2(2.0, -1.0, 6.0, -3.0, 2.2, 3.0, &matrix);
    status = GdipIsMatrixInvertible(matrix, &result);
    expect(Ok, status);
    expect(FALSE, result);
    GdipDeleteMatrix(matrix);
}

static void test_invert(void)
{
    GpStatus status;
    GpMatrix *matrix = NULL;
    GpMatrix *inverted = NULL;
    BOOL equal = FALSE;
    REAL elems[6];

    /* NULL */
    status = GdipInvertMatrix(NULL);
    expect(InvalidParameter, status);

    /* noninvertible */
    GdipCreateMatrix2(2.0, -1.0, 6.0, -3.0, 2.2, 3.0, &matrix);
    status = GdipInvertMatrix(matrix);
    expect(InvalidParameter, status);
    GdipDeleteMatrix(matrix);

    /* invertible */
    GdipCreateMatrix2(3.0, -2.0, 5.0, 2.0, 6.0, 3.0, &matrix);
    status = GdipInvertMatrix(matrix);
    expect(Ok, status);
    GdipCreateMatrix2(2.0/16.0, 2.0/16.0, -5.0/16.0, 3.0/16.0, 3.0/16.0, -21.0/16.0, &inverted);
    GdipIsMatrixEqual(matrix, inverted, &equal);
    expect(TRUE, equal);
    GdipDeleteMatrix(matrix);

    GdipCreateMatrix2(0.0006, 0, 0, 0.0006, 400, 400, &matrix);
    status = GdipInvertMatrix(matrix);
    expect(Ok, status);
    status = GdipGetMatrixElements(matrix, elems);
    expect(Ok, status);
    ok(compare_float(elems[0], 1666.666504, 1), "elems[0] = %.10g\n", elems[0]);
    ok(compare_float(elems[1], 0, 0), "elems[1] = %.10g\n", elems[1]);
    ok(compare_float(elems[2], 0, 0), "elems[2] = %.10g\n", elems[2]);
    ok(compare_float(elems[3], 1666.666504, 1), "elems[3] = %.10g\n", elems[3]);
    ok(compare_float(elems[4], -666666.6875, 1), "elems[4] = %.10g\n", elems[4]);
    ok(compare_float(elems[5], -666666.6875, 1), "elems[5] = %.10g\n", elems[5]);

    GdipDeleteMatrix(inverted);
    GdipDeleteMatrix(matrix);
}

static void test_shear(void)
{
    GpStatus status;
    GpMatrix *matrix  = NULL;
    GpMatrix *sheared = NULL;
    BOOL equal;

    /* NULL */
    status = GdipShearMatrix(NULL, 0.0, 0.0, MatrixOrderPrepend);
    expect(InvalidParameter, status);

    /* X only shearing, MatrixOrderPrepend */
    GdipCreateMatrix2(1.0, 2.0, 4.0, -1.0, 6.0, 3.0, &matrix);
    status = GdipShearMatrix(matrix, 1.5, 0.0, MatrixOrderPrepend);
    expect(Ok, status);
    GdipCreateMatrix2(1.0, 2.0, 5.5, 2.0, 6.0, 3.0, &sheared);
    GdipIsMatrixEqual(matrix, sheared, &equal);
    expect(TRUE, equal);
    GdipDeleteMatrix(sheared);
    GdipDeleteMatrix(matrix);

    /* X only shearing, MatrixOrderAppend */
    GdipCreateMatrix2(1.0, 2.0, 4.0, -1.0, 6.0, 3.0, &matrix);
    status = GdipShearMatrix(matrix, 1.5, 0.0, MatrixOrderAppend);
    expect(Ok, status);
    GdipCreateMatrix2(4.0, 2.0, 2.5, -1.0, 10.5, 3.0, &sheared);
    GdipIsMatrixEqual(matrix, sheared, &equal);
    expect(TRUE, equal);
    GdipDeleteMatrix(sheared);
    GdipDeleteMatrix(matrix);

    /* Y only shearing, MatrixOrderPrepend */
    GdipCreateMatrix2(1.0, 2.0, 4.0, -1.0, 6.0, 3.0, &matrix);
    status = GdipShearMatrix(matrix, 0.0, 1.5, MatrixOrderPrepend);
    expect(Ok, status);
    GdipCreateMatrix2(7.0, 0.5, 4.0, -1.0, 6.0, 3.0, &sheared);
    GdipIsMatrixEqual(matrix, sheared, &equal);
    expect(TRUE, equal);
    GdipDeleteMatrix(sheared);
    GdipDeleteMatrix(matrix);

    /* Y only shearing, MatrixOrderAppend */
    GdipCreateMatrix2(1.0, 2.0, 4.0, -1.0, 6.0, 3.0, &matrix);
    status = GdipShearMatrix(matrix, 0.0, 1.5, MatrixOrderAppend);
    expect(Ok, status);
    GdipCreateMatrix2(1.0, 3.5, 4.0, 5.0, 6.0, 12.0, &sheared);
    GdipIsMatrixEqual(matrix, sheared, &equal);
    expect(TRUE, equal);
    GdipDeleteMatrix(sheared);
    GdipDeleteMatrix(matrix);

    /* X,Y shearing, MatrixOrderPrepend */
    GdipCreateMatrix2(1.0, 2.0, 4.0, -1.0, 6.0, 3.0, &matrix);
    status = GdipShearMatrix(matrix, 4.0, 1.5, MatrixOrderPrepend);
    expect(Ok, status);
    GdipCreateMatrix2(7.0, 0.5, 8.0, 7.0, 6.0, 3.0, &sheared);
    GdipIsMatrixEqual(matrix, sheared, &equal);
    expect(TRUE, equal);
    GdipDeleteMatrix(sheared);
    GdipDeleteMatrix(matrix);

    /* X,Y shearing, MatrixOrderAppend */
    GdipCreateMatrix2(1.0, 2.0, 4.0, -1.0, 6.0, 3.0, &matrix);
    status = GdipShearMatrix(matrix, 4.0, 1.5, MatrixOrderAppend);
    expect(Ok, status);
    GdipCreateMatrix2(9.0, 3.5, 0.0, 5.0, 18.0, 12.0, &sheared);
    GdipIsMatrixEqual(matrix, sheared, &equal);
    expect(TRUE, equal);
    GdipDeleteMatrix(sheared);
    GdipDeleteMatrix(matrix);
}

static void test_constructor3(void)
{
    /* MSDN is on crack. GdipCreateMatrix3 makes a matrix that transforms the
     * corners of the given rectangle to the three points given. */
    GpMatrix *matrix;
    REAL values[6];
    GpRectF rc;
    GpPointF pt[3];
    GpStatus stat;

    rc.X = 10;
    rc.Y = 10;
    rc.Width = 10;
    rc.Height = 10;

    pt[0].X = 10;
    pt[0].Y = 10;
    pt[1].X = 20;
    pt[1].Y = 10;
    pt[2].X = 10;
    pt[2].Y = 20;

    stat = GdipCreateMatrix3(&rc, pt, &matrix);
    expect(Ok, stat);

    stat = GdipGetMatrixElements(matrix, values);
    expect(Ok, stat);

    expectf(1.0, values[0]);
    expectf(0.0, values[1]);
    expectf(0.0, values[2]);
    expectf(1.0, values[3]);
    expectf(0.0, values[4]);
    expectf(0.0, values[5]);

    GdipDeleteMatrix(matrix);

    pt[0].X = 20;
    pt[0].Y = 10;
    pt[1].X = 40;
    pt[1].Y = 10;
    pt[2].X = 20;
    pt[2].Y = 20;

    stat = GdipCreateMatrix3(&rc, pt, &matrix);
    expect(Ok, stat);

    stat = GdipGetMatrixElements(matrix, values);
    expect(Ok, stat);

    expectf(2.0, values[0]);
    expectf(0.0, values[1]);
    expectf(0.0, values[2]);
    expectf(1.0, values[3]);
    expectf(0.0, values[4]);
    expectf(0.0, values[5]);

    GdipDeleteMatrix(matrix);

    pt[0].X = 10;
    pt[0].Y = 20;
    pt[1].X = 20;
    pt[1].Y = 30;
    pt[2].X = 10;
    pt[2].Y = 30;

    stat = GdipCreateMatrix3(&rc, pt, &matrix);
    expect(Ok, stat);

    stat = GdipGetMatrixElements(matrix, values);
    expect(Ok, stat);

    expectf(1.0, values[0]);
    expectf(1.0, values[1]);
    expectf(0.0, values[2]);
    expectf(1.0, values[3]);
    expectf(0.0, values[4]);
    expectf(0.0, values[5]);

    GdipDeleteMatrix(matrix);
}

static void test_isidentity(void)
{
    GpMatrix *matrix;
    GpStatus stat;
    BOOL result;

    stat = GdipIsMatrixIdentity(NULL, NULL);
    expect(InvalidParameter, stat);

    stat = GdipIsMatrixIdentity(NULL, &result);
    expect(InvalidParameter, stat);

    stat = GdipCreateMatrix2(1.0, 0.0, 0.0, 1.0, 0.0, 0.0, &matrix);
    expect(Ok, stat);

    stat = GdipIsMatrixIdentity(matrix, NULL);
    expect(InvalidParameter, stat);

    result = FALSE;
    stat = GdipIsMatrixIdentity(matrix, &result);
    expect(Ok, stat);
    ok(!!result, "got %d\n", result);

    stat = GdipSetMatrixElements(matrix, 1.0, 0.0, 0.0, 1.0, 0.1, 0.0);
    expect(Ok, stat);

    result = TRUE;
    stat = GdipIsMatrixIdentity(matrix, &result);
    expect(Ok, stat);
    ok(!result, "got %d\n", result);

    GdipDeleteMatrix(matrix);
}

START_TEST(matrix)
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
    test_translate();
    test_scale();
    test_transform();
    test_isinvertible();
    test_invert();
    test_shear();
    test_constructor3();
    test_isidentity();

    GdiplusShutdown(gdiplusToken);
}
