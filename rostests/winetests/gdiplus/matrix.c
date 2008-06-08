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

#include "windows.h"
#include "gdiplus.h"
#include "wine/test.h"

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)

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

START_TEST(matrix)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_constructor_destructor();
    test_transform();

    GdiplusShutdown(gdiplusToken);
}
