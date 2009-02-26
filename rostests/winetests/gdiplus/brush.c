/*
 * Unit test suite for brushes
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

#include "windows.h"
#include "gdiplus.h"
#include "wine/test.h"
#include <math.h>

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)
#define expectf(expected, got) ok(fabs(expected - got) < 0.0001, "Expected %.2f, got %.2f\n", expected, got)

static void test_constructor_destructor(void)
{
    GpStatus status;
    GpSolidFill *brush = NULL;

    status = GdipCreateSolidFill((ARGB)0xdeadbeef, &brush);
    expect(Ok, status);
    ok(brush != NULL, "Expected brush to be initialized\n");

    status = GdipDeleteBrush(NULL);
    expect(InvalidParameter, status);

    status = GdipDeleteBrush((GpBrush*) brush);
    expect(Ok, status);
}

static void test_type(void)
{
    GpStatus status;
    GpBrushType bt;
    GpSolidFill *brush = NULL;

    GdipCreateSolidFill((ARGB)0xdeadbeef, &brush);

    status = GdipGetBrushType((GpBrush*)brush, &bt);
    expect(Ok, status);
    expect(BrushTypeSolidColor, bt);

    GdipDeleteBrush((GpBrush*) brush);
}
static GpPointF blendcount_ptf[] = {{0.0, 0.0},
                                    {50.0, 50.0}};
static void test_gradientblendcount(void)
{
    GpStatus status;
    GpPathGradient *brush;
    INT count;

    status = GdipCreatePathGradient(blendcount_ptf, 2, WrapModeClamp, &brush);
    expect(Ok, status);

    status = GdipGetPathGradientBlendCount(NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipGetPathGradientBlendCount(NULL, &count);
    expect(InvalidParameter, status);
    status = GdipGetPathGradientBlendCount(brush, NULL);
    expect(InvalidParameter, status);

    status = GdipGetPathGradientBlendCount(brush, &count);
    expect(Ok, status);
    expect(1, count);

    GdipDeleteBrush((GpBrush*) brush);
}

static GpPointF getblend_ptf[] = {{0.0, 0.0},
                                  {50.0, 50.0}};
static void test_getblend(void)
{
    GpStatus status;
    GpPathGradient *brush;
    REAL blends[4];
    REAL pos[4];

    status = GdipCreatePathGradient(getblend_ptf, 2, WrapModeClamp, &brush);
    expect(Ok, status);

    /* check some invalid parameters combinations */
    status = GdipGetPathGradientBlend(NULL, NULL,  NULL, -1);
    expect(InvalidParameter, status);
    status = GdipGetPathGradientBlend(brush,NULL,  NULL, -1);
    expect(InvalidParameter, status);
    status = GdipGetPathGradientBlend(NULL, blends,NULL, -1);
    expect(InvalidParameter, status);
    status = GdipGetPathGradientBlend(NULL, NULL,  pos,  -1);
    expect(InvalidParameter, status);
    status = GdipGetPathGradientBlend(NULL, NULL,  NULL,  1);
    expect(InvalidParameter, status);

    blends[0] = (REAL)0xdeadbeef;
    pos[0]    = (REAL)0xdeadbeef;
    status = GdipGetPathGradientBlend(brush, blends, pos, 1);
    expect(Ok, status);
    expectf(1.0, blends[0]);
    expectf((REAL)0xdeadbeef, pos[0]);

    GdipDeleteBrush((GpBrush*) brush);
}

static GpPointF getbounds_ptf[] = {{0.0, 20.0},
                                   {50.0, 50.0},
                                   {21.0, 25.0},
                                   {25.0, 46.0}};
static void test_getbounds(void)
{
    GpStatus status;
    GpPathGradient *brush;
    GpRectF bounds;

    status = GdipCreatePathGradient(getbounds_ptf, 4, WrapModeClamp, &brush);
    expect(Ok, status);

    status = GdipGetPathGradientRect(NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipGetPathGradientRect(brush, NULL);
    expect(InvalidParameter, status);
    status = GdipGetPathGradientRect(NULL, &bounds);
    expect(InvalidParameter, status);

    status = GdipGetPathGradientRect(brush, &bounds);
    expect(Ok, status);
    expectf(0.0, bounds.X);
    expectf(20.0, bounds.Y);
    expectf(50.0, bounds.Width);
    expectf(30.0, bounds.Height);

    GdipDeleteBrush((GpBrush*) brush);
}

static void test_getgamma(void)
{
    GpStatus status;
    GpLineGradient *line;
    GpPointF start, end;
    BOOL gamma;

    start.X = start.Y = 0.0;
    end.X = end.Y = 100.0;

    status = GdipCreateLineBrush(&start, &end, (ARGB)0xdeadbeef, 0xdeadbeef, WrapModeTile, &line);
    expect(Ok, status);

    /* NULL arguments */
    status = GdipGetLineGammaCorrection(NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipGetLineGammaCorrection(line, NULL);
    expect(InvalidParameter, status);
    status = GdipGetLineGammaCorrection(NULL, &gamma);
    expect(InvalidParameter, status);

    GdipDeleteBrush((GpBrush*)line);
}

static void test_transform(void)
{
    GpStatus status;
    GpTexture *texture;
    GpGraphics *graphics = NULL;
    GpBitmap *bitmap;
    HDC hdc = GetDC(0);
    GpMatrix *m, *m1;
    BOOL res;

    status = GdipCreateMatrix2(2.0, 0.0, 0.0, 0.0, 0.0, 0.0, &m);
    expect(Ok, status);

    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);
    status = GdipCreateBitmapFromGraphics(1, 1, graphics, &bitmap);
    expect(Ok, status);

    status = GdipCreateTexture((GpImage*)bitmap, WrapModeTile, &texture);
    expect(Ok, status);

    /* NULL */
    status = GdipGetTextureTransform(NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipGetTextureTransform(texture, NULL);
    expect(InvalidParameter, status);

    /* get default value - identity matrix */
    status = GdipGetTextureTransform(texture, m);
    expect(Ok, status);
    status = GdipIsMatrixIdentity(m, &res);
    expect(Ok, status);
    expect(TRUE, res);
    /* set and get then */
    status = GdipCreateMatrix2(2.0, 0.0, 0.0, 2.0, 0.0, 0.0, &m1);
    expect(Ok, status);
    status = GdipSetTextureTransform(texture, m1);
    expect(Ok, status);
    status = GdipGetTextureTransform(texture, m);
    expect(Ok, status);
    status = GdipIsMatrixEqual(m, m1, &res);
    expect(Ok, status);
    expect(TRUE, res);
    /* reset */
    status = GdipResetTextureTransform(texture);
    expect(Ok, status);
    status = GdipGetTextureTransform(texture, m);
    expect(Ok, status);
    status = GdipIsMatrixIdentity(m, &res);
    expect(Ok, status);
    expect(TRUE, res);

    status = GdipDeleteBrush((GpBrush*)texture);
    expect(Ok, status);

    status = GdipDeleteMatrix(m1);
    expect(Ok, status);
    status = GdipDeleteMatrix(m);
    expect(Ok, status);
    status = GdipDisposeImage((GpImage*)bitmap);
    expect(Ok, status);
    status = GdipDeleteGraphics(graphics);
    expect(Ok, status);
    ReleaseDC(0, hdc);
}

static void test_texturewrap(void)
{
    GpStatus status;
    GpTexture *texture;
    GpGraphics *graphics = NULL;
    GpBitmap *bitmap;
    HDC hdc = GetDC(0);
    GpWrapMode wrap;

    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);
    status = GdipCreateBitmapFromGraphics(1, 1, graphics, &bitmap);
    expect(Ok, status);

    status = GdipCreateTexture((GpImage*)bitmap, WrapModeTile, &texture);
    expect(Ok, status);

    /* NULL */
    status = GdipGetTextureWrapMode(NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipGetTextureWrapMode(texture, NULL);
    expect(InvalidParameter, status);
    status = GdipGetTextureWrapMode(NULL, &wrap);
    expect(InvalidParameter, status);

    /* get */
    wrap = WrapModeClamp;
    status = GdipGetTextureWrapMode(texture, &wrap);
    expect(Ok, status);
    expect(WrapModeTile, wrap);
    /* set, then get */
    wrap = WrapModeClamp;
    status = GdipSetTextureWrapMode(texture, wrap);
    expect(Ok, status);
    wrap = WrapModeTile;
    status = GdipGetTextureWrapMode(texture, &wrap);
    expect(Ok, status);
    expect(WrapModeClamp, wrap);

    status = GdipDeleteBrush((GpBrush*)texture);
    expect(Ok, status);
    status = GdipDisposeImage((GpImage*)bitmap);
    expect(Ok, status);
    status = GdipDeleteGraphics(graphics);
    expect(Ok, status);
    ReleaseDC(0, hdc);
}

static void test_gradientgetrect(void)
{
    GpLineGradient *brush;
    GpRectF rectf;
    GpStatus status;
    GpPointF pt1, pt2;

    pt1.X = pt1.Y = 1.0;
    pt2.X = pt2.Y = 100.0;
    status = GdipCreateLineBrush(&pt1, &pt2, 0, 0, WrapModeTile, &brush);
    expect(Ok, status);
    memset(&rectf, 0, sizeof(GpRectF));
    status = GdipGetLineRect(brush, &rectf);
    expect(Ok, status);
    expectf(1.0, rectf.X);
    expectf(1.0, rectf.Y);
    expectf(99.0, rectf.Width);
    expectf(99.0, rectf.Height);
    status = GdipDeleteBrush((GpBrush*)brush);
    /* vertical gradient */
    pt1.X = pt1.Y = pt2.X = 0.0;
    pt2.Y = 10.0;
    status = GdipCreateLineBrush(&pt1, &pt2, 0, 0, WrapModeTile, &brush);
    expect(Ok, status);
    memset(&rectf, 0, sizeof(GpRectF));
    status = GdipGetLineRect(brush, &rectf);
    expect(Ok, status);
    todo_wine expectf(-5.0, rectf.X);
    expectf(0.0, rectf.Y);
    todo_wine expectf(10.0, rectf.Width);
    expectf(10.0, rectf.Height);
    status = GdipDeleteBrush((GpBrush*)brush);
    /* horizontal gradient */
    pt1.X = pt1.Y = pt2.Y = 0.0;
    pt2.X = 10.0;
    status = GdipCreateLineBrush(&pt1, &pt2, 0, 0, WrapModeTile, &brush);
    expect(Ok, status);
    memset(&rectf, 0, sizeof(GpRectF));
    status = GdipGetLineRect(brush, &rectf);
    expect(Ok, status);
    expectf(0.0, rectf.X);
    todo_wine expectf(-5.0, rectf.Y);
    expectf(10.0, rectf.Width);
    todo_wine expectf(10.0, rectf.Height);
    status = GdipDeleteBrush((GpBrush*)brush);
    /* from rect with LinearGradientModeHorizontal */
    rectf.X = rectf.Y = 10.0;
    rectf.Width = rectf.Height = 100.0;
    status = GdipCreateLineBrushFromRect(&rectf, 0, 0, LinearGradientModeHorizontal,
        WrapModeTile, &brush);
    expect(Ok, status);
    memset(&rectf, 0, sizeof(GpRectF));
    status = GdipGetLineRect(brush, &rectf);
    expect(Ok, status);
    expectf(10.0, rectf.X);
    expectf(10.0, rectf.Y);
    expectf(100.0, rectf.Width);
    expectf(100.0, rectf.Height);
    status = GdipDeleteBrush((GpBrush*)brush);
}

START_TEST(brush)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_constructor_destructor();
    test_type();
    test_gradientblendcount();
    test_getblend();
    test_getbounds();
    test_getgamma();
    test_transform();
    test_texturewrap();
    test_gradientgetrect();

    GdiplusShutdown(gdiplusToken);
}
