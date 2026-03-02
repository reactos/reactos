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
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

#include "objbase.h"

#include "gdiplus.h"
#include "gdiplus_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdiplus);

GpStatus WINGDIPAPI GdipCloneCustomLineCap(GpCustomLineCap* from,
    GpCustomLineCap** to)
{
    TRACE("(%p, %p)\n", from, to);

    if(!from || !to)
        return InvalidParameter;

    if (from->type == CustomLineCapTypeDefault)
        *to = malloc(sizeof(GpCustomLineCap));
    else
        *to = malloc(sizeof(GpAdjustableArrowCap));

    if (!*to)
        return OutOfMemory;

    if (from->type == CustomLineCapTypeDefault)
        **to = *from;
    else
        *(GpAdjustableArrowCap *)*to = *(GpAdjustableArrowCap *)from;

    /* Duplicate path data */
    (*to)->pathdata.Points = malloc(from->pathdata.Count * sizeof(PointF));
    (*to)->pathdata.Types = malloc(from->pathdata.Count);

    if((!(*to)->pathdata.Types  || !(*to)->pathdata.Points) && (*to)->pathdata.Count){
        free((*to)->pathdata.Points);
        free((*to)->pathdata.Types);
        free(*to);
        return OutOfMemory;
    }

    memcpy((*to)->pathdata.Points, from->pathdata.Points, from->pathdata.Count
           * sizeof(PointF));
    memcpy((*to)->pathdata.Types, from->pathdata.Types, from->pathdata.Count);

    TRACE("<-- %p\n", *to);

    return Ok;
}

static GpStatus init_custom_linecap(GpCustomLineCap *cap, GpPathData *pathdata, BOOL fill, GpLineCap basecap,
    REAL base_inset)
{
    cap->fill = fill;

    cap->pathdata.Points = malloc(pathdata->Count * sizeof(PointF));
    cap->pathdata.Types = malloc(pathdata->Count);

    if ((!cap->pathdata.Types || !cap->pathdata.Points) && pathdata->Count)
    {
        free(cap->pathdata.Points);
        free(cap->pathdata.Types);
        cap->pathdata.Points = NULL;
        cap->pathdata.Types = NULL;
        return OutOfMemory;
    }

    if (pathdata->Points)
        memcpy(cap->pathdata.Points, pathdata->Points, pathdata->Count * sizeof(PointF));
    if (pathdata->Types)
        memcpy(cap->pathdata.Types, pathdata->Types, pathdata->Count);
    cap->pathdata.Count = pathdata->Count;

    cap->inset = base_inset;
    cap->basecap = basecap;
    cap->strokeStartCap = LineCapFlat;
    cap->strokeEndCap = LineCapFlat;
    cap->join = LineJoinMiter;
    cap->scale = 1.0;

    return Ok;
}

/* Custom line cap position (0, 0) is a place corresponding to the end of line.
*  If Custom Line Cap is too big and too far from position (0, 0),
*  then NotImplemented will be returned, due to floating point precision limitation. */
GpStatus WINGDIPAPI GdipCreateCustomLineCap(GpPath* fillPath, GpPath* strokePath,
    GpLineCap baseCap, REAL baseInset, GpCustomLineCap **customCap)
{
    GpPathData *pathdata;
    GpStatus stat;

    TRACE("%p %p %d %f %p\n", fillPath, strokePath, baseCap, baseInset, customCap);

    if(!customCap || !(fillPath || strokePath))
        return InvalidParameter;

    *customCap = calloc(1, sizeof(GpCustomLineCap));
    if(!*customCap) return OutOfMemory;

    if (strokePath)
        pathdata = &strokePath->pathdata;
    else
        pathdata = &fillPath->pathdata;

    stat = init_custom_linecap(*customCap, pathdata, fillPath != NULL, baseCap, baseInset);
    if (stat != Ok)
    {
        free(*customCap);
        return stat;
    }

    TRACE("<-- %p\n", *customCap);

    return Ok;
}

GpStatus WINGDIPAPI GdipDeleteCustomLineCap(GpCustomLineCap *customCap)
{
    TRACE("(%p)\n", customCap);

    if(!customCap)
        return InvalidParameter;

    free(customCap->pathdata.Points);
    free(customCap->pathdata.Types);
    free(customCap);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetCustomLineCapStrokeJoin(GpCustomLineCap* customCap,
    GpLineJoin* lineJoin)
{
    TRACE("(%p, %p)\n", customCap, lineJoin);

    if(!customCap || !lineJoin)
        return InvalidParameter;

    *lineJoin = customCap->join;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetCustomLineCapWidthScale(GpCustomLineCap* custom,
    REAL* widthScale)
{
    TRACE("(%p, %p)\n", custom, widthScale);

    if(!custom || !widthScale)
        return InvalidParameter;

    *widthScale = custom->scale;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetCustomLineCapStrokeCaps(GpCustomLineCap* custom,
    GpLineCap startcap, GpLineCap endcap)
{
    TRACE("(%p,%u,%u)\n", custom, startcap, endcap);

    if(!custom || startcap > LineCapTriangle || endcap > LineCapTriangle)
        return InvalidParameter;

    custom->strokeStartCap = startcap;
    custom->strokeEndCap = endcap;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetCustomLineCapBaseCap(GpCustomLineCap* custom,
    GpLineCap basecap)
{
    TRACE("(%p,%u)\n", custom, basecap);
    if(!custom || basecap > LineCapTriangle)
        return InvalidParameter;

    custom->basecap = basecap;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetCustomLineCapBaseInset(GpCustomLineCap* custom,
    REAL* inset)
{
    TRACE("(%p, %p)\n", custom, inset);

    if(!custom || !inset)
        return InvalidParameter;

    *inset = custom->inset;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetCustomLineCapBaseInset(GpCustomLineCap* custom,
    REAL inset)
{
    TRACE("(%p,%0.2f)\n", custom, inset);

    if(!custom)
        return InvalidParameter;

    custom->inset = inset;

    return Ok;
}

/*FIXME: LineJoin completely ignored now */
GpStatus WINGDIPAPI GdipSetCustomLineCapStrokeJoin(GpCustomLineCap* custom,
    GpLineJoin join)
{
    TRACE("(%p, %d)\n", custom, join);

    if(!custom)
        return InvalidParameter;

    custom->join = join;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetCustomLineCapWidthScale(GpCustomLineCap* custom, REAL width)
{
    TRACE("(%p,%0.2f)\n", custom, width);

    if(!custom)
        return InvalidParameter;

    custom->scale = width;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetCustomLineCapBaseCap(GpCustomLineCap *customCap, GpLineCap *baseCap)
{
    TRACE("(%p, %p)\n", customCap, baseCap);

    if(!customCap || !baseCap)
        return InvalidParameter;

    *baseCap = customCap->basecap;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetCustomLineCapType(GpCustomLineCap *customCap, CustomLineCapType *type)
{
    TRACE("(%p, %p)\n", customCap, type);

    if(!customCap || !type)
        return InvalidParameter;

    *type = customCap->type;
    return Ok;
}

static void arrowcap_update_path(GpAdjustableArrowCap *cap)
{
    static const BYTE types_filled[] =
    {
        PathPointTypeStart, PathPointTypeLine, PathPointTypeLine, PathPointTypeLine | PathPointTypeCloseSubpath
    };
    static const BYTE types_unfilled[] =
    {
        PathPointTypeStart, PathPointTypeLine, PathPointTypeLine
    };
    GpPointF *points;

    assert(cap->cap.pathdata.Count == 3 || cap->cap.pathdata.Count == 4);

    points = cap->cap.pathdata.Points;
    if (cap->cap.fill)
    {
        memcpy(cap->cap.pathdata.Types, types_filled, sizeof(types_filled));
        cap->cap.pathdata.Count = 4;
        points[0].X = cap->width / 2.0;
        points[0].Y = -cap->height;
        points[1].X = 0.0;
        points[1].Y = 0.0;
        points[2].X = -cap->width / 2.0;
        points[2].Y = -cap->height;
        points[3].X = 0.0;
        points[3].Y = -cap->height + cap->middle_inset;
    }
    else
    {
        memcpy(cap->cap.pathdata.Types, types_unfilled, sizeof(types_unfilled));
        cap->cap.pathdata.Count = 3;
        points[0].X = -cap->width / 2.0;
        points[0].Y = -cap->height;
        points[1].X = 0.0;
        points[1].Y = 0.0;
        points[2].X = cap->width / 2.0;
        points[2].Y = -cap->height;
    }

    if (cap->width == 0.0)
        cap->cap.inset = 0.0;
    else
        cap->cap.inset = cap->height / cap->width;
}

GpStatus WINGDIPAPI GdipCreateAdjustableArrowCap(REAL height, REAL width, BOOL fill,
    GpAdjustableArrowCap **cap)
{
    GpPathData pathdata;
    GpStatus stat;

    TRACE("(%0.2f,%0.2f,%i,%p)\n", height, width, fill, cap);

    if (!cap)
        return InvalidParameter;

    *cap = calloc(1, sizeof(**cap));
    if (!*cap)
        return OutOfMemory;

    /* We'll need 4 points at most. */
    pathdata.Count = 4;
    pathdata.Points = NULL;
    pathdata.Types = NULL;
    stat = init_custom_linecap(&(*cap)->cap, &pathdata, fill, LineCapTriangle, width != 0.0 ? height / width : 0.0);
    if (stat != Ok)
    {
        free(*cap);
        return stat;
    }

    (*cap)->cap.type = CustomLineCapTypeAdjustableArrow;
    (*cap)->height = height;
    (*cap)->width = width;
    (*cap)->middle_inset = 0.0;
    arrowcap_update_path(*cap);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetAdjustableArrowCapFillState(GpAdjustableArrowCap* cap, BOOL* fill)
{
    TRACE("(%p,%p)\n", cap, fill);

    if (!cap || !fill)
        return InvalidParameter;

    *fill = cap->cap.fill;
    return Ok;
}

GpStatus WINGDIPAPI GdipGetAdjustableArrowCapHeight(GpAdjustableArrowCap* cap, REAL* height)
{
    TRACE("(%p,%p)\n", cap, height);

    if (!cap || !height)
        return InvalidParameter;

    *height = cap->height;
    return Ok;
}

GpStatus WINGDIPAPI GdipGetAdjustableArrowCapMiddleInset(GpAdjustableArrowCap* cap, REAL* middle)
{
    TRACE("(%p,%p)\n", cap, middle);

    if (!cap || !middle)
        return InvalidParameter;

    *middle = cap->middle_inset;
    return Ok;
}

GpStatus WINGDIPAPI GdipGetAdjustableArrowCapWidth(GpAdjustableArrowCap* cap, REAL* width)
{
    TRACE("(%p,%p)\n", cap, width);

    if (!cap || !width)
        return InvalidParameter;

    *width = cap->width;
    return Ok;
}

GpStatus WINGDIPAPI GdipSetAdjustableArrowCapFillState(GpAdjustableArrowCap* cap, BOOL fill)
{
    TRACE("(%p,%i)\n", cap, fill);

    if (!cap)
        return InvalidParameter;

    cap->cap.fill = fill;
    arrowcap_update_path(cap);
    return Ok;
}

GpStatus WINGDIPAPI GdipSetAdjustableArrowCapHeight(GpAdjustableArrowCap* cap, REAL height)
{
    TRACE("(%p,%0.2f)\n", cap, height);

    if (!cap)
        return InvalidParameter;

    cap->height = height;
    arrowcap_update_path(cap);
    return Ok;
}

GpStatus WINGDIPAPI GdipSetAdjustableArrowCapMiddleInset(GpAdjustableArrowCap* cap, REAL middle)
{
    TRACE("(%p,%0.2f)\n", cap, middle);

    if (!cap)
        return InvalidParameter;

    cap->middle_inset = middle;
    arrowcap_update_path(cap);
    return Ok;
}

GpStatus WINGDIPAPI GdipSetAdjustableArrowCapWidth(GpAdjustableArrowCap* cap, REAL width)
{
    TRACE("(%p,%0.2f)\n", cap, width);

    if (!cap)
        return InvalidParameter;

    cap->width = width;
    arrowcap_update_path(cap);
    return Ok;
}
