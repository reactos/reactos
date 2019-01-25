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

#include <math.h>

#include "objbase.h"
#include "gdiplus.h"
#include "wine/test.h"

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)
#define expectf(expected, got) ok(fabs(expected - got) < 0.0001, "Expected %.2f, got %.2f\n", expected, got)

static HWND hwnd;

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

static void test_createHatchBrush(void)
{
    GpStatus status;
    GpHatch *brush;

    status = GdipCreateHatchBrush(HatchStyleMin, 1, 2, &brush);
    expect(Ok, status);
    ok(brush != NULL, "Expected the brush to be initialized.\n");

    GdipDeleteBrush((GpBrush *)brush);

    status = GdipCreateHatchBrush(HatchStyleMax, 1, 2, &brush);
    expect(Ok, status);
    ok(brush != NULL, "Expected the brush to be initialized.\n");

    GdipDeleteBrush((GpBrush *)brush);

    status = GdipCreateHatchBrush(HatchStyle05Percent, 1, 2, NULL);
    expect(InvalidParameter, status);

    status = GdipCreateHatchBrush((HatchStyle)(HatchStyleMin - 1), 1, 2, &brush);
    expect(InvalidParameter, status);

    status = GdipCreateHatchBrush((HatchStyle)(HatchStyleMax + 1), 1, 2, &brush);
    expect(InvalidParameter, status);
}

static void test_createLineBrushFromRectWithAngle(void)
{
    GpStatus status;
    GpLineGradient *brush;
    GpRectF rect1 = { 1, 3, 1, 2 };
    GpRectF rect2 = { 1, 3, -1, -2 };
    GpRectF rect3 = { 1, 3, 0, 1 };
    GpRectF rect4 = { 1, 3, 1, 0 };

    status = GdipCreateLineBrushFromRectWithAngle(&rect1, 10, 11, 0, TRUE, WrapModeTile, &brush);
    expect(Ok, status);
    GdipDeleteBrush((GpBrush *) brush);

    status = GdipCreateLineBrushFromRectWithAngle(&rect2, 10, 11, 135, TRUE, (WrapMode)(WrapModeTile - 1), &brush);
    expect(Ok, status);
    GdipDeleteBrush((GpBrush *) brush);

    status = GdipCreateLineBrushFromRectWithAngle(&rect2, 10, 11, -225, FALSE, (WrapMode)(WrapModeTile - 1), &brush);
    expect(Ok, status);
    GdipDeleteBrush((GpBrush *) brush);

    status = GdipCreateLineBrushFromRectWithAngle(&rect1, 10, 11, 405, TRUE, (WrapMode)(WrapModeClamp + 1), &brush);
    expect(Ok, status);
    GdipDeleteBrush((GpBrush *) brush);

    status = GdipCreateLineBrushFromRectWithAngle(&rect1, 10, 11, 45, FALSE, (WrapMode)(WrapModeClamp + 1), &brush);
    expect(Ok, status);
    GdipDeleteBrush((GpBrush *) brush);

    status = GdipCreateLineBrushFromRectWithAngle(&rect1, 10, 11, 90, TRUE, WrapModeTileFlipX, &brush);
    expect(Ok, status);

    status = GdipCreateLineBrushFromRectWithAngle(NULL, 10, 11, 90, TRUE, WrapModeTile, &brush);
    expect(InvalidParameter, status);

    status = GdipCreateLineBrushFromRectWithAngle(&rect3, 10, 11, 90, TRUE, WrapModeTileFlipXY, &brush);
    expect(OutOfMemory, status);

    status = GdipCreateLineBrushFromRectWithAngle(&rect4, 10, 11, 90, TRUE, WrapModeTileFlipXY, &brush);
    expect(OutOfMemory, status);

    status = GdipCreateLineBrushFromRectWithAngle(&rect3, 10, 11, 90, TRUE, WrapModeTileFlipXY, NULL);
    expect(InvalidParameter, status);

    status = GdipCreateLineBrushFromRectWithAngle(&rect4, 10, 11, 90, TRUE, WrapModeTileFlipXY, NULL);
    expect(InvalidParameter, status);

    status = GdipCreateLineBrushFromRectWithAngle(&rect3, 10, 11, 90, TRUE, WrapModeClamp, &brush);
    expect(InvalidParameter, status);

    status = GdipCreateLineBrushFromRectWithAngle(&rect4, 10, 11, 90, TRUE, WrapModeClamp, &brush);
    expect(InvalidParameter, status);

    status = GdipCreateLineBrushFromRectWithAngle(&rect1, 10, 11, 90, TRUE, WrapModeClamp, &brush);
    expect(InvalidParameter, status);

    status = GdipCreateLineBrushFromRectWithAngle(&rect1, 10, 11, 90, TRUE, WrapModeTile, NULL);
    expect(InvalidParameter, status);

    GdipDeleteBrush((GpBrush *) brush);
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
    GpLineGradient *line;
    GpGraphics *graphics = NULL;
    GpBitmap *bitmap;
    HDC hdc = GetDC(0);
    GpMatrix *m, *m1;
    BOOL res;
    GpPointF start, end;
    GpRectF rectf;
    REAL elements[6];

    /* GpTexture */
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



    status = GdipCreateFromHWND(hwnd, &graphics);
    expect(Ok, status);

    /* GpLineGradient */
    /* create with vertical gradient line */
    start.X = start.Y = end.X = 0.0;
    end.Y = 100.0;

    status = GdipCreateLineBrush(&start, &end, (ARGB)0xffff0000, 0xff00ff00, WrapModeTile, &line);
    expect(Ok, status);

    status = GdipCreateMatrix2(1.0, 0.0, 0.0, 1.0, 0.0, 0.0, &m);
    expect(Ok, status);

    /* NULL arguments */
    status = GdipResetLineTransform(NULL);
    expect(InvalidParameter, status);
    status = GdipSetLineTransform(NULL, m);
    expect(InvalidParameter, status);
    status = GdipSetLineTransform(line, NULL);
    expect(InvalidParameter, status);
    status = GdipGetLineTransform(NULL, m);
    expect(InvalidParameter, status);
    status = GdipGetLineTransform(line, NULL);
    expect(InvalidParameter, status);
    status = GdipScaleLineTransform(NULL, 1, 1, MatrixOrderPrepend);
    expect(InvalidParameter, status);
    status = GdipMultiplyLineTransform(NULL, m, MatrixOrderPrepend);
    expect(InvalidParameter, status);
    status = GdipTranslateLineTransform(NULL, 0, 0, MatrixOrderPrepend);
    expect(InvalidParameter, status);

    /* initial transform */
    status = GdipGetLineTransform(line, m);
    expect(Ok, status);

    status = GdipGetMatrixElements(m, elements);
    expect(Ok, status);
    expectf(0.0, elements[0]);
    expectf(1.0, elements[1]);
    expectf(-1.0, elements[2]);
    expectf(0.0, elements[3]);
    expectf(50.0, elements[4]);
    expectf(50.0, elements[5]);

    status = GdipGetLineRect(line, &rectf);
    expect(Ok, status);
    expectf(-50.0, rectf.X);
    expectf(0.0, rectf.Y);
    expectf(100.0, rectf.Width);
    expectf(100.0, rectf.Height);

    status = GdipFillRectangle(graphics, (GpBrush*)line, 0, 0, 200, 200);
    expect(Ok, status);

    /* manually set transform */
    GdipSetMatrixElements(m, 2.0, 0.0, 0.0, 4.0, 0.0, 0.0);

    status = GdipSetLineTransform(line, m);
    expect(Ok, status);

    status = GdipGetLineTransform(line, m);
    expect(Ok, status);

    status = GdipGetMatrixElements(m, elements);
    expect(Ok, status);
    expectf(2.0, elements[0]);
    expectf(0.0, elements[1]);
    expectf(0.0, elements[2]);
    expectf(4.0, elements[3]);
    expectf(0.0, elements[4]);
    expectf(0.0, elements[5]);

    status = GdipGetLineRect(line, &rectf);
    expect(Ok, status);
    expectf(-50.0, rectf.X);
    expectf(0.0, rectf.Y);
    expectf(100.0, rectf.Width);
    expectf(100.0, rectf.Height);

    status = GdipFillRectangle(graphics, (GpBrush*)line, 200, 0, 200, 200);
    expect(Ok, status);

    /* scale transform */
    status = GdipScaleLineTransform(line, 4.0, 0.5, MatrixOrderAppend);
    expect(Ok, status);

    status = GdipGetLineTransform(line, m);
    expect(Ok, status);

    status = GdipGetMatrixElements(m, elements);
    expect(Ok, status);
    expectf(8.0, elements[0]);
    expectf(0.0, elements[1]);
    expectf(0.0, elements[2]);
    expectf(2.0, elements[3]);
    expectf(0.0, elements[4]);
    expectf(0.0, elements[5]);

    status = GdipGetLineRect(line, &rectf);
    expect(Ok, status);
    expectf(-50.0, rectf.X);
    expectf(0.0, rectf.Y);
    expectf(100.0, rectf.Width);
    expectf(100.0, rectf.Height);

    status = GdipFillRectangle(graphics, (GpBrush*)line, 400, 0, 200, 200);
    expect(Ok, status);

    /* translate transform */
    status = GdipTranslateLineTransform(line, 10.0, -20.0, MatrixOrderAppend);
    expect(Ok, status);

    status = GdipGetLineTransform(line, m);
    expect(Ok, status);

    status = GdipGetMatrixElements(m, elements);
    expect(Ok, status);
    expectf(8.0, elements[0]);
    expectf(0.0, elements[1]);
    expectf(0.0, elements[2]);
    expectf(2.0, elements[3]);
    expectf(10.0, elements[4]);
    expectf(-20.0, elements[5]);

    status = GdipGetLineRect(line, &rectf);
    expect(Ok, status);
    expectf(-50.0, rectf.X);
    expectf(0.0, rectf.Y);
    expectf(100.0, rectf.Width);
    expectf(100.0, rectf.Height);

    status = GdipFillRectangle(graphics, (GpBrush*)line, 0, 200, 200, 200);
    expect(Ok, status);

    /* multiply transform */
    GdipSetMatrixElements(m, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
    GdipRotateMatrix(m, 45.0, MatrixOrderAppend);
    GdipScaleMatrix(m, 0.25, 0.5, MatrixOrderAppend);

    status = GdipMultiplyLineTransform(line, m, MatrixOrderAppend);
    expect(Ok, status);

    /* NULL transform does nothing */
    status = GdipMultiplyLineTransform(line, NULL, MatrixOrderAppend);
    expect(Ok, status);

    status = GdipGetLineTransform(line, m);
    expect(Ok, status);

    status = GdipGetMatrixElements(m, elements);
    expect(Ok, status);
    expectf(1.414214, elements[0]);
    expectf(2.828427, elements[1]);
    expectf(-0.353553, elements[2]);
    expectf(0.707107, elements[3]);
    expectf(5.303300, elements[4]);
    expectf(-3.535534, elements[5]);

    status = GdipGetLineRect(line, &rectf);
    expect(Ok, status);
    expectf(-50.0, rectf.X);
    expectf(0.0, rectf.Y);
    expectf(100.0, rectf.Width);
    expectf(100.0, rectf.Height);

    status = GdipFillRectangle(graphics, (GpBrush*)line, 200, 200, 200, 200);
    expect(Ok, status);

    /* reset transform sets to identity */
    status = GdipResetLineTransform(line);
    expect(Ok, status);

    status = GdipGetLineTransform(line, m);
    expect(Ok, status);

    status = GdipGetMatrixElements(m, elements);
    expect(Ok, status);
    expectf(1.0, elements[0]);
    expectf(0.0, elements[1]);
    expectf(0.0, elements[2]);
    expectf(1.0, elements[3]);
    expectf(0.0, elements[4]);
    expectf(0.0, elements[5]);

    status = GdipGetLineRect(line, &rectf);
    expect(Ok, status);
    expectf(-50.0, rectf.X);
    expectf(0.0, rectf.Y);
    expectf(100.0, rectf.Width);
    expectf(100.0, rectf.Height);

    status = GdipFillRectangle(graphics, (GpBrush*)line, 400, 200, 200, 200);
    expect(Ok, status);

    GdipDeleteBrush((GpBrush*)line);

    /* passing negative Width/Height to LinearGradientModeHorizontal */
    rectf.X = rectf.Y = 10.0;
    rectf.Width = rectf.Height = -100.0;
    status = GdipCreateLineBrushFromRect(&rectf, (ARGB)0xffff0000, 0xff00ff00,
            LinearGradientModeHorizontal, WrapModeTile, &line);
    expect(Ok, status);
    memset(&rectf, 0, sizeof(GpRectF));
    status = GdipGetLineRect(line, &rectf);
    expect(Ok, status);
    expectf(10.0, rectf.X);
    expectf(10.0, rectf.Y);
    expectf(-100.0, rectf.Width);
    expectf(-100.0, rectf.Height);
    status = GdipGetLineTransform(line, m);
    expect(Ok, status);
    status = GdipGetMatrixElements(m, elements);
    expect(Ok,status);
    expectf(1.0, elements[0]);
    expectf(0.0, elements[1]);
    expectf(0.0, elements[2]);
    expectf(1.0, elements[3]);
    expectf(0.0, elements[4]);
    expectf(0.0, elements[5]);
    status = GdipFillRectangle(graphics, (GpBrush*)line, 0, 400, 200, 200);
    expect(Ok, status);
    status = GdipDeleteBrush((GpBrush*)line);
    expect(Ok,status);

    if(0){
        /* enable to visually compare with Windows */
        MSG msg;
        while(GetMessageW(&msg, hwnd, 0, 0) > 0){
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    GdipDeleteMatrix(m);
    GdipDeleteGraphics(graphics);
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
    static const struct
    {
        LinearGradientMode mode;
        GpRectF rect;
        REAL transform[6];
    }
    create_from_rect[] =
    {
        { LinearGradientModeHorizontal, { 10.0f, 10.0f, -100.0f, -100.0f } },
        { LinearGradientModeHorizontal, { 10.0f, 10.0f, 100.0f, 100.0f } },
        { LinearGradientModeHorizontal, { 10.0f, -5.0f, 100.0f, 50.0f } },
        { LinearGradientModeHorizontal, { -5.0f, 10.0f, 100.0f, 50.0f } },
        { LinearGradientModeVertical, { 0.0f, 0.0f, 100.0f, 10.0f }, { 0.0f, 0.1f, -10.0f, -0.0f, 100.0f, 0.0f } },
        { LinearGradientModeVertical, { 10.0f, -12.0f, 100.0f, 105.0f }, { 0.0f, 1.05f, -0.952f, 0.0f, 98.571f, -22.5f } },
    };
    static const struct
    {
        GpPointF pt1, pt2;
        GpRectF rect;
        REAL transform[6];
    }
    create_from_pt[] =
    {
        { { 1.0f, 1.0f }, { 100.0f, 100.0f }, { 1.0f, 1.0f, 99.0f, 99.0f }, { 1.0f, 1.0f, -1.0f, 1.0f, 50.50f, -50.50f } },
        { { 0.0f, 0.0f }, { 0.0f, 10.0f }, { -5.0f, 0.0f, 10.0f, 10.0f }, { 0.0f, 1.0f, -1.0f, 0.0f, 5.0f, 5.0f } },
        { { 0.0f, 0.0f }, { 10.0f, 0.0f }, { 0.0f, -5.0f, 10.0f, 10.0f }, { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f } },
        /* Slope = -1 */
        { { 0.0f, 0.0f }, { 20.0f, -20.0f }, { 0.0f, -20.0f, 20.0f, 20.0f }, { 1.0f, -1.0f, 1.0f, 1.0f, 10.0f, 10.0f } },
        /* Slope = 1/100 */
        { { 0.0f, 0.0f }, { 100.0f, 1.0f }, { 0.0f, 0.0f, 100.0f, 1.0f }, { 1.0f, 0.01f, -0.02f, 2.0f, 0.01f, -1.0f } },
        { { 10.0f, 10.0f }, { -90.0f, 10.0f }, { -90.0f, -40.0f, 100.0f, 100.0f }, { -1.0f, 0.0f, 0.0f, -1.0f, -80.0f, 20.0f } },
    };
    static const struct
    {
        GpRectF rect;
        REAL angle;
        BOOL is_scalable;
        REAL transform[6];
    }
    create_with_angle[] =
    {
        { { 10.0f, 10.0f, -100.0f, -100.0f }, 0.0f, TRUE },
        { { 10.0f, 10.0f, -100.0f, -100.0f }, 0.0f, FALSE },
        { { 10.0f, 10.0f, 100.0f, 100.0f }, 0.0f, FALSE },
        { { 10.0f, 10.0f, 100.0f, 100.0f }, 0.0f, TRUE },
        { { 10.0f, -5.0f, 100.0f, 50.0f }, 0.0f, FALSE },
        { { 10.0f, -5.0f, 100.0f, 50.0f }, 0.0f, TRUE },
        { { -5.0f, 10.0f, 100.0f, 50.0f }, 0.0f, FALSE },
        { { -5.0f, 10.0f, 100.0f, 50.0f }, 0.0f, TRUE },
        { { 0.0f, 0.0f, 100.0f, 10.0f }, -90.0f, TRUE, { 0.0f, -0.1f, 10.0f, 0.0f, 0.0f, 10.0f } },
        { { 10.0f, -12.0f, 100.0f, 105.0f }, -90.0f, TRUE, { 0.0f, -1.05f, 0.952f, 0.0f, 21.429f, 103.5f } },
        { { 0.0f, 0.0f, 100.0f, 10.0f }, -90.0f, FALSE, { 0.0f, -0.1f, 10.0f, -0.0f, 0.0f, 10.0f } },
        { { 10.0f, -12.0f, 100.0f, 105.0f }, -90.0f, FALSE, { 0.0f, -1.05f, 0.952f, 0.0f, 21.429f, 103.5f } },
    };
    GpLineGradient *brush;
    GpMatrix *transform;
    REAL elements[6];
    GpStatus status;
    unsigned int i;
    ARGB colors[2];
    GpRectF rectf;

    status = GdipCreateMatrix(&transform);
    expect(Ok, status);

    for (i = 0; i < ARRAY_SIZE(create_from_pt); ++i)
    {
        status = GdipCreateLineBrush(&create_from_pt[i].pt1, &create_from_pt[i].pt2, 0x1, 0x2, WrapModeTile, &brush);
        ok(status == Ok, "Failed to create a brush, %d.\n", status);

        memset(&rectf, 0, sizeof(rectf));
        status = GdipGetLineRect(brush, &rectf);
        ok(status == Ok, "Failed to get brush rect, %d.\n", status);
        ok(!memcmp(&rectf, &create_from_pt[i].rect, sizeof(rectf)), "Unexpected brush rect.\n");

        status = GdipGetLineTransform(brush, transform);
        ok(status == Ok, "Failed to get brush transform, %d.\n", status);

        status = GdipGetMatrixElements(transform, elements);
        ok(status == Ok, "Failed to get matrix elements, %d.\n", status);

#define expectf2(expected, got) ok(fabs(expected - got) < 0.001, "%u: expected %.3f, got %.3f.\n", i, expected, got)
        expectf2(create_from_pt[i].transform[0], elements[0]);
        expectf2(create_from_pt[i].transform[1], elements[1]);
        expectf2(create_from_pt[i].transform[2], elements[2]);
        expectf2(create_from_pt[i].transform[3], elements[3]);
        expectf2(create_from_pt[i].transform[4], elements[4]);
        expectf2(create_from_pt[i].transform[5], elements[5]);
#undef expect2f

        status = GdipGetLineColors(brush, colors);
        ok(status == Ok, "Failed to get line colors, %d.\n", status);
        ok(colors[0] == 0x1 && colors[1] == 0x2, "Unexpected brush colors.\n");

        status = GdipDeleteBrush((GpBrush *)brush);
        ok(status == Ok, "Failed to delete a brush, %d.\n", status);
    }

    /* zero height rect */
    rectf.X = rectf.Y = 10.0;
    rectf.Width = 100.0;
    rectf.Height = 0.0;
    status = GdipCreateLineBrushFromRect(&rectf, 0, 0, LinearGradientModeVertical,
        WrapModeTile, &brush);
    expect(OutOfMemory, status);

    /* zero width rect */
    rectf.X = rectf.Y = 10.0;
    rectf.Width = 0.0;
    rectf.Height = 100.0;
    status = GdipCreateLineBrushFromRect(&rectf, 0, 0, LinearGradientModeHorizontal,
        WrapModeTile, &brush);
    expect(OutOfMemory, status);

    for (i = 0; i < ARRAY_SIZE(create_from_rect); ++i)
    {
        ARGB colors[2];
        BOOL ret;

        status = GdipCreateLineBrushFromRect(&create_from_rect[i].rect, 0x1, 0x2, create_from_rect[i].mode,
            WrapModeTile, &brush);
        ok(status == Ok, "Failed to create a brush, %d.\n", status);

        memset(&rectf, 0, sizeof(rectf));
        status = GdipGetLineRect(brush, &rectf);
        ok(status == Ok, "Failed to get brush rect, %d.\n", status);
        ok(!memcmp(&rectf, &create_from_rect[i].rect, sizeof(rectf)), "Unexpected brush rect.\n");

        status = GdipGetLineTransform(brush, transform);
        ok(status == Ok, "Failed to get brush transform, %d.\n", status);

        if (create_from_rect[i].mode == LinearGradientModeHorizontal)
        {
            status = GdipIsMatrixIdentity(transform, &ret);
            ok(status == Ok, "Unexpected ret value %d.\n", status);
        }
        else
        {
            status = GdipGetMatrixElements(transform, elements);
            ok(status == Ok, "Failed to get matrix elements, %d.\n", status);

#define expectf2(expected, got) ok(fabs(expected - got) < 0.001, "%u: expected %.3f, got %.3f.\n", i, expected, got)
            expectf2(create_from_rect[i].transform[0], elements[0]);
            expectf2(create_from_rect[i].transform[1], elements[1]);
            expectf2(create_from_rect[i].transform[2], elements[2]);
            expectf2(create_from_rect[i].transform[3], elements[3]);
            expectf2(create_from_rect[i].transform[4], elements[4]);
            expectf2(create_from_rect[i].transform[5], elements[5]);
#undef expectf2
        }

        status = GdipGetLineColors(brush, colors);
        ok(status == Ok, "Failed to get line colors, %d.\n", status);
        ok(colors[0] == 0x1 && colors[1] == 0x2, "Unexpected brush colors.\n");

        status = GdipDeleteBrush((GpBrush*)brush);
        ok(status == Ok, "Failed to delete a brush, %d.\n", status);
    }

    for (i = 0; i < ARRAY_SIZE(create_with_angle); ++i)
    {
        ARGB colors[2];
        BOOL ret;

        status = GdipCreateLineBrushFromRectWithAngle(&create_with_angle[i].rect, 0x1, 0x2, create_with_angle[i].angle,
            create_with_angle[i].is_scalable, WrapModeTile, &brush);
        ok(status == Ok, "Failed to create a brush, %d.\n", status);

        memset(&rectf, 0, sizeof(rectf));
        status = GdipGetLineRect(brush, &rectf);
        ok(status == Ok, "Failed to get brush rect, %d.\n", status);
        ok(!memcmp(&rectf, &create_with_angle[i].rect, sizeof(rectf)), "%u: unexpected brush rect {%f,%f,%f,%f}.\n",
            i, rectf.X, rectf.Y, rectf.Width, rectf.Height);

        status = GdipGetLineTransform(brush, transform);
        ok(status == Ok, "Failed to get brush transform, %d.\n", status);

        if (create_with_angle[i].angle == 0.0f)
        {
            status = GdipIsMatrixIdentity(transform, &ret);
            ok(status == Ok, "Unexpected ret value %d.\n", status);
        }
        else
        {
            status = GdipGetMatrixElements(transform, elements);
            ok(status == Ok, "Failed to get matrix elements, %d.\n", status);

#define expectf2(expected, got) ok(fabs(expected - got) < 0.001, "%u: expected %.3f, got %.3f.\n", i, expected, got)
            expectf2(create_with_angle[i].transform[0], elements[0]);
            expectf2(create_with_angle[i].transform[1], elements[1]);
            expectf2(create_with_angle[i].transform[2], elements[2]);
            expectf2(create_with_angle[i].transform[3], elements[3]);
            expectf2(create_with_angle[i].transform[4], elements[4]);
            expectf2(create_with_angle[i].transform[5], elements[5]);
#undef expectf2
        }

        status = GdipGetLineColors(brush, colors);
        ok(status == Ok, "Failed to get line colors, %d.\n", status);
        ok(colors[0] == 0x1 && colors[1] == 0x2, "Unexpected brush colors.\n");

        status = GdipDeleteBrush((GpBrush*)brush);
        ok(status == Ok, "Failed to delete a brush, %d.\n", status);
    }

    GdipDeleteMatrix(transform);
}

static void test_lineblend(void)
{
    GpLineGradient *brush;
    GpStatus status;
    GpPointF pt1, pt2;
    INT count=10;
    int i;
    const REAL factors[5] = {0.0f, 0.1f, 0.5f, 0.9f, 1.0f};
    const REAL positions[5] = {0.0f, 0.2f, 0.5f, 0.8f, 1.0f};
    const REAL two_positions[2] = {0.0f, 1.0f};
    const ARGB colors[5] = {0xff0000ff, 0xff00ff00, 0xff00ffff, 0xffff0000, 0xffffffff};
    REAL res_factors[6] = {0.3f, 0.0f, 0.0f, 0.0f, 0.0f};
    REAL res_positions[6] = {0.3f, 0.0f, 0.0f, 0.0f, 0.0f};
    ARGB res_colors[6] = {0xdeadbeef, 0, 0, 0, 0};

    pt1.X = pt1.Y = pt2.Y = pt2.X = 1.0;
    status = GdipCreateLineBrush(&pt1, &pt2, 0, 0, WrapModeTile, &brush);
    expect(OutOfMemory, status);

    pt1.X = pt1.Y = 1.0;
    pt2.X = pt2.Y = 100.0;
    status = GdipCreateLineBrush(&pt1, &pt2, 0, 0, WrapModeTile, &brush);
    expect(Ok, status);

    status = GdipGetLineBlendCount(NULL, &count);
    expect(InvalidParameter, status);

    status = GdipGetLineBlendCount(brush, NULL);
    expect(InvalidParameter, status);

    status = GdipGetLineBlendCount(brush, &count);
    expect(Ok, status);
    expect(1, count);

    status = GdipGetLineBlend(NULL, res_factors, res_positions, 1);
    expect(InvalidParameter, status);

    status = GdipGetLineBlend(brush, NULL, res_positions, 1);
    expect(InvalidParameter, status);

    status = GdipGetLineBlend(brush, res_factors, NULL, 1);
    expect(InvalidParameter, status);

    status = GdipGetLineBlend(brush, res_factors, res_positions, 0);
    expect(InvalidParameter, status);

    status = GdipGetLineBlend(brush, res_factors, res_positions, -1);
    expect(InvalidParameter, status);

    status = GdipGetLineBlend(brush, res_factors, res_positions, 1);
    expect(Ok, status);

    status = GdipGetLineBlend(brush, res_factors, res_positions, 2);
    expect(Ok, status);

    status = GdipSetLineBlend(NULL, factors, positions, 5);
    expect(InvalidParameter, status);

    status = GdipSetLineBlend(brush, NULL, positions, 5);
    expect(InvalidParameter, status);

    status = GdipSetLineBlend(brush, factors, NULL, 5);
    expect(InvalidParameter, status);

    status = GdipSetLineBlend(brush, factors, positions, 0);
    expect(InvalidParameter, status);

    status = GdipSetLineBlend(brush, factors, positions, -1);
    expect(InvalidParameter, status);

    /* leave off the 0.0 position */
    status = GdipSetLineBlend(brush, &factors[1], &positions[1], 4);
    expect(InvalidParameter, status);

    /* leave off the 1.0 position */
    status = GdipSetLineBlend(brush, factors, positions, 4);
    expect(InvalidParameter, status);

    status = GdipSetLineBlend(brush, factors, positions, 5);
    expect(Ok, status);

    status = GdipGetLineBlendCount(brush, &count);
    expect(Ok, status);
    expect(5, count);

    status = GdipGetLineBlend(brush, res_factors, res_positions, 4);
    expect(InsufficientBuffer, status);

    status = GdipGetLineBlend(brush, res_factors, res_positions, 5);
    expect(Ok, status);

    for (i=0; i<5; i++)
    {
        expectf(factors[i], res_factors[i]);
        expectf(positions[i], res_positions[i]);
    }

    status = GdipGetLineBlend(brush, res_factors, res_positions, 6);
    expect(Ok, status);

    status = GdipSetLineBlend(brush, factors, positions, 1);
    expect(Ok, status);

    status = GdipGetLineBlendCount(brush, &count);
    expect(Ok, status);
    expect(1, count);

    status = GdipGetLineBlend(brush, res_factors, res_positions, 1);
    expect(Ok, status);

    status = GdipGetLinePresetBlendCount(NULL, &count);
    expect(InvalidParameter, status);

    status = GdipGetLinePresetBlendCount(brush, NULL);
    expect(InvalidParameter, status);

    status = GdipGetLinePresetBlendCount(brush, &count);
    expect(Ok, status);
    expect(0, count);

    status = GdipGetLinePresetBlend(NULL, res_colors, res_positions, 1);
    expect(InvalidParameter, status);

    status = GdipGetLinePresetBlend(brush, NULL, res_positions, 1);
    expect(InvalidParameter, status);

    status = GdipGetLinePresetBlend(brush, res_colors, NULL, 1);
    expect(InvalidParameter, status);

    status = GdipGetLinePresetBlend(brush, res_colors, res_positions, 0);
    expect(InvalidParameter, status);

    status = GdipGetLinePresetBlend(brush, res_colors, res_positions, -1);
    expect(InvalidParameter, status);

    status = GdipGetLinePresetBlend(brush, res_colors, res_positions, 1);
    expect(InvalidParameter, status);

    status = GdipGetLinePresetBlend(brush, res_colors, res_positions, 2);
    expect(GenericError, status);

    status = GdipSetLinePresetBlend(NULL, colors, positions, 5);
    expect(InvalidParameter, status);

    status = GdipSetLinePresetBlend(brush, NULL, positions, 5);
    expect(InvalidParameter, status);

    status = GdipSetLinePresetBlend(brush, colors, NULL, 5);
    expect(InvalidParameter, status);

    status = GdipSetLinePresetBlend(brush, colors, positions, 0);
    expect(InvalidParameter, status);

    status = GdipSetLinePresetBlend(brush, colors, positions, -1);
    expect(InvalidParameter, status);

    status = GdipSetLinePresetBlend(brush, colors, positions, 1);
    expect(InvalidParameter, status);

    /* leave off the 0.0 position */
    status = GdipSetLinePresetBlend(brush, &colors[1], &positions[1], 4);
    expect(InvalidParameter, status);

    /* leave off the 1.0 position */
    status = GdipSetLinePresetBlend(brush, colors, positions, 4);
    expect(InvalidParameter, status);

    status = GdipSetLinePresetBlend(brush, colors, positions, 5);
    expect(Ok, status);

    status = GdipGetLinePresetBlendCount(brush, &count);
    expect(Ok, status);
    expect(5, count);

    status = GdipGetLinePresetBlend(brush, res_colors, res_positions, 4);
    expect(InsufficientBuffer, status);

    status = GdipGetLinePresetBlend(brush, res_colors, res_positions, 5);
    expect(Ok, status);

    for (i=0; i<5; i++)
    {
        expect(colors[i], res_colors[i]);
        expectf(positions[i], res_positions[i]);
    }

    status = GdipGetLinePresetBlend(brush, res_colors, res_positions, 6);
    expect(Ok, status);

    status = GdipSetLinePresetBlend(brush, colors, two_positions, 2);
    expect(Ok, status);

    status = GdipDeleteBrush((GpBrush*)brush);
    expect(Ok, status);
}

static void test_linelinearblend(void)
{
    GpLineGradient *brush;
    GpStatus status;
    GpPointF pt1, pt2;
    INT count=10;
    REAL res_factors[3] = {0.3f};
    REAL res_positions[3] = {0.3f};

    status = GdipSetLineLinearBlend(NULL, 0.6, 0.8);
    expect(InvalidParameter, status);

    pt1.X = pt1.Y = 1.0;
    pt2.X = pt2.Y = 100.0;
    status = GdipCreateLineBrush(&pt1, &pt2, 0, 0, WrapModeTile, &brush);
    expect(Ok, status);


    status = GdipSetLineLinearBlend(brush, 0.6, 0.8);
    expect(Ok, status);

    status = GdipGetLineBlendCount(brush, &count);
    expect(Ok, status);
    expect(3, count);

    status = GdipGetLineBlend(brush, res_factors, res_positions, 3);
    expect(Ok, status);
    expectf(0.0, res_factors[0]);
    expectf(0.0, res_positions[0]);
    expectf(0.8, res_factors[1]);
    expectf(0.6, res_positions[1]);
    expectf(0.0, res_factors[2]);
    expectf(1.0, res_positions[2]);


    status = GdipSetLineLinearBlend(brush, 0.0, 0.8);
    expect(Ok, status);

    status = GdipGetLineBlendCount(brush, &count);
    expect(Ok, status);
    expect(2, count);

    status = GdipGetLineBlend(brush, res_factors, res_positions, 3);
    expect(Ok, status);
    expectf(0.8, res_factors[0]);
    expectf(0.0, res_positions[0]);
    expectf(0.0, res_factors[1]);
    expectf(1.0, res_positions[1]);


    status = GdipSetLineLinearBlend(brush, 1.0, 0.8);
    expect(Ok, status);

    status = GdipGetLineBlendCount(brush, &count);
    expect(Ok, status);
    expect(2, count);

    status = GdipGetLineBlend(brush, res_factors, res_positions, 3);
    expect(Ok, status);
    expectf(0.0, res_factors[0]);
    expectf(0.0, res_positions[0]);
    expectf(0.8, res_factors[1]);
    expectf(1.0, res_positions[1]);

    status = GdipDeleteBrush((GpBrush*)brush);
    expect(Ok, status);
}

static void test_gradientsurroundcolorcount(void)
{
    GpStatus status;
    GpPathGradient *grad;
    ARGB color[3];
    INT count;

    status = GdipCreatePathGradient(blendcount_ptf, 2, WrapModeClamp, &grad);
    expect(Ok, status);

    count = 0;
    status = GdipGetPathGradientSurroundColorCount(grad, &count);
    expect(Ok, status);
    expect(2, count);

    color[0] = color[1] = color[2] = 0xdeadbeef;
    count = 3;
    status = GdipGetPathGradientSurroundColorsWithCount(grad, color, &count);
    expect(Ok, status);
    expect(1, count);
    expect(0xffffffff, color[0]);
    expect(0xffffffff, color[1]);
    expect(0xdeadbeef, color[2]);

    color[0] = color[1] = color[2] = 0xdeadbeef;
    count = 2;
    status = GdipGetPathGradientSurroundColorsWithCount(grad, color, &count);
    expect(Ok, status);
    expect(1, count);
    expect(0xffffffff, color[0]);
    expect(0xffffffff, color[1]);
    expect(0xdeadbeef, color[2]);

    color[0] = color[1] = color[2] = 0xdeadbeef;
    count = 1;
    status = GdipGetPathGradientSurroundColorsWithCount(grad, color, &count);
    expect(InvalidParameter, status);
    expect(1, count);
    expect(0xdeadbeef, color[0]);
    expect(0xdeadbeef, color[1]);
    expect(0xdeadbeef, color[2]);

    color[0] = color[1] = color[2] = 0xdeadbeef;
    count = 0;
    status = GdipGetPathGradientSurroundColorsWithCount(grad, color, &count);
    expect(InvalidParameter, status);
    expect(0, count);
    expect(0xdeadbeef, color[0]);
    expect(0xdeadbeef, color[1]);
    expect(0xdeadbeef, color[2]);

    count = 3;
    status = GdipSetPathGradientSurroundColorsWithCount(grad, color, &count);
    expect(InvalidParameter, status);

    count = 2;

    color[0] = 0x00ff0000;
    color[1] = 0x0000ff00;

    status = GdipSetPathGradientSurroundColorsWithCount(NULL, color, &count);
    expect(InvalidParameter, status);

    status = GdipSetPathGradientSurroundColorsWithCount(grad, NULL, &count);
    expect(InvalidParameter, status);

    /* WinXP crashes on this test */
    if(0)
    {
        status = GdipSetPathGradientSurroundColorsWithCount(grad, color, NULL);
        expect(InvalidParameter, status);
    }

    status = GdipSetPathGradientSurroundColorsWithCount(grad, color, &count);
    expect(Ok, status);
    expect(2, count);

    status = GdipGetPathGradientSurroundColorCount(NULL, &count);
    expect(InvalidParameter, status);

    status = GdipGetPathGradientSurroundColorCount(grad, NULL);
    expect(InvalidParameter, status);

    count = 0;
    status = GdipGetPathGradientSurroundColorCount(grad, &count);
    expect(Ok, status);
    expect(2, count);

    color[0] = color[1] = color[2] = 0xdeadbeef;
    count = 2;
    status = GdipGetPathGradientSurroundColorsWithCount(grad, color, &count);
    expect(Ok, status);
    expect(2, count);
    expect(0x00ff0000, color[0]);
    expect(0x0000ff00, color[1]);
    expect(0xdeadbeef, color[2]);

    count = 1;
    status = GdipSetPathGradientSurroundColorsWithCount(grad, color, &count);
    expect(Ok, status);
    expect(1, count);

    count = 0;
    status = GdipGetPathGradientSurroundColorCount(grad, &count);
    expect(Ok, status);
    expect(2, count);

    /* If all colors are the same, count is set to 1. */
    color[0] = color[1] = 0;
    count = 2;
    status = GdipSetPathGradientSurroundColorsWithCount(grad, color, &count);
    expect(Ok, status);
    expect(2, count);

    color[0] = color[1] = color[2] = 0xdeadbeef;
    count = 2;
    status = GdipGetPathGradientSurroundColorsWithCount(grad, color, &count);
    expect(Ok, status);
    expect(1, count);
    expect(0x00000000, color[0]);
    expect(0x00000000, color[1]);
    expect(0xdeadbeef, color[2]);

    color[0] = color[1] = 0xff00ff00;
    count = 2;
    status = GdipSetPathGradientSurroundColorsWithCount(grad, color, &count);
    expect(Ok, status);
    expect(2, count);

    color[0] = color[1] = color[2] = 0xdeadbeef;
    count = 2;
    status = GdipGetPathGradientSurroundColorsWithCount(grad, color, &count);
    expect(Ok, status);
    expect(1, count);
    expect(0xff00ff00, color[0]);
    expect(0xff00ff00, color[1]);
    expect(0xdeadbeef, color[2]);

    count = 0;
    status = GdipSetPathGradientSurroundColorsWithCount(grad, color, &count);
    expect(InvalidParameter, status);
    expect(0, count);

    GdipDeleteBrush((GpBrush*)grad);

    status = GdipCreatePathGradient(getbounds_ptf, 3, WrapModeClamp, &grad);
    expect(Ok, status);

    color[0] = color[1] = color[2] = 0xdeadbeef;
    count = 3;
    status = GdipGetPathGradientSurroundColorsWithCount(grad, color, &count);
    expect(Ok, status);
    expect(1, count);
    expect(0xffffffff, color[0]);
    expect(0xffffffff, color[1]);
    expect(0xffffffff, color[2]);

    color[0] = color[1] = color[2] = 0xdeadbeef;
    count = 2;
    status = GdipGetPathGradientSurroundColorsWithCount(grad, color, &count);
    expect(InvalidParameter, status);
    expect(2, count);
    expect(0xdeadbeef, color[0]);
    expect(0xdeadbeef, color[1]);
    expect(0xdeadbeef, color[2]);

    count = 0;
    status = GdipGetPathGradientSurroundColorCount(grad, &count);
    expect(Ok, status);
    expect(3, count);

    GdipDeleteBrush((GpBrush*)grad);
}

static void test_pathgradientpath(void)
{
    GpStatus status;
    GpPath *path=NULL;
    GpPathGradient *grad=NULL;

    status = GdipCreatePathGradient(blendcount_ptf, 2, WrapModeClamp, &grad);
    expect(Ok, status);

    status = GdipGetPathGradientPath(grad, NULL);
    expect(NotImplemented, status);

    status = GdipCreatePath(FillModeWinding, &path);
    expect(Ok, status);

    status = GdipGetPathGradientPath(NULL, path);
    expect(NotImplemented, status);

    status = GdipGetPathGradientPath(grad, path);
    expect(NotImplemented, status);

    status = GdipDeletePath(path);
    expect(Ok, status);

    status = GdipDeleteBrush((GpBrush*)grad);
    expect(Ok, status);
}

static void test_pathgradientcenterpoint(void)
{
    static const GpPointF path_points[] = {{0,0}, {3,0}, {0,4}};
    GpStatus status;
    GpPathGradient *grad;
    GpPointF point;

    status = GdipCreatePathGradient(path_points+1, 2, WrapModeClamp, &grad);
    expect(Ok, status);

    status = GdipGetPathGradientCenterPoint(NULL, &point);
    expect(InvalidParameter, status);

    status = GdipGetPathGradientCenterPoint(grad, NULL);
    expect(InvalidParameter, status);

    status = GdipGetPathGradientCenterPoint(grad, &point);
    expect(Ok, status);
    expectf(1.5, point.X);
    expectf(2.0, point.Y);

    status = GdipSetPathGradientCenterPoint(NULL, &point);
    expect(InvalidParameter, status);

    status = GdipSetPathGradientCenterPoint(grad, NULL);
    expect(InvalidParameter, status);

    point.X = 10.0;
    point.Y = 15.0;
    status = GdipSetPathGradientCenterPoint(grad, &point);
    expect(Ok, status);

    point.X = point.Y = -1;
    status = GdipGetPathGradientCenterPoint(grad, &point);
    expect(Ok, status);
    expectf(10.0, point.X);
    expectf(15.0, point.Y);

    status = GdipDeleteBrush((GpBrush*)grad);
    expect(Ok, status);

    status = GdipCreatePathGradient(path_points, 3, WrapModeClamp, &grad);
    expect(Ok, status);

    status = GdipGetPathGradientCenterPoint(grad, &point);
    expect(Ok, status);
    todo_wine expectf(1.0, point.X);
    todo_wine expectf(4.0/3.0, point.Y);

    status = GdipDeleteBrush((GpBrush*)grad);
    expect(Ok, status);
}

static void test_pathgradientpresetblend(void)
{
    static const GpPointF path_points[] = {{0,0}, {3,0}, {0,4}};
    GpStatus status;
    GpPathGradient *grad;
    INT count;
    int i;
    const REAL positions[5] = {0.0f, 0.2f, 0.5f, 0.8f, 1.0f};
    const REAL two_positions[2] = {0.0f, 1.0f};
    const ARGB colors[5] = {0xff0000ff, 0xff00ff00, 0xff00ffff, 0xffff0000, 0xffffffff};
    REAL res_positions[6] = {0.3f, 0.0f, 0.0f, 0.0f, 0.0f};
    ARGB res_colors[6] = {0xdeadbeef, 0, 0, 0, 0};

    status = GdipCreatePathGradient(path_points+1, 2, WrapModeClamp, &grad);
    expect(Ok, status);

    status = GdipGetPathGradientPresetBlendCount(NULL, &count);
    expect(InvalidParameter, status);

    status = GdipGetPathGradientPresetBlendCount(grad, NULL);
    expect(InvalidParameter, status);

    status = GdipGetPathGradientPresetBlendCount(grad, &count);
    expect(Ok, status);
    expect(0, count);

    status = GdipGetPathGradientPresetBlend(NULL, res_colors, res_positions, 1);
    expect(InvalidParameter, status);

    status = GdipGetPathGradientPresetBlend(grad, NULL, res_positions, 1);
    expect(InvalidParameter, status);

    status = GdipGetPathGradientPresetBlend(grad, res_colors, NULL, 1);
    expect(InvalidParameter, status);

    status = GdipGetPathGradientPresetBlend(grad, res_colors, res_positions, 0);
    expect(InvalidParameter, status);

    status = GdipGetPathGradientPresetBlend(grad, res_colors, res_positions, -1);
    expect(OutOfMemory, status);

    status = GdipGetPathGradientPresetBlend(grad, res_colors, res_positions, 1);
    expect(InvalidParameter, status);

    status = GdipGetPathGradientPresetBlend(grad, res_colors, res_positions, 2);
    expect(GenericError, status);

    status = GdipSetPathGradientPresetBlend(NULL, colors, positions, 5);
    expect(InvalidParameter, status);

    status = GdipSetPathGradientPresetBlend(grad, NULL, positions, 5);
    expect(InvalidParameter, status);

    if (0)
    {
        /* crashes on windows xp */
        status = GdipSetPathGradientPresetBlend(grad, colors, NULL, 5);
        expect(InvalidParameter, status);
    }

    status = GdipSetPathGradientPresetBlend(grad, colors, positions, 0);
    expect(InvalidParameter, status);

    status = GdipSetPathGradientPresetBlend(grad, colors, positions, -1);
    expect(InvalidParameter, status);

    status = GdipSetPathGradientPresetBlend(grad, colors, positions, 1);
    expect(InvalidParameter, status);

    /* leave off the 0.0 position */
    status = GdipSetPathGradientPresetBlend(grad, &colors[1], &positions[1], 4);
    expect(InvalidParameter, status);

    /* leave off the 1.0 position */
    status = GdipSetPathGradientPresetBlend(grad, colors, positions, 4);
    expect(InvalidParameter, status);

    status = GdipSetPathGradientPresetBlend(grad, colors, positions, 5);
    expect(Ok, status);

    status = GdipGetPathGradientPresetBlendCount(grad, &count);
    expect(Ok, status);
    expect(5, count);

    if (0)
    {
        /* Native GdipGetPathGradientPresetBlend seems to copy starting from
         * the end of each array and do no bounds checking. This is so braindead
         * I'm not going to copy it. */

        res_colors[0] = 0xdeadbeef;
        res_positions[0] = 0.3;

        status = GdipGetPathGradientPresetBlend(grad, &res_colors[1], &res_positions[1], 4);
        expect(Ok, status);

        expect(0xdeadbeef, res_colors[0]);
        expectf(0.3, res_positions[0]);

        for (i=1; i<5; i++)
        {
            expect(colors[i], res_colors[i]);
            expectf(positions[i], res_positions[i]);
        }

        status = GdipGetPathGradientPresetBlend(grad, res_colors, res_positions, 6);
        expect(Ok, status);

        for (i=0; i<5; i++)
        {
            expect(colors[i], res_colors[i+1]);
            expectf(positions[i], res_positions[i+1]);
        }
    }

    status = GdipGetPathGradientPresetBlend(grad, res_colors, res_positions, 5);
    expect(Ok, status);

    for (i=0; i<5; i++)
    {
        expect(colors[i], res_colors[i]);
        expectf(positions[i], res_positions[i]);
    }

    status = GdipGetPathGradientPresetBlend(grad, res_colors, res_positions, 0);
    expect(InvalidParameter, status);

    status = GdipGetPathGradientPresetBlend(grad, res_colors, res_positions, -1);
    expect(OutOfMemory, status);

    status = GdipGetPathGradientPresetBlend(grad, res_colors, res_positions, 1);
    expect(InvalidParameter, status);

    status = GdipSetPathGradientPresetBlend(grad, colors, two_positions, 2);
    expect(Ok, status);

    status = GdipDeleteBrush((GpBrush*)grad);
    expect(Ok, status);
}

static void test_pathgradientblend(void)
{
    static const GpPointF path_points[] = {{0,0}, {3,0}, {0,4}};
    GpPathGradient *brush;
    GpStatus status;
    INT count, i;
    const REAL factors[5] = {0.0f, 0.1f, 0.5f, 0.9f, 1.0f};
    const REAL positions[5] = {0.0f, 0.2f, 0.5f, 0.8f, 1.0f};
    REAL res_factors[6] = {0.3f, 0.0f, 0.0f, 0.0f, 0.0f};
    REAL res_positions[6] = {0.3f, 0.0f, 0.0f, 0.0f, 0.0f};

    status = GdipCreatePathGradient(path_points, 3, WrapModeClamp, &brush);
    expect(Ok, status);

    status = GdipGetPathGradientBlendCount(NULL, &count);
    expect(InvalidParameter, status);

    status = GdipGetPathGradientBlendCount(brush, NULL);
    expect(InvalidParameter, status);

    status = GdipGetPathGradientBlendCount(brush, &count);
    expect(Ok, status);
    expect(1, count);

    status = GdipGetPathGradientBlend(NULL, res_factors, res_positions, 1);
    expect(InvalidParameter, status);

    status = GdipGetPathGradientBlend(brush, NULL, res_positions, 1);
    expect(InvalidParameter, status);

    status = GdipGetPathGradientBlend(brush, res_factors, NULL, 1);
    expect(InvalidParameter, status);

    status = GdipGetPathGradientBlend(brush, res_factors, res_positions, 0);
    expect(InvalidParameter, status);

    status = GdipGetPathGradientBlend(brush, res_factors, res_positions, -1);
    expect(InvalidParameter, status);

    status = GdipGetPathGradientBlend(brush, res_factors, res_positions, 1);
    expect(Ok, status);

    status = GdipGetPathGradientBlend(brush, res_factors, res_positions, 2);
    expect(Ok, status);

    status = GdipSetPathGradientBlend(NULL, factors, positions, 5);
    expect(InvalidParameter, status);

    status = GdipSetPathGradientBlend(brush, NULL, positions, 5);
    expect(InvalidParameter, status);

    status = GdipSetPathGradientBlend(brush, factors, NULL, 5);
    expect(InvalidParameter, status);

    status = GdipSetPathGradientBlend(brush, factors, positions, 0);
    expect(InvalidParameter, status);

    status = GdipSetPathGradientBlend(brush, factors, positions, -1);
    expect(InvalidParameter, status);

    /* leave off the 0.0 position */
    status = GdipSetPathGradientBlend(brush, &factors[1], &positions[1], 4);
    expect(InvalidParameter, status);

    /* leave off the 1.0 position */
    status = GdipSetPathGradientBlend(brush, factors, positions, 4);
    expect(InvalidParameter, status);

    status = GdipSetPathGradientBlend(brush, factors, positions, 5);
    expect(Ok, status);

    status = GdipGetPathGradientBlendCount(brush, &count);
    expect(Ok, status);
    expect(5, count);

    status = GdipGetPathGradientBlend(brush, res_factors, res_positions, 4);
    expect(InsufficientBuffer, status);

    status = GdipGetPathGradientBlend(brush, res_factors, res_positions, 5);
    expect(Ok, status);

    for (i=0; i<5; i++)
    {
        expectf(factors[i], res_factors[i]);
        expectf(positions[i], res_positions[i]);
    }

    status = GdipGetPathGradientBlend(brush, res_factors, res_positions, 6);
    expect(Ok, status);

    status = GdipSetPathGradientBlend(brush, factors, positions, 1);
    expect(Ok, status);

    status = GdipGetPathGradientBlendCount(brush, &count);
    expect(Ok, status);
    expect(1, count);

    status = GdipGetPathGradientBlend(brush, res_factors, res_positions, 1);
    expect(Ok, status);

    status = GdipDeleteBrush((GpBrush*)brush);
    expect(Ok, status);
}

static void test_getHatchStyle(void)
{
    GpStatus status;
    GpHatch *brush;
    GpHatchStyle hatchStyle;

    GdipCreateHatchBrush(HatchStyleHorizontal, 11, 12, &brush);

    status = GdipGetHatchStyle(NULL, &hatchStyle);
    expect(InvalidParameter, status);

    status = GdipGetHatchStyle(brush, NULL);
    expect(InvalidParameter, status);

    status = GdipGetHatchStyle(brush, &hatchStyle);
    expect(Ok, status);
    expect(HatchStyleHorizontal, hatchStyle);

    GdipDeleteBrush((GpBrush *)brush);
}

START_TEST(brush)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    HMODULE hmsvcrt;
    int (CDECL * _controlfp_s)(unsigned int *cur, unsigned int newval, unsigned int mask);
    WNDCLASSA class;

    /* Enable all FP exceptions except _EM_INEXACT, which gdi32 can trigger */
    hmsvcrt = LoadLibraryA("msvcrt");
    _controlfp_s = (void*)GetProcAddress(hmsvcrt, "_controlfp_s");
    if (_controlfp_s) _controlfp_s(0, 0, 0x0008001e);

    memset( &class, 0, sizeof(class) );
    class.lpszClassName = "gdiplus_test";
    class.style = CS_HREDRAW | CS_VREDRAW;
    class.lpfnWndProc = DefWindowProcA;
    class.hInstance = GetModuleHandleA(0);
    class.hIcon = LoadIconA(0, (LPCSTR)IDI_APPLICATION);
    class.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassA( &class );
    hwnd = CreateWindowA( "gdiplus_test", "graphics test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, 0, 0, GetModuleHandleA(0), 0 );
    ok(hwnd != NULL, "Expected window to be created\n");

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_constructor_destructor();
    test_createHatchBrush();
    test_createLineBrushFromRectWithAngle();
    test_type();
    test_gradientblendcount();
    test_getblend();
    test_getbounds();
    test_getgamma();
    test_transform();
    test_texturewrap();
    test_gradientgetrect();
    test_lineblend();
    test_linelinearblend();
    test_gradientsurroundcolorcount();
    test_pathgradientpath();
    test_pathgradientcenterpoint();
    test_pathgradientpresetblend();
    test_pathgradientblend();
    test_getHatchStyle();

    GdiplusShutdown(gdiplusToken);
    DestroyWindow(hwnd);
}
