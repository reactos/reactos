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

GpStatus WINGDIPAPI GdipCloneCustomLineCap(GpCustomLineCap* from,
    GpCustomLineCap** to)
{
    TRACE("(%p, %p)\n", from, to);

    if(!from || !to)
        return InvalidParameter;

    *to = GdipAlloc(sizeof(GpCustomLineCap));
    if(!*to)   return OutOfMemory;

    memcpy(*to, from, sizeof(GpCustomLineCap));

    (*to)->pathdata.Points = GdipAlloc(from->pathdata.Count * sizeof(PointF));
    (*to)->pathdata.Types = GdipAlloc(from->pathdata.Count);

    if((!(*to)->pathdata.Types  || !(*to)->pathdata.Points) && (*to)->pathdata.Count){
        GdipFree((*to)->pathdata.Points);
        GdipFree((*to)->pathdata.Types);
        GdipFree(*to);
        return OutOfMemory;
    }

    memcpy((*to)->pathdata.Points, from->pathdata.Points, from->pathdata.Count
           * sizeof(PointF));
    memcpy((*to)->pathdata.Types, from->pathdata.Types, from->pathdata.Count);

    return Ok;
}

/* FIXME: Sometimes when fillPath is non-null and stroke path is null, the native
 * version of this function returns NotImplemented. I cannot figure out why. */
GpStatus WINGDIPAPI GdipCreateCustomLineCap(GpPath* fillPath, GpPath* strokePath,
    GpLineCap baseCap, REAL baseInset, GpCustomLineCap **customCap)
{
    GpPathData *pathdata;

    TRACE("%p %p %d %f %p\n", fillPath, strokePath, baseCap, baseInset, customCap);

    if(!customCap || !(fillPath || strokePath))
        return InvalidParameter;

    *customCap = GdipAlloc(sizeof(GpCustomLineCap));
    if(!*customCap)    return OutOfMemory;

    if(strokePath){
        (*customCap)->fill = FALSE;
        pathdata = &strokePath->pathdata;
    }
    else{
        (*customCap)->fill = TRUE;
        pathdata = &fillPath->pathdata;
    }

    (*customCap)->pathdata.Points = GdipAlloc(pathdata->Count * sizeof(PointF));
    (*customCap)->pathdata.Types = GdipAlloc(pathdata->Count);

    if((!(*customCap)->pathdata.Types || !(*customCap)->pathdata.Points) &&
        pathdata->Count){
        GdipFree((*customCap)->pathdata.Points);
        GdipFree((*customCap)->pathdata.Types);
        GdipFree(*customCap);
        return OutOfMemory;
    }

    memcpy((*customCap)->pathdata.Points, pathdata->Points, pathdata->Count
           * sizeof(PointF));
    memcpy((*customCap)->pathdata.Types, pathdata->Types, pathdata->Count);
    (*customCap)->pathdata.Count = pathdata->Count;

    (*customCap)->inset = baseInset;
    (*customCap)->cap = baseCap;
    (*customCap)->join = LineJoinMiter;
    (*customCap)->scale = 1.0;

    return Ok;
}

GpStatus WINGDIPAPI GdipDeleteCustomLineCap(GpCustomLineCap *customCap)
{
    TRACE("(%p)\n", customCap);

    if(!customCap)
        return InvalidParameter;

    GdipFree(customCap->pathdata.Points);
    GdipFree(customCap->pathdata.Types);
    GdipFree(customCap);

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
    GpLineCap start, GpLineCap end)
{
    static int calls;

    if(!custom)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetCustomLineCapBaseCap(GpCustomLineCap* custom,
    GpLineCap base)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
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
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
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

GpStatus WINGDIPAPI GdipSetCustomLineCapWidthScale(GpCustomLineCap* custom,
    REAL width)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetCustomLineCapBaseCap(GpCustomLineCap *customCap, GpLineCap *baseCap)
{
    TRACE("(%p, %p)\n", customCap, baseCap);

    if(!customCap || !baseCap)
        return InvalidParameter;

    *baseCap = customCap->cap;

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateAdjustableArrowCap(REAL height, REAL width, BOOL fill,
    GpAdjustableArrowCap **cap)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetAdjustableArrowCapFillState(GpAdjustableArrowCap* cap, BOOL* fill)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetAdjustableArrowCapHeight(GpAdjustableArrowCap* cap, REAL* height)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetAdjustableArrowCapMiddleInset(GpAdjustableArrowCap* cap, REAL* middle)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetAdjustableArrowCapWidth(GpAdjustableArrowCap* cap, REAL* width)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetAdjustableArrowCapFillState(GpAdjustableArrowCap* cap, BOOL fill)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetAdjustableArrowCapHeight(GpAdjustableArrowCap* cap, REAL height)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetAdjustableArrowCapMiddleInset(GpAdjustableArrowCap* cap, REAL middle)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetAdjustableArrowCapWidth(GpAdjustableArrowCap* cap, REAL width)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}
