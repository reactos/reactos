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

GpStatus WINGDIPAPI GdipCreatePathIter(GpPathIterator **iterator, GpPath* path)
{
    INT size;

    if(!iterator)
        return InvalidParameter;

    *iterator = GdipAlloc(sizeof(GpPathIterator));
    if(!*iterator)  return OutOfMemory;

    if(path){
        size = path->pathdata.Count;

        (*iterator)->pathdata.Types  = GdipAlloc(size);
        (*iterator)->pathdata.Points = GdipAlloc(size * sizeof(PointF));

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
    if(!iter)
        return InvalidParameter;

    GdipFree(iter->pathdata.Types);
    GdipFree(iter->pathdata.Points);
    GdipFree(iter);

    return Ok;
}

GpStatus WINGDIPAPI GdipPathIterCopyData(GpPathIterator* iterator,
    INT* resultCount, GpPointF* points, BYTE* types, INT startIndex, INT endIndex)
{
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

GpStatus WINGDIPAPI GdipPathIterNextSubpath(GpPathIterator* iterator,
    INT *resultCount, INT* startIndex, INT* endIndex, BOOL* isClosed)
{
    INT i, count;

    if(!iterator || !startIndex || !endIndex || !isClosed || !resultCount)
        return InvalidParameter;

    count = iterator->pathdata.Count;

    /* iterator created with NULL path */
    if(count == 0)
        return Ok;

    if(iterator->subpath_pos == count){
        *startIndex = *endIndex = *resultCount = 0;
        *isClosed = 1;
        return Ok;
    }

    *startIndex = iterator->subpath_pos;

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
    if(!iterator)
        return InvalidParameter;

    iterator->subpath_pos = 0;
    iterator->marker_pos = 0;
    iterator->pathtype_pos = 0;

    return Ok;
}

GpStatus WINGDIPAPI GdipPathIterGetCount(GpPathIterator* iterator, INT* count)
{
    if(!iterator || !count)
        return InvalidParameter;

    *count = iterator->pathdata.Count;

    return Ok;
}

GpStatus WINGDIPAPI GdipPathIterEnumerate(GpPathIterator* iterator, INT* resultCount,
    GpPointF *points, BYTE *types, INT count)
{
    if((count < 0) || !resultCount)
        return InvalidParameter;

    if(count == 0){
        *resultCount = 0;
        return Ok;
    }

    return GdipPathIterCopyData(iterator, resultCount, points, types, 0, count-1);
}
