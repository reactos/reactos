/*
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"

#define COBJMACROS
#include "objbase.h"
#include "olectl.h"
#include "ole2.h"

#include "gdiplus.h"
#include "gdiplus_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdiplus);

/*
    Unix stuff
    Code from http://www.johndcook.com/blog/2009/01/19/stand-alone-error-function-erf/
*/
double erf(double x)
{
    const float a1 =  0.254829592;
    const float a2 = -0.284496736;
    const float a3 =  1.421413741;
    const float a4 = -1.453152027;
    const float a5 =  1.061405429;
    const float p  =  0.3275911;
    float t, y, sign;

    /* Save the sign of x */
    sign = 1;
    if (x < 0)
        sign = -1;
    x = abs(x);

    /* A & S 7.1.26 */
    t = 1.0/(1.0 + p*x);
    y = 1.0 - (((((a5*t + a4)*t) + a3)*t + a2)*t + a1)*t*exp(-x*x);

    return sign*y;
}

/******************************************************************************
 * GdipCloneBrush [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCloneBrush(GpBrush *brush, GpBrush **clone)
{
    TRACE("(%p, %p)\n", brush, clone);

    if(!brush || !clone)
        return InvalidParameter;

    switch(brush->bt){
        case BrushTypeSolidColor:
        {
            GpSolidFill *fill;
            *clone = GdipAlloc(sizeof(GpSolidFill));
            if (!*clone) return OutOfMemory;

            fill = (GpSolidFill*)*clone;

            memcpy(*clone, brush, sizeof(GpSolidFill));

            (*clone)->gdibrush = CreateBrushIndirect(&(*clone)->lb);
            fill->bmp = ARGB2BMP(fill->color);
            break;
        }
        case BrushTypeHatchFill:
        {
            GpHatch *hatch = (GpHatch*)brush;

            return GdipCreateHatchBrush(hatch->hatchstyle, hatch->forecol, hatch->backcol, (GpHatch**)clone);
        }
        case BrushTypePathGradient:{
            GpPathGradient *src, *dest;
            INT count;

            *clone = GdipAlloc(sizeof(GpPathGradient));
            if (!*clone) return OutOfMemory;

            src = (GpPathGradient*) brush,
            dest = (GpPathGradient*) *clone;
            count = src->pathdata.Count;

            memcpy(dest, src, sizeof(GpPathGradient));

            dest->pathdata.Count = count;
            dest->pathdata.Points = GdipAlloc(count * sizeof(PointF));
            dest->pathdata.Types = GdipAlloc(count);

            if(!dest->pathdata.Points || !dest->pathdata.Types){
                GdipFree(dest->pathdata.Points);
                GdipFree(dest->pathdata.Types);
                GdipFree(dest);
                return OutOfMemory;
            }

            memcpy(dest->pathdata.Points, src->pathdata.Points, count * sizeof(PointF));
            memcpy(dest->pathdata.Types, src->pathdata.Types, count);

            /* blending */
            count = src->blendcount;
            dest->blendcount = count;
            dest->blendfac = GdipAlloc(count * sizeof(REAL));
            dest->blendpos = GdipAlloc(count * sizeof(REAL));

            if(!dest->blendfac || !dest->blendpos){
                GdipFree(dest->pathdata.Points);
                GdipFree(dest->pathdata.Types);
                GdipFree(dest->blendfac);
                GdipFree(dest->blendpos);
                GdipFree(dest);
                return OutOfMemory;
            }

            memcpy(dest->blendfac, src->blendfac, count * sizeof(REAL));
            memcpy(dest->blendpos, src->blendpos, count * sizeof(REAL));

            break;
        }
        case BrushTypeLinearGradient:{
            GpLineGradient *dest, *src;
            INT count, pcount;

            dest = GdipAlloc(sizeof(GpLineGradient));
            if(!dest)    return OutOfMemory;

            src = (GpLineGradient*)brush;

            memcpy(dest, src, sizeof(GpLineGradient));

            dest->brush.gdibrush = CreateSolidBrush(dest->brush.lb.lbColor);

            count = dest->blendcount;
            dest->blendfac = GdipAlloc(count * sizeof(REAL));
            dest->blendpos = GdipAlloc(count * sizeof(REAL));
            pcount = dest->pblendcount;
            if (pcount)
            {
                dest->pblendcolor = GdipAlloc(pcount * sizeof(ARGB));
                dest->pblendpos = GdipAlloc(pcount * sizeof(REAL));
            }

            if (!dest->blendfac || !dest->blendpos ||
                (pcount && (!dest->pblendcolor || !dest->pblendpos)))
            {
                GdipFree(dest->blendfac);
                GdipFree(dest->blendpos);
                GdipFree(dest->pblendcolor);
                GdipFree(dest->pblendpos);
                DeleteObject(dest->brush.gdibrush);
                GdipFree(dest);
                return OutOfMemory;
            }

            memcpy(dest->blendfac, src->blendfac, count * sizeof(REAL));
            memcpy(dest->blendpos, src->blendpos, count * sizeof(REAL));

            if (pcount)
            {
                memcpy(dest->pblendcolor, src->pblendcolor, pcount * sizeof(ARGB));
                memcpy(dest->pblendpos, src->pblendpos, pcount * sizeof(REAL));
            }

            *clone = &dest->brush;
            break;
        }
        case BrushTypeTextureFill:
        {
            GpStatus stat;
            GpTexture *texture = (GpTexture*)brush;
            GpTexture *new_texture;

            stat = GdipCreateTexture(texture->image, texture->wrap, &new_texture);

            if (stat == Ok)
            {
                memcpy(new_texture->transform, texture->transform, sizeof(GpMatrix));
                *clone = (GpBrush*)new_texture;
            }
            else
                *clone = NULL;

            return stat;
        }
        default:
            ERR("not implemented for brush type %d\n", brush->bt);
            return NotImplemented;
    }

    TRACE("<-- %p\n", *clone);
    return Ok;
}

static const char HatchBrushes[][8] = {
    { 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00 }, /* HatchStyleHorizontal */
    { 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08 }, /* HatchStyleVertical */
    { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 }, /* HatchStyleForwardDiagonal */
    { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 }, /* HatchStyleBackwardDiagonal */
    { 0x08, 0x08, 0x08, 0xff, 0x08, 0x08, 0x08, 0x08 }, /* HatchStyleCross */
    { 0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81 }, /* HatchStyleDiagonalCross */
    { 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x80 }, /* HatchStyle05Percent */
    { 0x00, 0x02, 0x00, 0x88, 0x00, 0x20, 0x00, 0x88 }, /* HatchStyle10Percent */
    { 0x00, 0x22, 0x00, 0xcc, 0x00, 0x22, 0x00, 0xcc }, /* HatchStyle20Percent */
    { 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc }, /* HatchStyle25Percent */
    { 0x00, 0xcc, 0x04, 0xcc, 0x00, 0xcc, 0x40, 0xcc }, /* HatchStyle30Percent */
    { 0x44, 0xcc, 0x22, 0xcc, 0x44, 0xcc, 0x22, 0xcc }, /* HatchStyle40Percent */
    { 0x55, 0xcc, 0x55, 0xcc, 0x55, 0xcc, 0x55, 0xcc }, /* HatchStyle50Percent */
    { 0x55, 0xcd, 0x55, 0xee, 0x55, 0xdc, 0x55, 0xee }, /* HatchStyle60Percent */
    { 0x55, 0xdd, 0x55, 0xff, 0x55, 0xdd, 0x55, 0xff }, /* HatchStyle70Percent */
    { 0x55, 0xff, 0x55, 0xff, 0x55, 0xff, 0x55, 0xff }, /* HatchStyle75Percent */
    { 0x55, 0xff, 0x59, 0xff, 0x55, 0xff, 0x99, 0xff }, /* HatchStyle80Percent */
    { 0x77, 0xff, 0xdd, 0xff, 0x77, 0xff, 0xfd, 0xff }, /* HatchStyle90Percent */
    { 0x11, 0x22, 0x44, 0x88, 0x11, 0x22, 0x44, 0x88 }, /* HatchStyleLightDownwardDiagonal */
    { 0x88, 0x44, 0x22, 0x11, 0x88, 0x44, 0x22, 0x11 }, /* HatchStyleLightUpwardDiagonal */
    { 0x99, 0x33, 0x66, 0xcc, 0x99, 0x33, 0x66, 0xcc }, /* HatchStyleDarkDownwardDiagonal */
    { 0xcc, 0x66, 0x33, 0x99, 0xcc, 0x66, 0x33, 0x99 }, /* HatchStyleDarkUpwardDiagonal */
    { 0xc1, 0x83, 0x07, 0x0e, 0x1c, 0x38, 0x70, 0xe0 }, /* HatchStyleWideDownwardDiagonal */
    { 0xe0, 0x70, 0x38, 0x1c, 0x0e, 0x07, 0x83, 0xc1 }, /* HatchStyleWideUpwardDiagonal */
    { 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88 }, /* HatchStyleLightVertical */
    { 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff }, /* HatchStyleLightHorizontal */
    { 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa }, /* HatchStyleNarrowVertical */
    { 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff }, /* HatchStyleNarrowHorizontal */
    { 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc }, /* HatchStyleDarkVertical */
    { 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff }, /* HatchStyleDarkHorizontal */
};

/******************************************************************************
 * GdipCreateHatchBrush [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateHatchBrush(HatchStyle hatchstyle, ARGB forecol, ARGB backcol, GpHatch **brush)
{
    COLORREF fgcol = ARGB2COLORREF(forecol);
    GpStatus stat = Ok;

    TRACE("(%d, %d, %d, %p)\n", hatchstyle, forecol, backcol, brush);

    if(!brush)  return InvalidParameter;

    *brush = GdipAlloc(sizeof(GpHatch));
    if (!*brush) return OutOfMemory;

    if (hatchstyle < sizeof(HatchBrushes) / sizeof(HatchBrushes[0]))
    {
        HBITMAP hbmp;
        HDC hdc;
        BITMAPINFOHEADER bmih;
        DWORD* bits;
        int x, y;

        hdc = CreateCompatibleDC(0);

        if (hdc)
        {
            bmih.biSize = sizeof(bmih);
            bmih.biWidth = 8;
            bmih.biHeight = 8;
            bmih.biPlanes = 1;
            bmih.biBitCount = 32;
            bmih.biCompression = BI_RGB;
            bmih.biSizeImage = 0;

            hbmp = CreateDIBSection(hdc, (BITMAPINFO*)&bmih, DIB_RGB_COLORS, (void**)&bits, NULL, 0);

            if (hbmp)
            {
                for (y=0; y<8; y++)
                    for (x=0; x<8; x++)
                        if ((HatchBrushes[hatchstyle][y] & (0x80 >> x)) != 0)
                            bits[y*8+x] = forecol;
                        else
                            bits[y*8+x] = backcol;
            }
            else
                stat = GenericError;

            DeleteDC(hdc);
        }
        else
            stat = GenericError;

        if (stat == Ok)
        {
            (*brush)->brush.lb.lbStyle = BS_PATTERN;
            (*brush)->brush.lb.lbColor = 0;
            (*brush)->brush.lb.lbHatch = (ULONG_PTR)hbmp;
            (*brush)->brush.gdibrush = CreateBrushIndirect(&(*brush)->brush.lb);

            DeleteObject(hbmp);
        }
    }
    else
    {
        FIXME("Unimplemented hatch style %d\n", hatchstyle);

        (*brush)->brush.lb.lbStyle = BS_SOLID;
        (*brush)->brush.lb.lbColor = fgcol;
        (*brush)->brush.lb.lbHatch = 0;
        (*brush)->brush.gdibrush = CreateBrushIndirect(&(*brush)->brush.lb);
    }

    if (stat == Ok)
    {
        (*brush)->brush.bt = BrushTypeHatchFill;
        (*brush)->forecol = forecol;
        (*brush)->backcol = backcol;
        (*brush)->hatchstyle = hatchstyle;
        TRACE("<-- %p\n", *brush);
    }
    else
    {
        GdipFree(*brush);
        *brush = NULL;
    }

    return stat;
}

/******************************************************************************
 * GdipCreateLineBrush [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateLineBrush(GDIPCONST GpPointF* startpoint,
    GDIPCONST GpPointF* endpoint, ARGB startcolor, ARGB endcolor,
    GpWrapMode wrap, GpLineGradient **line)
{
    COLORREF col = ARGB2COLORREF(startcolor);

    TRACE("(%s, %s, %x, %x, %d, %p)\n", debugstr_pointf(startpoint),
          debugstr_pointf(endpoint), startcolor, endcolor, wrap, line);

    if(!line || !startpoint || !endpoint || wrap == WrapModeClamp)
        return InvalidParameter;

    *line = GdipAlloc(sizeof(GpLineGradient));
    if(!*line)  return OutOfMemory;

    (*line)->brush.lb.lbStyle = BS_SOLID;
    (*line)->brush.lb.lbColor = col;
    (*line)->brush.lb.lbHatch = 0;
    (*line)->brush.gdibrush = CreateSolidBrush(col);
    (*line)->brush.bt = BrushTypeLinearGradient;

    (*line)->startpoint.X = startpoint->X;
    (*line)->startpoint.Y = startpoint->Y;
    (*line)->endpoint.X = endpoint->X;
    (*line)->endpoint.Y = endpoint->Y;
    (*line)->startcolor = startcolor;
    (*line)->endcolor = endcolor;
    (*line)->wrap = wrap;
    (*line)->gamma = FALSE;

    (*line)->rect.X = (startpoint->X < endpoint->X ? startpoint->X: endpoint->X);
    (*line)->rect.Y = (startpoint->Y < endpoint->Y ? startpoint->Y: endpoint->Y);
    (*line)->rect.Width  = fabs(startpoint->X - endpoint->X);
    (*line)->rect.Height = fabs(startpoint->Y - endpoint->Y);

    if ((*line)->rect.Width == 0)
    {
        (*line)->rect.X -= (*line)->rect.Height / 2.0f;
        (*line)->rect.Width = (*line)->rect.Height;
    }
    else if ((*line)->rect.Height == 0)
    {
        (*line)->rect.Y -= (*line)->rect.Width / 2.0f;
        (*line)->rect.Height = (*line)->rect.Width;
    }

    (*line)->blendcount = 1;
    (*line)->blendfac = GdipAlloc(sizeof(REAL));
    (*line)->blendpos = GdipAlloc(sizeof(REAL));

    if (!(*line)->blendfac || !(*line)->blendpos)
    {
        GdipFree((*line)->blendfac);
        GdipFree((*line)->blendpos);
        DeleteObject((*line)->brush.gdibrush);
        GdipFree(*line);
        *line = NULL;
        return OutOfMemory;
    }

    (*line)->blendfac[0] = 1.0f;
    (*line)->blendpos[0] = 1.0f;

    (*line)->pblendcolor = NULL;
    (*line)->pblendpos = NULL;
    (*line)->pblendcount = 0;

    TRACE("<-- %p\n", *line);

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateLineBrushI(GDIPCONST GpPoint* startpoint,
    GDIPCONST GpPoint* endpoint, ARGB startcolor, ARGB endcolor,
    GpWrapMode wrap, GpLineGradient **line)
{
    GpPointF stF;
    GpPointF endF;

    TRACE("(%p, %p, %x, %x, %d, %p)\n", startpoint, endpoint,
          startcolor, endcolor, wrap, line);

    if(!startpoint || !endpoint)
        return InvalidParameter;

    stF.X  = (REAL)startpoint->X;
    stF.Y  = (REAL)startpoint->Y;
    endF.X = (REAL)endpoint->X;
    endF.X = (REAL)endpoint->Y;

    return GdipCreateLineBrush(&stF, &endF, startcolor, endcolor, wrap, line);
}

GpStatus WINGDIPAPI GdipCreateLineBrushFromRect(GDIPCONST GpRectF* rect,
    ARGB startcolor, ARGB endcolor, LinearGradientMode mode, GpWrapMode wrap,
    GpLineGradient **line)
{
    GpPointF start, end;
    GpStatus stat;

    TRACE("(%p, %x, %x, %d, %d, %p)\n", rect, startcolor, endcolor, mode,
          wrap, line);

    if(!line || !rect)
        return InvalidParameter;

    switch (mode)
    {
    case LinearGradientModeHorizontal:
        start.X = rect->X;
        start.Y = rect->Y;
        end.X = rect->X + rect->Width;
        end.Y = rect->Y;
        break;
    case LinearGradientModeVertical:
        start.X = rect->X;
        start.Y = rect->Y;
        end.X = rect->X;
        end.Y = rect->Y + rect->Height;
        break;
    case LinearGradientModeForwardDiagonal:
        start.X = rect->X;
        start.Y = rect->Y;
        end.X = rect->X + rect->Width;
        end.Y = rect->Y + rect->Height;
        break;
    case LinearGradientModeBackwardDiagonal:
        start.X = rect->X + rect->Width;
        start.Y = rect->Y;
        end.X = rect->X;
        end.Y = rect->Y + rect->Height;
        break;
    default:
        return InvalidParameter;
    }

    stat = GdipCreateLineBrush(&start, &end, startcolor, endcolor, wrap, line);

    if (stat == Ok)
        (*line)->rect = *rect;

    return stat;
}

GpStatus WINGDIPAPI GdipCreateLineBrushFromRectI(GDIPCONST GpRect* rect,
    ARGB startcolor, ARGB endcolor, LinearGradientMode mode, GpWrapMode wrap,
    GpLineGradient **line)
{
    GpRectF rectF;

    TRACE("(%p, %x, %x, %d, %d, %p)\n", rect, startcolor, endcolor, mode,
          wrap, line);

    rectF.X      = (REAL) rect->X;
    rectF.Y      = (REAL) rect->Y;
    rectF.Width  = (REAL) rect->Width;
    rectF.Height = (REAL) rect->Height;

    return GdipCreateLineBrushFromRect(&rectF, startcolor, endcolor, mode, wrap, line);
}

/******************************************************************************
 * GdipCreateLineBrushFromRectWithAngle [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateLineBrushFromRectWithAngle(GDIPCONST GpRectF* rect,
    ARGB startcolor, ARGB endcolor, REAL angle, BOOL isAngleScalable, GpWrapMode wrap,
    GpLineGradient **line)
{
    GpStatus stat;
    LinearGradientMode mode;
    REAL width, height, exofs, eyofs;
    REAL sin_angle, cos_angle, sin_cos_angle;

    TRACE("(%p, %x, %x, %.2f, %d, %d, %p)\n", rect, startcolor, endcolor, angle, isAngleScalable,
          wrap, line);

    sin_angle = sinf(deg2rad(angle));
    cos_angle = cosf(deg2rad(angle));
    sin_cos_angle = sin_angle * cos_angle;

    if (isAngleScalable)
    {
        width = height = 1.0;
    }
    else
    {
        width = rect->Width;
        height = rect->Height;
    }

    if (sin_cos_angle >= 0)
        mode = LinearGradientModeForwardDiagonal;
    else
        mode = LinearGradientModeBackwardDiagonal;

    stat = GdipCreateLineBrushFromRect(rect, startcolor, endcolor, mode, wrap, line);

    if (stat == Ok)
    {
        if (sin_cos_angle >= 0)
        {
            exofs = width * sin_cos_angle + height * cos_angle * cos_angle;
            eyofs = width * sin_angle * sin_angle + height * sin_cos_angle;
        }
        else
        {
            exofs = width * sin_angle * sin_angle + height * sin_cos_angle;
            eyofs = -width * sin_cos_angle + height * sin_angle * sin_angle;
        }

        if (isAngleScalable)
        {
            exofs = exofs * rect->Width;
            eyofs = eyofs * rect->Height;
        }

        if (sin_angle >= 0)
        {
            (*line)->endpoint.X = rect->X + exofs;
            (*line)->endpoint.Y = rect->Y + eyofs;
        }
        else
        {
            (*line)->endpoint.X = (*line)->startpoint.X;
            (*line)->endpoint.Y = (*line)->startpoint.Y;
            (*line)->startpoint.X = rect->X + exofs;
            (*line)->startpoint.Y = rect->Y + eyofs;
        }
    }

    return stat;
}

GpStatus WINGDIPAPI GdipCreateLineBrushFromRectWithAngleI(GDIPCONST GpRect* rect,
    ARGB startcolor, ARGB endcolor, REAL angle, BOOL isAngleScalable, GpWrapMode wrap,
    GpLineGradient **line)
{
    TRACE("(%p, %x, %x, %.2f, %d, %d, %p)\n", rect, startcolor, endcolor, angle, isAngleScalable,
          wrap, line);

    return GdipCreateLineBrushFromRectI(rect, startcolor, endcolor, LinearGradientModeForwardDiagonal,
                                        wrap, line);
}

GpStatus WINGDIPAPI GdipCreatePathGradient(GDIPCONST GpPointF* points,
    INT count, GpWrapMode wrap, GpPathGradient **grad)
{
    COLORREF col = ARGB2COLORREF(0xffffffff);

    TRACE("(%p, %d, %d, %p)\n", points, count, wrap, grad);

    if(!points || !grad)
        return InvalidParameter;

    if(count <= 0)
        return OutOfMemory;

    *grad = GdipAlloc(sizeof(GpPathGradient));
    if (!*grad) return OutOfMemory;

    (*grad)->blendfac = GdipAlloc(sizeof(REAL));
    if(!(*grad)->blendfac){
        GdipFree(*grad);
        return OutOfMemory;
    }
    (*grad)->blendfac[0] = 1.0;
    (*grad)->blendpos    = NULL;
    (*grad)->blendcount  = 1;

    (*grad)->pathdata.Count = count;
    (*grad)->pathdata.Points = GdipAlloc(count * sizeof(PointF));
    (*grad)->pathdata.Types = GdipAlloc(count);

    if(!(*grad)->pathdata.Points || !(*grad)->pathdata.Types){
        GdipFree((*grad)->pathdata.Points);
        GdipFree((*grad)->pathdata.Types);
        GdipFree(*grad);
        return OutOfMemory;
    }

    memcpy((*grad)->pathdata.Points, points, count * sizeof(PointF));
    memset((*grad)->pathdata.Types, PathPointTypeLine, count);

    (*grad)->brush.lb.lbStyle = BS_SOLID;
    (*grad)->brush.lb.lbColor = col;
    (*grad)->brush.lb.lbHatch = 0;

    (*grad)->brush.gdibrush = CreateSolidBrush(col);
    (*grad)->brush.bt = BrushTypePathGradient;
    (*grad)->centercolor = 0xffffffff;
    (*grad)->wrap = wrap;
    (*grad)->gamma = FALSE;
    (*grad)->center.X = 0.0;
    (*grad)->center.Y = 0.0;
    (*grad)->focus.X = 0.0;
    (*grad)->focus.Y = 0.0;

    TRACE("<-- %p\n", *grad);

    return Ok;
}

GpStatus WINGDIPAPI GdipCreatePathGradientI(GDIPCONST GpPoint* points,
    INT count, GpWrapMode wrap, GpPathGradient **grad)
{
    GpPointF *pointsF;
    GpStatus ret;
    INT i;

    TRACE("(%p, %d, %d, %p)\n", points, count, wrap, grad);

    if(!points || !grad)
        return InvalidParameter;

    if(count <= 0)
        return OutOfMemory;

    pointsF = GdipAlloc(sizeof(GpPointF) * count);
    if(!pointsF)
        return OutOfMemory;

    for(i = 0; i < count; i++){
        pointsF[i].X = (REAL)points[i].X;
        pointsF[i].Y = (REAL)points[i].Y;
    }

    ret = GdipCreatePathGradient(pointsF, count, wrap, grad);
    GdipFree(pointsF);

    return ret;
}

/******************************************************************************
 * GdipCreatePathGradientFromPath [GDIPLUS.@]
 *
 * FIXME: path gradient brushes not truly supported (drawn as solid brushes)
 */
GpStatus WINGDIPAPI GdipCreatePathGradientFromPath(GDIPCONST GpPath* path,
    GpPathGradient **grad)
{
    COLORREF col = ARGB2COLORREF(0xffffffff);

    TRACE("(%p, %p)\n", path, grad);

    if(!path || !grad)
        return InvalidParameter;

    *grad = GdipAlloc(sizeof(GpPathGradient));
    if (!*grad) return OutOfMemory;

    (*grad)->blendfac = GdipAlloc(sizeof(REAL));
    if(!(*grad)->blendfac){
        GdipFree(*grad);
        return OutOfMemory;
    }
    (*grad)->blendfac[0] = 1.0;
    (*grad)->blendpos    = NULL;
    (*grad)->blendcount  = 1;

    (*grad)->pathdata.Count = path->pathdata.Count;
    (*grad)->pathdata.Points = GdipAlloc(path->pathdata.Count * sizeof(PointF));
    (*grad)->pathdata.Types = GdipAlloc(path->pathdata.Count);

    if(!(*grad)->pathdata.Points || !(*grad)->pathdata.Types){
        GdipFree((*grad)->pathdata.Points);
        GdipFree((*grad)->pathdata.Types);
        GdipFree(*grad);
        return OutOfMemory;
    }

    memcpy((*grad)->pathdata.Points, path->pathdata.Points,
           path->pathdata.Count * sizeof(PointF));
    memcpy((*grad)->pathdata.Types, path->pathdata.Types, path->pathdata.Count);

    (*grad)->brush.lb.lbStyle = BS_SOLID;
    (*grad)->brush.lb.lbColor = col;
    (*grad)->brush.lb.lbHatch = 0;

    (*grad)->brush.gdibrush = CreateSolidBrush(col);
    (*grad)->brush.bt = BrushTypePathGradient;
    (*grad)->centercolor = 0xffffffff;
    (*grad)->wrap = WrapModeClamp;
    (*grad)->gamma = FALSE;
    /* FIXME: this should be set to the "centroid" of the path by default */
    (*grad)->center.X = 0.0;
    (*grad)->center.Y = 0.0;
    (*grad)->focus.X = 0.0;
    (*grad)->focus.Y = 0.0;

    TRACE("<-- %p\n", *grad);

    return Ok;
}

/******************************************************************************
 * GdipCreateSolidFill [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateSolidFill(ARGB color, GpSolidFill **sf)
{
    COLORREF col = ARGB2COLORREF(color);

    TRACE("(%x, %p)\n", color, sf);

    if(!sf)  return InvalidParameter;

    *sf = GdipAlloc(sizeof(GpSolidFill));
    if (!*sf) return OutOfMemory;

    (*sf)->brush.lb.lbStyle = BS_SOLID;
    (*sf)->brush.lb.lbColor = col;
    (*sf)->brush.lb.lbHatch = 0;

    (*sf)->brush.gdibrush = CreateSolidBrush(col);
    (*sf)->brush.bt = BrushTypeSolidColor;
    (*sf)->color = color;
    (*sf)->bmp = ARGB2BMP(color);

    TRACE("<-- %p\n", *sf);

    return Ok;
}

/******************************************************************************
 * GdipCreateTexture [GDIPLUS.@]
 *
 * PARAMS
 *  image       [I] image to use
 *  wrapmode    [I] optional
 *  texture     [O] pointer to the resulting texturebrush
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: element of GpStatus
 */
GpStatus WINGDIPAPI GdipCreateTexture(GpImage *image, GpWrapMode wrapmode,
        GpTexture **texture)
{
    UINT width, height;
    GpImageAttributes attributes;
    GpStatus stat;

    TRACE("%p, %d %p\n", image, wrapmode, texture);

    if (!(image && texture))
        return InvalidParameter;

    stat = GdipGetImageWidth(image, &width);
    if (stat != Ok) return stat;
    stat = GdipGetImageHeight(image, &height);
    if (stat != Ok) return stat;
    attributes.wrap = wrapmode;

    return GdipCreateTextureIA(image, &attributes, 0, 0, width, height,
            texture);
}

/******************************************************************************
 * GdipCreateTexture2 [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateTexture2(GpImage *image, GpWrapMode wrapmode,
        REAL x, REAL y, REAL width, REAL height, GpTexture **texture)
{
    GpImageAttributes attributes;

    TRACE("%p %d %f %f %f %f %p\n", image, wrapmode,
            x, y, width, height, texture);

    attributes.wrap = wrapmode;
    return GdipCreateTextureIA(image, &attributes, x, y, width, height,
            texture);
}

/******************************************************************************
 * GdipCreateTextureIA [GDIPLUS.@]
 *
 * FIXME: imageattr ignored
 */
GpStatus WINGDIPAPI GdipCreateTextureIA(GpImage *image,
    GDIPCONST GpImageAttributes *imageattr, REAL x, REAL y, REAL width,
    REAL height, GpTexture **texture)
{
    HBITMAP hbm;
    GpStatus status;
    GpImage *new_image=NULL;

    TRACE("(%p, %p, %.2f, %.2f, %.2f, %.2f, %p)\n", image, imageattr, x, y, width, height,
           texture);

    if(!image || !texture || x < 0.0 || y < 0.0 || width < 0.0 || height < 0.0)
        return InvalidParameter;

    *texture = NULL;

    if(image->type != ImageTypeBitmap){
        FIXME("not implemented for image type %d\n", image->type);
        return NotImplemented;
    }

    status = GdipCloneBitmapArea(x, y, width, height, PixelFormatDontCare, (GpBitmap*)image, (GpBitmap**)&new_image);
    if (status != Ok)
        return status;

    hbm = ((GpBitmap*)new_image)->hbitmap;
    if(!hbm)
    {
        status = GenericError;
        goto exit;
    }

    *texture = GdipAlloc(sizeof(GpTexture));
    if (!*texture){
        status = OutOfMemory;
        goto exit;
    }

    if((status = GdipCreateMatrix(&(*texture)->transform)) != Ok){
        goto exit;
    }

    (*texture)->brush.lb.lbStyle = BS_PATTERN;
    (*texture)->brush.lb.lbColor = 0;
    (*texture)->brush.lb.lbHatch = (ULONG_PTR)hbm;

    (*texture)->brush.gdibrush = CreateBrushIndirect(&(*texture)->brush.lb);
    (*texture)->brush.bt = BrushTypeTextureFill;
    (*texture)->wrap = imageattr->wrap;
    (*texture)->image = new_image;

exit:
    if (status == Ok)
    {
        TRACE("<-- %p\n", *texture);
    }
    else
    {
        if (*texture)
        {
            GdipDeleteMatrix((*texture)->transform);
            GdipFree(*texture);
            *texture = NULL;
        }
        GdipDisposeImage(new_image);
        TRACE("<-- error %u\n", status);
    }

    return status;
}

/******************************************************************************
 * GdipCreateTextureIAI [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateTextureIAI(GpImage *image, GDIPCONST GpImageAttributes *imageattr,
    INT x, INT y, INT width, INT height, GpTexture **texture)
{
    TRACE("(%p, %p, %d, %d, %d, %d, %p)\n", image, imageattr, x, y, width, height,
           texture);

    return GdipCreateTextureIA(image,imageattr,(REAL)x,(REAL)y,(REAL)width,(REAL)height,texture);
}

GpStatus WINGDIPAPI GdipCreateTexture2I(GpImage *image, GpWrapMode wrapmode,
        INT x, INT y, INT width, INT height, GpTexture **texture)
{
    GpImageAttributes imageattr;

    TRACE("%p %d %d %d %d %d %p\n", image, wrapmode, x, y, width, height,
            texture);

    imageattr.wrap = wrapmode;

    return GdipCreateTextureIA(image, &imageattr, x, y, width, height, texture);
}

GpStatus WINGDIPAPI GdipGetBrushType(GpBrush *brush, GpBrushType *type)
{
    TRACE("(%p, %p)\n", brush, type);

    if(!brush || !type)  return InvalidParameter;

    *type = brush->bt;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetHatchBackgroundColor(GpHatch *brush, ARGB *backcol)
{
    TRACE("(%p, %p)\n", brush, backcol);

    if(!brush || !backcol)  return InvalidParameter;

    *backcol = brush->backcol;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetHatchForegroundColor(GpHatch *brush, ARGB *forecol)
{
    TRACE("(%p, %p)\n", brush, forecol);

    if(!brush || !forecol)  return InvalidParameter;

    *forecol = brush->forecol;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetHatchStyle(GpHatch *brush, HatchStyle *hatchstyle)
{
    TRACE("(%p, %p)\n", brush, hatchstyle);

    if(!brush || !hatchstyle)  return InvalidParameter;

    *hatchstyle = brush->hatchstyle;

    return Ok;
}

GpStatus WINGDIPAPI GdipDeleteBrush(GpBrush *brush)
{
    TRACE("(%p)\n", brush);

    if(!brush)  return InvalidParameter;

    switch(brush->bt)
    {
        case BrushTypePathGradient:
            GdipFree(((GpPathGradient*) brush)->pathdata.Points);
            GdipFree(((GpPathGradient*) brush)->pathdata.Types);
            GdipFree(((GpPathGradient*) brush)->blendfac);
            GdipFree(((GpPathGradient*) brush)->blendpos);
            break;
        case BrushTypeSolidColor:
            if (((GpSolidFill*)brush)->bmp)
                DeleteObject(((GpSolidFill*)brush)->bmp);
            break;
        case BrushTypeLinearGradient:
            GdipFree(((GpLineGradient*)brush)->blendfac);
            GdipFree(((GpLineGradient*)brush)->blendpos);
            GdipFree(((GpLineGradient*)brush)->pblendcolor);
            GdipFree(((GpLineGradient*)brush)->pblendpos);
            break;
        case BrushTypeTextureFill:
            GdipDeleteMatrix(((GpTexture*)brush)->transform);
            GdipDisposeImage(((GpTexture*)brush)->image);
            break;
        default:
            break;
    }

    DeleteObject(brush->gdibrush);
    GdipFree(brush);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetLineGammaCorrection(GpLineGradient *line,
    BOOL *usinggamma)
{
    TRACE("(%p, %p)\n", line, usinggamma);

    if(!line || !usinggamma)
        return InvalidParameter;

    *usinggamma = line->gamma;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetLineWrapMode(GpLineGradient *brush, GpWrapMode *wrapmode)
{
    TRACE("(%p, %p)\n", brush, wrapmode);

    if(!brush || !wrapmode)
        return InvalidParameter;

    *wrapmode = brush->wrap;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientBlend(GpPathGradient *brush, REAL *blend,
    REAL *positions, INT count)
{
    TRACE("(%p, %p, %p, %d)\n", brush, blend, positions, count);

    if(!brush || !blend || !positions || count <= 0)
        return InvalidParameter;

    if(count < brush->blendcount)
        return InsufficientBuffer;

    memcpy(blend, brush->blendfac, count*sizeof(REAL));
    if(brush->blendcount > 1){
        memcpy(positions, brush->blendpos, count*sizeof(REAL));
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientBlendCount(GpPathGradient *brush, INT *count)
{
    TRACE("(%p, %p)\n", brush, count);

    if(!brush || !count)
        return InvalidParameter;

    *count = brush->blendcount;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientCenterPoint(GpPathGradient *grad,
    GpPointF *point)
{
    TRACE("(%p, %p)\n", grad, point);

    if(!grad || !point)
        return InvalidParameter;

    point->X = grad->center.X;
    point->Y = grad->center.Y;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientCenterPointI(GpPathGradient *grad,
    GpPoint *point)
{
    GpStatus ret;
    GpPointF ptf;

    TRACE("(%p, %p)\n", grad, point);

    if(!point)
        return InvalidParameter;

    ret = GdipGetPathGradientCenterPoint(grad,&ptf);

    if(ret == Ok){
        point->X = roundr(ptf.X);
        point->Y = roundr(ptf.Y);
    }

    return ret;
}

GpStatus WINGDIPAPI GdipGetPathGradientFocusScales(GpPathGradient *grad,
    REAL *x, REAL *y)
{
    TRACE("(%p, %p, %p)\n", grad, x, y);

    if(!grad || !x || !y)
        return InvalidParameter;

    *x = grad->focus.X;
    *y = grad->focus.Y;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientGammaCorrection(GpPathGradient *grad,
    BOOL *gamma)
{
    TRACE("(%p, %p)\n", grad, gamma);

    if(!grad || !gamma)
        return InvalidParameter;

    *gamma = grad->gamma;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientPointCount(GpPathGradient *grad,
    INT *count)
{
    TRACE("(%p, %p)\n", grad, count);

    if(!grad || !count)
        return InvalidParameter;

    *count = grad->pathdata.Count;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientRect(GpPathGradient *brush, GpRectF *rect)
{
    GpRectF r;
    GpPath* path;
    GpStatus stat;

    TRACE("(%p, %p)\n", brush, rect);

    if(!brush || !rect)
        return InvalidParameter;

    stat = GdipCreatePath2(brush->pathdata.Points, brush->pathdata.Types,
                           brush->pathdata.Count, FillModeAlternate, &path);
    if(stat != Ok)  return stat;

    stat = GdipGetPathWorldBounds(path, &r, NULL, NULL);
    if(stat != Ok){
        GdipDeletePath(path);
        return stat;
    }

    memcpy(rect, &r, sizeof(GpRectF));

    GdipDeletePath(path);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientRectI(GpPathGradient *brush, GpRect *rect)
{
    GpRectF rectf;
    GpStatus stat;

    TRACE("(%p, %p)\n", brush, rect);

    if(!brush || !rect)
        return InvalidParameter;

    stat = GdipGetPathGradientRect(brush, &rectf);
    if(stat != Ok)  return stat;

    rect->X = roundr(rectf.X);
    rect->Y = roundr(rectf.Y);
    rect->Width  = roundr(rectf.Width);
    rect->Height = roundr(rectf.Height);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientSurroundColorsWithCount(GpPathGradient
    *grad, ARGB *argb, INT *count)
{
    static int calls;

    TRACE("(%p,%p,%p)\n", grad, argb, count);

    if(!grad || !argb || !count || (*count < grad->pathdata.Count))
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetPathGradientWrapMode(GpPathGradient *brush,
    GpWrapMode *wrapmode)
{
    TRACE("(%p, %p)\n", brush, wrapmode);

    if(!brush || !wrapmode)
        return InvalidParameter;

    *wrapmode = brush->wrap;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetSolidFillColor(GpSolidFill *sf, ARGB *argb)
{
    TRACE("(%p, %p)\n", sf, argb);

    if(!sf || !argb)
        return InvalidParameter;

    *argb = sf->color;

    return Ok;
}

/******************************************************************************
 * GdipGetTextureImage [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetTextureImage(GpTexture *brush, GpImage **image)
{
    TRACE("(%p, %p)\n", brush, image);

    if(!brush || !image)
        return InvalidParameter;

    return GdipCloneImage(brush->image, image);
}

/******************************************************************************
 * GdipGetTextureTransform [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetTextureTransform(GpTexture *brush, GpMatrix *matrix)
{
    TRACE("(%p, %p)\n", brush, matrix);

    if(!brush || !matrix)
        return InvalidParameter;

    memcpy(matrix, brush->transform, sizeof(GpMatrix));

    return Ok;
}

/******************************************************************************
 * GdipGetTextureWrapMode [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetTextureWrapMode(GpTexture *brush, GpWrapMode *wrapmode)
{
    TRACE("(%p, %p)\n", brush, wrapmode);

    if(!brush || !wrapmode)
        return InvalidParameter;

    *wrapmode = brush->wrap;

    return Ok;
}

/******************************************************************************
 * GdipMultiplyTextureTransform [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipMultiplyTextureTransform(GpTexture* brush,
    GDIPCONST GpMatrix *matrix, GpMatrixOrder order)
{
    TRACE("(%p, %p, %d)\n", brush, matrix, order);

    if(!brush || !matrix)
        return InvalidParameter;

    return GdipMultiplyMatrix(brush->transform, matrix, order);
}

/******************************************************************************
 * GdipResetTextureTransform [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipResetTextureTransform(GpTexture* brush)
{
    TRACE("(%p)\n", brush);

    if(!brush)
        return InvalidParameter;

    return GdipSetMatrixElements(brush->transform, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
}

/******************************************************************************
 * GdipScaleTextureTransform [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipScaleTextureTransform(GpTexture* brush,
    REAL sx, REAL sy, GpMatrixOrder order)
{
    TRACE("(%p, %.2f, %.2f, %d)\n", brush, sx, sy, order);

    if(!brush)
        return InvalidParameter;

    return GdipScaleMatrix(brush->transform, sx, sy, order);
}

GpStatus WINGDIPAPI GdipSetLineBlend(GpLineGradient *brush,
    GDIPCONST REAL *factors, GDIPCONST REAL* positions, INT count)
{
    REAL *new_blendfac, *new_blendpos;

    TRACE("(%p, %p, %p, %i)\n", brush, factors, positions, count);

    if(!brush || !factors || !positions || count <= 0 ||
       (count >= 2 && (positions[0] != 0.0f || positions[count-1] != 1.0f)))
        return InvalidParameter;

    new_blendfac = GdipAlloc(count * sizeof(REAL));
    new_blendpos = GdipAlloc(count * sizeof(REAL));

    if (!new_blendfac || !new_blendpos)
    {
        GdipFree(new_blendfac);
        GdipFree(new_blendpos);
        return OutOfMemory;
    }

    memcpy(new_blendfac, factors, count * sizeof(REAL));
    memcpy(new_blendpos, positions, count * sizeof(REAL));

    GdipFree(brush->blendfac);
    GdipFree(brush->blendpos);

    brush->blendcount = count;
    brush->blendfac = new_blendfac;
    brush->blendpos = new_blendpos;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetLineBlend(GpLineGradient *brush, REAL *factors,
    REAL *positions, INT count)
{
    TRACE("(%p, %p, %p, %i)\n", brush, factors, positions, count);

    if (!brush || !factors || !positions || count <= 0)
        return InvalidParameter;

    if (count < brush->blendcount)
        return InsufficientBuffer;

    memcpy(factors, brush->blendfac, brush->blendcount * sizeof(REAL));
    memcpy(positions, brush->blendpos, brush->blendcount * sizeof(REAL));

    return Ok;
}

GpStatus WINGDIPAPI GdipGetLineBlendCount(GpLineGradient *brush, INT *count)
{
    TRACE("(%p, %p)\n", brush, count);

    if (!brush || !count)
        return InvalidParameter;

    *count = brush->blendcount;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetLineGammaCorrection(GpLineGradient *line,
    BOOL usegamma)
{
    TRACE("(%p, %d)\n", line, usegamma);

    if(!line)
        return InvalidParameter;

    line->gamma = usegamma;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetLineSigmaBlend(GpLineGradient *line, REAL focus,
    REAL scale)
{
    REAL factors[33];
    REAL positions[33];
    int num_points = 0;
    int i;
    const int precision = 16;
    REAL erf_range; /* we use values erf(-erf_range) through erf(+erf_range) */
    REAL min_erf;
    REAL scale_erf;

    TRACE("(%p, %0.2f, %0.2f)\n", line, focus, scale);

    if(!line || focus < 0.0 || focus > 1.0 || scale < 0.0 || scale > 1.0)
        return InvalidParameter;

    /* we want 2 standard deviations */
    erf_range = 2.0 / sqrt(2);

    /* calculate the constants we need to normalize the error function to be
        between 0.0 and scale over the range we need */
    min_erf = erf(-erf_range);
    scale_erf = scale / (-2.0 * min_erf);

    if (focus != 0.0)
    {
        positions[0] = 0.0;
        factors[0] = 0.0;
        for (i=1; i<precision; i++)
        {
            positions[i] = focus * i / precision;
            factors[i] = scale_erf * (erf(2 * erf_range * i / precision - erf_range) - min_erf);
        }
        num_points += precision;
    }

    positions[num_points] = focus;
    factors[num_points] = scale;
    num_points += 1;

    if (focus != 1.0)
    {
        for (i=1; i<precision; i++)
        {
            positions[i+num_points-1] = (focus + ((1.0-focus) * i / precision));
            factors[i+num_points-1] = scale_erf * (erf(erf_range - 2 * erf_range * i / precision) - min_erf);
        }
        num_points += precision;
        positions[num_points-1] = 1.0;
        factors[num_points-1] = 0.0;
    }

    return GdipSetLineBlend(line, factors, positions, num_points);
}

GpStatus WINGDIPAPI GdipSetLineWrapMode(GpLineGradient *line,
    GpWrapMode wrap)
{
    TRACE("(%p, %d)\n", line, wrap);

    if(!line || wrap == WrapModeClamp)
        return InvalidParameter;

    line->wrap = wrap;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientBlend(GpPathGradient *brush, GDIPCONST REAL *blend,
    GDIPCONST REAL *pos, INT count)
{
    static int calls;

    TRACE("(%p,%p,%p,%i)\n", brush, blend, pos, count);

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetPathGradientPresetBlend(GpPathGradient *brush,
    GDIPCONST ARGB *blend, GDIPCONST REAL *pos, INT count)
{
    FIXME("(%p,%p,%p,%i): stub\n", brush, blend, pos, count);
    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetPathGradientCenterColor(GpPathGradient *grad,
    ARGB argb)
{
    TRACE("(%p, %x)\n", grad, argb);

    if(!grad)
        return InvalidParameter;

    grad->centercolor = argb;
    grad->brush.lb.lbColor = ARGB2COLORREF(argb);

    DeleteObject(grad->brush.gdibrush);
    grad->brush.gdibrush = CreateSolidBrush(grad->brush.lb.lbColor);

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientCenterPoint(GpPathGradient *grad,
    GpPointF *point)
{
    TRACE("(%p, %s)\n", grad, debugstr_pointf(point));

    if(!grad || !point)
        return InvalidParameter;

    grad->center.X = point->X;
    grad->center.Y = point->Y;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientCenterPointI(GpPathGradient *grad,
    GpPoint *point)
{
    GpPointF ptf;

    TRACE("(%p, %p)\n", grad, point);

    if(!point)
        return InvalidParameter;

    ptf.X = (REAL)point->X;
    ptf.Y = (REAL)point->Y;

    return GdipSetPathGradientCenterPoint(grad,&ptf);
}

GpStatus WINGDIPAPI GdipSetPathGradientFocusScales(GpPathGradient *grad,
    REAL x, REAL y)
{
    TRACE("(%p, %.2f, %.2f)\n", grad, x, y);

    if(!grad)
        return InvalidParameter;

    grad->focus.X = x;
    grad->focus.Y = y;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientGammaCorrection(GpPathGradient *grad,
    BOOL gamma)
{
    TRACE("(%p, %d)\n", grad, gamma);

    if(!grad)
        return InvalidParameter;

    grad->gamma = gamma;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientSigmaBlend(GpPathGradient *grad,
    REAL focus, REAL scale)
{
    static int calls;

    TRACE("(%p,%0.2f,%0.2f)\n", grad, focus, scale);

    if(!grad || focus < 0.0 || focus > 1.0 || scale < 0.0 || scale > 1.0)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetPathGradientSurroundColorsWithCount(GpPathGradient
    *grad, ARGB *argb, INT *count)
{
    static int calls;

    TRACE("(%p,%p,%p)\n", grad, argb, count);

    if(!grad || !argb || !count || (*count <= 0) ||
        (*count > grad->pathdata.Count))
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetPathGradientWrapMode(GpPathGradient *grad,
    GpWrapMode wrap)
{
    TRACE("(%p, %d)\n", grad, wrap);

    if(!grad)
        return InvalidParameter;

    grad->wrap = wrap;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetSolidFillColor(GpSolidFill *sf, ARGB argb)
{
    TRACE("(%p, %x)\n", sf, argb);

    if(!sf)
        return InvalidParameter;

    sf->color = argb;
    sf->brush.lb.lbColor = ARGB2COLORREF(argb);

    DeleteObject(sf->brush.gdibrush);
    sf->brush.gdibrush = CreateSolidBrush(sf->brush.lb.lbColor);

    return Ok;
}

/******************************************************************************
 * GdipSetTextureTransform [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipSetTextureTransform(GpTexture *texture,
    GDIPCONST GpMatrix *matrix)
{
    TRACE("(%p, %p)\n", texture, matrix);

    if(!texture || !matrix)
        return InvalidParameter;

    memcpy(texture->transform, matrix, sizeof(GpMatrix));

    return Ok;
}

/******************************************************************************
 * GdipSetTextureWrapMode [GDIPLUS.@]
 *
 * WrapMode not used, only stored
 */
GpStatus WINGDIPAPI GdipSetTextureWrapMode(GpTexture *brush, GpWrapMode wrapmode)
{
    TRACE("(%p, %d)\n", brush, wrapmode);

    if(!brush)
        return InvalidParameter;

    brush->wrap = wrapmode;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetLineColors(GpLineGradient *brush, ARGB color1,
    ARGB color2)
{
    TRACE("(%p, %x, %x)\n", brush, color1, color2);

    if(!brush)
        return InvalidParameter;

    brush->startcolor = color1;
    brush->endcolor   = color2;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetLineColors(GpLineGradient *brush, ARGB *colors)
{
    TRACE("(%p, %p)\n", brush, colors);

    if(!brush || !colors)
        return InvalidParameter;

    colors[0] = brush->startcolor;
    colors[1] = brush->endcolor;

    return Ok;
}

/******************************************************************************
 * GdipRotateTextureTransform [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipRotateTextureTransform(GpTexture* brush, REAL angle,
    GpMatrixOrder order)
{
    TRACE("(%p, %.2f, %d)\n", brush, angle, order);

    if(!brush)
        return InvalidParameter;

    return GdipRotateMatrix(brush->transform, angle, order);
}

GpStatus WINGDIPAPI GdipSetLineLinearBlend(GpLineGradient *brush, REAL focus,
    REAL scale)
{
    REAL factors[3];
    REAL positions[3];
    int num_points = 0;

    TRACE("(%p,%.2f,%.2f)\n", brush, focus, scale);

    if (!brush) return InvalidParameter;

    if (focus != 0.0)
    {
        factors[num_points] = 0.0;
        positions[num_points] = 0.0;
        num_points++;
    }

    factors[num_points] = scale;
    positions[num_points] = focus;
    num_points++;

    if (focus != 1.0)
    {
        factors[num_points] = 0.0;
        positions[num_points] = 1.0;
        num_points++;
    }

    return GdipSetLineBlend(brush, factors, positions, num_points);
}

GpStatus WINGDIPAPI GdipSetLinePresetBlend(GpLineGradient *brush,
    GDIPCONST ARGB *blend, GDIPCONST REAL* positions, INT count)
{
    ARGB *new_color;
    REAL *new_pos;
    TRACE("(%p,%p,%p,%i)\n", brush, blend, positions, count);

    if (!brush || !blend || !positions || count < 2 ||
        positions[0] != 0.0f || positions[count-1] != 1.0f)
    {
        return InvalidParameter;
    }

    new_color = GdipAlloc(count * sizeof(ARGB));
    new_pos = GdipAlloc(count * sizeof(REAL));
    if (!new_color || !new_pos)
    {
        GdipFree(new_color);
        GdipFree(new_pos);
        return OutOfMemory;
    }

    memcpy(new_color, blend, sizeof(ARGB) * count);
    memcpy(new_pos, positions, sizeof(REAL) * count);

    GdipFree(brush->pblendcolor);
    GdipFree(brush->pblendpos);

    brush->pblendcolor = new_color;
    brush->pblendpos = new_pos;
    brush->pblendcount = count;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetLinePresetBlend(GpLineGradient *brush,
    ARGB *blend, REAL* positions, INT count)
{
    if (!brush || !blend || !positions || count < 2)
        return InvalidParameter;

    if (brush->pblendcount == 0)
        return GenericError;

    if (count < brush->pblendcount)
        return InsufficientBuffer;

    memcpy(blend, brush->pblendcolor, sizeof(ARGB) * brush->pblendcount);
    memcpy(positions, brush->pblendpos, sizeof(REAL) * brush->pblendcount);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetLinePresetBlendCount(GpLineGradient *brush,
    INT *count)
{
    if (!brush || !count)
        return InvalidParameter;

    *count = brush->pblendcount;

    return Ok;
}

GpStatus WINGDIPAPI GdipResetLineTransform(GpLineGradient *brush)
{
    static int calls;

    TRACE("(%p)\n", brush);

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetLineTransform(GpLineGradient *brush,
    GDIPCONST GpMatrix *matrix)
{
    static int calls;

    TRACE("(%p,%p)\n", brush,  matrix);

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipScaleLineTransform(GpLineGradient *brush, REAL sx, REAL sy,
    GpMatrixOrder order)
{
    static int calls;

    TRACE("(%p,%0.2f,%0.2f,%u)\n", brush, sx, sy, order);

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipTranslateLineTransform(GpLineGradient* brush,
        REAL dx, REAL dy, GpMatrixOrder order)
{
    FIXME("stub: %p %f %f %d\n", brush, dx, dy, order);

    return NotImplemented;
}

/******************************************************************************
 * GdipTranslateTextureTransform [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipTranslateTextureTransform(GpTexture* brush, REAL dx, REAL dy,
    GpMatrixOrder order)
{
    TRACE("(%p, %.2f, %.2f, %d)\n", brush, dx, dy, order);

    if(!brush)
        return InvalidParameter;

    return GdipTranslateMatrix(brush->transform, dx, dy, order);
}

GpStatus WINGDIPAPI GdipGetLineRect(GpLineGradient *brush, GpRectF *rect)
{
    TRACE("(%p, %p)\n", brush, rect);

    if(!brush || !rect)
        return InvalidParameter;

    *rect = brush->rect;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetLineRectI(GpLineGradient *brush, GpRect *rect)
{
    GpRectF  rectF;
    GpStatus ret;

    TRACE("(%p, %p)\n", brush, rect);

    if(!rect)
        return InvalidParameter;

    ret = GdipGetLineRect(brush, &rectF);

    if(ret == Ok){
        rect->X      = roundr(rectF.X);
        rect->Y      = roundr(rectF.Y);
        rect->Width  = roundr(rectF.Width);
        rect->Height = roundr(rectF.Height);
    }

    return ret;
}

GpStatus WINGDIPAPI GdipRotateLineTransform(GpLineGradient* brush,
    REAL angle, GpMatrixOrder order)
{
    static int calls;

    TRACE("(%p,%0.2f,%u)\n", brush, angle, order);

    if(!brush)
        return InvalidParameter;

    if(!(calls++))
        FIXME("(%p, %.2f, %d) stub\n", brush, angle, order);

    return NotImplemented;
}
