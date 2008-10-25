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

GpStatus WINGDIPAPI GdipClonePen(GpPen *pen, GpPen **clonepen)
{
    if(!pen || !clonepen)
        return InvalidParameter;

    *clonepen = GdipAlloc(sizeof(GpPen));
    if(!*clonepen)  return OutOfMemory;

    **clonepen = *pen;

    GdipCloneCustomLineCap(pen->customstart, &(*clonepen)->customstart);
    GdipCloneCustomLineCap(pen->customend, &(*clonepen)->customend);
    GdipCloneBrush(pen->brush, &(*clonepen)->brush);

    return Ok;
}

GpStatus WINGDIPAPI GdipCreatePen1(ARGB color, REAL width, GpUnit unit,
    GpPen **pen)
{
    GpBrush *brush;
    GpStatus status;

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

    if(!pen || !brush)
        return InvalidParameter;

    gp_pen = GdipAlloc(sizeof(GpPen));
    if(!gp_pen)    return OutOfMemory;

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

    if(!((gp_pen->unit == UnitWorld) || (gp_pen->unit == UnitPixel))) {
        FIXME("UnitWorld, UnitPixel only supported units\n");
        GdipFree(gp_pen);
        return NotImplemented;
    }

    GdipCloneBrush(brush, &clone_brush);
    gp_pen->brush = clone_brush;

    *pen = gp_pen;

    return Ok;
}

GpStatus WINGDIPAPI GdipDeletePen(GpPen *pen)
{
    if(!pen)    return InvalidParameter;

    GdipDeleteBrush(pen->brush);
    GdipDeleteCustomLineCap(pen->customstart);
    GdipDeleteCustomLineCap(pen->customend);
    GdipFree(pen->dashes);
    GdipFree(pen);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenBrushFill(GpPen *pen, GpBrush **brush)
{
    if(!pen || !brush)
        return InvalidParameter;

    return GdipCloneBrush(pen->brush, brush);
}

GpStatus WINGDIPAPI GdipGetPenColor(GpPen *pen, ARGB *argb)
{
    if(!pen || !argb)
        return InvalidParameter;

    if(pen->brush->bt != BrushTypeSolidColor)
        return NotImplemented;

    return GdipGetSolidFillColor(((GpSolidFill*)pen->brush), argb);
}

GpStatus WINGDIPAPI GdipGetPenCustomEndCap(GpPen *pen, GpCustomLineCap** customCap)
{
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
    if(!pen || !dashCap)
        return InvalidParameter;

    *dashCap = pen->dashcap;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenDashCount(GpPen *pen, INT *count)
{
    if(!pen || !count)
        return InvalidParameter;

    *count = pen->numdashes;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenDashOffset(GpPen *pen, REAL *offset)
{
    if(!pen || !offset)
        return InvalidParameter;

    *offset = pen->offset;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenDashStyle(GpPen *pen, GpDashStyle *dash)
{
    if(!pen || !dash)
        return InvalidParameter;

    *dash = pen->dash;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenEndCap(GpPen *pen, GpLineCap *endCap)
{
    if(!pen || !endCap)
        return InvalidParameter;

    *endCap = pen->endcap;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenLineJoin(GpPen *pen, GpLineJoin *lineJoin)
{
    if(!pen || !lineJoin)
        return InvalidParameter;

    *lineJoin = pen->join;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenMode(GpPen *pen, GpPenAlignment *mode)
{
    if(!pen || !mode)
        return InvalidParameter;

    *mode = pen->align;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenMiterLimit(GpPen *pen, REAL *miterLimit)
{
    if(!pen || !miterLimit)
        return InvalidParameter;

    *miterLimit = pen->miterlimit;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenStartCap(GpPen *pen, GpLineCap *startCap)
{
    if(!pen || !startCap)
        return InvalidParameter;

    *startCap = pen->startcap;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenUnit(GpPen *pen, GpUnit *unit)
{
    if(!pen || !unit)
        return InvalidParameter;

    *unit = pen->unit;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenWidth(GpPen *pen, REAL *width)
{
    if(!pen || !width)
        return InvalidParameter;

    *width = pen->width;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenBrushFill(GpPen *pen, GpBrush *brush)
{
    if(!pen || !brush)
        return InvalidParameter;

    GdipDeleteBrush(pen->brush);
    return GdipCloneBrush(brush, &pen->brush);
}

GpStatus WINGDIPAPI GdipSetPenColor(GpPen *pen, ARGB argb)
{
    if(!pen)
        return InvalidParameter;

    if(pen->brush->bt != BrushTypeSolidColor)
        return NotImplemented;

    return GdipSetSolidFillColor(((GpSolidFill*)pen->brush), argb);
}

GpStatus WINGDIPAPI GdipSetPenCustomEndCap(GpPen *pen, GpCustomLineCap* customCap)
{
    GpCustomLineCap * cap;
    GpStatus ret;

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
    REAL sum = 0;

    if(!pen || !dash)
        return InvalidParameter;

    if(count <= 0)
        return OutOfMemory;

    for(i = 0; i < count; i++){
        sum += dash[i];
        if(dash[i] < 0.0)
            return InvalidParameter;
    }

    if(sum == 0.0 && count)
        return InvalidParameter;

    GdipFree(pen->dashes);
    pen->dashes = NULL;

    if(count > 0)
        pen->dashes = GdipAlloc(count * sizeof(REAL));
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
    if(!pen)
        return InvalidParameter;

    pen->dashcap = dashCap;

    return Ok;
}

/* FIXME: dash offset not used */
GpStatus WINGDIPAPI GdipSetPenDashOffset(GpPen *pen, REAL offset)
{
    if(!pen)
        return InvalidParameter;

    pen->offset = offset;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenDashStyle(GpPen *pen, GpDashStyle dash)
{
    if(!pen)
        return InvalidParameter;

    if(dash != DashStyleCustom){
        GdipFree(pen->dashes);
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
    if(!pen)    return InvalidParameter;

    pen->join = join;
    pen->style &= ~(PS_JOIN_ROUND | PS_JOIN_BEVEL | PS_JOIN_MITER);
    pen->style |= gdip_to_gdi_join(join);

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenMiterLimit(GpPen *pen, REAL limit)
{
    if(!pen)
        return InvalidParameter;

    pen->miterlimit = limit;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenStartCap(GpPen *pen, GpLineCap cap)
{
    if(!pen)    return InvalidParameter;

    GdipDeleteCustomLineCap(pen->customstart);
    pen->customstart = NULL;
    pen->startcap = cap;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenWidth(GpPen *pen, REAL width)
{
    if(!pen)    return InvalidParameter;

    pen->width = width;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenMode(GpPen *pen, GpPenAlignment mode)
{
    if(!pen)    return InvalidParameter;

    pen->align = mode;

    return Ok;
}
