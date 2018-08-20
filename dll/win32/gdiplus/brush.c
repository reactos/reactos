/*
 * Copyright (C) 2007 Google (Evan Stade)
 * Copyright (C) 2003-2004,2007 Novell, Inc. http://www.novell.com (Ravindra (rkumar@novell.com))
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

#ifdef __REACTOS__
/*
    Unix stuff
    Code from http://www.johndcook.com/blog/2009/01/19/stand-alone-error-function-erf/
*/
double erf(double x)
{
    const float a1 =  0.254829592f;
    const float a2 = -0.284496736f;
    const float a3 =  1.421413741f;
    const float a4 = -1.453152027f;
    const float a5 =  1.061405429f;
    const float p  =  0.3275911f;
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
#endif

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
            *clone = heap_alloc_zero(sizeof(GpSolidFill));
            if (!*clone) return OutOfMemory;
            memcpy(*clone, brush, sizeof(GpSolidFill));
            break;
        }
        case BrushTypeHatchFill:
        {
            GpHatch *hatch = (GpHatch*)brush;

            return GdipCreateHatchBrush(hatch->hatchstyle, hatch->forecol, hatch->backcol, (GpHatch**)clone);
        }
        case BrushTypePathGradient:{
            GpPathGradient *src, *dest;
            INT count, pcount;
            GpStatus stat;

            *clone = heap_alloc_zero(sizeof(GpPathGradient));
            if (!*clone) return OutOfMemory;

            src = (GpPathGradient*) brush,
            dest = (GpPathGradient*) *clone;

            memcpy(dest, src, sizeof(GpPathGradient));

            stat = GdipClonePath(src->path, &dest->path);

            if(stat != Ok){
                heap_free(dest);
                return stat;
            }

            dest->transform = src->transform;

            /* blending */
            count = src->blendcount;
            dest->blendcount = count;
            dest->blendfac = heap_alloc_zero(count * sizeof(REAL));
            dest->blendpos = heap_alloc_zero(count * sizeof(REAL));
            dest->surroundcolors = heap_alloc_zero(dest->surroundcolorcount * sizeof(ARGB));
            pcount = dest->pblendcount;
            if (pcount)
            {
                dest->pblendcolor = heap_alloc_zero(pcount * sizeof(ARGB));
                dest->pblendpos = heap_alloc_zero(pcount * sizeof(REAL));
            }

            if(!dest->blendfac || !dest->blendpos || !dest->surroundcolors ||
               (pcount && (!dest->pblendcolor || !dest->pblendpos))){
                GdipDeletePath(dest->path);
                heap_free(dest->blendfac);
                heap_free(dest->blendpos);
                heap_free(dest->surroundcolors);
                heap_free(dest->pblendcolor);
                heap_free(dest->pblendpos);
                heap_free(dest);
                return OutOfMemory;
            }

            memcpy(dest->blendfac, src->blendfac, count * sizeof(REAL));
            memcpy(dest->blendpos, src->blendpos, count * sizeof(REAL));
            memcpy(dest->surroundcolors, src->surroundcolors, dest->surroundcolorcount * sizeof(ARGB));

            if (pcount)
            {
                memcpy(dest->pblendcolor, src->pblendcolor, pcount * sizeof(ARGB));
                memcpy(dest->pblendpos, src->pblendpos, pcount * sizeof(REAL));
            }

            break;
        }
        case BrushTypeLinearGradient:{
            GpLineGradient *dest, *src;
            INT count, pcount;

            dest = heap_alloc_zero(sizeof(GpLineGradient));
            if(!dest)    return OutOfMemory;

            src = (GpLineGradient*)brush;

            memcpy(dest, src, sizeof(GpLineGradient));

            count = dest->blendcount;
            dest->blendfac = heap_alloc_zero(count * sizeof(REAL));
            dest->blendpos = heap_alloc_zero(count * sizeof(REAL));
            pcount = dest->pblendcount;
            if (pcount)
            {
                dest->pblendcolor = heap_alloc_zero(pcount * sizeof(ARGB));
                dest->pblendpos = heap_alloc_zero(pcount * sizeof(REAL));
            }

            if (!dest->blendfac || !dest->blendpos ||
                (pcount && (!dest->pblendcolor || !dest->pblendpos)))
            {
                heap_free(dest->blendfac);
                heap_free(dest->blendpos);
                heap_free(dest->pblendcolor);
                heap_free(dest->pblendpos);
                heap_free(dest);
                return OutOfMemory;
            }

            dest->transform = src->transform;

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
            UINT width, height;

            stat = GdipGetImageWidth(texture->image, &width);
            if (stat != Ok) return stat;
            stat = GdipGetImageHeight(texture->image, &height);
            if (stat != Ok) return stat;

            stat = GdipCreateTextureIA(texture->image, texture->imageattributes, 0, 0, width, height, &new_texture);

            if (stat == Ok)
            {
                new_texture->transform = texture->transform;
                *clone = &new_texture->brush;
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

GpStatus get_hatch_data(GpHatchStyle hatchstyle, const char **result)
{
    if (hatchstyle < ARRAY_SIZE(HatchBrushes))
    {
        *result = HatchBrushes[hatchstyle];
        return Ok;
    }
    else
        return NotImplemented;
}

/******************************************************************************
 * GdipCreateHatchBrush [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateHatchBrush(GpHatchStyle hatchstyle, ARGB forecol, ARGB backcol, GpHatch **brush)
{
    TRACE("(%d, %d, %d, %p)\n", hatchstyle, forecol, backcol, brush);

    if(!brush)  return InvalidParameter;

    if(hatchstyle < HatchStyleMin || hatchstyle > HatchStyleMax)
        return InvalidParameter;

    *brush = heap_alloc_zero(sizeof(GpHatch));
    if (!*brush) return OutOfMemory;

    (*brush)->brush.bt = BrushTypeHatchFill;
    (*brush)->forecol = forecol;
    (*brush)->backcol = backcol;
    (*brush)->hatchstyle = hatchstyle;
    TRACE("<-- %p\n", *brush);

    return Ok;
}

static GpStatus create_line_brush(const GpRectF *rect, ARGB startcolor, ARGB endcolor,
    GpWrapMode wrap, GpLineGradient **line)
{
    *line = heap_alloc_zero(sizeof(GpLineGradient));
    if(!*line)  return OutOfMemory;

    (*line)->brush.bt = BrushTypeLinearGradient;
    (*line)->startcolor = startcolor;
    (*line)->endcolor = endcolor;
    (*line)->wrap = wrap;
    (*line)->gamma = FALSE;
    (*line)->rect = *rect;
    (*line)->blendcount = 1;
    (*line)->blendfac = heap_alloc_zero(sizeof(REAL));
    (*line)->blendpos = heap_alloc_zero(sizeof(REAL));

    if (!(*line)->blendfac || !(*line)->blendpos)
    {
        heap_free((*line)->blendfac);
        heap_free((*line)->blendpos);
        heap_free(*line);
        *line = NULL;
        return OutOfMemory;
    }

    (*line)->blendfac[0] = 1.0f;
    (*line)->blendpos[0] = 1.0f;

    (*line)->pblendcolor = NULL;
    (*line)->pblendpos = NULL;
    (*line)->pblendcount = 0;

    GdipSetMatrixElements(&(*line)->transform, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);

    return Ok;
}

static void linegradient_init_transform(const GpPointF *startpoint, const GpPointF *endpoint, GpLineGradient *line)
{
    float trans_x = line->rect.X + (line->rect.Width / 2.f);
    float trans_y = line->rect.Y + (line->rect.Height / 2.f);
    float dx = endpoint->X - startpoint->X;
    float dy = endpoint->Y - startpoint->Y;
    float t_cos, t_sin, w_ratio, h_ratio;
    float h;
    GpMatrix rot;

    h = sqrtf(dx * dx + dy * dy);

    t_cos = dx / h;
    t_sin = dy / h;

    w_ratio = (fabs(t_cos) * line->rect.Width + fabs(t_sin) * line->rect.Height) / line->rect.Width;
    h_ratio = (fabs(t_sin) * line->rect.Width + fabs(t_cos) * line->rect.Height) / line->rect.Height;

    GdipSetMatrixElements(&line->transform, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);

    GdipSetMatrixElements(&rot, t_cos, t_sin, -1.f * t_sin, t_cos, 0, 0);

    /* center about the origin */
    GdipTranslateMatrix(&line->transform, -trans_x, -trans_y, MatrixOrderAppend);

    /* scale to normalize gradient along gradient line (?) */
    GdipScaleMatrix(&line->transform, w_ratio, h_ratio, MatrixOrderAppend);

    /* rotate so the gradient is horizontal */
    GdipMultiplyMatrix(&line->transform, &rot, MatrixOrderAppend);

    /* restore original offset in new coords */
    GdipTranslateMatrix(&line->transform, trans_x, trans_y, MatrixOrderAppend);
}

/******************************************************************************
 * GdipCreateLineBrush [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateLineBrush(GDIPCONST GpPointF* startpoint,
    GDIPCONST GpPointF* endpoint, ARGB startcolor, ARGB endcolor,
    GpWrapMode wrap, GpLineGradient **line)
{
    GpStatus stat;
    GpRectF rect;

    TRACE("(%s, %s, %x, %x, %d, %p)\n", debugstr_pointf(startpoint),
          debugstr_pointf(endpoint), startcolor, endcolor, wrap, line);

    if(!line || !startpoint || !endpoint || wrap == WrapModeClamp)
        return InvalidParameter;

    if (startpoint->X == endpoint->X && startpoint->Y == endpoint->Y)
        return OutOfMemory;

    rect.X = startpoint->X < endpoint->X ? startpoint->X : endpoint->X;
    rect.Y = startpoint->Y < endpoint->Y ? startpoint->Y : endpoint->Y;
    rect.Width = fabs(startpoint->X - endpoint->X);
    rect.Height = fabs(startpoint->Y - endpoint->Y);

    if (rect.Width == 0.0f)
    {
        rect.X -= rect.Height / 2.0f;
        rect.Width = rect.Height;
    }
    else if (rect.Height == 0.0f)
    {
        rect.Y -= rect.Width / 2.0f;
        rect.Height = rect.Width;
    }

    stat = create_line_brush(&rect, startcolor, endcolor, wrap, line);
    if (stat != Ok)
        return stat;

    linegradient_init_transform(startpoint, endpoint, *line);

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
    endF.Y = (REAL)endpoint->Y;

    return GdipCreateLineBrush(&stF, &endF, startcolor, endcolor, wrap, line);
}

GpStatus WINGDIPAPI GdipCreateLineBrushFromRect(GDIPCONST GpRectF* rect,
    ARGB startcolor, ARGB endcolor, LinearGradientMode mode, GpWrapMode wrap,
    GpLineGradient **line)
{
    float angle;

    TRACE("(%p, %x, %x, %d, %d, %p)\n", rect, startcolor, endcolor, mode,
          wrap, line);

    if(!line || !rect)
        return InvalidParameter;

    switch (mode)
    {
    case LinearGradientModeHorizontal:
        angle = 0.0f;
        break;
    case LinearGradientModeVertical:
        angle = 90.0f;
        break;
    case LinearGradientModeForwardDiagonal:
        angle = 45.0f;
        break;
    case LinearGradientModeBackwardDiagonal:
        angle = 135.0f;
        break;
    default:
        return InvalidParameter;
    }

    return GdipCreateLineBrushFromRectWithAngle(rect, startcolor, endcolor, angle, TRUE, wrap, line);
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
    REAL exofs, eyofs, far_x, far_y;
    REAL sin_angle, cos_angle, sin_cos_angle;
    GpPointF start, end;

    TRACE("(%p, %x, %x, %.2f, %d, %d, %p)\n", rect, startcolor, endcolor, angle, isAngleScalable,
          wrap, line);

    if (!rect || !line || wrap == WrapModeClamp)
        return InvalidParameter;

    if (!rect->Width || !rect->Height)
        return OutOfMemory;

    angle = fmodf(angle, 360);
    if (angle < 0)
        angle += 360;

    if (isAngleScalable)
    {
        float add_angle = 0;

        while(angle >= 90) {
            angle -= 180;
            add_angle += (float)M_PI;
        }

        if (angle != 90 && angle != -90)
            angle = atan((rect->Width / rect->Height) * tan(deg2rad(angle)));
        else
            angle = deg2rad(angle);
        angle += add_angle;
    }
    else
    {
        angle = deg2rad(angle);
    }

    sin_angle = sinf(angle);
    cos_angle = cosf(angle);
    sin_cos_angle = sin_angle * cos_angle;

    far_x = rect->X + rect->Width;
    far_y = rect->Y + rect->Height;

    if (angle == 0.0f)
    {
        start.X = min(rect->X, far_x);
        start.Y = rect->Y;
        end.X = max(rect->X, far_x);
        end.Y = rect->Y;
    }
    else if (sin_cos_angle >= 0)
    {
        start.X = min(rect->X, far_x);
        start.Y = min(rect->Y, far_y);
        end.X = max(rect->X, far_x);
        end.Y = max(rect->Y, far_y);
    }
    else
    {
        start.X = max(rect->X, far_x);
        start.Y = min(rect->Y, far_y);
        end.X = min(rect->X, far_x);
        end.Y = max(rect->Y, far_y);
    }

    stat = create_line_brush(rect, startcolor, endcolor, wrap, line);
    if (stat != Ok || angle == 0.0f)
        return stat;

    if (sin_cos_angle >= 0)
    {
        exofs = rect->Height * sin_cos_angle + rect->Width * cos_angle * cos_angle;
        eyofs = rect->Height * sin_angle * sin_angle + rect->Width * sin_cos_angle;
    }
    else
    {
        exofs = rect->Width * sin_angle * sin_angle + rect->Height * sin_cos_angle;
        eyofs = -rect->Width * sin_cos_angle + rect->Height * sin_angle * sin_angle;
    }

    if (sin_angle >= 0)
    {
        end.X = rect->X + exofs;
        end.Y = rect->Y + eyofs;
    }
    else
    {
        end.X = start.X;
        end.Y = start.Y;
        start.X = rect->X + exofs;
        start.Y = rect->Y + eyofs;
    }

    linegradient_init_transform(&start, &end, *line);

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

static GpStatus create_path_gradient(GpPath *path, ARGB centercolor, GpPathGradient **grad)
{
    GpRectF bounds;

    if(!path || !grad)
        return InvalidParameter;

    if (path->pathdata.Count < 2)
        return OutOfMemory;

    GdipGetPathWorldBounds(path, &bounds, NULL, NULL);

    *grad = heap_alloc_zero(sizeof(GpPathGradient));
    if (!*grad)
    {
        return OutOfMemory;
    }

    GdipSetMatrixElements(&(*grad)->transform, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);

    (*grad)->blendfac = heap_alloc_zero(sizeof(REAL));
    (*grad)->blendpos = heap_alloc_zero(sizeof(REAL));
    (*grad)->surroundcolors = heap_alloc_zero(sizeof(ARGB));
    if(!(*grad)->blendfac || !(*grad)->blendpos || !(*grad)->surroundcolors){
        heap_free((*grad)->blendfac);
        heap_free((*grad)->blendpos);
        heap_free((*grad)->surroundcolors);
        heap_free(*grad);
        *grad = NULL;
        return OutOfMemory;
    }
    (*grad)->blendfac[0] = 1.0;
    (*grad)->blendpos[0] = 1.0;
    (*grad)->blendcount  = 1;

    (*grad)->path = path;

    (*grad)->brush.bt = BrushTypePathGradient;
    (*grad)->centercolor = centercolor;
    (*grad)->wrap = WrapModeClamp;
    (*grad)->gamma = FALSE;
    /* FIXME: this should be set to the "centroid" of the path by default */
    (*grad)->center.X = bounds.X + bounds.Width / 2;
    (*grad)->center.Y = bounds.Y + bounds.Height / 2;
    (*grad)->focus.X = 0.0;
    (*grad)->focus.Y = 0.0;
    (*grad)->surroundcolors[0] = 0xffffffff;
    (*grad)->surroundcolorcount = 1;

    TRACE("<-- %p\n", *grad);

    return Ok;
}

GpStatus WINGDIPAPI GdipCreatePathGradient(GDIPCONST GpPointF* points,
    INT count, GpWrapMode wrap, GpPathGradient **grad)
{
    GpStatus stat;
    GpPath *path;

    TRACE("(%p, %d, %d, %p)\n", points, count, wrap, grad);

    if(!grad)
        return InvalidParameter;

    if(!points || count <= 0)
        return OutOfMemory;

    stat = GdipCreatePath(FillModeAlternate, &path);

    if (stat == Ok)
    {
        stat = GdipAddPathLine2(path, points, count);

        if (stat == Ok)
            stat = create_path_gradient(path, 0xff000000, grad);

        if (stat != Ok)
            GdipDeletePath(path);
    }

    if (stat == Ok)
        (*grad)->wrap = wrap;

    return stat;
}

GpStatus WINGDIPAPI GdipCreatePathGradientI(GDIPCONST GpPoint* points,
    INT count, GpWrapMode wrap, GpPathGradient **grad)
{
    GpStatus stat;
    GpPath *path;

    TRACE("(%p, %d, %d, %p)\n", points, count, wrap, grad);

    if(!grad)
        return InvalidParameter;

    if(!points || count <= 0)
        return OutOfMemory;

    stat = GdipCreatePath(FillModeAlternate, &path);

    if (stat == Ok)
    {
        stat = GdipAddPathLine2I(path, points, count);

        if (stat == Ok)
            stat = create_path_gradient(path, 0xff000000, grad);

        if (stat != Ok)
            GdipDeletePath(path);
    }

    if (stat == Ok)
        (*grad)->wrap = wrap;

    return stat;
}

/******************************************************************************
 * GdipCreatePathGradientFromPath [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreatePathGradientFromPath(GDIPCONST GpPath* path,
    GpPathGradient **grad)
{
    GpStatus stat;
    GpPath *new_path;

    TRACE("(%p, %p)\n", path, grad);

    if(!grad)
        return InvalidParameter;

    if (!path)
        return OutOfMemory;

    stat = GdipClonePath((GpPath*)path, &new_path);

    if (stat == Ok)
    {
        stat = create_path_gradient(new_path, 0xffffffff, grad);

        if (stat != Ok)
            GdipDeletePath(new_path);
    }

    return stat;
}

/******************************************************************************
 * GdipCreateSolidFill [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateSolidFill(ARGB color, GpSolidFill **sf)
{
    TRACE("(%x, %p)\n", color, sf);

    if(!sf)  return InvalidParameter;

    *sf = heap_alloc_zero(sizeof(GpSolidFill));
    if (!*sf) return OutOfMemory;

    (*sf)->brush.bt = BrushTypeSolidColor;
    (*sf)->color = color;

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
    GpImageAttributes *attributes;
    GpStatus stat;

    TRACE("%p, %d %p\n", image, wrapmode, texture);

    if (!(image && texture))
        return InvalidParameter;

    stat = GdipGetImageWidth(image, &width);
    if (stat != Ok) return stat;
    stat = GdipGetImageHeight(image, &height);
    if (stat != Ok) return stat;

    stat = GdipCreateImageAttributes(&attributes);

    if (stat == Ok)
    {
        attributes->wrap = wrapmode;

        stat = GdipCreateTextureIA(image, attributes, 0, 0, width, height,
            texture);

        GdipDisposeImageAttributes(attributes);
    }

    return stat;
}

/******************************************************************************
 * GdipCreateTexture2 [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateTexture2(GpImage *image, GpWrapMode wrapmode,
        REAL x, REAL y, REAL width, REAL height, GpTexture **texture)
{
    GpImageAttributes *attributes;
    GpStatus stat;

    TRACE("%p %d %f %f %f %f %p\n", image, wrapmode,
            x, y, width, height, texture);

    stat = GdipCreateImageAttributes(&attributes);

    if (stat == Ok)
    {
        attributes->wrap = wrapmode;

        stat = GdipCreateTextureIA(image, attributes, x, y, width, height,
                texture);

        GdipDisposeImageAttributes(attributes);
    }

    return stat;
}

/******************************************************************************
 * GdipCreateTextureIA [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateTextureIA(GpImage *image,
    GDIPCONST GpImageAttributes *imageattr, REAL x, REAL y, REAL width,
    REAL height, GpTexture **texture)
{
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

    *texture = heap_alloc_zero(sizeof(GpTexture));
    if (!*texture){
        status = OutOfMemory;
        goto exit;
    }

    GdipSetMatrixElements(&(*texture)->transform, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);

    if (imageattr)
    {
        status = GdipCloneImageAttributes(imageattr, &(*texture)->imageattributes);
    }
    else
    {
        status = GdipCreateImageAttributes(&(*texture)->imageattributes);
        if (status == Ok)
            (*texture)->imageattributes->wrap = WrapModeTile;
    }
    if (status == Ok)
    {
        (*texture)->brush.bt = BrushTypeTextureFill;
        (*texture)->image = new_image;
    }

exit:
    if (status == Ok)
    {
        TRACE("<-- %p\n", *texture);
    }
    else
    {
        if (*texture)
        {
            GdipDisposeImageAttributes((*texture)->imageattributes);
            heap_free(*texture);
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
    GpImageAttributes *imageattr;
    GpStatus stat;

    TRACE("%p %d %d %d %d %d %p\n", image, wrapmode, x, y, width, height,
            texture);

    stat = GdipCreateImageAttributes(&imageattr);

    if (stat == Ok)
    {
        imageattr->wrap = wrapmode;

        stat = GdipCreateTextureIA(image, imageattr, x, y, width, height, texture);
        GdipDisposeImageAttributes(imageattr);
    }

    return stat;
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

GpStatus WINGDIPAPI GdipGetHatchStyle(GpHatch *brush, GpHatchStyle *hatchstyle)
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
            GdipDeletePath(((GpPathGradient*) brush)->path);
            heap_free(((GpPathGradient*) brush)->blendfac);
            heap_free(((GpPathGradient*) brush)->blendpos);
            heap_free(((GpPathGradient*) brush)->surroundcolors);
            heap_free(((GpPathGradient*) brush)->pblendcolor);
            heap_free(((GpPathGradient*) brush)->pblendpos);
            break;
        case BrushTypeLinearGradient:
            heap_free(((GpLineGradient*)brush)->blendfac);
            heap_free(((GpLineGradient*)brush)->blendpos);
            heap_free(((GpLineGradient*)brush)->pblendcolor);
            heap_free(((GpLineGradient*)brush)->pblendpos);
            break;
        case BrushTypeTextureFill:
            GdipDisposeImage(((GpTexture*)brush)->image);
            GdipDisposeImageAttributes(((GpTexture*)brush)->imageattributes);
            heap_free(((GpTexture*)brush)->bitmap_bits);
            break;
        default:
            break;
    }

    heap_free(brush);

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

    if(!brush || !wrapmode || brush->brush.bt != BrushTypeLinearGradient)
        return InvalidParameter;

    *wrapmode = brush->wrap;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientBlend(GpPathGradient *brush, REAL *blend,
    REAL *positions, INT count)
{
    TRACE("(%p, %p, %p, %d)\n", brush, blend, positions, count);

    if(!brush || !blend || !positions || count <= 0 || brush->brush.bt != BrushTypePathGradient)
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

    if(!brush || !count || brush->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

    *count = brush->blendcount;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientCenterPoint(GpPathGradient *grad,
    GpPointF *point)
{
    TRACE("(%p, %p)\n", grad, point);

    if(!grad || !point || grad->brush.bt != BrushTypePathGradient)
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
        point->X = gdip_round(ptf.X);
        point->Y = gdip_round(ptf.Y);
    }

    return ret;
}

GpStatus WINGDIPAPI GdipGetPathGradientCenterColor(GpPathGradient *grad,
    ARGB *colors)
{
    TRACE("(%p,%p)\n", grad, colors);

    if (!grad || !colors || grad->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

    *colors = grad->centercolor;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientFocusScales(GpPathGradient *grad,
    REAL *x, REAL *y)
{
    TRACE("(%p, %p, %p)\n", grad, x, y);

    if(!grad || !x || !y || grad->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

    *x = grad->focus.X;
    *y = grad->focus.Y;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientGammaCorrection(GpPathGradient *grad,
    BOOL *gamma)
{
    TRACE("(%p, %p)\n", grad, gamma);

    if(!grad || !gamma || grad->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

    *gamma = grad->gamma;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientPath(GpPathGradient *grad, GpPath *path)
{
    static int calls;

    TRACE("(%p, %p)\n", grad, path);

    if (!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetPathGradientPointCount(GpPathGradient *grad,
    INT *count)
{
    TRACE("(%p, %p)\n", grad, count);

    if(!grad || !count || grad->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

    *count = grad->path->pathdata.Count;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientRect(GpPathGradient *brush, GpRectF *rect)
{
    GpStatus stat;

    TRACE("(%p, %p)\n", brush, rect);

    if(!brush || !rect || brush->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

    stat = GdipGetPathWorldBounds(brush->path, rect, NULL, NULL);

    return stat;
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

    rect->X = gdip_round(rectf.X);
    rect->Y = gdip_round(rectf.Y);
    rect->Width  = gdip_round(rectf.Width);
    rect->Height = gdip_round(rectf.Height);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientSurroundColorsWithCount(GpPathGradient
    *grad, ARGB *argb, INT *count)
{
    INT i;

    TRACE("(%p,%p,%p)\n", grad, argb, count);

    if(!grad || !argb || !count || (*count < grad->path->pathdata.Count) || grad->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

    for (i=0; i<grad->path->pathdata.Count; i++)
    {
        if (i < grad->surroundcolorcount)
            argb[i] = grad->surroundcolors[i];
        else
            argb[i] = grad->surroundcolors[grad->surroundcolorcount-1];
    }

    *count = grad->surroundcolorcount;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientSurroundColorCount(GpPathGradient *brush, INT *count)
{
    TRACE("(%p, %p)\n", brush, count);

    if (!brush || !count || brush->brush.bt != BrushTypePathGradient)
       return InvalidParameter;

    /* Yes, this actually returns the number of points in the path (which is the
     * required size of a buffer to get the surround colors), rather than the
     * number of surround colors. The real count is returned when getting the
     * colors. */
    *count = brush->path->pathdata.Count;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientWrapMode(GpPathGradient *brush,
    GpWrapMode *wrapmode)
{
    TRACE("(%p, %p)\n", brush, wrapmode);

    if(!brush || !wrapmode || brush->brush.bt != BrushTypePathGradient)
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

    *matrix = brush->transform;

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

    *wrapmode = brush->imageattributes->wrap;

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

    return GdipMultiplyMatrix(&brush->transform, matrix, order);
}

/******************************************************************************
 * GdipResetTextureTransform [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipResetTextureTransform(GpTexture* brush)
{
    TRACE("(%p)\n", brush);

    if(!brush)
        return InvalidParameter;

    return GdipSetMatrixElements(&brush->transform, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
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

    return GdipScaleMatrix(&brush->transform, sx, sy, order);
}

GpStatus WINGDIPAPI GdipSetLineBlend(GpLineGradient *brush,
    GDIPCONST REAL *factors, GDIPCONST REAL* positions, INT count)
{
    REAL *new_blendfac, *new_blendpos;

    TRACE("(%p, %p, %p, %i)\n", brush, factors, positions, count);

    if(!brush || !factors || !positions || count <= 0 || brush->brush.bt != BrushTypeLinearGradient ||
       (count >= 2 && (positions[0] != 0.0f || positions[count-1] != 1.0f)))
        return InvalidParameter;

    new_blendfac = heap_alloc_zero(count * sizeof(REAL));
    new_blendpos = heap_alloc_zero(count * sizeof(REAL));

    if (!new_blendfac || !new_blendpos)
    {
        heap_free(new_blendfac);
        heap_free(new_blendpos);
        return OutOfMemory;
    }

    memcpy(new_blendfac, factors, count * sizeof(REAL));
    memcpy(new_blendpos, positions, count * sizeof(REAL));

    heap_free(brush->blendfac);
    heap_free(brush->blendpos);

    brush->blendcount = count;
    brush->blendfac = new_blendfac;
    brush->blendpos = new_blendpos;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetLineBlend(GpLineGradient *brush, REAL *factors,
    REAL *positions, INT count)
{
    TRACE("(%p, %p, %p, %i)\n", brush, factors, positions, count);

    if (!brush || !factors || !positions || count <= 0 || brush->brush.bt != BrushTypeLinearGradient)
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

    if (!brush || !count || brush->brush.bt != BrushTypeLinearGradient)
        return InvalidParameter;

    *count = brush->blendcount;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetLineGammaCorrection(GpLineGradient *line,
    BOOL usegamma)
{
    TRACE("(%p, %d)\n", line, usegamma);

    if(!line || line->brush.bt != BrushTypeLinearGradient)
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

    if(!line || focus < 0.0 || focus > 1.0 || scale < 0.0 || scale > 1.0 || line->brush.bt != BrushTypeLinearGradient)
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

    if(!line || wrap == WrapModeClamp || line->brush.bt != BrushTypeLinearGradient)
        return InvalidParameter;

    line->wrap = wrap;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientBlend(GpPathGradient *brush, GDIPCONST REAL *blend,
    GDIPCONST REAL *pos, INT count)
{
    REAL *new_blendfac, *new_blendpos;

    TRACE("(%p,%p,%p,%i)\n", brush, blend, pos, count);

    if(!brush || !blend || !pos || count <= 0 || brush->brush.bt != BrushTypePathGradient ||
       (count >= 2 && (pos[0] != 0.0f || pos[count-1] != 1.0f)))
        return InvalidParameter;

    new_blendfac = heap_alloc_zero(count * sizeof(REAL));
    new_blendpos = heap_alloc_zero(count * sizeof(REAL));

    if (!new_blendfac || !new_blendpos)
    {
        heap_free(new_blendfac);
        heap_free(new_blendpos);
        return OutOfMemory;
    }

    memcpy(new_blendfac, blend, count * sizeof(REAL));
    memcpy(new_blendpos, pos, count * sizeof(REAL));

    heap_free(brush->blendfac);
    heap_free(brush->blendpos);

    brush->blendcount = count;
    brush->blendfac = new_blendfac;
    brush->blendpos = new_blendpos;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientLinearBlend(GpPathGradient *brush,
    REAL focus, REAL scale)
{
    REAL factors[3];
    REAL positions[3];
    int num_points = 0;

    TRACE("(%p,%0.2f,%0.2f)\n", brush, focus, scale);

    if (!brush || brush->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

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

    return GdipSetPathGradientBlend(brush, factors, positions, num_points);
}

GpStatus WINGDIPAPI GdipSetPathGradientPresetBlend(GpPathGradient *brush,
    GDIPCONST ARGB *blend, GDIPCONST REAL *pos, INT count)
{
    ARGB *new_color;
    REAL *new_pos;
    TRACE("(%p,%p,%p,%i)\n", brush, blend, pos, count);

    if (!brush || !blend || !pos || count < 2 || brush->brush.bt != BrushTypePathGradient ||
        pos[0] != 0.0f || pos[count-1] != 1.0f)
    {
        return InvalidParameter;
    }

    new_color = heap_alloc_zero(count * sizeof(ARGB));
    new_pos = heap_alloc_zero(count * sizeof(REAL));
    if (!new_color || !new_pos)
    {
        heap_free(new_color);
        heap_free(new_pos);
        return OutOfMemory;
    }

    memcpy(new_color, blend, sizeof(ARGB) * count);
    memcpy(new_pos, pos, sizeof(REAL) * count);

    heap_free(brush->pblendcolor);
    heap_free(brush->pblendpos);

    brush->pblendcolor = new_color;
    brush->pblendpos = new_pos;
    brush->pblendcount = count;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientPresetBlend(GpPathGradient *brush,
    ARGB *blend, REAL *pos, INT count)
{
    TRACE("(%p,%p,%p,%i)\n", brush, blend, pos, count);

    if (count < 0)
        return OutOfMemory;

    if (!brush || !blend || !pos || count < 2 || brush->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

    if (brush->pblendcount == 0)
        return GenericError;

    if (count != brush->pblendcount)
    {
        /* Native lines up the ends of each array, and copies the destination size. */
        FIXME("Braindead behavior on wrong-sized buffer not implemented.\n");
        return InvalidParameter;
    }

    memcpy(blend, brush->pblendcolor, sizeof(ARGB) * brush->pblendcount);
    memcpy(pos, brush->pblendpos, sizeof(REAL) * brush->pblendcount);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientPresetBlendCount(GpPathGradient *brush,
    INT *count)
{
    TRACE("(%p,%p)\n", brush, count);

    if (!brush || !count || brush->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

    *count = brush->pblendcount;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientCenterColor(GpPathGradient *grad,
    ARGB argb)
{
    TRACE("(%p, %x)\n", grad, argb);

    if(!grad || grad->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

    grad->centercolor = argb;
    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientCenterPoint(GpPathGradient *grad,
    GpPointF *point)
{
    TRACE("(%p, %s)\n", grad, debugstr_pointf(point));

    if(!grad || !point || grad->brush.bt != BrushTypePathGradient)
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

    if(!grad || grad->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

    grad->focus.X = x;
    grad->focus.Y = y;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientGammaCorrection(GpPathGradient *grad,
    BOOL gamma)
{
    TRACE("(%p, %d)\n", grad, gamma);

    if(!grad || grad->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

    grad->gamma = gamma;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientPath(GpPathGradient *grad, GDIPCONST GpPath *path)
{
    static int calls;

    TRACE("(%p, %p)\n", grad, path);

    if (!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetPathGradientSigmaBlend(GpPathGradient *grad,
    REAL focus, REAL scale)
{
    REAL factors[33];
    REAL positions[33];
    int num_points = 0;
    int i;
    const int precision = 16;
    REAL erf_range; /* we use values erf(-erf_range) through erf(+erf_range) */
    REAL min_erf;
    REAL scale_erf;

    TRACE("(%p,%0.2f,%0.2f)\n", grad, focus, scale);

    if(!grad || focus < 0.0 || focus > 1.0 || scale < 0.0 || scale > 1.0 || grad->brush.bt != BrushTypePathGradient)
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

    return GdipSetPathGradientBlend(grad, factors, positions, num_points);
}

GpStatus WINGDIPAPI GdipSetPathGradientSurroundColorsWithCount(GpPathGradient
    *grad, GDIPCONST ARGB *argb, INT *count)
{
    ARGB *new_surroundcolors;
    INT i, num_colors;

    TRACE("(%p,%p,%p)\n", grad, argb, count);

    if(!grad || !argb || !count || (*count <= 0) || grad->brush.bt != BrushTypePathGradient ||
        (*count > grad->path->pathdata.Count))
        return InvalidParameter;

    num_colors = *count;

    /* If all colors are the same, only store 1 color. */
    if (*count > 1)
    {
        for (i=1; i < num_colors; i++)
            if (argb[i] != argb[i-1])
                break;

        if (i == num_colors)
            num_colors = 1;
    }

    new_surroundcolors = heap_alloc_zero(num_colors * sizeof(ARGB));
    if (!new_surroundcolors)
        return OutOfMemory;

    memcpy(new_surroundcolors, argb, num_colors * sizeof(ARGB));

    heap_free(grad->surroundcolors);

    grad->surroundcolors = new_surroundcolors;
    grad->surroundcolorcount = num_colors;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientWrapMode(GpPathGradient *grad,
    GpWrapMode wrap)
{
    TRACE("(%p, %d)\n", grad, wrap);

    if(!grad || grad->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

    grad->wrap = wrap;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientTransform(GpPathGradient *grad,
    GpMatrix *matrix)
{
    TRACE("(%p,%p)\n", grad, matrix);

    if (!grad || !matrix || grad->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

    grad->transform = *matrix;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientTransform(GpPathGradient *grad,
    GpMatrix *matrix)
{
    TRACE("(%p,%p)\n", grad, matrix);

    if (!grad || !matrix || grad->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

    *matrix = grad->transform;

    return Ok;
}

GpStatus WINGDIPAPI GdipMultiplyPathGradientTransform(GpPathGradient *grad,
    GDIPCONST GpMatrix *matrix, GpMatrixOrder order)
{
    TRACE("(%p,%p,%i)\n", grad, matrix, order);

    if (!grad || grad->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

    return GdipMultiplyMatrix(&grad->transform, matrix, order);
}

GpStatus WINGDIPAPI GdipResetPathGradientTransform(GpPathGradient *grad)
{
    TRACE("(%p)\n", grad);

    if (!grad || grad->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

    return GdipSetMatrixElements(&grad->transform, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
}

GpStatus WINGDIPAPI GdipRotatePathGradientTransform(GpPathGradient *grad,
    REAL angle, GpMatrixOrder order)
{
    TRACE("(%p,%0.2f,%i)\n", grad, angle, order);

    if (!grad || grad->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

    return GdipRotateMatrix(&grad->transform, angle, order);
}

GpStatus WINGDIPAPI GdipScalePathGradientTransform(GpPathGradient *grad,
    REAL sx, REAL sy, GpMatrixOrder order)
{
    TRACE("(%p,%0.2f,%0.2f,%i)\n", grad, sx, sy, order);

    if (!grad || grad->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

    return GdipScaleMatrix(&grad->transform, sx, sy, order);
}

GpStatus WINGDIPAPI GdipTranslatePathGradientTransform(GpPathGradient *grad,
    REAL dx, REAL dy, GpMatrixOrder order)
{
    TRACE("(%p,%0.2f,%0.2f,%i)\n", grad, dx, dy, order);

    if (!grad || grad->brush.bt != BrushTypePathGradient)
        return InvalidParameter;

    return GdipTranslateMatrix(&grad->transform, dx, dy, order);
}

GpStatus WINGDIPAPI GdipSetSolidFillColor(GpSolidFill *sf, ARGB argb)
{
    TRACE("(%p, %x)\n", sf, argb);

    if(!sf)
        return InvalidParameter;

    sf->color = argb;
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

    texture->transform = *matrix;

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

    brush->imageattributes->wrap = wrapmode;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetLineColors(GpLineGradient *brush, ARGB color1,
    ARGB color2)
{
    TRACE("(%p, %x, %x)\n", brush, color1, color2);

    if(!brush || brush->brush.bt != BrushTypeLinearGradient)
        return InvalidParameter;

    brush->startcolor = color1;
    brush->endcolor   = color2;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetLineColors(GpLineGradient *brush, ARGB *colors)
{
    TRACE("(%p, %p)\n", brush, colors);

    if(!brush || !colors || brush->brush.bt != BrushTypeLinearGradient)
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

    return GdipRotateMatrix(&brush->transform, angle, order);
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

    if (!brush || !blend || !positions || count < 2 || brush->brush.bt != BrushTypeLinearGradient ||
        positions[0] != 0.0f || positions[count-1] != 1.0f)
    {
        return InvalidParameter;
    }

    new_color = heap_alloc_zero(count * sizeof(ARGB));
    new_pos = heap_alloc_zero(count * sizeof(REAL));
    if (!new_color || !new_pos)
    {
        heap_free(new_color);
        heap_free(new_pos);
        return OutOfMemory;
    }

    memcpy(new_color, blend, sizeof(ARGB) * count);
    memcpy(new_pos, positions, sizeof(REAL) * count);

    heap_free(brush->pblendcolor);
    heap_free(brush->pblendpos);

    brush->pblendcolor = new_color;
    brush->pblendpos = new_pos;
    brush->pblendcount = count;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetLinePresetBlend(GpLineGradient *brush,
    ARGB *blend, REAL* positions, INT count)
{
    if (!brush || !blend || !positions || count < 2 || brush->brush.bt != BrushTypeLinearGradient)
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
    if (!brush || !count || brush->brush.bt != BrushTypeLinearGradient)
        return InvalidParameter;

    *count = brush->pblendcount;

    return Ok;
}

GpStatus WINGDIPAPI GdipResetLineTransform(GpLineGradient *brush)
{
    TRACE("(%p)\n", brush);

    if(!brush)
        return InvalidParameter;

    return GdipSetMatrixElements(&brush->transform, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
}

GpStatus WINGDIPAPI GdipSetLineTransform(GpLineGradient *brush,
    GDIPCONST GpMatrix *matrix)
{
    TRACE("(%p,%p)\n", brush,  matrix);

    if(!brush || !matrix)
        return InvalidParameter;

    brush->transform = *matrix;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetLineTransform(GpLineGradient *brush, GpMatrix *matrix)
{
    TRACE("(%p,%p)\n", brush, matrix);

    if(!brush || !matrix)
        return InvalidParameter;

    *matrix = brush->transform;

    return Ok;
}

GpStatus WINGDIPAPI GdipScaleLineTransform(GpLineGradient *brush, REAL sx, REAL sy,
    GpMatrixOrder order)
{
    TRACE("(%p,%0.2f,%0.2f,%u)\n", brush, sx, sy, order);

    if(!brush)
        return InvalidParameter;

    return GdipScaleMatrix(&brush->transform, sx, sy, order);
}

GpStatus WINGDIPAPI GdipMultiplyLineTransform(GpLineGradient *brush,
    GDIPCONST GpMatrix *matrix, GpMatrixOrder order)
{
    TRACE("(%p,%p,%u)\n", brush, matrix, order);

    if(!brush)
        return InvalidParameter;

    if(!matrix)
        return Ok;

    return GdipMultiplyMatrix(&brush->transform, matrix, order);
}

GpStatus WINGDIPAPI GdipTranslateLineTransform(GpLineGradient *brush,
        REAL dx, REAL dy, GpMatrixOrder order)
{
    TRACE("(%p,%f,%f,%d)\n", brush, dx, dy, order);

    if(!brush)
        return InvalidParameter;

    return GdipTranslateMatrix(&brush->transform, dx, dy, order);
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

    return GdipTranslateMatrix(&brush->transform, dx, dy, order);
}

GpStatus WINGDIPAPI GdipGetLineRect(GpLineGradient *brush, GpRectF *rect)
{
    TRACE("(%p, %p)\n", brush, rect);

    if(!brush || !rect || brush->brush.bt != BrushTypeLinearGradient)
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
        rect->X      = gdip_round(rectF.X);
        rect->Y      = gdip_round(rectF.Y);
        rect->Width  = gdip_round(rectF.Width);
        rect->Height = gdip_round(rectF.Height);
    }

    return ret;
}

GpStatus WINGDIPAPI GdipRotateLineTransform(GpLineGradient* brush,
    REAL angle, GpMatrixOrder order)
{
    static int calls;

    TRACE("(%p,%0.2f,%u)\n", brush, angle, order);

    if(!brush || brush->brush.bt != BrushTypeLinearGradient)
        return InvalidParameter;

    if(!(calls++))
        FIXME("(%p, %.2f, %d) stub\n", brush, angle, order);

    return NotImplemented;
}
