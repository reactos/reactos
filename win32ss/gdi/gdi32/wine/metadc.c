/*
 * Metafile DC functions
 *
 * Copyright 1999 Huw D M Davies
 * Copyright 1993, 1994, 1996 Alexandre Julliard
 * Copyright 2021 Jacek Caban for CodeWeavers
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

#include "wine/config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "winerror.h"
#include "gdi_private.h"
#ifdef __REACTOS__
#include "wine/winternl.h"
#else
#include "winternl.h"
#endif
#include "wine/wingdi16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(metafile);

struct metadc
{
    HDC         hdc;
    METAHEADER *mh;           /* Pointer to metafile header */
    UINT        handles_size, cur_handles;
    HGDIOBJ    *handles;
    HANDLE      hFile;          /* Handle for disk based MetaFile */
    HPEN        pen;
    HBRUSH      brush;
    HFONT       font;
};

#define HANDLE_LIST_INC 20


struct metadc *get_metadc_ptr( HDC hdc )
{
    struct metadc *metafile = GdiGetClientObjLink(hdc);//get_gdi_client_ptr( hdc, NTGDI_OBJ_METADC );
    if (!metafile) SetLastError( ERROR_INVALID_HANDLE );
    return metafile;
}

static BOOL metadc_write_record( struct metadc *metadc, METARECORD *mr, unsigned int rlen )
{
    DWORD len, size;
    METAHEADER *mh;

    len = metadc->mh->mtSize * sizeof(WORD) + rlen;
    size = HeapSize( GetProcessHeap(), 0, metadc->mh );
    if (len > size)
    {
        size += size / sizeof(WORD) + rlen;
        mh = HeapReAlloc( GetProcessHeap(), 0, metadc->mh, size );
        if (!mh) return FALSE;
        metadc->mh = mh;
    }
    memcpy( (WORD *)metadc->mh + metadc->mh->mtSize, mr, rlen );

    metadc->mh->mtSize += rlen / sizeof(WORD);
    metadc->mh->mtMaxRecord = max( metadc->mh->mtMaxRecord, rlen / sizeof(WORD) );
    return TRUE;
}

static BOOL metadc_record( HDC hdc, METARECORD *mr, DWORD rlen )
{
    struct metadc *metadc;

    if (!(metadc = get_metadc_ptr( hdc ))) return FALSE;
    return metadc_write_record( metadc, mr, rlen );
}

static BOOL metadc_param0( HDC hdc, short func )
{
    METARECORD mr;

    mr.rdSize = FIELD_OFFSET(METARECORD, rdParm[0]) / sizeof(WORD);
    mr.rdFunction = func;
    return metadc_record( hdc, &mr, mr.rdSize * sizeof(WORD) );
}

static BOOL metadc_param1( HDC hdc, short func, short param )
{
    METARECORD mr;

    mr.rdSize = sizeof(mr) / sizeof(WORD);
    mr.rdFunction = func;
    mr.rdParm[0] = param;
    return metadc_record( hdc, &mr, mr.rdSize * sizeof(WORD) );
}

static BOOL metadc_param2( HDC hdc, short func, short param1, short param2 )
{
    unsigned char buffer[FIELD_OFFSET(METARECORD, rdParm)
                   + 2*RTL_FIELD_SIZE(METARECORD, rdParm)];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = sizeof(buffer) / sizeof(WORD);
    mr->rdFunction = func;
    mr->rdParm[0] = param2;
    mr->rdParm[1] = param1;
    return metadc_record( hdc, mr, sizeof(buffer) );
}

static BOOL metadc_param4( HDC hdc, short func, short param1, short param2,
                           short param3, short param4 )
{
    unsigned char buffer[FIELD_OFFSET(METARECORD, rdParm)
                   + 4*RTL_FIELD_SIZE(METARECORD, rdParm)];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = sizeof(buffer) / sizeof(WORD);
    mr->rdFunction = func;
    mr->rdParm[0] = param4;
    mr->rdParm[1] = param3;
    mr->rdParm[2] = param2;
    mr->rdParm[3] = param1;
    return metadc_record( hdc, mr, sizeof(buffer) );
}

static BOOL metadc_param5( HDC hdc, short func, short param1, short param2,
                           short param3, short param4, short param5 )
{
    unsigned char buffer[FIELD_OFFSET(METARECORD, rdParm)
                   + 5*RTL_FIELD_SIZE(METARECORD, rdParm)];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = sizeof(buffer) / sizeof(WORD);
    mr->rdFunction = func;
    mr->rdParm[0] = param5;
    mr->rdParm[1] = param4;
    mr->rdParm[2] = param3;
    mr->rdParm[3] = param2;
    mr->rdParm[4] = param1;
    return metadc_record( hdc, mr, sizeof(buffer) );
}

static BOOL metadc_param6( HDC hdc, short func, short param1, short param2,
                           short param3, short param4, short param5,
                           short param6 )
{
    unsigned char buffer[FIELD_OFFSET(METARECORD, rdParm)
                   + 6*RTL_FIELD_SIZE(METARECORD, rdParm)];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = sizeof(buffer) / sizeof(WORD);
    mr->rdFunction = func;
    mr->rdParm[0] = param6;
    mr->rdParm[1] = param5;
    mr->rdParm[2] = param4;
    mr->rdParm[3] = param3;
    mr->rdParm[4] = param2;
    mr->rdParm[5] = param1;
    return metadc_record( hdc, mr, sizeof(buffer) );
}

static BOOL metadc_param8( HDC hdc, short func, short param1, short param2,
                           short param3, short param4, short param5,
                           short param6, short param7, short param8)
{
    unsigned char buffer[FIELD_OFFSET(METARECORD, rdParm)
                   + 8*RTL_FIELD_SIZE(METARECORD, rdParm)];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = sizeof(buffer) / sizeof(WORD);
    mr->rdFunction = func;
    mr->rdParm[0] = param8;
    mr->rdParm[1] = param7;
    mr->rdParm[2] = param6;
    mr->rdParm[3] = param5;
    mr->rdParm[4] = param4;
    mr->rdParm[5] = param3;
    mr->rdParm[6] = param2;
    mr->rdParm[7] = param1;
    return metadc_record( hdc, mr, sizeof(buffer) );
}

BOOL METADC_SaveDC( HDC hdc )
{
    return metadc_param0( hdc, META_SAVEDC );
}

BOOL METADC_RestoreDC( HDC hdc, INT level )
{
    return metadc_param1( hdc, META_RESTOREDC, level );
}

BOOL METADC_SetTextAlign( HDC hdc, UINT align )
{
    return metadc_param2( hdc, META_SETTEXTALIGN, HIWORD(align), LOWORD(align) );
}

BOOL METADC_SetBkMode( HDC hdc, INT mode )
{
    return metadc_param1( hdc, META_SETBKMODE, (WORD)mode );
}

BOOL METADC_SetBkColor( HDC hdc, COLORREF color )
{
    return metadc_param2( hdc, META_SETBKCOLOR, HIWORD(color), LOWORD(color) );
}

BOOL METADC_SetTextColor( HDC hdc, COLORREF color )
{
    return metadc_param2( hdc, META_SETTEXTCOLOR, HIWORD(color), LOWORD(color) );
}

BOOL METADC_SetROP2( HDC hdc, INT rop )
{
    return metadc_param1( hdc, META_SETROP2, (WORD)rop );
}

BOOL METADC_SetRelAbs( HDC hdc, INT mode )
{
    return metadc_param1( hdc, META_SETRELABS, (WORD)mode );
}

BOOL METADC_SetPolyFillMode( HDC hdc, INT mode )
{
    return metadc_param1( hdc, META_SETPOLYFILLMODE, mode );
}

BOOL METADC_SetStretchBltMode( HDC hdc, INT mode )
{
    return metadc_param1( hdc, META_SETSTRETCHBLTMODE, mode );
}

BOOL METADC_IntersectClipRect( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    return metadc_param4( hdc, META_INTERSECTCLIPRECT, left, top, right, bottom );
}

BOOL METADC_ExcludeClipRect( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    return metadc_param4( hdc, META_EXCLUDECLIPRECT, left, top, right, bottom );
}

BOOL METADC_OffsetClipRgn( HDC hdc, INT x, INT y )
{
    return metadc_param2( hdc, META_OFFSETCLIPRGN, x, y );
}

BOOL METADC_SetLayout( HDC hdc, DWORD layout )
{
    return metadc_param2( hdc, META_SETLAYOUT, HIWORD(layout), LOWORD(layout) );
}

BOOL METADC_SetMapMode( HDC hdc, INT mode )
{
    return metadc_param1( hdc, META_SETMAPMODE, mode );
}

BOOL METADC_SetViewportExtEx( HDC hdc, INT x, INT y )
{
    return metadc_param2( hdc, META_SETVIEWPORTEXT, x, y );
}

BOOL METADC_SetViewportOrgEx( HDC hdc, INT x, INT y )
{
    return metadc_param2( hdc, META_SETVIEWPORTORG, x, y );
}

BOOL METADC_SetWindowExtEx( HDC hdc, INT x, INT y )
{
    return metadc_param2( hdc, META_SETWINDOWEXT, x, y );
}

BOOL METADC_SetWindowOrgEx( HDC hdc, INT x, INT y )
{
    return metadc_param2( hdc, META_SETWINDOWORG, x, y );
}

BOOL METADC_OffsetViewportOrgEx( HDC hdc, INT x, INT y )
{
    return metadc_param2( hdc, META_OFFSETVIEWPORTORG, x, y );
}

BOOL METADC_OffsetWindowOrgEx( HDC hdc, INT x, INT y )
{
    return metadc_param2( hdc, META_OFFSETWINDOWORG, x, y );
}

BOOL METADC_ScaleViewportExtEx( HDC hdc, INT x_num, INT x_denom, INT y_num, INT y_denom )
{
    return metadc_param4( hdc, META_SCALEVIEWPORTEXT, x_num, x_denom, y_num, y_denom );
}

BOOL METADC_ScaleWindowExtEx( HDC hdc, INT x_num, INT x_denom, INT y_num, INT y_denom )
{
    return metadc_param4( hdc, META_SCALEWINDOWEXT, x_num, x_denom, y_num, y_denom );
}

BOOL METADC_SetTextJustification( HDC hdc, INT extra, INT breaks )
{
    return metadc_param2( hdc, META_SETTEXTJUSTIFICATION, extra, breaks );
}

BOOL METADC_SetTextCharacterExtra( HDC hdc, INT extra )
{
    return metadc_param1( hdc, META_SETTEXTCHAREXTRA, extra );
}

BOOL METADC_SetMapperFlags( HDC hdc, DWORD flags )
{
    return metadc_param2( hdc, META_SETMAPPERFLAGS, HIWORD(flags), LOWORD(flags) );
}

BOOL METADC_MoveTo( HDC hdc, INT x, INT y )
{
    return metadc_param2( hdc, META_MOVETO, x, y );
}

BOOL METADC_LineTo( HDC hdc, INT x, INT y )
{
    return metadc_param2( hdc, META_LINETO, x, y );
}

BOOL METADC_Arc( HDC hdc, INT left, INT top, INT right, INT bottom,
                 INT xstart, INT ystart, INT xend, INT yend )
{
     return metadc_param8( hdc, META_ARC, left, top, right, bottom,
                           xstart, ystart, xend, yend );
}

BOOL METADC_Pie( HDC hdc, INT left, INT top, INT right, INT bottom,
                 INT xstart, INT ystart, INT xend, INT yend )
{
    return metadc_param8( hdc, META_PIE, left, top, right, bottom,
                          xstart, ystart, xend, yend );
}

BOOL METADC_Chord( HDC hdc, INT left, INT top, INT right, INT bottom,
                   INT xstart, INT ystart, INT xend, INT yend )
{
    return metadc_param8( hdc, META_CHORD, left, top, right, bottom,
                          xstart, ystart, xend, yend );
}

BOOL METADC_Ellipse( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    return metadc_param4( hdc, META_ELLIPSE, left, top, right, bottom );
}

BOOL METADC_Rectangle( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    return metadc_param4( hdc, META_RECTANGLE, left, top, right, bottom );
}

BOOL METADC_RoundRect( HDC hdc, INT left, INT top, INT right,
                       INT bottom, INT ell_width, INT ell_height )
{
    return metadc_param6( hdc, META_ROUNDRECT, left, top, right, bottom,
                          ell_width, ell_height );
}

BOOL METADC_SetPixel( HDC hdc, INT x, INT y, COLORREF color )
{
    return metadc_param4( hdc, META_SETPIXEL, x, y, HIWORD(color), LOWORD(color) );
}

static BOOL metadc_poly( HDC hdc, short func, POINTS *pt, short count )
{
    BOOL ret;
    DWORD len;
    METARECORD *mr;

    len = sizeof(METARECORD) + count * 4;
    if (!(mr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len )))
        return FALSE;

    mr->rdSize = len / 2;
    mr->rdFunction = func;
    *(mr->rdParm) = count;
    memcpy(mr->rdParm + 1, pt, count * 4);
    ret = metadc_record( hdc, mr, mr->rdSize * 2);
    HeapFree( GetProcessHeap(), 0, mr);
    return ret;
}

BOOL METADC_Polyline( HDC hdc, const POINT *pt, INT count )
{
    int i;
    POINTS *pts;
    BOOL ret;

    pts = HeapAlloc( GetProcessHeap(), 0, sizeof(POINTS) * count );
    if(!pts) return FALSE;
    for (i=count;i--;)
    {
        pts[i].x = pt[i].x;
        pts[i].y = pt[i].y;
    }
    ret = metadc_poly( hdc, META_POLYLINE, pts, count );

    HeapFree( GetProcessHeap(), 0, pts );
    return ret;
}

BOOL METADC_Polygon( HDC hdc, const POINT *pt, INT count )
{
    int i;
    POINTS *pts;
    BOOL ret;

    pts = HeapAlloc( GetProcessHeap(), 0, sizeof(POINTS) * count );
    if(!pts) return FALSE;
    for (i = count; i--;)
    {
        pts[i].x = pt[i].x;
        pts[i].y = pt[i].y;
    }
    ret = metadc_poly( hdc, META_POLYGON, pts, count );

    HeapFree( GetProcessHeap(), 0, pts );
    return ret;
}

BOOL METADC_PolyPolygon( HDC hdc, const POINT *pt, const INT *counts, UINT polygons )
{
    BOOL ret;
    DWORD len;
    METARECORD *mr;
    unsigned int i,j;
    POINTS *pts;
    INT16 totalpoint16 = 0;
    INT16 * pointcounts;

    for (i = 0; i < polygons; i++)
         totalpoint16 += counts[i];

    /* allocate space for all points */
    pts=HeapAlloc( GetProcessHeap(), 0, sizeof(POINTS) * totalpoint16 );
    pointcounts = HeapAlloc( GetProcessHeap(), 0, sizeof(INT16) * totalpoint16 );

    /* copy point counts */
    for (i = 0; i < polygons; i++)
          pointcounts[i] = counts[i];

    /* convert all points */
    for (j = totalpoint16; j--;)
    {
        pts[j].x = pt[j].x;
        pts[j].y = pt[j].y;
    }

    len = sizeof(METARECORD) + sizeof(WORD) + polygons * sizeof(INT16) +
        totalpoint16 * sizeof(*pts);

    if (!(mr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len )))
    {
         HeapFree( GetProcessHeap(), 0, pts );
         HeapFree( GetProcessHeap(), 0, pointcounts );
         return FALSE;
    }

    mr->rdSize = len / sizeof(WORD);
    mr->rdFunction = META_POLYPOLYGON;
    *mr->rdParm = polygons;
    memcpy( mr->rdParm + 1, pointcounts, polygons * sizeof(INT16) );
    memcpy( mr->rdParm + 1+polygons, pts , totalpoint16 * sizeof(*pts) );
    ret = metadc_record( hdc, mr, mr->rdSize * sizeof(WORD) );

    HeapFree( GetProcessHeap(), 0, pts );
    HeapFree( GetProcessHeap(), 0, pointcounts );
    HeapFree( GetProcessHeap(), 0, mr);
    return ret;
}

BOOL METADC_ExtFloodFill( HDC hdc, INT x, INT y, COLORREF color, UINT fill_type )
{
    return metadc_param5( hdc, META_EXTFLOODFILL, x, y, HIWORD(color), LOWORD(color), fill_type );
}

static UINT metadc_add_handle( struct metadc *metadc, HGDIOBJ obj )
{
    UINT16 index;

    for (index = 0; index < metadc->handles_size; index++)
        if (metadc->handles[index] == 0) break;
    if(index == metadc->handles_size)
    {
        metadc->handles_size += HANDLE_LIST_INC;
        metadc->handles = HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                       metadc->handles,
                                       metadc->handles_size * sizeof(metadc->handles[0]) );
    }
    metadc->handles[index] = get_full_gdi_handle( obj );

    metadc->cur_handles++;
    if (metadc->cur_handles > metadc->mh->mtNoObjects)
        metadc->mh->mtNoObjects++;

    return index ; /* index 0 is not reserved for metafiles */
}

static BOOL metadc_remove_handle( struct metadc *metadc, UINT index )
{
    BOOL ret = FALSE;

    if (index < metadc->handles_size && metadc->handles[index])
    {
        metadc->handles[index] = 0;
        metadc->cur_handles--;
        ret = TRUE;
    }
    return ret;
}

static INT16 metadc_create_brush( struct metadc *metadc, HBRUSH brush )
{
    DWORD size;
    METARECORD *mr;
    LOGBRUSH logbrush;
    BOOL r;

    if (!GetObjectA( brush, sizeof(logbrush), &logbrush )) return -1;

    switch (logbrush.lbStyle)
    {
    case BS_SOLID:
    case BS_NULL:
    case BS_HATCHED:
        {
            LOGBRUSH16 lb16;

            lb16.lbStyle = logbrush.lbStyle;
            lb16.lbColor = logbrush.lbColor;
            lb16.lbHatch = logbrush.lbHatch;
            size = sizeof(METARECORD) + sizeof(LOGBRUSH16) - sizeof(WORD);
            mr = HeapAlloc( GetProcessHeap(), 0, size );
            mr->rdSize = size / sizeof(WORD);
            mr->rdFunction = META_CREATEBRUSHINDIRECT;
            memcpy( mr->rdParm, &lb16, sizeof(LOGBRUSH16) );
            break;
        }
    case BS_PATTERN:
    case BS_DIBPATTERN:
        {
            unsigned char buffer[FIELD_OFFSET(BITMAPINFO, bmiColors)
                         + 256*RTL_FIELD_SIZE(BITMAPINFO, bmiColors)];
            BITMAPINFO *dst_info, *src_info = (BITMAPINFO *)buffer;
            DWORD info_size;
            UINT usage;

            if (!get_brush_bitmap_info( brush, src_info, NULL, &usage )) goto done;

            info_size = get_dib_info_size( src_info, usage );
            size = FIELD_OFFSET( METARECORD, rdParm[2] ) +
                info_size + src_info->bmiHeader.biSizeImage;

            if (!(mr = HeapAlloc( GetProcessHeap(), 0, size ))) goto done;
            mr->rdFunction = META_DIBCREATEPATTERNBRUSH;
            mr->rdSize = size / sizeof(WORD);
            mr->rdParm[0] = logbrush.lbStyle;
            mr->rdParm[1] = usage;
            dst_info = (BITMAPINFO *)(mr->rdParm + 2);
            get_brush_bitmap_info( brush, dst_info, (char *)dst_info + info_size, NULL );
            if (dst_info->bmiHeader.biClrUsed == 1 << dst_info->bmiHeader.biBitCount)
                dst_info->bmiHeader.biClrUsed = 0;
            break;
        }

        default:
            FIXME( "Unknown brush style %x\n", logbrush.lbStyle );
            return 0;
    }

    r = metadc_write_record( metadc, mr, mr->rdSize * sizeof(WORD) );
    HeapFree(GetProcessHeap(), 0, mr);
    if (!r) return -1;
done:
    return metadc_add_handle( metadc, brush );
}

static INT16 metadc_create_region( struct metadc *metadc, HRGN hrgn )
{
    DWORD len;
    METARECORD *mr;
    RGNDATA *rgndata;
    RECT *cur_rect, *end_rect;
    WORD bands = 0, max_bounds = 0;
    WORD *param, *start_band;
    BOOL ret;

    if (!(len = GetRegionData( hrgn, 0, NULL ))) return -1;
    if (!(rgndata = HeapAlloc( GetProcessHeap(), 0, len )))
    {
        WARN( "Can't alloc rgndata buffer\n" );
        return -1;
    }
    GetRegionData( hrgn, len, rgndata );

    /* Overestimate of length:
     * Assume every rect is a separate band -> 6 WORDs per rect,
     * see MF_Play_MetaCreateRegion for format details.
     */
    len = sizeof(METARECORD) + 20 + rgndata->rdh.nCount * 12;
    if (!(mr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len )))
    {
        WARN( "Can't alloc METARECORD buffer\n" );
        HeapFree( GetProcessHeap(), 0, rgndata );
        return -1;
    }

    param = mr->rdParm + 11;
    start_band = NULL;

    end_rect = (RECT *)rgndata->Buffer + rgndata->rdh.nCount;
    for (cur_rect = (RECT *)rgndata->Buffer; cur_rect < end_rect; cur_rect++)
    {
        if (start_band && cur_rect->top == start_band[1])
        {
            *param++ = cur_rect->left;
            *param++ = cur_rect->right;
        }
        else
        {
            if (start_band)
            {
                *start_band = param - start_band - 3;
                *param++ = *start_band;
                if (*start_band > max_bounds)
                    max_bounds = *start_band;
                bands++;
            }
            start_band = param++;
            *param++ = cur_rect->top;
            *param++ = cur_rect->bottom;
            *param++ = cur_rect->left;
            *param++ = cur_rect->right;
        }
    }

    if (start_band)
    {
        *start_band = param - start_band - 3;
        *param++ = *start_band;
        if (*start_band > max_bounds)
            max_bounds = *start_band;
        bands++;
    }

    mr->rdParm[0] = 0;
    mr->rdParm[1] = 6;
    mr->rdParm[2] = 0x2f6;
    mr->rdParm[3] = 0;
    mr->rdParm[4] = (WORD)((param - &mr->rdFunction) * sizeof(WORD));
    mr->rdParm[5] = bands;
    mr->rdParm[6] = max_bounds;
    mr->rdParm[7] = rgndata->rdh.rcBound.left;
    mr->rdParm[8] = rgndata->rdh.rcBound.top;
    mr->rdParm[9] = rgndata->rdh.rcBound.right;
    mr->rdParm[10] = rgndata->rdh.rcBound.bottom;
    mr->rdFunction = META_CREATEREGION;
    mr->rdSize = param - (WORD *)mr;
    ret = metadc_write_record( metadc, mr, mr->rdSize * 2 );
    HeapFree( GetProcessHeap(), 0, mr );
    HeapFree( GetProcessHeap(), 0, rgndata );
    if (!ret)
    {
        WARN("MFDRV_WriteRecord failed\n");
        return -1;
    }
    return metadc_add_handle( metadc, hrgn );
}

BOOL METADC_PaintRgn( HDC hdc, HRGN hrgn )
{
    struct metadc *metadc;
    INT16 index;
    if (!(metadc = get_metadc_ptr( hdc ))) return FALSE;
    index = metadc_create_region( metadc, hrgn );
    if(index == -1) return FALSE;
    return metadc_param1( hdc, META_PAINTREGION, index );
}

BOOL METADC_InvertRgn( HDC hdc, HRGN hrgn )
{
    struct metadc *metadc;
    INT16 index;
    if (!(metadc = get_metadc_ptr( hdc ))) return FALSE;
    index = metadc_create_region( metadc, hrgn );
    if (index == -1) return FALSE;
    return metadc_param1( hdc, META_INVERTREGION, index );
}

BOOL METADC_FillRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush )
{
    struct metadc *metadc;
    INT16 rgn, brush;

    if (!(metadc = get_metadc_ptr( hdc ))) return FALSE;

    rgn = metadc_create_region( metadc, hrgn );
    if (rgn == -1) return FALSE;
    brush = metadc_create_brush( metadc, hbrush );
    if (!brush) return FALSE;
    return metadc_param2( hdc, META_FILLREGION, rgn, brush );
}

BOOL METADC_FrameRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush, INT x, INT y )
{
    struct metadc *metadc;
    INT16 rgn, brush;

    if (!(metadc = get_metadc_ptr( hdc ))) return FALSE;
    rgn = metadc_create_region( metadc, hrgn );
    if (rgn == -1) return FALSE;
    brush = metadc_create_brush( metadc, hbrush );
    if (!brush) return FALSE;
    return metadc_param4( hdc, META_FRAMEREGION, rgn, brush, x, y );
}

BOOL METADC_PatBlt( HDC hdc, INT left, INT top, INT width, INT height, DWORD rop )
{
    return metadc_param6( hdc, META_PATBLT, left, top, width, height,
                          HIWORD(rop), LOWORD(rop) );
}

static BOOL metadc_stretchblt( HDC hdc, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                               HDC hdc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                               DWORD rop, WORD type )
{
    BITMAPINFO src_info = {{ sizeof( src_info.bmiHeader ) }};
    UINT bmi_size, size, bpp;
    int i = 0, bitmap_offset;
    BITMAPINFO *bmi;
    METARECORD *mr;
    HBITMAP bitmap;
    BOOL ret;

    if (!(bitmap = GetCurrentObject( hdc_src, OBJ_BITMAP ))) return FALSE;
    if (!GetDIBits( hdc_src, bitmap, 0, INT_MAX, NULL, &src_info, DIB_RGB_COLORS )) return FALSE;

    bpp = src_info.bmiHeader.biBitCount;
    if (bpp <= 8)
        bmi_size = sizeof(BITMAPINFOHEADER) + (1L << bpp) * sizeof(RGBQUAD);
    else if (bpp == 16 || bpp == 32)
        bmi_size = sizeof(BITMAPINFOHEADER) + 3 * sizeof(RGBQUAD);
    else
        bmi_size = sizeof(BITMAPINFOHEADER);

    bitmap_offset = type == META_DIBBITBLT ? 8 : 10;
    size = FIELD_OFFSET( METARECORD, rdParm[bitmap_offset] ) + bmi_size +
        src_info.bmiHeader.biSizeImage;
    if (!(mr = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;
    mr->rdFunction = type;
    bmi = (BITMAPINFO *)&mr->rdParm[bitmap_offset];
    bmi->bmiHeader = src_info.bmiHeader;
    TRACE( "size = %u  rop=%x\n", size, rop );

    ret = GetDIBits( hdc_src, bitmap, 0, src_info.bmiHeader.biHeight, (BYTE *)bmi + bmi_size,
                     bmi, DIB_RGB_COLORS );
    if (ret)
    {
        mr->rdSize = size / sizeof(WORD);
        mr->rdParm[i++] = LOWORD(rop);
        mr->rdParm[i++] = HIWORD(rop);
        if (bitmap_offset > 8)
        {
            mr->rdParm[i++] = height_src;
            mr->rdParm[i++] = width_src;
        }
        mr->rdParm[i++] = y_src;
        mr->rdParm[i++] = x_src;
        mr->rdParm[i++] = height_dst;
        mr->rdParm[i++] = width_dst;
        mr->rdParm[i++] = y_dst;
        mr->rdParm[i++] = x_dst;
        ret = metadc_record( hdc, mr, size );
    }

    HeapFree( GetProcessHeap(), 0, mr);
    return ret;
}

BOOL METADC_BitBlt( HDC hdc, INT x_dst, INT y_dst, INT width, INT height,
                    HDC hdc_src, INT x_src, INT y_src, DWORD rop )
{
    return metadc_stretchblt( hdc, x_dst, y_dst, width, height,
                              hdc_src, x_src, y_src, width, height, rop, META_DIBBITBLT );
}

BOOL METADC_StretchBlt( HDC hdc_dst, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                        HDC hdc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                        DWORD rop )
{
    return metadc_stretchblt( hdc_dst, x_dst, y_dst, width_dst, height_dst,
                              hdc_src, x_src, y_src, width_src, height_src, rop, META_DIBSTRETCHBLT );
}

INT METADC_StretchDIBits( HDC hdc, INT x_dst, INT y_dst, INT width_dst,
                          INT height_dst, INT x_src, INT y_src, INT width_src,
                          INT height_src, const void *bits,
                          const BITMAPINFO *info, UINT usage, DWORD rop )
{
    DWORD infosize = get_dib_info_size( info, usage );
    DWORD len = sizeof(METARECORD) + 10 * sizeof(WORD) + infosize + info->bmiHeader.biSizeImage;
    METARECORD *mr;

    if (!(mr = HeapAlloc( GetProcessHeap(), 0, len ))) return 0;

    mr->rdSize = len / sizeof(WORD);
    mr->rdFunction = META_STRETCHDIB;
    mr->rdParm[0] = LOWORD(rop);
    mr->rdParm[1] = HIWORD(rop);
    mr->rdParm[2] = usage;
    mr->rdParm[3] = height_src;
    mr->rdParm[4] = width_src;
    mr->rdParm[5] = y_src;
    mr->rdParm[6] = x_src;
    mr->rdParm[7] = height_dst;
    mr->rdParm[8] = width_dst;
    mr->rdParm[9] = y_dst;
    mr->rdParm[10] = x_dst;
    memcpy( mr->rdParm + 11, info, infosize );
    memcpy( mr->rdParm + 11 + infosize / 2, bits, info->bmiHeader.biSizeImage );
    metadc_record( hdc, mr, mr->rdSize * sizeof(WORD) );
    HeapFree( GetProcessHeap(), 0, mr );
    return height_src;
}

INT METADC_SetDIBitsToDevice( HDC hdc, INT x_dst, INT y_dst, DWORD width, DWORD height,
                              INT x_src, INT y_src, UINT startscan, UINT lines,
                              const void *bits, const BITMAPINFO *info, UINT coloruse )

{
    DWORD infosize = get_dib_info_size(info, coloruse);
    DWORD len = sizeof(METARECORD) + 8 * sizeof(WORD) + infosize + info->bmiHeader.biSizeImage;
    METARECORD *mr;

    if (!(mr = HeapAlloc( GetProcessHeap(), 0, len ))) return 0;

    mr->rdSize = len / sizeof(WORD);
    mr->rdFunction = META_SETDIBTODEV;
    mr->rdParm[0] = coloruse;
    mr->rdParm[1] = lines;
    mr->rdParm[2] = startscan;
    mr->rdParm[3] = y_src;
    mr->rdParm[4] = x_src;
    mr->rdParm[5] = height;
    mr->rdParm[6] = width;
    mr->rdParm[7] = y_dst;
    mr->rdParm[8] = x_dst;
    memcpy( mr->rdParm + 9, info, infosize );
    memcpy( mr->rdParm + 9 + infosize / sizeof(WORD), bits, info->bmiHeader.biSizeImage );
    metadc_record( hdc, mr, mr->rdSize * sizeof(WORD) );
    HeapFree( GetProcessHeap(), 0, mr );
    return lines;
}

static BOOL metadc_text( HDC hdc, short x, short y, UINT16 flags, const RECT16 *rect,
                         const char *str, short count, const INT16 *dx )
{
    BOOL ret;
    DWORD len;
    METARECORD *mr;
    BOOL isrect = flags & (ETO_CLIPPED | ETO_OPAQUE);

    len = sizeof(METARECORD) + (((count + 1) >> 1) * 2) + 2 * sizeof(short)
            + sizeof(UINT16);
    if (isrect) len += sizeof(RECT16);
    if (dx) len += count * sizeof(INT16);
    if (!(mr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len ))) return FALSE;

    mr->rdSize = len / sizeof(WORD);
    mr->rdFunction = META_EXTTEXTOUT;
    mr->rdParm[0] = y;
    mr->rdParm[1] = x;
    mr->rdParm[2] = count;
    mr->rdParm[3] = flags;
    if (isrect) memcpy( mr->rdParm + 4, rect, sizeof(RECT16) );
    memcpy( mr->rdParm + (isrect ? 8 : 4), str, count );
    if (dx)
        memcpy( mr->rdParm + (isrect ? 8 : 4) + ((count + 1) >> 1), dx, count * sizeof(INT16) );
    ret = metadc_record( hdc, mr, mr->rdSize * sizeof(WORD) );
    HeapFree( GetProcessHeap(), 0, mr );
    return ret;
}

BOOL METADC_ExtTextOut( HDC hdc, INT x, INT y, UINT flags, const RECT *lprect,
                        const WCHAR *str, UINT count, const INT *dx )
{
    RECT16 rect16;
    LPINT16 lpdx16 = NULL;
    BOOL ret;
    unsigned int i, j;
    char *ascii;
    DWORD len;
    CHARSETINFO csi;
    int charset = GetTextCharset( hdc );
    UINT cp = CP_ACP;

    if (TranslateCharsetInfo( ULongToPtr(charset), &csi, TCI_SRCCHARSET ))
        cp = csi.ciACP;
    else
    {
        switch(charset)
        {
        case OEM_CHARSET:
            cp = GetOEMCP();
            break;
        case DEFAULT_CHARSET:
            cp = GetACP();
            break;

        case VISCII_CHARSET:
        case TCVN_CHARSET:
        case KOI8_CHARSET:
        case ISO3_CHARSET:
        case ISO4_CHARSET:
        case ISO10_CHARSET:
        case CELTIC_CHARSET:
            /* FIXME: These have no place here, but because x11drv
               enumerates fonts with these (made up) charsets some apps
               might use them and then the FIXME below would become
               annoying.  Now we could pick the intended codepage for
               each of these, but since it's broken anyway we'll just
               use CP_ACP and hope it'll go away...
            */
            cp = CP_ACP;
            break;

        default:
            FIXME("Can't find codepage for charset %d\n", charset);
            break;
        }
    }


    TRACE( "cp = %d\n", cp );
    len = WideCharToMultiByte( cp, 0, str, count, NULL, 0, NULL, NULL );
    ascii = HeapAlloc( GetProcessHeap(), 0, len );
    WideCharToMultiByte( cp, 0, str, count, ascii, len, NULL, NULL );
    TRACE( "mapped %s -> %s\n", debugstr_wn(str, count), debugstr_an(ascii, len) );


    if (lprect)
    {
        rect16.left   = lprect->left;
        rect16.top    = lprect->top;
        rect16.right  = lprect->right;
        rect16.bottom = lprect->bottom;
    }

    if (dx)
    {
        lpdx16 = HeapAlloc( GetProcessHeap(), 0, sizeof(INT16) * len );
        for (i = j = 0; i < len; )
            if (IsDBCSLeadByteEx( cp, ascii[i] ))
            {
                lpdx16[i++] = dx[j++];
                lpdx16[i++] = 0;
            }
            else
                lpdx16[i++] = dx[j++];
    }

    ret = metadc_text( hdc, x, y, flags, lprect ? &rect16 : NULL, ascii, len, lpdx16 );
    HeapFree( GetProcessHeap(), 0, ascii );
    HeapFree( GetProcessHeap(), 0, lpdx16 );
    return ret;
}

BOOL METADC_ExtSelectClipRgn( HDC hdc, HRGN hrgn, INT mode )
{
    struct metadc *metadc;
    INT16 rgn;
    INT ret;

    if (!(metadc = get_metadc_ptr( hdc ))) return FALSE;
    if (mode != RGN_COPY) return ERROR;
    if (!hrgn) return NULLREGION;
    rgn = metadc_create_region( metadc, hrgn );
    if(rgn == -1) return ERROR;
    ret = metadc_param1( hdc, META_SELECTOBJECT, rgn ) ? NULLREGION : ERROR;
    metadc_param1( hdc, META_DELETEOBJECT, rgn );
    metadc_remove_handle( metadc, rgn );
    return ret;
}

static INT16 metadc_find_object( struct metadc *metadc, HGDIOBJ obj )
{
    INT16 index;

    for (index = 0; index < metadc->handles_size; index++)
        if (metadc->handles[index] == obj) return index;

    return -1;
}

void METADC_DeleteObject( HDC hdc, HGDIOBJ obj )
{
    struct metadc *metadc = get_metadc_ptr( hdc );
    METARECORD mr;
    INT16 index;

    if ((index = metadc_find_object( metadc, obj )) < 0) return;
    if (obj == metadc->pen || obj == metadc->brush || obj == metadc->font)
    {
        WARN( "deleting selected object %p\n", obj );
        return;
    }

    mr.rdSize = sizeof(mr) / sizeof(WORD);
    mr.rdFunction = META_DELETEOBJECT;
    mr.rdParm[0] = index;

    metadc_write_record( metadc, &mr, mr.rdSize * sizeof(WORD) );

    metadc->handles[index] = 0;
    metadc->cur_handles--;
}

static BOOL metadc_select_object( HDC hdc, INT16 index)
{
    METARECORD mr;

    mr.rdSize = sizeof mr / 2;
    mr.rdFunction = META_SELECTOBJECT;
    mr.rdParm[0] = index;
    return metadc_record( hdc, &mr, mr.rdSize * sizeof(WORD) );
}

static HBRUSH METADC_SelectBrush( HDC hdc, HBRUSH hbrush )
{
    struct metadc *metadc = get_metadc_ptr( hdc );
    INT16 index;
    HBRUSH ret;

    index = metadc_find_object( metadc, hbrush );
    if( index < 0 )
    {
        index = metadc_create_brush( metadc, hbrush );
        if( index < 0 )
            return 0;
        GDI_hdc_using_object( hbrush, hdc );//, METADC_DeleteObject );
    }
    if (!metadc_select_object( hdc, index )) return 0;
    ret = metadc->brush;
    metadc->brush = hbrush;
    return ret;
}

static UINT16 metadc_create_font( struct metadc *metadc, HFONT font, LOGFONTW *logfont )
{
    unsigned char buffer[FIELD_OFFSET(METARECORD, rdParm) + sizeof(LOGFONT16)];
    METARECORD *mr = (METARECORD *)&buffer;
    LOGFONT16 *font16;
    INT written;

    mr->rdSize = sizeof(buffer) / sizeof(WORD);
    mr->rdFunction = META_CREATEFONTINDIRECT;
    font16 = (LOGFONT16 *)&mr->rdParm;

    font16->lfHeight         = logfont->lfHeight;
    font16->lfWidth          = logfont->lfWidth;
    font16->lfEscapement     = logfont->lfEscapement;
    font16->lfOrientation    = logfont->lfOrientation;
    font16->lfWeight         = logfont->lfWeight;
    font16->lfItalic         = logfont->lfItalic;
    font16->lfUnderline      = logfont->lfUnderline;
    font16->lfStrikeOut      = logfont->lfStrikeOut;
    font16->lfCharSet        = logfont->lfCharSet;
    font16->lfOutPrecision   = logfont->lfOutPrecision;
    font16->lfClipPrecision  = logfont->lfClipPrecision;
    font16->lfQuality        = logfont->lfQuality;
    font16->lfPitchAndFamily = logfont->lfPitchAndFamily;
    written = WideCharToMultiByte( CP_ACP, 0, logfont->lfFaceName, -1, font16->lfFaceName,
                                   LF_FACESIZE - 1, NULL, NULL );
    /* Zero pad the facename buffer, so that we don't write uninitialized data to disk */
    memset( font16->lfFaceName + written, 0, LF_FACESIZE - written );

    if (!metadc_write_record( metadc, mr, mr->rdSize * 2 ))
        return 0;
    return metadc_add_handle( metadc, font );
}

static HFONT METADC_SelectFont( HDC hdc, HFONT hfont )
{
    struct metadc *metadc = get_metadc_ptr( hdc );
    LOGFONTW font;
    INT16 index;
    HFONT ret;

    index = metadc_find_object( metadc, hfont );
    if (index < 0)
    {
        if (!GetObjectW( hfont, sizeof(font), &font ))
            return 0;
        index = metadc_create_font( metadc, hfont, &font );
        if( index < 0 )
            return 0;
        GDI_hdc_using_object( hfont, hdc );//, METADC_DeleteObject );
    }
    if (!metadc_select_object( hdc, index )) return 0;
    ret = metadc->font;
    metadc->font = hfont;
    return ret;
}

static UINT16 metadc_create_pen( struct metadc *metadc, HPEN pen, LOGPEN16 *logpen )
{
    unsigned char buffer[FIELD_OFFSET(METARECORD, rdParm)
        + (sizeof(*logpen) / sizeof(WORD)) * RTL_FIELD_SIZE(METARECORD, rdParm)];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = sizeof(buffer) / sizeof(WORD);
    mr->rdFunction = META_CREATEPENINDIRECT;
    memcpy( mr->rdParm, logpen, sizeof(*logpen) );
    if (!metadc_write_record( metadc, mr, mr->rdSize * sizeof(WORD) )) return 0;
    return metadc_add_handle( metadc, pen );
}

static HPEN METADC_SelectPen( HDC hdc, HPEN hpen )
{
    struct metadc *metadc = get_metadc_ptr( hdc );
    LOGPEN16 logpen;
    INT16 index;
    HPEN ret;

    index = metadc_find_object( metadc, hpen );
    if (index < 0)
    {
        /* must be an extended pen */
        INT size = GetObjectW( hpen, 0, NULL );

        if (!size) return 0;

        if (size == sizeof(LOGPEN))
        {
            LOGPEN pen;

            GetObjectW( hpen, sizeof(pen), &pen );
            logpen.lopnStyle   = pen.lopnStyle;
            logpen.lopnWidth.x = pen.lopnWidth.x;
            logpen.lopnWidth.y = pen.lopnWidth.y;
            logpen.lopnColor   = pen.lopnColor;
        }
        else  /* must be an extended pen */
        {
            EXTLOGPEN *elp = HeapAlloc( GetProcessHeap(), 0, size );

            GetObjectW( hpen, size, elp );
            /* FIXME: add support for user style pens */
            logpen.lopnStyle = elp->elpPenStyle;
            logpen.lopnWidth.x = elp->elpWidth;
            logpen.lopnWidth.y = 0;
            logpen.lopnColor = elp->elpColor;

            HeapFree( GetProcessHeap(), 0, elp );
        }

        index = metadc_create_pen( metadc, hpen, &logpen );
        if( index < 0 )
            return 0;
        GDI_hdc_using_object( hpen, hdc );//, METADC_DeleteObject );
    }

    if (!metadc_select_object( hdc, index )) return 0;
    ret = metadc->pen;
    metadc->pen = hpen;
    return ret;
}

static BOOL metadc_create_palette( struct metadc *metadc, HPALETTE palette,
                                   LOGPALETTE *log_palette, int sizeofPalette )
{
    int index;
    BOOL ret;
    METARECORD *mr;

    mr = HeapAlloc( GetProcessHeap(), 0, sizeof(METARECORD) + sizeofPalette - sizeof(WORD) );
    if (!mr) return FALSE;
    mr->rdSize = (sizeof(METARECORD) + sizeofPalette - sizeof(WORD)) / sizeof(WORD);
    mr->rdFunction = META_CREATEPALETTE;
    memcpy(&(mr->rdParm), log_palette, sizeofPalette);
    if (!metadc_write_record( metadc, mr, mr->rdSize * sizeof(WORD) ))
    {
        HeapFree(GetProcessHeap(), 0, mr);
        return FALSE;
    }

    mr->rdSize = sizeof(METARECORD) / sizeof(WORD);
    mr->rdFunction = META_SELECTPALETTE;

    if ((index = metadc_add_handle( metadc, palette )) == -1) ret = FALSE;
    else
    {
        mr->rdParm[0] = index;
        ret = metadc_write_record( metadc, mr, mr->rdSize * sizeof(WORD) );
    }
    HeapFree( GetProcessHeap(), 0, mr );
    return ret;
}

BOOL METADC_SelectPalette( HDC hdc, HPALETTE palette )
{
    struct metadc *metadc;
    PLOGPALETTE log_palette;
    WORD count = 0;
    BOOL  ret;
    int size;

    if (!(metadc = get_metadc_ptr( hdc ))) return FALSE;
    GetObjectA( palette, sizeof(WORD), &count );
    if (!count) return 0;

    size = sizeof(LOGPALETTE) + (count - 1) * sizeof(PALETTEENTRY);
    log_palette = HeapAlloc( GetProcessHeap(), 0, size );
    if (!log_palette) return 0;

    log_palette->palVersion = 0x300;
    log_palette->palNumEntries = count;

    GetPaletteEntries( palette, 0, count, log_palette->palPalEntry );

    ret = metadc_create_palette( metadc, palette, log_palette, size );

    HeapFree( GetProcessHeap(), 0, log_palette );
    return ret;
}

BOOL METADC_RealizePalette( HDC hdc )
{
    return metadc_param0( hdc, META_REALIZEPALETTE );
}

HGDIOBJ METADC_SelectObject( HDC hdc, HGDIOBJ obj )
{
    switch (GDI_HANDLE_GET_TYPE( obj ))
    {
    case GDILoObjType_LO_BRUSH_TYPE:
        return METADC_SelectBrush( hdc, obj );
    case GDILoObjType_LO_FONT_TYPE:
        return METADC_SelectFont( hdc, obj );
    case GDILoObjType_LO_PEN_TYPE:
    case GDILoObjType_LO_EXTPEN_TYPE:
        return METADC_SelectPen( hdc, obj );
    default:
        SetLastError( ERROR_INVALID_FUNCTION );
        return 0;
    }
}

BOOL METADC_ExtEscape( HDC hdc, INT escape, INT input_size, LPCSTR input, INT output_size, LPVOID output )
{
    METARECORD *mr;
    DWORD len;
    BOOL ret;

    if (output_size) return FALSE;  /* escapes that require output cannot work in metafiles */

    len = sizeof(*mr) + sizeof(WORD) + ((input_size + 1) & ~1);
    if (!(mr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len ))) return FALSE;
    mr->rdSize = len / sizeof(WORD);
    mr->rdFunction = META_ESCAPE;
    mr->rdParm[0] = escape;
    mr->rdParm[1] = input_size;
    memcpy( &mr->rdParm[2], input, input_size );
    ret = metadc_record( hdc, mr, len );
    HeapFree(GetProcessHeap(), 0, mr);
    return ret;
}

INT METADC_GetDeviceCaps( HDC hdc, INT cap )
{
    if (!get_metadc_ptr( hdc )) return 0;

    switch(cap)
    {
    case TECHNOLOGY:
        return DT_METAFILE;
    case TEXTCAPS:
        return 0;
    default:
        TRACE(" unsupported capability %d, will return 0\n", cap );
    }
    return 0;
}

static void metadc_free( struct metadc *metadc )
{
    DWORD index;

    CloseHandle( metadc->hFile );
    HeapFree( GetProcessHeap(), 0, metadc->mh );
    for(index = 0; index < metadc->handles_size; index++)
        if(metadc->handles[index])
            GDI_hdc_not_using_object( metadc->handles[index], metadc->hdc );
    HeapFree( GetProcessHeap(), 0, metadc->handles );
    HeapFree( GetProcessHeap(), 0, metadc );
}

BOOL METADC_DeleteDC( HDC hdc )
{
    struct metadc *metadc;

    if (!(metadc = get_metadc_ptr( hdc ))) return FALSE;
    if (!GdiDeleteClientObj( hdc )) return FALSE;
    metadc_free( metadc );
    return TRUE;
}

/**********************************************************************
 *           CreateMetaFileW   (GDI32.@)
 *
 *  Create a new DC and associate it with a metafile. Pass a filename
 *  to create a disk-based metafile, NULL to create a memory metafile.
 */
HDC WINAPI CreateMetaFileW( const WCHAR *filename )
{
    struct metadc *metadc;
    HANDLE hdc;

    TRACE( "%s\n", debugstr_w(filename) );

    metadc = HeapAlloc( GetProcessHeap(), 0, sizeof(*metadc) );
    if (!metadc)
    {
        return NULL;
    }
    if (!(metadc->mh = HeapAlloc( GetProcessHeap(), 0, sizeof(*metadc->mh) )))
    {
        HeapFree( GetProcessHeap(), 0, metadc );
        return NULL;
    }

    if (!(hdc = GdiCreateClientObj( metadc, GDILoObjType_LO_METADC16_TYPE )))
    {
        HeapFree( GetProcessHeap(), 0, metadc );
        return NULL;
    }

    metadc->hdc = hdc;

    metadc->handles = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                 HANDLE_LIST_INC * sizeof(metadc->handles[0]) );
    metadc->handles_size = HANDLE_LIST_INC;
    metadc->cur_handles = 0;

    metadc->hFile = 0;

    metadc->mh->mtHeaderSize   = sizeof(METAHEADER) / sizeof(WORD);
    metadc->mh->mtVersion      = 0x0300;
    metadc->mh->mtSize         = metadc->mh->mtHeaderSize;
    metadc->mh->mtNoObjects    = 0;
    metadc->mh->mtMaxRecord    = 0;
    metadc->mh->mtNoParameters = 0;
    metadc->mh->mtType         = METAFILE_MEMORY;

    metadc->pen   = GetStockObject( BLACK_PEN );
    metadc->brush = GetStockObject( WHITE_BRUSH );
    metadc->font  = GetStockObject( DEVICE_DEFAULT_FONT );

    SetVirtualResolution( hdc, 0, 0, 0, 0);

    if (filename)  /* disk based metafile */
    {
        HANDLE file = CreateFileW( filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
        if (file == INVALID_HANDLE_VALUE)
        {
            HeapFree( GetProcessHeap(), 0, metadc );
            GdiDeleteClientObj( hdc );
            return 0;
        }
        metadc->hFile = file;
    }

    TRACE( "returning %p\n", hdc );
    return hdc;
}

/**********************************************************************
 *           CreateMetaFileA   (GDI32.@)
 */
HDC WINAPI CreateMetaFileA( const char *filename )
{
    LPWSTR filenameW;
    DWORD len;
    HDC hdc;

    if (!filename) return CreateMetaFileW( NULL );

    len = MultiByteToWideChar( CP_ACP, 0, filename, -1, NULL, 0 );
    filenameW = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, filename, -1, filenameW, len );

    hdc = CreateMetaFileW( filenameW );

    HeapFree( GetProcessHeap(), 0, filenameW );
    return hdc;
}

/******************************************************************
 *           CloseMetaFile   (GDI32.@)
 *
 *  Stop recording graphics operations in metafile associated with
 *  hdc and retrieve metafile.
 */
HMETAFILE WINAPI CloseMetaFile( HDC hdc )
{
    struct metadc *metadc;
    DWORD bytes_written;
    HMETAFILE hmf;

    TRACE( "(%p)\n", hdc );

    if (!(metadc = get_metadc_ptr( hdc ))) return FALSE;

    /* Construct the end of metafile record - this is documented
     * in SDK Knowledgebase Q99334.
     */
    if (!metadc_param0( hdc, META_EOF )) return FALSE;
    if (!GdiDeleteClientObj( hdc )) return FALSE;

    if (metadc->hFile && !WriteFile( metadc->hFile, metadc->mh, metadc->mh->mtSize * sizeof(WORD),
                                     &bytes_written, NULL ))
    {
        metadc_free( metadc );
        return FALSE;
    }

    /* Now allocate a global handle for the metafile */
    hmf = MF_Create_HMETAFILE( metadc->mh );
    if (hmf) metadc->mh = NULL;  /* So it won't be deleted */
    metadc_free( metadc );
    return hmf;
}
