/*
 * Copyright (C) 2007 Google (Evan Stade)
 * Copyright (C) 2008 Nikolay Sivov <bunglehead at gmail dot com>
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

GpStatus WINGDIPAPI GdipCreatePathIter(GpPathIterator **iterator, GpPath* path)
{
    INT size;

    TRACE("(%p, %p)\n", iterator, path);

    if(!iterator)
        return InvalidParameter;

    *iterator = calloc(1, sizeof(GpPathIterator));
    if(!*iterator) return OutOfMemory;

    if(path){
        size = path->pathdata.Count;

        (*iterator)->pathdata.Types  = malloc(size);
        (*iterator)->pathdata.Points = malloc(size * sizeof(PointF));

        memcpy((*iterator)->pathdata.Types, path->pathdata.Types, size);
        memcpy((*iterator)->pathdata.Points, path->pathdata.Points,size * sizeof(PointF));
        (*iterator)->pathdata.Count = size;
    }
    else{
        (*iterator)->pathdata.Types  = NULL;
        (*iterator)->pathdata.Points = NULL;
        (*iterator)->pathdata.Count  = 0;
    }

    (*iterator)->subpath_pos  = 0;
    (*iterator)->marker_pos   = 0;
    (*iterator)->pathtype_pos = 0;

    return Ok;
}

GpStatus WINGDIPAPI GdipDeletePathIter(GpPathIterator *iter)
{
    TRACE("(%p)\n", iter);

    if(!iter)
        return InvalidParameter;

    free(iter->pathdata.Types);
    free(iter->pathdata.Points);
    free(iter);

    return Ok;
}

GpStatus WINGDIPAPI GdipPathIterCopyData(GpPathIterator* iterator,
    INT* resultCount, GpPointF* points, BYTE* types, INT startIndex, INT endIndex)
{
    TRACE("(%p, %p, %p, %p, %d, %d)\n", iterator, resultCount, points, types,
           startIndex, endIndex);

    if(!iterator || !types || !points)
        return InvalidParameter;

    if(endIndex > iterator->pathdata.Count - 1 || startIndex < 0 ||
        endIndex < startIndex){
        *resultCount = 0;
        return Ok;
    }

    *resultCount = endIndex - startIndex + 1;

    memcpy(types, &(iterator->pathdata.Types[startIndex]), *resultCount);
    memcpy(points, &(iterator->pathdata.Points[startIndex]),
        *resultCount * sizeof(PointF));

    return Ok;
}

GpStatus WINGDIPAPI GdipPathIterHasCurve(GpPathIterator* iterator, BOOL* hasCurve)
{
    INT i;

    TRACE("(%p, %p)\n", iterator, hasCurve);

    if(!iterator)
        return InvalidParameter;

    *hasCurve = FALSE;

    for(i = 0; i < iterator->pathdata.Count; i++)
        if((iterator->pathdata.Types[i] & PathPointTypePathTypeMask) == PathPointTypeBezier){
            *hasCurve = TRUE;
            break;
        }

    return Ok;
}

GpStatus WINGDIPAPI GdipPathIterGetSubpathCount(GpPathIterator* iterator, INT* count)
{
    INT i;

    TRACE("(%p, %p)\n", iterator, count);

    if(!iterator || !count)
        return InvalidParameter;

    *count = 0;
    for(i = 0; i < iterator->pathdata.Count; i++){
        if(iterator->pathdata.Types[i] == PathPointTypeStart)
            (*count)++;
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipPathIterNextMarker(GpPathIterator* iterator, INT *resultCount,
    INT* startIndex, INT* endIndex)
{
    INT i;

    TRACE("(%p, %p, %p, %p)\n", iterator, resultCount, startIndex, endIndex);

    if(!iterator || !startIndex || !endIndex)
        return InvalidParameter;

    *resultCount = 0;

    /* first call could start with second point as all subsequent, cause
       path couldn't contain only one */
    for(i = iterator->marker_pos + 1; i < iterator->pathdata.Count; i++){
        if((iterator->pathdata.Types[i] & PathPointTypePathMarker) ||
           (i == iterator->pathdata.Count - 1)){
            *startIndex = iterator->marker_pos;
            if(iterator->marker_pos > 0) (*startIndex)++;
            *endIndex   = iterator->marker_pos = i;
            *resultCount= *endIndex - *startIndex + 1;
            break;
        }
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipPathIterNextMarkerPath(GpPathIterator* iterator, INT* result,
    GpPath* path)
{
    INT start, end;

    TRACE("(%p, %p, %p)\n", iterator, result, path);

    if(!iterator || !result)
        return InvalidParameter;

    GdipPathIterNextMarker(iterator, result, &start, &end);
    /* return path */
    if(((*result) > 0) && path){
        GdipResetPath(path);

        if(!lengthen_path(path, *result))
            return OutOfMemory;

        memcpy(path->pathdata.Points, &(iterator->pathdata.Points[start]), sizeof(GpPointF)*(*result));
        memcpy(path->pathdata.Types,  &(iterator->pathdata.Types[start]),  sizeof(BYTE)*(*result));
        path->pathdata.Count = *result;
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipPathIterNextSubpath(GpPathIterator* iterator,
    INT *resultCount, INT* startIndex, INT* endIndex, BOOL* isClosed)
{
    INT i, count;

    TRACE("(%p, %p, %p, %p, %p)\n", iterator, resultCount, startIndex,
           endIndex, isClosed);

    if(!iterator || !startIndex || !endIndex || !isClosed || !resultCount)
        return InvalidParameter;

    count = iterator->pathdata.Count;

    /* iterator created with NULL path */
    if(count == 0){
        *resultCount = 0;
        return Ok;
    }

    if(iterator->subpath_pos == count){
        *startIndex = *endIndex = *resultCount = 0;
        *isClosed = TRUE;
        return Ok;
    }

    *startIndex = iterator->subpath_pos;
    /* Set new pathtype position */
    iterator->pathtype_pos = iterator->subpath_pos;

    for(i = iterator->subpath_pos + 1; i < count &&
        !(iterator->pathdata.Types[i] == PathPointTypeStart); i++);

    *endIndex = i - 1;
    iterator->subpath_pos = i;

    *resultCount = *endIndex - *startIndex + 1;

    if(iterator->pathdata.Types[*endIndex] & PathPointTypeCloseSubpath)
        *isClosed = TRUE;
    else
        *isClosed = FALSE;

    return Ok;
}

GpStatus WINGDIPAPI GdipPathIterRewind(GpPathIterator *iterator)
{
    TRACE("(%p)\n", iterator);

    if(!iterator)
        return InvalidParameter;

    iterator->subpath_pos = 0;
    iterator->marker_pos = 0;
    iterator->pathtype_pos = 0;

    return Ok;
}

GpStatus WINGDIPAPI GdipPathIterGetCount(GpPathIterator* iterator, INT* count)
{
    TRACE("(%p, %p)\n", iterator, count);

    if(!iterator || !count)
        return InvalidParameter;

    *count = iterator->pathdata.Count;

    return Ok;
}

GpStatus WINGDIPAPI GdipPathIterEnumerate(GpPathIterator* iterator, INT* resultCount,
    GpPointF *points, BYTE *types, INT count)
{
    TRACE("(%p, %p, %p, %p, %d)\n", iterator, resultCount, points, types, count);

    if((count < 0) || !resultCount)
        return InvalidParameter;

    if(count == 0){
        *resultCount = 0;
        return Ok;
    }

    return GdipPathIterCopyData(iterator, resultCount, points, types, 0, count-1);
}

GpStatus WINGDIPAPI GdipPathIterIsValid(GpPathIterator* iterator, BOOL* valid)
{
    TRACE("(%p, %p)\n", iterator, valid);

    if(!iterator || !valid)
        return InvalidParameter;

    *valid = TRUE;

    return Ok;
}

GpStatus WINGDIPAPI GdipPathIterNextPathType(GpPathIterator* iter, INT* result,
    BYTE* type, INT* start, INT* end)
{
    INT i, stopIndex;
    TRACE("(%p, %p, %p, %p, %p)\n", iter, result, type, start, end);

    if (!iter || !result || !type || !start || !end)
        return InvalidParameter;

    i = iter->pathtype_pos;
    stopIndex = iter->subpath_pos;
    if (i >= stopIndex)  {
        *result = 0;
        return Ok;
    }
    if ((iter->pathdata.Types[i] & PathPointTypePathTypeMask) == PathPointTypeStart)
        i++;

    *start = i - 1;
    if ((i < stopIndex) &&
       ((iter->pathdata.Types[i] & PathPointTypePathTypeMask) != PathPointTypeStart))
    {
        *type = iter->pathdata.Types[i] & PathPointTypePathTypeMask;
        i++;
        for ( ; i < stopIndex; i++) {
            if ((iter->pathdata.Types[i] & PathPointTypePathTypeMask) != *type)
                break;
        }
    }
    iter->pathtype_pos = i;
    *end = i - 1;
    *result = *end - *start + 1;
    return Ok;
}

GpStatus WINGDIPAPI GdipPathIterNextSubpathPath(GpPathIterator* iter, INT* result,
    GpPath* path, BOOL* closed)
{
    INT start, end;

    TRACE("(%p, %p, %p, %p)\n", iter, result, path, closed);

    if(!iter || !result || !closed)
        return InvalidParameter;

    GdipPathIterNextSubpath(iter, result, &start, &end, closed);
    /* return path */
    if(((*result) > 0) && path){
        GdipResetPath(path);

        if(!lengthen_path(path, *result))
            return OutOfMemory;

        memcpy(path->pathdata.Points, &(iter->pathdata.Points[start]), sizeof(GpPointF)*(*result));
        memcpy(path->pathdata.Types,  &(iter->pathdata.Types[start]),  sizeof(BYTE)*(*result));
        path->pathdata.Count = *result;
    }

    return Ok;
}
