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
#include "wingdi.h"

#include "objbase.h"

#include "gdiplus.h"
#include "gdiplus_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdiplus);

static DWORD gdip_to_gdi_dash(GpDashStyle dash)
{
    switch(dash){
        case DashStyleSolid:
            return PS_SOLID;
        case DashStyleDash:
            return PS_DASH;
        case DashStyleDot:
            return PS_DOT;
        case DashStyleDashDot:
            return PS_DASHDOT;
        case DashStyleDashDotDot:
            return PS_DASHDOTDOT;
        case DashStyleCustom:
            return PS_USERSTYLE;
        default:
            ERR("Not a member of GpDashStyle enumeration\n");
            return 0;
    }
}

static DWORD gdip_to_gdi_join(GpLineJoin join)
{
    switch(join){
        case LineJoinRound:
            return PS_JOIN_ROUND;
        case LineJoinBevel:
            return PS_JOIN_BEVEL;
        case LineJoinMiter:
        case LineJoinMiterClipped:
            return PS_JOIN_MITER;
        default:
            ERR("Not a member of GpLineJoin enumeration\n");
            return 0;
    }
}

static GpPenType bt_to_pt(GpBrushType bt)
{
    switch(bt){
        case BrushTypeSolidColor:
            return PenTypeSolidColor;
        case BrushTypeHatchFill:
            return PenTypeHatchFill;
        case BrushTypeTextureFill:
            return PenTypeTextureFill;
        case BrushTypePathGradient:
            return PenTypePathGradient;
        case BrushTypeLinearGradient:
            return PenTypeLinearGradient;
        default:
            return PenTypeUnknown;
    }
}

GpStatus WINGDIPAPI GdipClonePen(GpPen *pen, GpPen **clonepen)
{
    GpStatus stat;

    TRACE("(%p, %p)\n", pen, clonepen);

    if(!pen || !clonepen)
        return InvalidParameter;

    *clonepen = malloc(sizeof(GpPen));
    if(!*clonepen) return OutOfMemory;

    **clonepen = *pen;

    (*clonepen)->customstart = NULL;
    (*clonepen)->customend = NULL;
    (*clonepen)->brush = NULL;
    (*clonepen)->dashes = NULL;
    (*clonepen)->compound_array = NULL;

    stat = GdipCloneBrush(pen->brush, &(*clonepen)->brush);

    if (stat == Ok && pen->customstart)
        stat = GdipCloneCustomLineCap(pen->customstart, &(*clonepen)->customstart);

    if (stat == Ok && pen->customend)
        stat = GdipCloneCustomLineCap(pen->customend, &(*clonepen)->customend);

    if (stat == Ok && pen->dashes)
    {
        (*clonepen)->dashes = malloc(pen->numdashes * sizeof(REAL));
        if ((*clonepen)->dashes)
            memcpy((*clonepen)->dashes, pen->dashes, pen->numdashes * sizeof(REAL));
        else
            stat = OutOfMemory;
    }

    if (stat == Ok && pen->compound_array)
    {
        (*clonepen)->compound_array = malloc(pen->compound_array_size * sizeof(REAL));
        if ((*clonepen)->compound_array)
            memcpy((*clonepen)->compound_array, pen->compound_array, pen->compound_array_size * sizeof(REAL));
        else
            stat = OutOfMemory;
    }

    if (stat != Ok)
    {
        GdipDeletePen(*clonepen);
        *clonepen = NULL;
        return stat;
    }

    TRACE("<-- %p\n", *clonepen);

    return Ok;
}

GpStatus WINGDIPAPI GdipCreatePen1(ARGB color, REAL width, GpUnit unit,
    GpPen **pen)
{
    GpBrush *brush;
    GpStatus status;

    TRACE("(%lx, %.2f, %d, %p)\n", color, width, unit, pen);

    GdipCreateSolidFill(color, (GpSolidFill **)(&brush));
    status = GdipCreatePen2(brush, width, unit, pen);
    GdipDeleteBrush(brush);
    return status;
}

GpStatus WINGDIPAPI GdipCreatePen2(GpBrush *brush, REAL width, GpUnit unit,
    GpPen **pen)
{
    GpPen *gp_pen;
    GpBrush *clone_brush;

    TRACE("(%p, %.2f, %d, %p)\n", brush, width, unit, pen);

    if(!pen || !brush)
        return InvalidParameter;

    gp_pen = calloc(1, sizeof(GpPen));
    if(!gp_pen) return OutOfMemory;

    gp_pen->style = GP_DEFAULT_PENSTYLE;
    gp_pen->width = width;
    gp_pen->unit = unit;
    gp_pen->endcap = LineCapFlat;
    gp_pen->join = LineJoinMiter;
    gp_pen->miterlimit = 10.0;
    gp_pen->dash = DashStyleSolid;
    gp_pen->offset = 0.0;
    gp_pen->customstart = NULL;
    gp_pen->customend = NULL;
    gp_pen->compound_array = NULL;
    gp_pen->compound_array_size = 0;
    GdipSetMatrixElements(&gp_pen->transform, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);

    if(!((gp_pen->unit == UnitWorld) || (gp_pen->unit == UnitPixel))) {
        FIXME("UnitWorld, UnitPixel only supported units\n");
        free(gp_pen);
        return NotImplemented;
    }

    GdipCloneBrush(brush, &clone_brush);
    gp_pen->brush = clone_brush;

    *pen = gp_pen;

    TRACE("<-- %p\n", *pen);

    return Ok;
}

GpStatus WINGDIPAPI GdipDeletePen(GpPen *pen)
{
    TRACE("(%p)\n", pen);

    if(!pen)    return InvalidParameter;

    GdipDeleteBrush(pen->brush);
    GdipDeleteCustomLineCap(pen->customstart);
    GdipDeleteCustomLineCap(pen->customend);
    free(pen->compound_array);
    free(pen->dashes);
    free(pen);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenBrushFill(GpPen *pen, GpBrush **brush)
{
    TRACE("(%p, %p)\n", pen, brush);

    if(!pen || !brush)
        return InvalidParameter;

    return GdipCloneBrush(pen->brush, brush);
}

GpStatus WINGDIPAPI GdipGetPenColor(GpPen *pen, ARGB *argb)
{
    TRACE("(%p, %p)\n", pen, argb);

    if(!pen || !argb)
        return InvalidParameter;

    if(pen->brush->bt != BrushTypeSolidColor)
        return NotImplemented;

    return GdipGetSolidFillColor(((GpSolidFill*)pen->brush), argb);
}

GpStatus WINGDIPAPI GdipGetPenCustomEndCap(GpPen *pen, GpCustomLineCap** customCap)
{
    TRACE("(%p, %p)\n", pen, customCap);

    if(!pen || !customCap)
        return InvalidParameter;

    if(!pen->customend){
        *customCap = NULL;
        return Ok;
    }

    return GdipCloneCustomLineCap(pen->customend, customCap);
}

GpStatus WINGDIPAPI GdipGetPenCustomStartCap(GpPen *pen, GpCustomLineCap** customCap)
{
    TRACE("(%p, %p)\n", pen, customCap);

    if(!pen || !customCap)
        return InvalidParameter;

    if(!pen->customstart){
        *customCap = NULL;
        return Ok;
    }

    return GdipCloneCustomLineCap(pen->customstart, customCap);
}

GpStatus WINGDIPAPI GdipGetPenDashArray(GpPen *pen, REAL *dash, INT count)
{
    TRACE("(%p, %p, %d)\n", pen, dash, count);

    if(!pen || !dash || count > pen->numdashes)
        return InvalidParameter;

    /* note: if you pass a negative value for count, it crashes native gdiplus. */
    if(count < 0)
        return GenericError;

    memcpy(dash, pen->dashes, count * sizeof(REAL));

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenDashCap197819(GpPen *pen, GpDashCap *dashCap)
{
    TRACE("(%p, %p)\n", pen, dashCap);

    if(!pen || !dashCap)
        return InvalidParameter;

    *dashCap = pen->dashcap;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenDashCount(GpPen *pen, INT *count)
{
    TRACE("(%p, %p)\n", pen, count);

    if(!pen || !count)
        return InvalidParameter;

    *count = pen->numdashes;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenDashOffset(GpPen *pen, REAL *offset)
{
    TRACE("(%p, %p)\n", pen, offset);

    if(!pen || !offset)
        return InvalidParameter;

    *offset = pen->offset;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenDashStyle(GpPen *pen, GpDashStyle *dash)
{
    TRACE("(%p, %p)\n", pen, dash);

    if(!pen || !dash)
        return InvalidParameter;

    *dash = pen->dash;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenEndCap(GpPen *pen, GpLineCap *endCap)
{
    TRACE("(%p, %p)\n", pen, endCap);

    if(!pen || !endCap)
        return InvalidParameter;

    *endCap = pen->endcap;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenFillType(GpPen *pen, GpPenType* type)
{
    TRACE("(%p, %p)\n", pen, type);

    if(!pen || !type)
        return InvalidParameter;

    *type = bt_to_pt(pen->brush->bt);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenLineJoin(GpPen *pen, GpLineJoin *lineJoin)
{
    TRACE("(%p, %p)\n", pen, lineJoin);

    if(!pen || !lineJoin)
        return InvalidParameter;

    *lineJoin = pen->join;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenMode(GpPen *pen, GpPenAlignment *mode)
{
    TRACE("(%p, %p)\n", pen, mode);

    if(!pen || !mode)
        return InvalidParameter;

    *mode = pen->align;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenMiterLimit(GpPen *pen, REAL *miterLimit)
{
    TRACE("(%p, %p)\n", pen, miterLimit);

    if(!pen || !miterLimit)
        return InvalidParameter;

    *miterLimit = pen->miterlimit;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenStartCap(GpPen *pen, GpLineCap *startCap)
{
    TRACE("(%p, %p)\n", pen, startCap);

    if(!pen || !startCap)
        return InvalidParameter;

    *startCap = pen->startcap;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenUnit(GpPen *pen, GpUnit *unit)
{
    TRACE("(%p, %p)\n", pen, unit);

    if(!pen || !unit)
        return InvalidParameter;

    *unit = pen->unit;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenWidth(GpPen *pen, REAL *width)
{
    TRACE("(%p, %p)\n", pen, width);

    if(!pen || !width)
        return InvalidParameter;

    *width = pen->width;

    return Ok;
}

GpStatus WINGDIPAPI GdipResetPenTransform(GpPen *pen)
{
    TRACE("(%p)\n", pen);

    if(!pen)
        return InvalidParameter;

    GdipSetMatrixElements(&pen->transform, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenTransform(GpPen *pen, GpMatrix *matrix)
{
    static int calls;
    BOOL result;

    TRACE("(%p, %s)\n", pen, debugstr_matrix(matrix));

    if(!pen || !matrix)
        return InvalidParameter;

    if(!(calls++))
        FIXME("(%p,%p) Semi-stub\n", pen, matrix);

    GdipIsMatrixInvertible(matrix, &result);
    if (!result)
        return InvalidParameter;
    pen->transform = *matrix;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenTransform(GpPen *pen, GpMatrix *matrix)
{
    TRACE("(%p,%s)\n", pen, debugstr_matrix(matrix));

    if(!pen || !matrix)
        return InvalidParameter;

    *matrix = pen->transform;

    return Ok;
}

GpStatus WINGDIPAPI GdipTranslatePenTransform(GpPen *pen, REAL dx, REAL dy, GpMatrixOrder order)
{
    TRACE("(%p,%0.2f,%0.2f,%u)\n", pen, dx, dy, order);

    if(!pen)
        return InvalidParameter;

    return GdipTranslateMatrix(&pen->transform, dx, dy, order);
}

GpStatus WINGDIPAPI GdipScalePenTransform(GpPen *pen, REAL sx, REAL sy, GpMatrixOrder order)
{
    TRACE("(%p,%0.2f,%0.2f,%u)\n", pen, sx, sy, order);

    if(!pen)
        return InvalidParameter;

    return GdipScaleMatrix(&pen->transform, sx, sy, order);
}

GpStatus WINGDIPAPI GdipRotatePenTransform(GpPen *pen, REAL angle, GpMatrixOrder order)
{
    TRACE("(%p,%0.2f,%u)\n", pen, angle, order);

    if(!pen)
        return InvalidParameter;

    return GdipRotateMatrix(&pen->transform, angle, order);
}

GpStatus WINGDIPAPI GdipMultiplyPenTransform(GpPen *pen, GDIPCONST GpMatrix *matrix,
    GpMatrixOrder order)
{
    TRACE("(%p,%s,%u)\n", pen, debugstr_matrix(matrix), order);

    if(!pen)
        return InvalidParameter;

    return GdipMultiplyMatrix(&pen->transform, matrix, order);
}

GpStatus WINGDIPAPI GdipSetPenBrushFill(GpPen *pen, GpBrush *brush)
{
    TRACE("(%p, %p)\n", pen, brush);

    if(!pen || !brush)
        return InvalidParameter;

    GdipDeleteBrush(pen->brush);
    return GdipCloneBrush(brush, &pen->brush);
}

GpStatus WINGDIPAPI GdipSetPenColor(GpPen *pen, ARGB argb)
{
    TRACE("(%p, %lx)\n", pen, argb);

    if(!pen)
        return InvalidParameter;

    if(pen->brush->bt != BrushTypeSolidColor)
        return NotImplemented;

    return GdipSetSolidFillColor(((GpSolidFill*)pen->brush), argb);
}

GpStatus WINGDIPAPI GdipGetPenCompoundArray(GpPen *pen, REAL *compoundarray, INT count)
{
    TRACE("(%p, %p, %i)\n", pen, compoundarray, count);

    if (!pen || !compoundarray || count > pen->compound_array_size)
        return InvalidParameter;
    if (pen->compound_array && count > 0)
        memcpy(compoundarray, pen->compound_array, count * sizeof(REAL));
    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenCompoundCount(GpPen *pen, INT *count)
{
    TRACE("(%p, %p)\n", pen, count);

    if (!pen || !count)
        return InvalidParameter;
    *count = pen->compound_array_size;
    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenCompoundArray(GpPen *pen, GDIPCONST REAL *compoundarray,
    INT count)
{
    INT i;
    REAL *tmp;
    TRACE("(%p, %p, %i)\n", pen, compoundarray, count);

    if(!pen || !compoundarray || count < 2 || count%2 == 1 || *compoundarray < 0.0 || *compoundarray > 1.0)
        return InvalidParameter;

    for(i = 1; i<count; i++)
    {
        if((compoundarray[i] < compoundarray[i - 1]) || (compoundarray[i] > 1.0))
            return InvalidParameter;
    }

    tmp = malloc(count * sizeof(REAL));
    if(!tmp)
        return OutOfMemory;
    free(pen->compound_array);
    pen->compound_array = tmp;
    memcpy(pen->compound_array, compoundarray, count * sizeof(REAL));
    pen->compound_array_size = count;
    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenCustomEndCap(GpPen *pen, GpCustomLineCap* customCap)
{
    GpCustomLineCap * cap;
    GpStatus ret;

    TRACE("(%p, %p)\n", pen, customCap);

    /* native crashes on pen == NULL, customCap != NULL */
    if(!customCap) return InvalidParameter;

    if((ret = GdipCloneCustomLineCap(customCap, &cap)) == Ok){
        GdipDeleteCustomLineCap(pen->customend);
        pen->endcap = LineCapCustom;
        pen->customend = cap;
    }

    return ret;
}

GpStatus WINGDIPAPI GdipSetPenCustomStartCap(GpPen *pen, GpCustomLineCap* customCap)
{
    GpCustomLineCap * cap;
    GpStatus ret;

    TRACE("(%p, %p)\n", pen, customCap);

    /* native crashes on pen == NULL, customCap != NULL */
    if(!customCap) return InvalidParameter;

    if((ret = GdipCloneCustomLineCap(customCap, &cap)) == Ok){
        GdipDeleteCustomLineCap(pen->customstart);
        pen->startcap = LineCapCustom;
        pen->customstart = cap;
    }

    return ret;
}

GpStatus WINGDIPAPI GdipSetPenDashArray(GpPen *pen, GDIPCONST REAL *dash,
    INT count)
{
    INT i;

    TRACE("(%p, %p, %d)\n", pen, dash, count);

    if(!pen || !dash)
        return InvalidParameter;

    if(count <= 0)
        return OutOfMemory;

    for(i = 0; i < count; i++){
        if(dash[i] <= 0.0)
            return InvalidParameter;
    }

    free(pen->dashes);
    pen->dashes = NULL;

    if(count > 0)
        pen->dashes = malloc(count * sizeof(REAL));
    if(!pen->dashes){
        pen->numdashes = 0;
        return OutOfMemory;
    }

    GdipSetPenDashStyle(pen, DashStyleCustom);
    memcpy(pen->dashes, dash, count * sizeof(REAL));
    pen->numdashes = count;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenDashCap197819(GpPen *pen, GpDashCap dashCap)
{
    TRACE("(%p, %d)\n", pen, dashCap);

    if(!pen)
        return InvalidParameter;

    pen->dashcap = dashCap;

    return Ok;
}

/* FIXME: dash offset not used */
GpStatus WINGDIPAPI GdipSetPenDashOffset(GpPen *pen, REAL offset)
{
    TRACE("(%p, %.2f)\n", pen, offset);

    if(!pen)
        return InvalidParameter;

    pen->offset = offset;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenDashStyle(GpPen *pen, GpDashStyle dash)
{
    TRACE("(%p, %d)\n", pen, dash);

    if(!pen)
        return InvalidParameter;

    if(dash != DashStyleCustom){
        free(pen->dashes);
        pen->dashes = NULL;
        pen->numdashes = 0;
    }

    pen->dash = dash;
    pen->style &= ~(PS_ALTERNATE | PS_SOLID | PS_DASH | PS_DOT | PS_DASHDOT |
                    PS_DASHDOTDOT | PS_NULL | PS_USERSTYLE | PS_INSIDEFRAME);
    pen->style |= gdip_to_gdi_dash(dash);

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenEndCap(GpPen *pen, GpLineCap cap)
{
    TRACE("(%p, %d)\n", pen, cap);

    if(!pen)    return InvalidParameter;

    /* The old custom cap gets deleted even if the new style is LineCapCustom. */
    GdipDeleteCustomLineCap(pen->customend);
    pen->customend = NULL;
    pen->endcap = cap;

    return Ok;
}

/* FIXME: startcap, dashcap not used. */
GpStatus WINGDIPAPI GdipSetPenLineCap197819(GpPen *pen, GpLineCap start,
    GpLineCap end, GpDashCap dash)
{
    TRACE("%p, %d, %d, %d)\n", pen, start, end, dash);

    if(!pen)
        return InvalidParameter;

    GdipDeleteCustomLineCap(pen->customend);
    GdipDeleteCustomLineCap(pen->customstart);
    pen->customend = NULL;
    pen->customstart = NULL;

    pen->startcap = start;
    pen->endcap = end;
    pen->dashcap = dash;

    return Ok;
}

/* FIXME: Miter line joins behave a bit differently than they do in windows.
 * Both kinds of miter joins clip if the angle is less than 11 degrees. */
GpStatus WINGDIPAPI GdipSetPenLineJoin(GpPen *pen, GpLineJoin join)
{
    TRACE("(%p, %d)\n", pen, join);

    if(!pen)    return InvalidParameter;

    pen->join = join;
    pen->style &= ~(PS_JOIN_ROUND | PS_JOIN_BEVEL | PS_JOIN_MITER);
    pen->style |= gdip_to_gdi_join(join);

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenMiterLimit(GpPen *pen, REAL limit)
{
    TRACE("(%p, %.2f)\n", pen, limit);

    if(!pen)
        return InvalidParameter;

    pen->miterlimit = limit;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenStartCap(GpPen *pen, GpLineCap cap)
{
    TRACE("(%p, %d)\n", pen, cap);

    if(!pen)    return InvalidParameter;

    GdipDeleteCustomLineCap(pen->customstart);
    pen->customstart = NULL;
    pen->startcap = cap;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenWidth(GpPen *pen, REAL width)
{
    TRACE("(%p, %.2f)\n", pen, width);

    if(!pen)    return InvalidParameter;

    pen->width = width;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenMode(GpPen *pen, GpPenAlignment mode)
{
    TRACE("(%p, %d)\n", pen, mode);

    if(!pen)    return InvalidParameter;

    pen->align = mode;

    return Ok;
}
