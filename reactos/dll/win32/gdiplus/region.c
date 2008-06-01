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
 * Data returned by GdipGetRegionData (for rectangle based regions)
 * looks something like this:
 *
 * struct region_data_header
 * {
 *   DWORD size;     size in bytes of the data - 8.
 *   DWORD magic1;   probably a checksum.
 *   DWORD magic2;   always seems to be 0xdbc01001 - version?
 *   DWORD num_ops;  number of combining ops * 2
 * };
 *
 * Then follows a sequence of combining ops and RECTFs.
 *
 * Combining ops are just stored as their CombineMode value.
 *
 * Each RECTF is preceded by the DWORD 0x10000000.  An empty rect is
 * stored as 0x10000002 (with no following RECTF) and an infinite rect
 * is stored as 0x10000003 (again with no following RECTF).
 *
 * The combining ops are stored in the reverse order to the RECTFs and in the
 * reverse order to which the region was constructed.
 *
 * When two or more complex regions (ie those with more than one rect)
 * are combined, the combining op for the two regions comes first,
 * then the combining ops for the rects in region 1, followed by the
 * rects for region 1, then follows the combining ops for region 2 and
 * finally region 2's rects.  Presumably you're supposed to use the
 * 0x10000000 rect header to find the end of the op list (the count of
 * the rects in each region is not stored).
 *
 * When a simple region (1 rect) is combined, it's treated as if a single rect
 * is being combined.
 *
 */

GpStatus WINGDIPAPI GdipCreateRegion(GpRegion **region)
{
    FIXME("(%p): stub\n", region);

    *region = NULL;
    return NotImplemented;
}

GpStatus WINGDIPAPI GdipCreateRegionPath(GpPath *path, GpRegion **region)
{
    FIXME("(%p, %p): stub\n", path, region);

    *region = NULL;
    return NotImplemented;
}

GpStatus WINGDIPAPI GdipDeleteRegion(GpRegion *region)
{
    FIXME("(%p): stub\n", region);
    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetRegionHRgn(GpRegion *region, GpGraphics *graphics, HRGN *hrgn)
{
    FIXME("(%p, %p, %p): stub\n", region, graphics, hrgn);

    *hrgn = NULL;
    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetEmpty(GpRegion *region)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetInfinite(GpRegion *region)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}
