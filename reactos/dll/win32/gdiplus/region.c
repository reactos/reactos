/*
 * Copyright (C) 2008 Google (Lei Zhang)
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

/**********************************************************
 *
 * Data returned by GdipGetRegionData looks something like this:
 *
 * struct region_data_header
 * {
 *   DWORD size;     size in bytes of the data - 8.
 *   DWORD magic1;   probably a checksum.
 *   DWORD magic2;   always seems to be 0xdbc01001 - version?
 *   DWORD num_ops;  number of combining ops * 2
 * };
 *
 * Then follows a sequence of combining ops and region elements.
 *
 * A region element is either a RECTF or some path data.
 *
 * Combining ops are just stored as their CombineMode value.
 *
 * Each RECTF is preceded by the DWORD 0x10000000.  An empty rect is
 * stored as 0x10000002 (with no following RECTF) and an infinite rect
 * is stored as 0x10000003 (again with no following RECTF).
 *
 * Path data is preceded by the DWORD 0x10000001.  Then follows a
 * DWORD size and then size bytes of data.
 *
 * The combining ops are stored in the reverse order to the region
 * elements and in the reverse order to which the region was
 * constructed.
 *
 * When two or more complex regions (ie those with more than one
 * element) are combined, the combining op for the two regions comes
 * first, then the combining ops for the region elements in region 1,
 * followed by the region elements for region 1, then follows the
 * combining ops for region 2 and finally region 2's region elements.
 * Presumably you're supposed to use the 0x1000000x header to find the
 * end of the op list (the count of the elements in each region is not
 * stored).
 *
 * When a simple region (1 element) is combined, it's treated as if a
 * single rect/path is being combined.
 *
 */

#define FLAGS_NOFLAGS   0x0
#define FLAGS_INTPATH   0x4000

/* Header size as far as header->size is concerned. This doesn't include
 * header->size or header->checksum
 */
static const INT sizeheader_size = sizeof(DWORD) * 2;

typedef struct packed_point
{
    short X;
    short Y;
} packed_point;

/* Everything is measured in DWORDS; round up if there's a remainder */
static inline INT get_pathtypes_size(const GpPath* path)
{
    INT needed = path->pathdata.Count / sizeof(DWORD);

    if (path->pathdata.Count % sizeof(DWORD) > 0)
        needed++;

    return needed * sizeof(DWORD);
}

static inline INT get_element_size(const region_element* element)
{
    INT needed = sizeof(DWORD); /* DWORD for the type */
    switch(element->type)
    {
        case RegionDataRect:
            return needed + sizeof(GpRect);
        case RegionDataPath:
             needed += element->elementdata.pathdata.pathheader.size;
             needed += sizeof(DWORD); /* Extra DWORD for pathheader.size */
             return needed;
        case RegionDataEmptyRect:
        case RegionDataInfiniteRect:
            return needed;
        default:
            needed += get_element_size(element->elementdata.combine.left);
            needed += get_element_size(element->elementdata.combine.right);
            return needed;
    }

    return 0;
}

/* Does not check parameters, caller must do that */
static inline GpStatus init_region(GpRegion* region, const RegionType type)
{
    region->node.type       = type;
    region->header.checksum = 0xdeadbeef;
    region->header.magic    = VERSION_MAGIC;
    region->header.num_children  = 0;
    region->header.size     = sizeheader_size + get_element_size(&region->node);

    return Ok;
}

static inline GpStatus clone_element(const region_element* element,
        region_element** element2)
{
    GpStatus stat;

    /* root node is allocated with GpRegion */
    if(!*element2){
        *element2 = GdipAlloc(sizeof(region_element));
        if (!*element2)
            return OutOfMemory;
    }

    (*element2)->type = element->type;

    switch (element->type)
    {
        case RegionDataRect:
            (*element2)->elementdata.rect = element->elementdata.rect;
            break;
        case RegionDataEmptyRect:
        case RegionDataInfiniteRect:
            break;
        case RegionDataPath:
            (*element2)->elementdata.pathdata.pathheader = element->elementdata.pathdata.pathheader;
            stat = GdipClonePath(element->elementdata.pathdata.path,
                    &(*element2)->elementdata.pathdata.path);
            if (stat != Ok) goto clone_out;
            break;
        default:
            (*element2)->elementdata.combine.left  = NULL;
            (*element2)->elementdata.combine.right = NULL;

            stat = clone_element(element->elementdata.combine.left,
                    &(*element2)->elementdata.combine.left);
            if (stat != Ok) goto clone_out;
            stat = clone_element(element->elementdata.combine.right,
                    &(*element2)->elementdata.combine.right);
            if (stat != Ok) goto clone_out;
            break;
    }

    return Ok;

clone_out:
    delete_element(*element2);
    *element2 = NULL;
    return stat;
}

/* Common code for CombineRegion*
 * All the caller has to do is get its format into an element
 */
static inline void fuse_region(GpRegion* region, region_element* left,
        region_element* right, const CombineMode mode)
{
    region->node.type = mode;
    region->node.elementdata.combine.left = left;
    region->node.elementdata.combine.right = right;

    region->header.size = sizeheader_size + get_element_size(&region->node);
    region->header.num_children += 2;
}

/*****************************************************************************
 * GdipCloneRegion [GDIPLUS.@]
 *
 * Creates a deep copy of the region
 *
 * PARAMS
 *  region  [I] source region
 *  clone   [O] resulting clone
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: InvalidParameter or OutOfMemory
 */
GpStatus WINGDIPAPI GdipCloneRegion(GpRegion *region, GpRegion **clone)
{
    region_element *element;

    TRACE("%p %p\n", region, clone);

    if (!(region && clone))
        return InvalidParameter;

    *clone = GdipAlloc(sizeof(GpRegion));
    if (!*clone)
        return OutOfMemory;
    element = &(*clone)->node;

    (*clone)->header = region->header;
    return clone_element(&region->node, &element);
}

/*****************************************************************************
 * GdipCombineRegionPath [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCombineRegionPath(GpRegion *region, GpPath *path, CombineMode mode)
{
    GpRegion *path_region;
    region_element *left, *right = NULL;
    GpStatus stat;

    TRACE("%p %p %d\n", region, path, mode);

    if (!(region && path))
        return InvalidParameter;

    stat = GdipCreateRegionPath(path, &path_region);
    if (stat != Ok)
        return stat;

    /* simply replace region data */
    if(mode == CombineModeReplace){
        delete_element(&region->node);
        memcpy(region, path_region, sizeof(GpRegion));
        return Ok;
    }

    left = GdipAlloc(sizeof(region_element));
    if (!left)
        goto out;
    *left = region->node;

    stat = clone_element(&path_region->node, &right);
    if (stat != Ok)
        goto out;

    fuse_region(region, left, right, mode);

    GdipDeleteRegion(path_region);
    return Ok;

out:
    GdipFree(left);
    GdipDeleteRegion(path_region);
    return stat;
}

/*****************************************************************************
 * GdipCombineRegionRect [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCombineRegionRect(GpRegion *region,
        GDIPCONST GpRectF *rect, CombineMode mode)
{
    GpRegion *rect_region;
    region_element *left, *right = NULL;
    GpStatus stat;

    TRACE("%p %p %d\n", region, rect, mode);

    if (!(region && rect))
        return InvalidParameter;

    stat = GdipCreateRegionRect(rect, &rect_region);
    if (stat != Ok)
        return stat;

    /* simply replace region data */
    if(mode == CombineModeReplace){
        delete_element(&region->node);
        memcpy(region, rect_region, sizeof(GpRegion));
        return Ok;
    }

    left = GdipAlloc(sizeof(region_element));
    if (!left)
        goto out;
    memcpy(left, &region->node, sizeof(region_element));

    stat = clone_element(&rect_region->node, &right);
    if (stat != Ok)
        goto out;

    fuse_region(region, left, right, mode);

    GdipDeleteRegion(rect_region);
    return Ok;

out:
    GdipFree(left);
    GdipDeleteRegion(rect_region);
    return stat;
}

/*****************************************************************************
 * GdipCombineRegionRectI [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCombineRegionRectI(GpRegion *region,
        GDIPCONST GpRect *rect, CombineMode mode)
{
    GpRectF rectf;

    TRACE("%p %p %d\n", region, rect, mode);

    if (!rect)
        return InvalidParameter;

    rectf.X = (REAL)rect->X;
    rectf.Y = (REAL)rect->Y;
    rectf.Height = (REAL)rect->Height;
    rectf.Width = (REAL)rect->Width;

    return GdipCombineRegionRect(region, &rectf, mode);
}

GpStatus WINGDIPAPI GdipCombineRegionRegion(GpRegion *region1,
        GpRegion *region2, CombineMode mode)
{
    region_element *left, *right = NULL;
    GpStatus stat;
    GpRegion *reg2copy;

    TRACE("%p %p %d\n", region1, region2, mode);

    if(!(region1 && region2))
        return InvalidParameter;

    /* simply replace region data */
    if(mode == CombineModeReplace){
        stat = GdipCloneRegion(region2, &reg2copy);
        if(stat != Ok)  return stat;

        delete_element(&region1->node);
        memcpy(region1, reg2copy, sizeof(GpRegion));
        GdipFree(reg2copy);
        return Ok;
    }

    left  = GdipAlloc(sizeof(region_element));
    if (!left)
        return OutOfMemory;

    *left = region1->node;
    stat = clone_element(&region2->node, &right);
    if (stat != Ok)
    {
        GdipFree(left);
        return OutOfMemory;
    }

    fuse_region(region1, left, right, mode);
    region1->header.num_children += region2->header.num_children;

    return Ok;
}

/*****************************************************************************
 * GdipCreateRegion [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateRegion(GpRegion **region)
{
    TRACE("%p\n", region);

    if(!region)
        return InvalidParameter;

    *region = GdipAlloc(sizeof(GpRegion));
    if(!*region)
        return OutOfMemory;

    return init_region(*region, RegionDataInfiniteRect);
}

/*****************************************************************************
 * GdipCreateRegionPath [GDIPLUS.@]
 *
 * Creates a GpRegion from a GpPath
 *
 * PARAMS
 *  path    [I] path to base the region on
 *  region  [O] pointer to the newly allocated region
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: InvalidParameter
 *
 * NOTES
 *  If a path has no floating point points, its points will be stored as shorts
 *  (INTPATH)
 *
 *  If a path is empty, it is considered to be an INTPATH
 */
GpStatus WINGDIPAPI GdipCreateRegionPath(GpPath *path, GpRegion **region)
{
    region_element* element;
    GpPoint  *pointsi;
    GpPointF *pointsf;

    GpStatus stat;
    DWORD flags = FLAGS_INTPATH;
    INT count, i;

    TRACE("%p, %p\n", path, region);

    if (!(path && region))
        return InvalidParameter;

    *region = GdipAlloc(sizeof(GpRegion));
    if(!*region)
        return OutOfMemory;
    stat = init_region(*region, RegionDataPath);
    if (stat != Ok)
    {
        GdipDeleteRegion(*region);
        return stat;
    }
    element = &(*region)->node;
    count = path->pathdata.Count;

    /* Test to see if the path is an Integer path */
    if (count)
    {
        pointsi = GdipAlloc(sizeof(GpPoint) * count);
        pointsf = GdipAlloc(sizeof(GpPointF) * count);
        if (!(pointsi && pointsf))
        {
            GdipFree(pointsi);
            GdipFree(pointsf);
            GdipDeleteRegion(*region);
            return OutOfMemory;
        }

        stat = GdipGetPathPointsI(path, pointsi, count);
        if (stat != Ok)
        {
            GdipDeleteRegion(*region);
            return stat;
        }
        stat = GdipGetPathPoints(path, pointsf, count);
        if (stat != Ok)
        {
            GdipDeleteRegion(*region);
            return stat;
        }

        for (i = 0; i < count; i++)
        {
            if (!(pointsi[i].X == pointsf[i].X &&
                  pointsi[i].Y == pointsf[i].Y ))
            {
                flags = FLAGS_NOFLAGS;
                break;
            }
        }
        GdipFree(pointsi);
        GdipFree(pointsf);
    }

    stat = GdipClonePath(path, &element->elementdata.pathdata.path);
    if (stat != Ok)
    {
        GdipDeleteRegion(*region);
        return stat;
    }

    /* 3 for headers, once again size doesn't count itself */
    element->elementdata.pathdata.pathheader.size = ((sizeof(DWORD) * 3));
    switch(flags)
    {
        /* Floats, sent out as floats */
        case FLAGS_NOFLAGS:
            element->elementdata.pathdata.pathheader.size +=
                (sizeof(DWORD) * count * 2);
            break;
        /* INTs, sent out as packed shorts */
        case FLAGS_INTPATH:
            element->elementdata.pathdata.pathheader.size +=
                (sizeof(DWORD) * count);
            break;
        default:
            FIXME("Unhandled flags (%08x). Expect wrong results.\n", flags);
    }
    element->elementdata.pathdata.pathheader.size += get_pathtypes_size(path);
    element->elementdata.pathdata.pathheader.magic = VERSION_MAGIC;
    element->elementdata.pathdata.pathheader.count = count;
    element->elementdata.pathdata.pathheader.flags = flags;
    (*region)->header.size = sizeheader_size + get_element_size(element);

    return Ok;
}

/*****************************************************************************
 * GdipCreateRegionRect [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateRegionRect(GDIPCONST GpRectF *rect,
        GpRegion **region)
{
    GpStatus stat;

    TRACE("%p, %p\n", rect, region);

    if (!(rect && region))
        return InvalidParameter;

    *region = GdipAlloc(sizeof(GpRegion));
    stat = init_region(*region, RegionDataRect);
    if(stat != Ok)
    {
        GdipDeleteRegion(*region);
        return stat;
    }

    (*region)->node.elementdata.rect.X = rect->X;
    (*region)->node.elementdata.rect.Y = rect->Y;
    (*region)->node.elementdata.rect.Width = rect->Width;
    (*region)->node.elementdata.rect.Height = rect->Height;

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateRegionRectI(GDIPCONST GpRect *rect,
        GpRegion **region)
{
    GpRectF rectf;

    TRACE("%p, %p\n", rect, region);

    rectf.X = (REAL)rect->X;
    rectf.Y = (REAL)rect->Y;
    rectf.Width = (REAL)rect->Width;
    rectf.Height = (REAL)rect->Height;

    return GdipCreateRegionRect(&rectf, region);
}

GpStatus WINGDIPAPI GdipCreateRegionRgnData(GDIPCONST BYTE *data, INT size, GpRegion **region)
{
    FIXME("(%p, %d, %p): stub\n", data, size, region);

    *region = NULL;
    return NotImplemented;
}


/******************************************************************************
 * GdipCreateRegionHrgn [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateRegionHrgn(HRGN hrgn, GpRegion **region)
{
    union {
        RGNDATA data;
        char buf[sizeof(RGNDATAHEADER) + sizeof(RECT)];
    } rdata;
    DWORD size;
    GpRectF rectf;
    GpPath *path;
    GpStatus stat;

    TRACE("(%p, %p)\n", hrgn, region);

    if(!region || !(size = GetRegionData(hrgn, 0, NULL)))
        return InvalidParameter;

    if(size > sizeof(RGNDATAHEADER) + sizeof(RECT)){
        FIXME("Only simple rect regions supported.\n");
        *region = NULL;
        return NotImplemented;
    }

    if(!GetRegionData(hrgn, sizeof(rdata), &rdata.data))
        return GenericError;

    /* return empty region */
    if(IsRectEmpty(&rdata.data.rdh.rcBound)){
        stat = GdipCreateRegion(region);
        if(stat == Ok)
            GdipSetEmpty(*region);
        return stat;
    }

    rectf.X = (REAL)rdata.data.rdh.rcBound.left;
    rectf.Y = (REAL)rdata.data.rdh.rcBound.top;
    rectf.Width  = (REAL)rdata.data.rdh.rcBound.right - rectf.X;
    rectf.Height = (REAL)rdata.data.rdh.rcBound.bottom - rectf.Y;

    stat = GdipCreatePath(FillModeAlternate, &path);
    if(stat != Ok)
        return stat;

    GdipAddPathRectangle(path, rectf.X, rectf.Y, rectf.Width, rectf.Height);

    stat = GdipCreateRegionPath(path, region);
    GdipDeletePath(path);

    return stat;
}

GpStatus WINGDIPAPI GdipDeleteRegion(GpRegion *region)
{
    TRACE("%p\n", region);

    if (!region)
        return InvalidParameter;

    delete_element(&region->node);
    GdipFree(region);

    return Ok;
}

/*****************************************************************************
 * GdipGetRegionBounds [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetRegionBounds(GpRegion *region, GpGraphics *graphics, GpRectF *rect)
{
    HRGN hrgn;
    RECT r;
    GpStatus status;

    TRACE("(%p, %p, %p)\n", region, graphics, rect);

    if(!region || !graphics || !rect)
        return InvalidParameter;

    status = GdipGetRegionHRgn(region, graphics, &hrgn);
    if(status != Ok)
        return status;

    /* infinite */
    if(!hrgn){
        rect->X = rect->Y = -(REAL)(1 << 22);
        rect->Width = rect->Height = (REAL)(1 << 23);
        return Ok;
    }

    if(!GetRgnBox(hrgn, &r)){
        DeleteObject(hrgn);
        return GenericError;
    }

    rect->X = r.left;
    rect->Y = r.top;
    rect->Width  = r.right  - r.left;
    rect->Height = r.bottom - r.top;

    return Ok;
}

/*****************************************************************************
 * GdipGetRegionBoundsI [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetRegionBoundsI(GpRegion *region, GpGraphics *graphics, GpRect *rect)
{
    GpRectF rectf;
    GpStatus status;

    TRACE("(%p, %p, %p)\n", region, graphics, rect);

    if(!rect)
        return InvalidParameter;

    status = GdipGetRegionBounds(region, graphics, &rectf);
    if(status == Ok){
        rect->X = roundr(rectf.X);
        rect->Y = roundr(rectf.X);
        rect->Width  = roundr(rectf.Width);
        rect->Height = roundr(rectf.Height);
    }

    return status;
}

static inline void write_dword(DWORD* location, INT* offset, const DWORD write)
{
    location[*offset] = write;
    (*offset)++;
}

static inline void write_float(DWORD* location, INT* offset, const FLOAT write)
{
    ((FLOAT*)location)[*offset] = write;
    (*offset)++;
}

static inline void write_packed_point(DWORD* location, INT* offset,
        const GpPointF* write)
{
    packed_point point;

    point.X = write->X;
    point.Y = write->Y;
    memcpy(location + *offset, &point, sizeof(packed_point));
    (*offset)++;
}

static inline void write_path_types(DWORD* location, INT* offset,
        const GpPath* path)
{
    memcpy(location + *offset, path->pathdata.Types, path->pathdata.Count);

    /* The unwritten parts of the DWORD (if any) must be cleared */
    if (path->pathdata.Count % sizeof(DWORD))
        ZeroMemory(((BYTE*)location) + (*offset * sizeof(DWORD)) +
                path->pathdata.Count,
                sizeof(DWORD) - path->pathdata.Count % sizeof(DWORD));
    *offset += (get_pathtypes_size(path) / sizeof(DWORD));
}

static void write_element(const region_element* element, DWORD *buffer,
        INT* filled)
{
    write_dword(buffer, filled, element->type);
    switch (element->type)
    {
        case CombineModeReplace:
        case CombineModeIntersect:
        case CombineModeUnion:
        case CombineModeXor:
        case CombineModeExclude:
        case CombineModeComplement:
            write_element(element->elementdata.combine.left, buffer, filled);
            write_element(element->elementdata.combine.right, buffer, filled);
            break;
        case RegionDataRect:
            write_float(buffer, filled, element->elementdata.rect.X);
            write_float(buffer, filled, element->elementdata.rect.Y);
            write_float(buffer, filled, element->elementdata.rect.Width);
            write_float(buffer, filled, element->elementdata.rect.Height);
            break;
        case RegionDataPath:
        {
            INT i;
            const GpPath* path = element->elementdata.pathdata.path;

            memcpy(buffer + *filled, &element->elementdata.pathdata.pathheader,
                    sizeof(element->elementdata.pathdata.pathheader));
            *filled += sizeof(element->elementdata.pathdata.pathheader) / sizeof(DWORD);
            switch (element->elementdata.pathdata.pathheader.flags)
            {
                case FLAGS_NOFLAGS:
                    for (i = 0; i < path->pathdata.Count; i++)
                    {
                        write_float(buffer, filled, path->pathdata.Points[i].X);
                        write_float(buffer, filled, path->pathdata.Points[i].Y);
                    }
                    break;
                case FLAGS_INTPATH:
                    for (i = 0; i < path->pathdata.Count; i++)
                    {
                        write_packed_point(buffer, filled,
                                &path->pathdata.Points[i]);
                    }
            }
            write_path_types(buffer, filled, path);
            break;
        }
        case RegionDataEmptyRect:
        case RegionDataInfiniteRect:
            break;
    }
}

/*****************************************************************************
 * GdipGetRegionData [GDIPLUS.@]
 *
 * Returns the header, followed by combining ops and region elements.
 *
 * PARAMS
 *  region  [I] region to retrieve from
 *  buffer  [O] buffer to hold the resulting data
 *  size    [I] size of the buffer
 *  needed  [O] (optional) how much data was written
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: InvalidParamter
 *
 * NOTES
 *  The header contains the size, a checksum, a version string, and the number
 *  of children. The size does not count itself or the checksum.
 *  Version is always something like 0xdbc01001 or 0xdbc01002
 *
 *  An element is a RECT, or PATH; Combining ops are stored as their
 *  CombineMode value. Special regions (infinite, empty) emit just their
 *  op-code; GpRectFs emit their code followed by their points; GpPaths emit
 *  their code followed by a second header for the path followed by the actual
 *  path data. Followed by the flags for each point. The pathheader contains
 *  the size of the data to follow, a version number again, followed by a count
 *  of how many points, and any special flags which may apply. 0x4000 means its
 *  a path of shorts instead of FLOAT.
 *
 *  Combining Ops are stored in reverse order from when they were constructed;
 *  the output is a tree where the left side combining area is always taken
 *  first.
 */
GpStatus WINGDIPAPI GdipGetRegionData(GpRegion *region, BYTE *buffer, UINT size,
        UINT *needed)
{
    INT filled = 0;

    TRACE("%p, %p, %d, %p\n", region, buffer, size, needed);

    if (!(region && buffer && size))
        return InvalidParameter;

    memcpy(buffer, &region->header, sizeof(region->header));
    filled += sizeof(region->header) / sizeof(DWORD);
    /* With few exceptions, everything written is DWORD aligned,
     * so use that as our base */
    write_element(&region->node, (DWORD*)buffer, &filled);

    if (needed)
        *needed = filled * sizeof(DWORD);

    return Ok;
}

/*****************************************************************************
 * GdipGetRegionDataSize [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetRegionDataSize(GpRegion *region, UINT *needed)
{
    TRACE("%p, %p\n", region, needed);

    if (!(region && needed))
        return InvalidParameter;

    /* header.size doesn't count header.size and header.checksum */
    *needed = region->header.size + sizeof(DWORD) * 2;

    return Ok;
}

static GpStatus get_path_hrgn(GpPath *path, GpGraphics *graphics, HRGN *hrgn)
{
    HDC new_hdc=NULL;
    GpStatus stat;
    INT save_state;

    if (!graphics)
    {
        new_hdc = GetDC(0);
        if (!new_hdc)
            return OutOfMemory;

        stat = GdipCreateFromHDC(new_hdc, &graphics);
        if (stat != Ok)
        {
            ReleaseDC(0, new_hdc);
            return stat;
        }
    }

    save_state = SaveDC(graphics->hdc);
    EndPath(graphics->hdc);

    SetPolyFillMode(graphics->hdc, (path->fill == FillModeAlternate ? ALTERNATE
                                                                    : WINDING));

    stat = trace_path(graphics, path);
    if (stat == Ok)
    {
        *hrgn = PathToRegion(graphics->hdc);
        stat = *hrgn ? Ok : OutOfMemory;
    }

    RestoreDC(graphics->hdc, save_state);
    if (new_hdc)
    {
        ReleaseDC(0, new_hdc);
        GdipDeleteGraphics(graphics);
    }

    return stat;
}

static GpStatus get_region_hrgn(struct region_element *element, GpGraphics *graphics, HRGN *hrgn)
{
    switch (element->type)
    {
        case RegionDataInfiniteRect:
            *hrgn = NULL;
            return Ok;
        case RegionDataEmptyRect:
            *hrgn = CreateRectRgn(0, 0, 0, 0);
            return *hrgn ? Ok : OutOfMemory;
        case RegionDataPath:
            return get_path_hrgn(element->elementdata.pathdata.path, graphics, hrgn);
        case RegionDataRect:
        {
            GpPath* path;
            GpStatus stat;
            GpRectF* rc = &element->elementdata.rect;

            stat = GdipCreatePath(FillModeAlternate, &path);
            if (stat != Ok)
                return stat;
            stat = GdipAddPathRectangle(path, rc->X, rc->Y, rc->Width, rc->Height);

            if (stat == Ok)
                stat = get_path_hrgn(path, graphics, hrgn);

            GdipDeletePath(path);

            return stat;
        }
        case CombineModeIntersect:
        case CombineModeUnion:
        case CombineModeXor:
        case CombineModeExclude:
        case CombineModeComplement:
        {
            HRGN left, right;
            GpStatus stat;
            int ret;

            stat = get_region_hrgn(element->elementdata.combine.left, graphics, &left);
            if (stat != Ok)
            {
                *hrgn = NULL;
                return stat;
            }

            if (left == NULL)
            {
                /* existing region is infinite */
                switch (element->type)
                {
                    case CombineModeIntersect:
                        return get_region_hrgn(element->elementdata.combine.right, graphics, hrgn);
                    case CombineModeXor: case CombineModeExclude:
                        FIXME("cannot exclude from an infinite region\n");
                        /* fall-through */
                    case CombineModeUnion: case CombineModeComplement:
                        *hrgn = NULL;
                        return Ok;
                }
            }

            stat = get_region_hrgn(element->elementdata.combine.right, graphics, &right);
            if (stat != Ok)
            {
                DeleteObject(left);
                *hrgn = NULL;
                return stat;
            }

            if (right == NULL)
            {
                /* new region is infinite */
                switch (element->type)
                {
                    case CombineModeIntersect:
                        *hrgn = left;
                        return Ok;
                    case CombineModeXor: case CombineModeComplement:
                        FIXME("cannot exclude from an infinite region\n");
                        /* fall-through */
                    case CombineModeUnion: case CombineModeExclude:
                        DeleteObject(left);
                        *hrgn = NULL;
                        return Ok;
                }
            }

            switch (element->type)
            {
                case CombineModeIntersect:
                    ret = CombineRgn(left, left, right, RGN_AND);
                    break;
                case CombineModeUnion:
                    ret = CombineRgn(left, left, right, RGN_OR);
                    break;
                case CombineModeXor:
                    ret = CombineRgn(left, left, right, RGN_XOR);
                    break;
                case CombineModeExclude:
                    ret = CombineRgn(left, left, right, RGN_DIFF);
                    break;
                case CombineModeComplement:
                    ret = CombineRgn(left, right, left, RGN_DIFF);
                    break;
                default:
                    ret = ERROR;
            }

            DeleteObject(right);

            if (ret == ERROR)
            {
                DeleteObject(left);
                *hrgn = NULL;
                return GenericError;
            }

            *hrgn = left;
            return Ok;
        }
        default:
            FIXME("GdipGetRegionHRgn unimplemented for region type=%x\n", element->type);
            *hrgn = NULL;
            return NotImplemented;
    }
}

/*****************************************************************************
 * GdipGetRegionHRgn [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetRegionHRgn(GpRegion *region, GpGraphics *graphics, HRGN *hrgn)
{
    TRACE("(%p, %p, %p)\n", region, graphics, hrgn);

    if (!region || !hrgn)
        return InvalidParameter;

    return get_region_hrgn(&region->node, graphics, hrgn);
}

GpStatus WINGDIPAPI GdipIsEmptyRegion(GpRegion *region, GpGraphics *graphics, BOOL *res)
{
    TRACE("(%p, %p, %p)\n", region, graphics, res);

    if(!region || !graphics || !res)
        return InvalidParameter;

    *res = (region->node.type == RegionDataEmptyRect);

    return Ok;
}

/*****************************************************************************
 * GdipIsEqualRegion [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipIsEqualRegion(GpRegion *region, GpRegion *region2, GpGraphics *graphics,
                                      BOOL *res)
{
    HRGN hrgn1, hrgn2;
    GpStatus stat;

    TRACE("(%p, %p, %p, %p)\n", region, region2, graphics, res);

    if(!region || !region2 || !graphics || !res)
        return InvalidParameter;

    stat = GdipGetRegionHRgn(region, graphics, &hrgn1);
    if(stat != Ok)
        return stat;
    stat = GdipGetRegionHRgn(region2, graphics, &hrgn2);
    if(stat != Ok){
        DeleteObject(hrgn1);
        return stat;
    }

    *res = EqualRgn(hrgn1, hrgn2);

    /* one of GpRegions is infinite */
    if(*res == ERROR)
        *res = (!hrgn1 && !hrgn2);

    DeleteObject(hrgn1);
    DeleteObject(hrgn2);

    return Ok;
}

/*****************************************************************************
 * GdipIsInfiniteRegion [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipIsInfiniteRegion(GpRegion *region, GpGraphics *graphics, BOOL *res)
{
    /* I think graphics is ignored here */
    TRACE("(%p, %p, %p)\n", region, graphics, res);

    if(!region || !graphics || !res)
        return InvalidParameter;

    *res = (region->node.type == RegionDataInfiniteRect);

    return Ok;
}

/*****************************************************************************
 * GdipSetEmpty [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipSetEmpty(GpRegion *region)
{
    GpStatus stat;

    TRACE("%p\n", region);

    if (!region)
        return InvalidParameter;

    delete_element(&region->node);
    stat = init_region(region, RegionDataEmptyRect);

    return stat;
}

GpStatus WINGDIPAPI GdipSetInfinite(GpRegion *region)
{
    GpStatus stat;

    TRACE("%p\n", region);

    if (!region)
        return InvalidParameter;

    delete_element(&region->node);
    stat = init_region(region, RegionDataInfiniteRect);

    return stat;
}

GpStatus WINGDIPAPI GdipTransformRegion(GpRegion *region, GpMatrix *matrix)
{
    FIXME("(%p, %p): stub\n", region, matrix);

    return NotImplemented;
}

/* Translates GpRegion elements with specified offsets */
static void translate_region_element(region_element* element, REAL dx, REAL dy)
{
    INT i;

    switch(element->type)
    {
        case RegionDataEmptyRect:
        case RegionDataInfiniteRect:
            return;
        case RegionDataRect:
            element->elementdata.rect.X += dx;
            element->elementdata.rect.Y += dy;
            return;
        case RegionDataPath:
            for(i = 0; i < element->elementdata.pathdata.path->pathdata.Count; i++){
                element->elementdata.pathdata.path->pathdata.Points[i].X += dx;
                element->elementdata.pathdata.path->pathdata.Points[i].Y += dy;
            }
            return;
        default:
            translate_region_element(element->elementdata.combine.left,  dx, dy);
            translate_region_element(element->elementdata.combine.right, dx, dy);
            return;
    }
}

/*****************************************************************************
 * GdipTranslateRegion [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipTranslateRegion(GpRegion *region, REAL dx, REAL dy)
{
    TRACE("(%p, %f, %f)\n", region, dx, dy);

    if(!region)
        return InvalidParameter;

    translate_region_element(&region->node, dx, dy);

    return Ok;
}

/*****************************************************************************
 * GdipTranslateRegionI [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipTranslateRegionI(GpRegion *region, INT dx, INT dy)
{
    TRACE("(%p, %d, %d)\n", region, dx, dy);

    return GdipTranslateRegion(region, (REAL)dx, (REAL)dy);
}
