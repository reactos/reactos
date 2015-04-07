/*
 * Copyright (C) 2008 Google (Lei Zhang)
 * Copyright (C) 2013 Dmitry Timoshkov
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

#include "gdiplus_private.h"

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

struct memory_buffer
{
    const BYTE *buffer;
    INT size, pos;
};

struct region_header
{
    DWORD size;
    DWORD checksum;
    DWORD magic;
    DWORD num_children;
};

struct path_header
{
    DWORD size;
    DWORD magic;
    DWORD count;
    DWORD flags;
};

/* Header size as far as header->size is concerned. This doesn't include
 * header->size or header->checksum
 */
static const INT sizeheader_size = sizeof(DWORD) * 2;

typedef struct packed_point
{
    short X;
    short Y;
} packed_point;

/* Test to see if the path could be stored as an array of shorts */
static BOOL is_integer_path(const GpPath *path)
{
    int i;

    if (!path->pathdata.Count) return FALSE;

    for (i = 0; i < path->pathdata.Count; i++)
    {
        short x, y;
        x = gdip_round(path->pathdata.Points[i].X);
        y = gdip_round(path->pathdata.Points[i].Y);
        if (path->pathdata.Points[i].X != (REAL)x || path->pathdata.Points[i].Y != (REAL)y)
            return FALSE;
    }
    return TRUE;
}

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
        {
            const GpPath *path = element->elementdata.path;
            DWORD flags = is_integer_path(path) ? FLAGS_INTPATH : FLAGS_NOFLAGS;
            /* 3 for headers, once again size doesn't count itself */
            needed += sizeof(DWORD) * 3;
            if (flags & FLAGS_INTPATH)
                needed += 2 * sizeof(SHORT) * path->pathdata.Count;
            else
                needed += 2 * sizeof(FLOAT) * path->pathdata.Count;

            needed += get_pathtypes_size(path);
            needed += sizeof(DWORD); /* Extra DWORD for pathheader.size */
            return needed;
        }
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
    region->node.type    = type;
    region->num_children = 0;

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
            return Ok;
        case RegionDataEmptyRect:
        case RegionDataInfiniteRect:
            return Ok;
        case RegionDataPath:
            stat = GdipClonePath(element->elementdata.path, &(*element2)->elementdata.path);
            if (stat == Ok) return Ok;
            break;
        default:
            (*element2)->elementdata.combine.left  = NULL;
            (*element2)->elementdata.combine.right = NULL;

            stat = clone_element(element->elementdata.combine.left,
                    &(*element2)->elementdata.combine.left);
            if (stat == Ok)
            {
                stat = clone_element(element->elementdata.combine.right,
                        &(*element2)->elementdata.combine.right);
                if (stat == Ok) return Ok;
            }
            break;
    }

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
    region->num_children += 2;
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

    (*clone)->num_children = region->num_children;
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
        GdipFree(path_region);
        return Ok;
    }

    left = GdipAlloc(sizeof(region_element));
    if (left)
    {
        *left = region->node;
        stat = clone_element(&path_region->node, &right);
        if (stat == Ok)
        {
            fuse_region(region, left, right, mode);
            GdipDeleteRegion(path_region);
            return Ok;
        }
    }
    else
        stat = OutOfMemory;

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

    TRACE("%p %s %d\n", region, debugstr_rectf(rect), mode);

    if (!(region && rect))
        return InvalidParameter;

    stat = GdipCreateRegionRect(rect, &rect_region);
    if (stat != Ok)
        return stat;

    /* simply replace region data */
    if(mode == CombineModeReplace){
        delete_element(&region->node);
        memcpy(region, rect_region, sizeof(GpRegion));
        GdipFree(rect_region);
        return Ok;
    }

    left = GdipAlloc(sizeof(region_element));
    if (left)
    {
        memcpy(left, &region->node, sizeof(region_element));
        stat = clone_element(&rect_region->node, &right);
        if (stat == Ok)
        {
            fuse_region(region, left, right, mode);
            GdipDeleteRegion(rect_region);
            return Ok;
        }
    }
    else
        stat = OutOfMemory;

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

/*****************************************************************************
 * GdipCombineRegionRegion [GDIPLUS.@]
 */
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
    region1->num_children += region2->num_children;

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

    TRACE("=> %p\n", *region);

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
    GpStatus stat;

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

    stat = GdipClonePath(path, &element->elementdata.path);
    if (stat != Ok)
    {
        GdipDeleteRegion(*region);
        return stat;
    }

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

/*****************************************************************************
 * GdipCreateRegionRectI [GDIPLUS.@]
 */
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

static inline void init_memory_buffer(struct memory_buffer *mbuf, const BYTE *buffer, INT size)
{
    mbuf->buffer = buffer;
    mbuf->size = size;
    mbuf->pos = 0;
}

static inline const void *buffer_read(struct memory_buffer *mbuf, INT size)
{
    if (mbuf->size - mbuf->pos >= size)
    {
        const void *data = mbuf->buffer + mbuf->pos;
        mbuf->pos += size;
        return data;
    }
    return NULL;
}

static GpStatus read_element(struct memory_buffer *mbuf, GpRegion *region, region_element *node, INT *count)
{
    GpStatus status;
    const DWORD *type;

    type = buffer_read(mbuf, sizeof(DWORD));
    if (!type) return Ok;

    TRACE("type %#x\n", *type);

    node->type = *type;

    switch (node->type)
    {
    case CombineModeReplace:
    case CombineModeIntersect:
    case CombineModeUnion:
    case CombineModeXor:
    case CombineModeExclude:
    case CombineModeComplement:
    {
        region_element *left, *right;

        left = GdipAlloc(sizeof(region_element));
        if (!left) return OutOfMemory;
        right = GdipAlloc(sizeof(region_element));
        if (!right)
        {
            GdipFree(left);
            return OutOfMemory;
        }

        status = read_element(mbuf, region, left, count);
        if (status == Ok)
        {
            status = read_element(mbuf, region, right, count);
            if (status == Ok)
            {
                node->elementdata.combine.left = left;
                node->elementdata.combine.right = right;
                region->num_children += 2;
                return Ok;
            }
        }

        GdipFree(left);
        GdipFree(right);
        return status;
    }

    case RegionDataRect:
    {
        const GpRectF *rc;

        rc = buffer_read(mbuf, sizeof(GpRectF));
        if (!rc)
        {
            ERR("failed to read rect data\n");
            return InvalidParameter;
        }

        node->elementdata.rect = *rc;
        *count += 1;
        return Ok;
    }

    case RegionDataPath:
    {
        GpPath *path;
        const struct path_header *path_header;
        const BYTE *types;

        path_header = buffer_read(mbuf, sizeof(struct path_header));
        if (!path_header)
        {
            ERR("failed to read path header\n");
            return InvalidParameter;
        }
        if (path_header->magic != VERSION_MAGIC)
        {
            ERR("invalid path header magic %#x\n", path_header->magic);
            return InvalidParameter;
        }

        /* Windows always fails to create an empty path in a region */
        if (!path_header->count)
        {
            TRACE("refusing to create an empty path in a region\n");
            return GenericError;
        }

        status = GdipCreatePath(FillModeAlternate, &path);
        if (status) return status;

        node->elementdata.path = path;

        if (!lengthen_path(path, path_header->count))
            return OutOfMemory;

        path->pathdata.Count = path_header->count;

        if (path_header->flags & ~FLAGS_INTPATH)
            FIXME("unhandled path flags %#x\n", path_header->flags);

        if (path_header->flags & FLAGS_INTPATH)
        {
            const packed_point *pt;
            DWORD i;

            pt = buffer_read(mbuf, sizeof(packed_point) * path_header->count);
            if (!pt)
            {
                ERR("failed to read packed %u path points\n", path_header->count);
                return InvalidParameter;
            }

            for (i = 0; i < path_header->count; i++)
            {
                path->pathdata.Points[i].X = (REAL)pt[i].X;
                path->pathdata.Points[i].Y = (REAL)pt[i].Y;
            }
        }
        else
        {
            const GpPointF *ptf;

            ptf = buffer_read(mbuf, sizeof(GpPointF) * path_header->count);
            if (!ptf)
            {
                ERR("failed to read %u path points\n", path_header->count);
                return InvalidParameter;
            }
            memcpy(path->pathdata.Points, ptf, sizeof(GpPointF) * path_header->count);
        }

        types = buffer_read(mbuf, path_header->count);
        if (!types)
        {
            ERR("failed to read %u path types\n", path_header->count);
            return InvalidParameter;
        }
        memcpy(path->pathdata.Types, types, path_header->count);
        if (path_header->count & 3)
        {
            if (!buffer_read(mbuf, 4 - (path_header->count & 3)))
            {
                ERR("failed to read rounding %u bytes\n", 4 - (path_header->count & 3));
                return InvalidParameter;
            }
        }

        *count += 1;
        return Ok;
    }

    case RegionDataEmptyRect:
    case RegionDataInfiniteRect:
        *count += 1;
        return Ok;

    default:
        FIXME("element type %#x is not supported\n", *type);
        break;
    }

    return InvalidParameter;
}

GpStatus WINGDIPAPI GdipCreateRegionRgnData(GDIPCONST BYTE *data, INT size, GpRegion **region)
{
    GpStatus status;
    struct memory_buffer mbuf;
    const struct region_header *region_header;
    INT count;

    if (!data || !size) return InvalidParameter;

    TRACE("%p, %d, %p\n", data, size, region);

    init_memory_buffer(&mbuf, data, size);

    region_header = buffer_read(&mbuf, sizeof(struct region_header));
    if (!region_header || region_header->magic != VERSION_MAGIC)
        return InvalidParameter;

    status = GdipCreateRegion(region);
    if (status != Ok) return status;

    count = 0;
    status = read_element(&mbuf, *region, &(*region)->node, &count);
    if (status == Ok && !count)
        status = InvalidParameter;

    if (status != Ok)
        GdipDeleteRegion(*region);

    return status;
}


/******************************************************************************
 * GdipCreateRegionHrgn [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateRegionHrgn(HRGN hrgn, GpRegion **region)
{
    DWORD size;
    LPRGNDATA buf;
    LPRECT rect;
    GpStatus stat;
    GpPath* path;
    GpRegion* local;
    DWORD i;

    TRACE("(%p, %p)\n", hrgn, region);

    if(!region || !(size = GetRegionData(hrgn, 0, NULL)))
        return InvalidParameter;

    buf = GdipAlloc(size);
    if(!buf)
        return OutOfMemory;

    if(!GetRegionData(hrgn, size, buf)){
        GdipFree(buf);
        return GenericError;
    }

    if(buf->rdh.nCount == 0){
        if((stat = GdipCreateRegion(&local)) != Ok){
            GdipFree(buf);
            return stat;
        }
        if((stat = GdipSetEmpty(local)) != Ok){
            GdipFree(buf);
            GdipDeleteRegion(local);
            return stat;
        }
        *region = local;
        GdipFree(buf);
        return Ok;
    }

    if((stat = GdipCreatePath(FillModeAlternate, &path)) != Ok){
        GdipFree(buf);
        return stat;
    }

    rect = (LPRECT)buf->Buffer;
    for(i = 0; i < buf->rdh.nCount; i++){
        if((stat = GdipAddPathRectangle(path, (REAL)rect->left, (REAL)rect->top,
                        (REAL)(rect->right - rect->left), (REAL)(rect->bottom - rect->top))) != Ok){
            GdipFree(buf);
            GdipDeletePath(path);
            return stat;
        }
        rect++;
    }

    stat = GdipCreateRegionPath(path, region);

    GdipFree(buf);
    GdipDeletePath(path);
    return stat;
}

/*****************************************************************************
 * GdipDeleteRegion [GDIPLUS.@]
 */
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

    /* Contrary to MSDN, native ignores the graphics transform. */
    status = GdipGetRegionHRgn(region, NULL, &hrgn);
    if(status != Ok)
        return status;

    /* infinite */
    if(!hrgn){
        rect->X = rect->Y = -(REAL)(1 << 22);
        rect->Width = rect->Height = (REAL)(1 << 23);
        TRACE("%p => infinite\n", region);
        return Ok;
    }

    if(GetRgnBox(hrgn, &r)){
        rect->X = r.left;
        rect->Y = r.top;
        rect->Width  = r.right  - r.left;
        rect->Height = r.bottom - r.top;
        TRACE("%p => %s\n", region, debugstr_rectf(rect));
    }
    else
        status = GenericError;

    DeleteObject(hrgn);

    return status;
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
        rect->X = gdip_round(rectf.X);
        rect->Y = gdip_round(rectf.Y);
        rect->Width  = gdip_round(rectf.Width);
        rect->Height = gdip_round(rectf.Height);
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
    packed_point *point = (packed_point *)(location + *offset);
    point->X = gdip_round(write->X);
    point->Y = gdip_round(write->Y);
    (*offset)++;
}

static inline void write_path_types(DWORD* location, INT* offset,
        const GpPath* path)
{
    INT rounded_size = get_pathtypes_size(path);

    memcpy(location + *offset, path->pathdata.Types, path->pathdata.Count);

    /* The unwritten parts of the DWORD (if any) must be cleared */
    if (rounded_size - path->pathdata.Count)
        ZeroMemory(((BYTE*)location) + (*offset * sizeof(DWORD)) +
                path->pathdata.Count, rounded_size - path->pathdata.Count);
    *offset += rounded_size / sizeof(DWORD);
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
            const GpPath* path = element->elementdata.path;
            struct path_header *pathheader;

            pathheader = (struct path_header *)(buffer + *filled);

            pathheader->flags = is_integer_path(path) ? FLAGS_INTPATH : FLAGS_NOFLAGS;
            /* 3 for headers, once again size doesn't count itself */
            pathheader->size = sizeof(DWORD) * 3;
            if (pathheader->flags & FLAGS_INTPATH)
                pathheader->size += 2 * sizeof(SHORT) * path->pathdata.Count;
            else
                pathheader->size += 2 * sizeof(FLOAT) * path->pathdata.Count;
            pathheader->size += get_pathtypes_size(path);
            pathheader->magic = VERSION_MAGIC;
            pathheader->count = path->pathdata.Count;

            *filled += 4;

            switch (pathheader->flags & FLAGS_INTPATH)
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
                    break;
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
 *  FAILURE: InvalidParameter
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
    struct region_header *region_header;
    INT filled = 0;
    UINT required;
    GpStatus status;

    TRACE("%p, %p, %d, %p\n", region, buffer, size, needed);

    if (!region || !buffer || !size)
        return InvalidParameter;

    status = GdipGetRegionDataSize(region, &required);
    if (status != Ok) return status;
    if (size < required)
    {
        if (needed) *needed = size;
        return InsufficientBuffer;
    }

    region_header = (struct region_header *)buffer;
    region_header->size = sizeheader_size + get_element_size(&region->node);
    region_header->checksum = 0;
    region_header->magic = VERSION_MAGIC;
    region_header->num_children = region->num_children;
    filled += 4;
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
    *needed = sizeof(DWORD) * 2 + sizeheader_size + get_element_size(&region->node);

    return Ok;
}

static GpStatus get_path_hrgn(GpPath *path, GpGraphics *graphics, HRGN *hrgn)
{
    HDC new_hdc=NULL;
    GpGraphics *new_graphics=NULL;
    GpStatus stat;
    INT save_state;

    if (!graphics)
    {
        new_hdc = CreateCompatibleDC(0);
        if (!new_hdc)
            return OutOfMemory;

        stat = GdipCreateFromHDC(new_hdc, &new_graphics);
        graphics = new_graphics;
        if (stat != Ok)
        {
            DeleteDC(new_hdc);
            return stat;
        }
    }
    else if (!graphics->hdc)
    {
        graphics->hdc = new_hdc = CreateCompatibleDC(0);
        if (!new_hdc)
            return OutOfMemory;
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
        DeleteDC(new_hdc);
        if (new_graphics)
            GdipDeleteGraphics(new_graphics);
        else
            graphics->hdc = NULL;
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
            return get_path_hrgn(element->elementdata.path, graphics, hrgn);
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
                        left = CreateRectRgn(-4194304, -4194304, 4194304, 4194304);
                        break;
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
                        right = CreateRectRgn(-4194304, -4194304, 4194304, 4194304);
                        break;
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
    GpStatus status;
    GpRectF rect;

    TRACE("(%p, %p, %p)\n", region, graphics, res);

    if(!region || !graphics || !res)
        return InvalidParameter;

    status = GdipGetRegionBounds(region, graphics, &rect);
    if (status != Ok) return status;

    *res = rect.Width == 0.0 && rect.Height == 0.0;
    TRACE("=> %d\n", *res);

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
 * GdipIsVisibleRegionRect [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipIsVisibleRegionRect(GpRegion* region, REAL x, REAL y, REAL w, REAL h, GpGraphics *graphics, BOOL *res)
{
    HRGN hrgn;
    GpStatus stat;
    RECT rect;

    TRACE("(%p, %.2f, %.2f, %.2f, %.2f, %p, %p)\n", region, x, y, w, h, graphics, res);

    if(!region || !res)
        return InvalidParameter;

    if((stat = GdipGetRegionHRgn(region, NULL, &hrgn)) != Ok)
        return stat;

    /* infinite */
    if(!hrgn){
        *res = TRUE;
        return Ok;
    }

    rect.left = ceilr(x);
    rect.top = ceilr(y);
    rect.right = ceilr(x + w);
    rect.bottom = ceilr(y + h);

    *res = RectInRegion(hrgn, &rect);

    DeleteObject(hrgn);

    return Ok;
}

/*****************************************************************************
 * GdipIsVisibleRegionRectI [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipIsVisibleRegionRectI(GpRegion* region, INT x, INT y, INT w, INT h, GpGraphics *graphics, BOOL *res)
{
    TRACE("(%p, %d, %d, %d, %d, %p, %p)\n", region, x, y, w, h, graphics, res);
    if(!region || !res)
        return InvalidParameter;

    return GdipIsVisibleRegionRect(region, (REAL)x, (REAL)y, (REAL)w, (REAL)h, graphics, res);
}

/*****************************************************************************
 * GdipIsVisibleRegionPoint [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipIsVisibleRegionPoint(GpRegion* region, REAL x, REAL y, GpGraphics *graphics, BOOL *res)
{
    HRGN hrgn;
    GpStatus stat;

    TRACE("(%p, %.2f, %.2f, %p, %p)\n", region, x, y, graphics, res);

    if(!region || !res)
        return InvalidParameter;

    if((stat = GdipGetRegionHRgn(region, NULL, &hrgn)) != Ok)
        return stat;

    /* infinite */
    if(!hrgn){
        *res = TRUE;
        return Ok;
    }

    *res = PtInRegion(hrgn, gdip_round(x), gdip_round(y));

    DeleteObject(hrgn);

    return Ok;
}

/*****************************************************************************
 * GdipIsVisibleRegionPointI [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipIsVisibleRegionPointI(GpRegion* region, INT x, INT y, GpGraphics *graphics, BOOL *res)
{
    TRACE("(%p, %d, %d, %p, %p)\n", region, x, y, graphics, res);

    return GdipIsVisibleRegionPoint(region, (REAL)x, (REAL)y, graphics, res);
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

/* Transforms GpRegion elements with given matrix */
static GpStatus transform_region_element(region_element* element, GpMatrix *matrix)
{
    GpStatus stat;

    switch(element->type)
    {
        case RegionDataEmptyRect:
        case RegionDataInfiniteRect:
            return Ok;
        case RegionDataRect:
        {
            /* We can't transform a rectangle, so convert it to a path. */
            GpRegion *new_region;
            GpPath *path;

            stat = GdipCreatePath(FillModeAlternate, &path);
            if (stat == Ok)
            {
                stat = GdipAddPathRectangle(path,
                    element->elementdata.rect.X, element->elementdata.rect.Y,
                    element->elementdata.rect.Width, element->elementdata.rect.Height);

                if (stat == Ok)
                    stat = GdipCreateRegionPath(path, &new_region);

                GdipDeletePath(path);
            }

            if (stat == Ok)
            {
                /* Steal the element from the created region. */
                memcpy(element, &new_region->node, sizeof(region_element));
                GdipFree(new_region);
            }
            else
                return stat;
        }
        /* Fall-through to do the actual conversion. */
        case RegionDataPath:
            if (!element->elementdata.path->pathdata.Count)
                return Ok;

            stat = GdipTransformMatrixPoints(matrix,
                element->elementdata.path->pathdata.Points,
                element->elementdata.path->pathdata.Count);
            return stat;
        default:
            stat = transform_region_element(element->elementdata.combine.left, matrix);
            if (stat == Ok)
                stat = transform_region_element(element->elementdata.combine.right, matrix);
            return stat;
    }
}

GpStatus WINGDIPAPI GdipTransformRegion(GpRegion *region, GpMatrix *matrix)
{
    TRACE("(%p, %p)\n", region, matrix);

    if (!region || !matrix)
        return InvalidParameter;

    return transform_region_element(&region->node, matrix);
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
            for(i = 0; i < element->elementdata.path->pathdata.Count; i++){
                element->elementdata.path->pathdata.Points[i].X += dx;
                element->elementdata.path->pathdata.Points[i].Y += dy;
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

static GpStatus get_region_scans_data(GpRegion *region, GpMatrix *matrix, LPRGNDATA *data)
{
    GpRegion *region_copy;
    GpStatus stat;
    HRGN hrgn;
    DWORD data_size;

    stat = GdipCloneRegion(region, &region_copy);

    if (stat == Ok)
    {
        stat = GdipTransformRegion(region_copy, matrix);

        if (stat == Ok)
            stat = GdipGetRegionHRgn(region_copy, NULL, &hrgn);

        if (stat == Ok)
        {
            if (hrgn)
            {
                data_size = GetRegionData(hrgn, 0, NULL);

                *data = GdipAlloc(data_size);

                if (*data)
                    GetRegionData(hrgn, data_size, *data);
                else
                    stat = OutOfMemory;

                DeleteObject(hrgn);
            }
            else
            {
                data_size = sizeof(RGNDATAHEADER) + sizeof(RECT);

                *data = GdipAlloc(data_size);

                if (*data)
                {
                    (*data)->rdh.dwSize = sizeof(RGNDATAHEADER);
                    (*data)->rdh.iType = RDH_RECTANGLES;
                    (*data)->rdh.nCount = 1;
                    (*data)->rdh.nRgnSize = sizeof(RECT);
                    (*data)->rdh.rcBound.left = (*data)->rdh.rcBound.top = -0x400000;
                    (*data)->rdh.rcBound.right = (*data)->rdh.rcBound.bottom = 0x400000;

                    memcpy((*data)->Buffer, &(*data)->rdh.rcBound, sizeof(RECT));
                }
                else
                    stat = OutOfMemory;
            }
        }

        GdipDeleteRegion(region_copy);
    }

    return stat;
}

GpStatus WINGDIPAPI GdipGetRegionScansCount(GpRegion *region, UINT *count, GpMatrix *matrix)
{
    GpStatus stat;
    LPRGNDATA data;

    TRACE("(%p, %p, %p)\n", region, count, matrix);

    if (!region || !count || !matrix)
        return InvalidParameter;

    stat = get_region_scans_data(region, matrix, &data);

    if (stat == Ok)
    {
        *count = data->rdh.nCount;
        GdipFree(data);
    }

    return stat;
}

GpStatus WINGDIPAPI GdipGetRegionScansI(GpRegion *region, GpRect *scans, INT *count, GpMatrix *matrix)
{
    GpStatus stat;
    DWORD i;
    LPRGNDATA data;
    RECT *rects;

    if (!region || !count || !matrix)
        return InvalidParameter;

    stat = get_region_scans_data(region, matrix, &data);

    if (stat == Ok)
    {
        *count = data->rdh.nCount;
        rects = (RECT*)data->Buffer;

        if (scans)
        {
            for (i=0; i<data->rdh.nCount; i++)
            {
                scans[i].X = rects[i].left;
                scans[i].Y = rects[i].top;
                scans[i].Width = rects[i].right - rects[i].left;
                scans[i].Height = rects[i].bottom - rects[i].top;
            }
        }

        GdipFree(data);
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipGetRegionScans(GpRegion *region, GpRectF *scans, INT *count, GpMatrix *matrix)
{
    GpStatus stat;
    DWORD i;
    LPRGNDATA data;
    RECT *rects;

    if (!region || !count || !matrix)
        return InvalidParameter;

    stat = get_region_scans_data(region, matrix, &data);

    if (stat == Ok)
    {
        *count = data->rdh.nCount;
        rects = (RECT*)data->Buffer;

        if (scans)
        {
            for (i=0; i<data->rdh.nCount; i++)
            {
                scans[i].X = rects[i].left;
                scans[i].Y = rects[i].top;
                scans[i].Width = rects[i].right - rects[i].left;
                scans[i].Height = rects[i].bottom - rects[i].top;
            }
        }

        GdipFree(data);
    }

    return Ok;
}
