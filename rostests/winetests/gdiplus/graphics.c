/*
 * Unit test suite for graphics objects
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
#include "wingdi.h"
#include "wine/test.h"

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)
#define TABLE_LEN (23)

static void test_constructor_destructor(void)
{
    GpStatus stat;
    GpGraphics *graphics = NULL;
    HDC hdc = GetDC(0);

    stat = GdipCreateFromHDC(NULL, &graphics);
    expect(OutOfMemory, stat);
    stat = GdipDeleteGraphics(graphics);
    expect(InvalidParameter, stat);

    stat = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, stat);
    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    stat = GdipCreateFromHWND(NULL, &graphics);
    expect(Ok, stat);
    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    stat = GdipCreateFromHWNDICM(NULL, &graphics);
    expect(Ok, stat);
    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(NULL);
    expect(InvalidParameter, stat);
    ReleaseDC(0, hdc);
}

typedef struct node{
    GraphicsState data;
    struct node * next;
} node;

/* Linked list prepend function. */
static void log_state(GraphicsState data, node ** log)
{
    node * new_entry = HeapAlloc(GetProcessHeap(), 0, sizeof(node));

    new_entry->data = data;
    new_entry->next = *log;
    *log = new_entry;
}

/* Checks if there are duplicates in the list, and frees it. */
static void check_no_duplicates(node * log)
{
    INT dups = 0;
    node * temp = NULL;
    node * temp2 = NULL;
    node * orig = log;

    if(!log)
        goto end;

    do{
        temp = log;
        while((temp = temp->next)){
            if(log->data == temp->data){
                dups++;
                break;
            }
            if(dups > 0)
                break;
        }
    }while((log = log->next));

    temp = orig;
    do{
        temp2 = temp->next;
        HeapFree(GetProcessHeap(), 0, temp);
        temp = temp2;
    }while(temp);

end:
    expect(0, dups);
}

static void test_save_restore(void)
{
    GpStatus stat;
    GraphicsState state_a, state_b, state_c;
    InterpolationMode mode;
    GpGraphics *graphics1, *graphics2;
    node * state_log = NULL;
    HDC hdc = GetDC(0);
    state_a = state_b = state_c = 0xdeadbeef;

    /* Invalid saving. */
    GdipCreateFromHDC(hdc, &graphics1);
    stat = GdipSaveGraphics(graphics1, NULL);
    expect(InvalidParameter, stat);
    stat = GdipSaveGraphics(NULL, &state_a);
    expect(InvalidParameter, stat);
    GdipDeleteGraphics(graphics1);

    log_state(state_a, &state_log);

    /* Basic save/restore. */
    GdipCreateFromHDC(hdc, &graphics1);
    GdipSetInterpolationMode(graphics1, InterpolationModeBilinear);
    stat = GdipSaveGraphics(graphics1, &state_a);
    expect(Ok, stat);
    GdipSetInterpolationMode(graphics1, InterpolationModeBicubic);
    stat = GdipRestoreGraphics(graphics1, state_a);
    expect(Ok, stat);
    GdipGetInterpolationMode(graphics1, &mode);
    todo_wine
        expect(InterpolationModeBilinear, mode);
    GdipDeleteGraphics(graphics1);

    log_state(state_a, &state_log);

    /* Restoring garbage doesn't affect saves. */
    GdipCreateFromHDC(hdc, &graphics1);
    GdipSetInterpolationMode(graphics1, InterpolationModeBilinear);
    GdipSaveGraphics(graphics1, &state_a);
    GdipSetInterpolationMode(graphics1, InterpolationModeBicubic);
    GdipSaveGraphics(graphics1, &state_b);
    GdipSetInterpolationMode(graphics1, InterpolationModeNearestNeighbor);
    stat = GdipRestoreGraphics(graphics1, 0xdeadbeef);
    expect(Ok, stat);
    GdipRestoreGraphics(graphics1, state_b);
    GdipGetInterpolationMode(graphics1, &mode);
    todo_wine
        expect(InterpolationModeBicubic, mode);
    GdipRestoreGraphics(graphics1, state_a);
    GdipGetInterpolationMode(graphics1, &mode);
    todo_wine
        expect(InterpolationModeBilinear, mode);
    GdipDeleteGraphics(graphics1);

    log_state(state_a, &state_log);
    log_state(state_b, &state_log);

    /* Restoring older state invalidates newer saves (but not older saves). */
    GdipCreateFromHDC(hdc, &graphics1);
    GdipSetInterpolationMode(graphics1, InterpolationModeBilinear);
    GdipSaveGraphics(graphics1, &state_a);
    GdipSetInterpolationMode(graphics1, InterpolationModeBicubic);
    GdipSaveGraphics(graphics1, &state_b);
    GdipSetInterpolationMode(graphics1, InterpolationModeNearestNeighbor);
    GdipSaveGraphics(graphics1, &state_c);
    GdipSetInterpolationMode(graphics1, InterpolationModeHighQualityBilinear);
    GdipRestoreGraphics(graphics1, state_b);
    GdipGetInterpolationMode(graphics1, &mode);
    todo_wine
        expect(InterpolationModeBicubic, mode);
    GdipRestoreGraphics(graphics1, state_c);
    GdipGetInterpolationMode(graphics1, &mode);
    todo_wine
        expect(InterpolationModeBicubic, mode);
    GdipRestoreGraphics(graphics1, state_a);
    GdipGetInterpolationMode(graphics1, &mode);
    todo_wine
        expect(InterpolationModeBilinear, mode);
    GdipDeleteGraphics(graphics1);

    log_state(state_a, &state_log);
    log_state(state_b, &state_log);
    log_state(state_c, &state_log);

    /* Restoring older save from one graphics object does not invalidate
     * newer save from other graphics object. */
    GdipCreateFromHDC(hdc, &graphics1);
    GdipCreateFromHDC(hdc, &graphics2);
    GdipSetInterpolationMode(graphics1, InterpolationModeBilinear);
    GdipSaveGraphics(graphics1, &state_a);
    GdipSetInterpolationMode(graphics2, InterpolationModeBicubic);
    GdipSaveGraphics(graphics2, &state_b);
    GdipSetInterpolationMode(graphics1, InterpolationModeNearestNeighbor);
    GdipSetInterpolationMode(graphics2, InterpolationModeNearestNeighbor);
    GdipRestoreGraphics(graphics1, state_a);
    GdipGetInterpolationMode(graphics1, &mode);
    todo_wine
        expect(InterpolationModeBilinear, mode);
    GdipRestoreGraphics(graphics2, state_b);
    GdipGetInterpolationMode(graphics2, &mode);
    todo_wine
        expect(InterpolationModeBicubic, mode);
    GdipDeleteGraphics(graphics1);
    GdipDeleteGraphics(graphics2);

    /* You can't restore a state to a graphics object that didn't save it. */
    GdipCreateFromHDC(hdc, &graphics1);
    GdipCreateFromHDC(hdc, &graphics2);
    GdipSetInterpolationMode(graphics1, InterpolationModeBilinear);
    GdipSaveGraphics(graphics1, &state_a);
    GdipSetInterpolationMode(graphics1, InterpolationModeNearestNeighbor);
    GdipSetInterpolationMode(graphics2, InterpolationModeNearestNeighbor);
    GdipRestoreGraphics(graphics2, state_a);
    GdipGetInterpolationMode(graphics2, &mode);
    expect(InterpolationModeNearestNeighbor, mode);
    GdipDeleteGraphics(graphics1);
    GdipDeleteGraphics(graphics2);

    log_state(state_a, &state_log);

    /* The same state value should never be returned twice. */
    todo_wine
        check_no_duplicates(state_log);

    ReleaseDC(0, hdc);
}

static void test_GdipDrawArc(void)
{
    GpStatus status;
    GpGraphics *graphics = NULL;
    GpPen *pen = NULL;
    HDC hdc = GetDC(0);

    /* make a graphics object and pen object */
    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);
    ok(hdc != NULL, "Expected HDC to be initialized\n");

    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);
    ok(graphics != NULL, "Expected graphics to be initialized\n");

    status = GdipCreatePen1((ARGB)0xffff00ff, 10.0f, UnitPixel, &pen);
    expect(Ok, status);
    ok(pen != NULL, "Expected pen to be initialized\n");

    /* InvalidParameter cases: null graphics, null pen, non-positive width, non-positive height */
    status = GdipDrawArc(NULL, NULL, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    expect(InvalidParameter, status);

    status = GdipDrawArc(graphics, NULL, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0);
    expect(InvalidParameter, status);

    status = GdipDrawArc(NULL, pen, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0);
    expect(InvalidParameter, status);

    status = GdipDrawArc(graphics, pen, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0);
    expect(InvalidParameter, status);

    status = GdipDrawArc(graphics, pen, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0);
    expect(InvalidParameter, status);

    /* successful case */
    status = GdipDrawArc(graphics, pen, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0);
    expect(Ok, status);

    GdipDeletePen(pen);
    GdipDeleteGraphics(graphics);

    ReleaseDC(0, hdc);
}

static void test_GdipDrawArcI(void)
{
    GpStatus status;
    GpGraphics *graphics = NULL;
    GpPen *pen = NULL;
    HDC hdc = GetDC(0);

    /* make a graphics object and pen object */
    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);
    ok(hdc != NULL, "Expected HDC to be initialized\n");

    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);
    ok(graphics != NULL, "Expected graphics to be initialized\n");

    status = GdipCreatePen1((ARGB)0xffff00ff, 10.0f, UnitPixel, &pen);
    expect(Ok, status);
    ok(pen != NULL, "Expected pen to be initialized\n");

    /* InvalidParameter cases: null graphics, null pen, non-positive width, non-positive height */
    status = GdipDrawArcI(NULL, NULL, 0, 0, 0, 0, 0, 0);
    expect(InvalidParameter, status);

    status = GdipDrawArcI(graphics, NULL, 0, 0, 1, 1, 0, 0);
    expect(InvalidParameter, status);

    status = GdipDrawArcI(NULL, pen, 0, 0, 1, 1, 0, 0);
    expect(InvalidParameter, status);

    status = GdipDrawArcI(graphics, pen, 0, 0, 1, 0, 0, 0);
    expect(InvalidParameter, status);

    status = GdipDrawArcI(graphics, pen, 0, 0, 0, 1, 0, 0);
    expect(InvalidParameter, status);

    /* successful case */
    status = GdipDrawArcI(graphics, pen, 0, 0, 1, 1, 0, 0);
    expect(Ok, status);

    GdipDeletePen(pen);
    GdipDeleteGraphics(graphics);

    ReleaseDC(0, hdc);
}

static void test_GdipDrawBezierI(void)
{
    GpStatus status;
    GpGraphics *graphics = NULL;
    GpPen *pen = NULL;
    HDC hdc = GetDC(0);

    /* make a graphics object and pen object */
    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);
    ok(hdc != NULL, "Expected HDC to be initialized\n");

    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);
    ok(graphics != NULL, "Expected graphics to be initialized\n");

    status = GdipCreatePen1((ARGB)0xffff00ff, 10.0f, UnitPixel, &pen);
    expect(Ok, status);
    ok(pen != NULL, "Expected pen to be initialized\n");

    /* InvalidParameter cases: null graphics, null pen */
    status = GdipDrawBezierI(NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0);
    expect(InvalidParameter, status);

    status = GdipDrawBezierI(graphics, NULL, 0, 0, 0, 0, 0, 0, 0, 0);
    expect(InvalidParameter, status);

    status = GdipDrawBezierI(NULL, pen, 0, 0, 0, 0, 0, 0, 0, 0);
    expect(InvalidParameter, status);

    /* successful case */
    status = GdipDrawBezierI(graphics, pen, 0, 0, 0, 0, 0, 0, 0, 0);
    expect(Ok, status);

    GdipDeletePen(pen);
    GdipDeleteGraphics(graphics);

    ReleaseDC(0, hdc);
}

static void test_GdipDrawLineI(void)
{
    GpStatus status;
    GpGraphics *graphics = NULL;
    GpPen *pen = NULL;
    HDC hdc = GetDC(0);

    /* make a graphics object and pen object */
    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);
    ok(hdc != NULL, "Expected HDC to be initialized\n");

    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);
    ok(graphics != NULL, "Expected graphics to be initialized\n");

    status = GdipCreatePen1((ARGB)0xffff00ff, 10.0f, UnitPixel, &pen);
    expect(Ok, status);
    ok(pen != NULL, "Expected pen to be initialized\n");

    /* InvalidParameter cases: null graphics, null pen */
    status = GdipDrawLineI(NULL, NULL, 0, 0, 0, 0);
    expect(InvalidParameter, status);

    status = GdipDrawLineI(graphics, NULL, 0, 0, 0, 0);
    expect(InvalidParameter, status);

    status = GdipDrawLineI(NULL, pen, 0, 0, 0, 0);
    expect(InvalidParameter, status);

    /* successful case */
    status = GdipDrawLineI(graphics, pen, 0, 0, 0, 0);
    expect(Ok, status);

    GdipDeletePen(pen);
    GdipDeleteGraphics(graphics);

    ReleaseDC(0, hdc);
}

static void test_GdipDrawLinesI(void)
{
    GpStatus status;
    GpGraphics *graphics = NULL;
    GpPen *pen = NULL;
    GpPoint *ptf = NULL;
    HDC hdc = GetDC(0);

    /* make a graphics object and pen object */
    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);
    ok(hdc != NULL, "Expected HDC to be initialized\n");

    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);
    ok(graphics != NULL, "Expected graphics to be initialized\n");

    status = GdipCreatePen1((ARGB)0xffff00ff, 10.0f, UnitPixel, &pen);
    expect(Ok, status);
    ok(pen != NULL, "Expected pen to be initialized\n");

    /* make some arbitrary valid points*/
    ptf = GdipAlloc(2 * sizeof(GpPointF));

    ptf[0].X = 1;
    ptf[0].Y = 1;

    ptf[1].X = 2;
    ptf[1].Y = 2;

    /* InvalidParameter cases: null graphics, null pen, null points, count < 2*/
    status = GdipDrawLinesI(NULL, NULL, NULL, 0);
    expect(InvalidParameter, status);

    status = GdipDrawLinesI(graphics, pen, ptf, 0);
    expect(InvalidParameter, status);

    status = GdipDrawLinesI(graphics, NULL, ptf, 2);
    expect(InvalidParameter, status);

    status = GdipDrawLinesI(NULL, pen, ptf, 2);
    expect(InvalidParameter, status);

    /* successful case */
    status = GdipDrawLinesI(graphics, pen, ptf, 2);
    expect(Ok, status);

    GdipFree(ptf);
    GdipDeletePen(pen);
    GdipDeleteGraphics(graphics);

    ReleaseDC(0, hdc);
}

static void test_Get_Release_DC(void)
{
    GpStatus status;
    GpGraphics *graphics = NULL;
    GpPen *pen;
    GpSolidFill *brush;
    GpPath *path;
    HDC hdc = GetDC(0);
    HDC retdc;
    REAL r;
    CompositingQuality quality;
    CompositingMode compmode;
    InterpolationMode intmode;
    GpMatrix *m;
    GpRegion *region;
    GpUnit unit;
    PixelOffsetMode offsetmode;
    SmoothingMode smoothmode;
    TextRenderingHint texthint;
    GpPointF ptf[5];
    GpPoint  pt[5];
    GpRectF  rectf[2];
    GpRect   rect[2];
    GpRegion *clip;
    INT i;
    BOOL res;
    ARGB color = 0x00000000;
    HRGN hrgn = CreateRectRgn(0, 0, 10, 10);

    pt[0].X = 10;
    pt[0].Y = 10;
    pt[1].X = 20;
    pt[1].Y = 15;
    pt[2].X = 40;
    pt[2].Y = 80;
    pt[3].X = -20;
    pt[3].Y = 20;
    pt[4].X = 50;
    pt[4].Y = 110;

    for(i = 0; i < 5;i++){
        ptf[i].X = (REAL)pt[i].X;
        ptf[i].Y = (REAL)pt[i].Y;
    }

    rect[0].X = 0;
    rect[0].Y = 0;
    rect[0].Width  = 50;
    rect[0].Height = 70;
    rect[1].X = 0;
    rect[1].Y = 0;
    rect[1].Width  = 10;
    rect[1].Height = 20;

    for(i = 0; i < 2;i++){
        rectf[i].X = (REAL)rect[i].X;
        rectf[i].Y = (REAL)rect[i].Y;
        rectf[i].Height = (REAL)rect[i].Height;
        rectf[i].Width  = (REAL)rect[i].Width;
    }

    GdipCreateMatrix(&m);
    GdipCreateRegion(&region);
    GdipCreateSolidFill((ARGB)0xdeadbeef, &brush);
    GdipCreatePath(FillModeAlternate, &path);
    GdipCreateRegion(&clip);

    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);
    ok(graphics != NULL, "Expected graphics to be initialized\n");
    status = GdipCreatePen1((ARGB)0xffff00ff, 10.0f, UnitPixel, &pen);
    expect(Ok, status);

    /* NULL arguments */
    status = GdipGetDC(NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipGetDC(graphics, NULL);
    expect(InvalidParameter, status);
    status = GdipGetDC(NULL, &retdc);
    expect(InvalidParameter, status);

    status = GdipReleaseDC(NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipReleaseDC(graphics, NULL);
    expect(InvalidParameter, status);
    status = GdipReleaseDC(NULL, (HDC)0xdeadbeef);
    expect(InvalidParameter, status);

    /* Release without Get */
    status = GdipReleaseDC(graphics, hdc);
    expect(InvalidParameter, status);

    retdc = NULL;
    status = GdipGetDC(graphics, &retdc);
    expect(Ok, status);
    ok(retdc == hdc, "Invalid HDC returned\n");
    /* call it once more */
    status = GdipGetDC(graphics, &retdc);
    expect(ObjectBusy, status);

    /* try all Graphics calls here */
    status = Ok;
    status = GdipDrawArc(graphics, pen, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawArcI(graphics, pen, 0, 0, 1, 1, 0.0, 0.0);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawBezier(graphics, pen, 0.0, 10.0, 20.0, 15.0, 35.0, -10.0, 10.0, 10.0);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawBezierI(graphics, pen, 0, 0, 0, 0, 0, 0, 0, 0);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawBeziers(graphics, pen, ptf, 5);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawBeziersI(graphics, pen, pt, 5);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawClosedCurve(graphics, pen, ptf, 5);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawClosedCurveI(graphics, pen, pt, 5);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawClosedCurve2(graphics, pen, ptf, 5, 1.0);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawClosedCurve2I(graphics, pen, pt, 5, 1.0);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawCurve(graphics, pen, ptf, 5);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawCurveI(graphics, pen, pt, 5);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawCurve2(graphics, pen, ptf, 5, 1.0);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawCurve2I(graphics, pen, pt, 5, 1.0);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawEllipse(graphics, pen, 0.0, 0.0, 100.0, 50.0);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawEllipseI(graphics, pen, 0, 0, 100, 50);
    expect(ObjectBusy, status); status = Ok;
    /* GdipDrawImage/GdipDrawImageI */
    /* GdipDrawImagePointsRect/GdipDrawImagePointsRectI */
    /* GdipDrawImageRectRect/GdipDrawImageRectRectI */
    /* GdipDrawImageRect/GdipDrawImageRectI */
    status = GdipDrawLine(graphics, pen, 0.0, 0.0, 100.0, 200.0);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawLineI(graphics, pen, 0, 0, 100, 200);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawLines(graphics, pen, ptf, 5);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawLinesI(graphics, pen, pt, 5);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawPath(graphics, pen, path);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawPie(graphics, pen, 0.0, 0.0, 100.0, 100.0, 0.0, 90.0);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawPieI(graphics, pen, 0, 0, 100, 100, 0.0, 90.0);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawRectangle(graphics, pen, 0.0, 0.0, 100.0, 300.0);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawRectangleI(graphics, pen, 0, 0, 100, 300);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawRectangles(graphics, pen, rectf, 2);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawRectanglesI(graphics, pen, rect, 2);
    expect(ObjectBusy, status); status = Ok;
    /* GdipDrawString */
    status = GdipFillClosedCurve2(graphics, (GpBrush*)brush, ptf, 5, 1.0, FillModeAlternate);
    expect(ObjectBusy, status); status = Ok;
    status = GdipFillClosedCurve2I(graphics, (GpBrush*)brush, pt, 5, 1.0, FillModeAlternate);
    expect(ObjectBusy, status); status = Ok;
    status = GdipFillEllipse(graphics, (GpBrush*)brush, 0.0, 0.0, 100.0, 100.0);
    expect(ObjectBusy, status); status = Ok;
    status = GdipFillEllipseI(graphics, (GpBrush*)brush, 0, 0, 100, 100);
    expect(ObjectBusy, status); status = Ok;
    status = GdipFillPath(graphics, (GpBrush*)brush, path);
    expect(ObjectBusy, status); status = Ok;
    status = GdipFillPie(graphics, (GpBrush*)brush, 0.0, 0.0, 100.0, 100.0, 0.0, 15.0);
    expect(ObjectBusy, status); status = Ok;
    status = GdipFillPieI(graphics, (GpBrush*)brush, 0, 0, 100, 100, 0.0, 15.0);
    expect(ObjectBusy, status); status = Ok;
    status = GdipFillPolygon(graphics, (GpBrush*)brush, ptf, 5, FillModeAlternate);
    expect(ObjectBusy, status); status = Ok;
    status = GdipFillPolygonI(graphics, (GpBrush*)brush, pt, 5, FillModeAlternate);
    expect(ObjectBusy, status); status = Ok;
    status = GdipFillPolygon2(graphics, (GpBrush*)brush, ptf, 5);
    expect(ObjectBusy, status); status = Ok;
    status = GdipFillPolygon2I(graphics, (GpBrush*)brush, pt, 5);
    expect(ObjectBusy, status); status = Ok;
    status = GdipFillRectangle(graphics, (GpBrush*)brush, 0.0, 0.0, 100.0, 100.0);
    expect(ObjectBusy, status); status = Ok;
    status = GdipFillRectangleI(graphics, (GpBrush*)brush, 0, 0, 100, 100);
    expect(ObjectBusy, status); status = Ok;
    status = GdipFillRectangles(graphics, (GpBrush*)brush, rectf, 2);
    expect(ObjectBusy, status); status = Ok;
    status = GdipFillRectanglesI(graphics, (GpBrush*)brush, rect, 2);
    expect(ObjectBusy, status); status = Ok;
    status = GdipFillRegion(graphics, (GpBrush*)brush, region);
    expect(ObjectBusy, status); status = Ok;
    status = GdipFlush(graphics, FlushIntentionFlush);
    expect(ObjectBusy, status); status = Ok;
    status = GdipGetClipBounds(graphics, rectf);
    expect(ObjectBusy, status); status = Ok;
    status = GdipGetClipBoundsI(graphics, rect);
    expect(ObjectBusy, status); status = Ok;
    status = GdipGetCompositingMode(graphics, &compmode);
    expect(ObjectBusy, status); status = Ok;
    status = GdipGetCompositingQuality(graphics, &quality);
    expect(ObjectBusy, status); status = Ok;
    status = GdipGetInterpolationMode(graphics, &intmode);
    expect(ObjectBusy, status); status = Ok;
    status = GdipGetNearestColor(graphics, &color);
    expect(ObjectBusy, status); status = Ok;
    status = GdipGetPageScale(graphics, &r);
    expect(ObjectBusy, status); status = Ok;
    status = GdipGetPageUnit(graphics, &unit);
    expect(ObjectBusy, status); status = Ok;
    status = GdipGetPixelOffsetMode(graphics, &offsetmode);
    expect(ObjectBusy, status); status = Ok;
    status = GdipGetSmoothingMode(graphics, &smoothmode);
    expect(ObjectBusy, status); status = Ok;
    status = GdipGetTextRenderingHint(graphics, &texthint);
    expect(ObjectBusy, status); status = Ok;
    status = GdipGetWorldTransform(graphics, m);
    expect(ObjectBusy, status); status = Ok;
    status = GdipGraphicsClear(graphics, 0xdeadbeef);
    expect(ObjectBusy, status); status = Ok;
    status = GdipIsVisiblePoint(graphics, 0.0, 0.0, &res);
    expect(ObjectBusy, status); status = Ok;
    status = GdipIsVisiblePointI(graphics, 0, 0, &res);
    expect(ObjectBusy, status); status = Ok;
    /* GdipMeasureCharacterRanges */
    /* GdipMeasureString */
    status = GdipResetClip(graphics);
    expect(ObjectBusy, status); status = Ok;
    status = GdipResetWorldTransform(graphics);
    expect(ObjectBusy, status); status = Ok;
    /* GdipRestoreGraphics */
    status = GdipRotateWorldTransform(graphics, 15.0, MatrixOrderPrepend);
    expect(ObjectBusy, status); status = Ok;
    /*  GdipSaveGraphics */
    status = GdipScaleWorldTransform(graphics, 1.0, 1.0, MatrixOrderPrepend);
    expect(ObjectBusy, status); status = Ok;
    status = GdipSetCompositingMode(graphics, CompositingModeSourceOver);
    expect(ObjectBusy, status); status = Ok;
    status = GdipSetCompositingQuality(graphics, CompositingQualityDefault);
    expect(ObjectBusy, status); status = Ok;
    status = GdipSetInterpolationMode(graphics, InterpolationModeDefault);
    expect(ObjectBusy, status); status = Ok;
    status = GdipSetPageScale(graphics, 1.0);
    expect(ObjectBusy, status); status = Ok;
    status = GdipSetPageUnit(graphics, UnitWorld);
    expect(ObjectBusy, status); status = Ok;
    status = GdipSetPixelOffsetMode(graphics, PixelOffsetModeDefault);
    expect(ObjectBusy, status); status = Ok;
    status = GdipSetSmoothingMode(graphics, SmoothingModeDefault);
    expect(ObjectBusy, status); status = Ok;
    status = GdipSetTextRenderingHint(graphics, TextRenderingHintSystemDefault);
    expect(ObjectBusy, status); status = Ok;
    status = GdipSetWorldTransform(graphics, m);
    expect(ObjectBusy, status); status = Ok;
    status = GdipTranslateWorldTransform(graphics, 0.0, 0.0, MatrixOrderPrepend);
    expect(ObjectBusy, status); status = Ok;
    status = GdipSetClipHrgn(graphics, hrgn, CombineModeReplace);
    expect(ObjectBusy, status); status = Ok;
    status = GdipSetClipPath(graphics, path, CombineModeReplace);
    expect(ObjectBusy, status); status = Ok;
    status = GdipSetClipRect(graphics, 0.0, 0.0, 10.0, 10.0, CombineModeReplace);
    expect(ObjectBusy, status); status = Ok;
    status = GdipSetClipRectI(graphics, 0, 0, 10, 10, CombineModeReplace);
    expect(ObjectBusy, status); status = Ok;
    status = GdipSetClipRegion(graphics, clip, CombineModeReplace);
    expect(ObjectBusy, status); status = Ok;
    status = GdipTranslateClip(graphics, 0.0, 0.0);
    expect(ObjectBusy, status); status = Ok;
    status = GdipTranslateClipI(graphics, 0, 0);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawPolygon(graphics, pen, ptf, 5);
    expect(ObjectBusy, status); status = Ok;
    status = GdipDrawPolygonI(graphics, pen, pt, 5);
    expect(ObjectBusy, status); status = Ok;
    status = GdipGetDpiX(graphics, &r);
    expect(ObjectBusy, status); status = Ok;
    status = GdipGetDpiY(graphics, &r);
    expect(ObjectBusy, status); status = Ok;
    status = GdipMultiplyWorldTransform(graphics, m, MatrixOrderPrepend);
    status = GdipGetClip(graphics, region);
    expect(ObjectBusy, status); status = Ok;
    status = GdipTransformPoints(graphics, CoordinateSpacePage, CoordinateSpaceWorld, ptf, 5);
    expect(ObjectBusy, status); status = Ok;
    /* try to delete before release */
    status = GdipDeleteGraphics(graphics);
    expect(ObjectBusy, status);

    status = GdipReleaseDC(graphics, retdc);
    expect(Ok, status);

    GdipDeletePen(pen);
    GdipDeleteGraphics(graphics);

    GdipDeletePath(path);
    GdipDeleteBrush((GpBrush*)brush);
    GdipDeleteRegion(region);
    GdipDeleteMatrix(m);
    DeleteObject(hrgn);

    ReleaseDC(0, hdc);
}

static void test_transformpoints(void)
{
    GpStatus status;
    GpGraphics *graphics = NULL;
    HDC hdc = GetDC(0);
    GpPointF ptf[5];
    INT i;

    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);

    for(i = 0; i < 5; i++){
        ptf[i].X = 200.0 + i * 50.0 * (i % 2);
        ptf[i].Y = 200.0 + i * 50.0 * !(i % 2);
    }

    /* NULL arguments */
    status = GdipTransformPoints(NULL, CoordinateSpacePage, CoordinateSpaceWorld, NULL, 0);
    expect(InvalidParameter, status);
    status = GdipTransformPoints(graphics, CoordinateSpacePage, CoordinateSpaceWorld, NULL, 0);
    expect(InvalidParameter, status);
    status = GdipTransformPoints(graphics, CoordinateSpacePage, CoordinateSpaceWorld, ptf, 0);
    expect(InvalidParameter, status);
    status = GdipTransformPoints(graphics, CoordinateSpacePage, CoordinateSpaceWorld, ptf, -1);
    expect(InvalidParameter, status);

    GdipDeleteGraphics(graphics);
    ReleaseDC(0, hdc);
}

static void test_get_set_clip(void)
{
    GpStatus status;
    GpGraphics *graphics = NULL;
    HDC hdc = GetDC(0);
    GpRegion *clip;
    GpRectF rect;
    BOOL res;

    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);

    rect.X = rect.Y = 0.0;
    rect.Height = rect.Width = 100.0;

    status = GdipCreateRegionRect(&rect, &clip);

    /* NULL arguments */
    status = GdipGetClip(NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipGetClip(graphics, NULL);
    expect(InvalidParameter, status);
    status = GdipGetClip(NULL, clip);
    expect(InvalidParameter, status);

    status = GdipSetClipRegion(NULL, NULL, CombineModeReplace);
    expect(InvalidParameter, status);
    status = GdipSetClipRegion(graphics, NULL, CombineModeReplace);
    expect(InvalidParameter, status);

    status = GdipSetClipPath(NULL, NULL, CombineModeReplace);
    expect(InvalidParameter, status);
    status = GdipSetClipPath(graphics, NULL, CombineModeReplace);
    expect(InvalidParameter, status);

    res = FALSE;
    status = GdipGetClip(graphics, clip);
    expect(Ok, status);
    status = GdipIsInfiniteRegion(clip, graphics, &res);
    expect(Ok, status);
    expect(TRUE, res);

    /* remains infinite after reset */
    res = FALSE;
    status = GdipResetClip(graphics);
    expect(Ok, status);
    status = GdipGetClip(graphics, clip);
    expect(Ok, status);
    status = GdipIsInfiniteRegion(clip, graphics, &res);
    expect(Ok, status);
    expect(TRUE, res);

    /* set to empty and then reset to infinite */
    status = GdipSetEmpty(clip);
    expect(Ok, status);
    status = GdipSetClipRegion(graphics, clip, CombineModeReplace);
    expect(Ok, status);

    status = GdipGetClip(graphics, clip);
    expect(Ok, status);
    res = FALSE;
    status = GdipIsEmptyRegion(clip, graphics, &res);
    expect(Ok, status);
    expect(TRUE, res);
    status = GdipResetClip(graphics);
    expect(Ok, status);
    status = GdipGetClip(graphics, clip);
    expect(Ok, status);
    res = FALSE;
    status = GdipIsInfiniteRegion(clip, graphics, &res);
    expect(Ok, status);
    expect(TRUE, res);

    GdipDeleteRegion(clip);

    GdipDeleteGraphics(graphics);
    ReleaseDC(0, hdc);
}

static void test_isempty(void)
{
    GpStatus status;
    GpGraphics *graphics = NULL;
    HDC hdc = GetDC(0);
    GpRegion *clip;
    BOOL res;

    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);

    status = GdipCreateRegion(&clip);
    expect(Ok, status);

    /* NULL */
    status = GdipIsClipEmpty(NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipIsClipEmpty(graphics, NULL);
    expect(InvalidParameter, status);
    status = GdipIsClipEmpty(NULL, &res);
    expect(InvalidParameter, status);

    /* default is infinite */
    res = TRUE;
    status = GdipIsClipEmpty(graphics, &res);
    expect(Ok, status);
    expect(FALSE, res);

    GdipDeleteRegion(clip);

    GdipDeleteGraphics(graphics);
    ReleaseDC(0, hdc);
}

static void test_clear(void)
{
    GpStatus status;

    status = GdipGraphicsClear(NULL, 0xdeadbeef);
    expect(InvalidParameter, status);
}

static void test_textcontrast(void)
{
    GpStatus status;
    HDC hdc = GetDC(0);
    GpGraphics *graphics;
    UINT contrast;

    status = GdipGetTextContrast(NULL, NULL);
    expect(InvalidParameter, status);

    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);

    status = GdipGetTextContrast(graphics, NULL);
    expect(InvalidParameter, status);
    status = GdipGetTextContrast(graphics, &contrast);
    expect(4, contrast);

    GdipDeleteGraphics(graphics);
    ReleaseDC(0, hdc);
}

static void test_GdipDrawString(void)
{
    GpStatus status;
    GpGraphics *graphics = NULL;
    GpFont *fnt = NULL;
    RectF  rect;
    GpStringFormat *format;
    GpBrush *brush;
    LOGFONTA logfont;
    HDC hdc = GetDC(0);
    static const WCHAR string[] = {'T','e','s','t',0};

    memset(&logfont,0,sizeof(logfont));
    strcpy(logfont.lfFaceName,"Arial");
    logfont.lfHeight = 12;
    logfont.lfCharSet = DEFAULT_CHARSET;

    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);

    status = GdipCreateFontFromLogfontA(hdc, &logfont, &fnt);
    if (status == FileNotFound)
    {
        skip("Arial not installed.\n");
        return;
    }
    expect(Ok, status);

    status = GdipCreateSolidFill((ARGB)0xdeadbeef, (GpSolidFill**)&brush);
    expect(Ok, status);

    status = GdipCreateStringFormat(0,0,&format);
    expect(Ok, status);

    rect.X = 0;
    rect.Y = 0;
    rect.Width = 0;
    rect.Height = 12;

    status = GdipDrawString(graphics, string, 4, fnt, &rect, format, brush);
    expect(Ok, status);

    GdipDeleteGraphics(graphics);
    GdipDeleteBrush(brush);
    GdipDeleteFont(fnt);
    GdipDeleteStringFormat(format);

    ReleaseDC(0, hdc);
}


START_TEST(graphics)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_constructor_destructor();
    test_save_restore();
    test_GdipDrawBezierI();
    test_GdipDrawArc();
    test_GdipDrawArcI();
    test_GdipDrawLineI();
    test_GdipDrawLinesI();
    test_GdipDrawString();
    test_Get_Release_DC();
    test_transformpoints();
    test_get_set_clip();
    test_isempty();
    test_clear();
    test_textcontrast();

    GdiplusShutdown(gdiplusToken);
}
