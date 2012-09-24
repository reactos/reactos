/*
 * Scroll windows and DCs
 *
 * Copyright 1993  David W. Metcalfe
 * Copyright 1995, 1996 Alex Korobka
 * Copyright 2001 Alexandre Julliard
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

#include "config.h"

#include <stdarg.h>
#include <X11/Xlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "x11drv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(scroll);

static void dump_region( const char *p, HRGN hrgn)
{
    DWORD i, size;
    RGNDATA *data = NULL;
    RECT *rect;

    if (!hrgn) {
        TRACE( "%s null region\n", p );
        return;
    }
    if (!(size = GetRegionData( hrgn, 0, NULL ))) {
        return;
    }
    if (!(data = HeapAlloc( GetProcessHeap(), 0, size ))) return;
    GetRegionData( hrgn, size, data );
    TRACE("%s %d rects:", p, data->rdh.nCount );
    for (i = 0, rect = (RECT *)data->Buffer; i<20 && i < data->rdh.nCount; i++, rect++)
        TRACE( " %s", wine_dbgstr_rect( rect));
    TRACE("\n");
    HeapFree( GetProcessHeap(), 0, data );
}

/*************************************************************************
 *		ScrollDC   (X11DRV.@)
 */
BOOL CDECL X11DRV_ScrollDC( HDC hdc, INT dx, INT dy, const RECT *lprcScroll,
                            const RECT *lprcClip, HRGN hrgnUpdate, LPRECT lprcUpdate )
{
    RECT rcSrc, rcClip, offset;
    INT dxdev, dydev, res;
    HRGN DstRgn, clipRgn, visrgn;
    INT code = X11DRV_START_EXPOSURES;

    TRACE("dx,dy %d,%d rcScroll %s rcClip %s hrgnUpdate %p lprcUpdate %p\n",
            dx, dy, wine_dbgstr_rect(lprcScroll), wine_dbgstr_rect(lprcClip),
            hrgnUpdate, lprcUpdate);
    /* enable X-exposure events */
    if (hrgnUpdate || lprcUpdate)
        ExtEscape( hdc, X11DRV_ESCAPE, sizeof(code), (LPSTR)&code, 0, NULL );
    /* get the visible region */
    visrgn=CreateRectRgn( 0, 0, 0, 0);
    GetRandomRgn( hdc, visrgn, SYSRGN);
    if( !(GetVersion() & 0x80000000)) {
        /* Window NT/2k/XP */
        POINT org;
        GetDCOrgEx(hdc, &org);
        OffsetRgn( visrgn, -org.x, -org.y);
    }
    /* intersect with the clipping Region if the DC has one */
    clipRgn = CreateRectRgn( 0, 0, 0, 0);
    if (GetClipRgn( hdc, clipRgn) != 1) {
        DeleteObject(clipRgn);
        clipRgn=NULL;
    } else
        CombineRgn( visrgn, visrgn, clipRgn, RGN_AND);
    /* only those pixels in the scroll rectangle that remain in the clipping
     * rect are scrolled. */
    if( lprcClip) 
        rcClip = *lprcClip;
    else
        GetClipBox( hdc, &rcClip);
    rcSrc = rcClip;
    OffsetRect( &rcClip, -dx, -dy);
    IntersectRect( &rcSrc, &rcSrc, &rcClip);
    /* if an scroll rectangle is specified, only the pixels within that
     * rectangle are scrolled */
    if( lprcScroll)
        IntersectRect( &rcSrc, &rcSrc, lprcScroll);
    /* now convert to device coordinates */
    LPtoDP(hdc, (LPPOINT)&rcSrc, 2);
    TRACE("source rect: %s\n", wine_dbgstr_rect(&rcSrc));
    /* also dx and dy */
    SetRect(&offset, 0, 0, dx, dy);
    LPtoDP(hdc, (LPPOINT)&offset, 2);
    dxdev = offset.right - offset.left;
    dydev = offset.bottom - offset.top;
    /* now intersect with the visible region to get the pixels that will
     * actually scroll */
    DstRgn = CreateRectRgnIndirect( &rcSrc);
    res = CombineRgn( DstRgn, DstRgn, visrgn, RGN_AND);
    /* and translate, giving the destination region */
    OffsetRgn( DstRgn, dxdev, dydev);
    if( TRACE_ON( scroll)) dump_region( "Destination scroll region: ", DstRgn);
    /* if there are any, do it */
    if( res > NULLREGION) {
        RECT rect ;
        /* clip to the destination region, so we can BitBlt with a simple
         * bounding rectangle */
        if( clipRgn)
            ExtSelectClipRgn( hdc, DstRgn, RGN_AND);
        else
            SelectClipRgn( hdc, DstRgn);
        GetRgnBox( DstRgn, &rect);
        DPtoLP(hdc, (LPPOINT)&rect, 2);
        TRACE("destination rect: %s\n", wine_dbgstr_rect(&rect));

        BitBlt( hdc, rect.left, rect.top,
                    rect.right - rect.left, rect.bottom - rect.top,
                    hdc, rect.left - dx, rect.top - dy, SRCCOPY);
    }
    /* compute the update areas.  This is the combined clip rectangle
     * minus the scrolled region, and intersected with the visible
     * region. */
    if (hrgnUpdate || lprcUpdate)
    {
        HRGN hrgn = hrgnUpdate;
        HRGN ExpRgn = 0;

        /* collect all the exposures */
        code = X11DRV_END_EXPOSURES;
        ExtEscape( hdc, X11DRV_ESCAPE, sizeof(code), (LPSTR)&code,
                sizeof(ExpRgn), (LPSTR)&ExpRgn );
        /* Intersect clip and scroll rectangles, allowing NULL values */ 
        if( lprcScroll)
            if( lprcClip)
                IntersectRect( &rcClip, lprcClip, lprcScroll);
            else
                rcClip = *lprcScroll;
        else
            if( lprcClip)
                rcClip = *lprcClip;
            else
                GetClipBox( hdc, &rcClip);
        /* Convert the combined clip rectangle to device coordinates */
        LPtoDP(hdc, (LPPOINT)&rcClip, 2);
        if( hrgn )
            SetRectRgn( hrgn, rcClip.left, rcClip.top, rcClip.right,
                    rcClip.bottom);
        else
            hrgn = CreateRectRgnIndirect( &rcClip);
        CombineRgn( hrgn, hrgn, visrgn, RGN_AND);
        CombineRgn( hrgn, hrgn, DstRgn, RGN_DIFF);
        /* add the exposures to this */
        if( ExpRgn) {
            if( TRACE_ON( scroll)) dump_region( "Expose region: ", ExpRgn);
            CombineRgn( hrgn, hrgn, ExpRgn, RGN_OR);
            DeleteObject( ExpRgn);
        }
        if( TRACE_ON( scroll)) dump_region( "Update region: ", hrgn);
        if( lprcUpdate) {
            GetRgnBox( hrgn, lprcUpdate );
            /* Put the lprcUpdate in logical coordinates */
            DPtoLP( hdc, (LPPOINT)lprcUpdate, 2 );
            TRACE("returning lprcUpdate %s\n", wine_dbgstr_rect(lprcUpdate));
        }
        if( !hrgnUpdate)
            DeleteObject( hrgn);
    }
    /* restore original clipping region */
    SelectClipRgn( hdc, clipRgn);
    DeleteObject( visrgn);
    DeleteObject( DstRgn);
    if( clipRgn) DeleteObject( clipRgn);
    return TRUE;
}
