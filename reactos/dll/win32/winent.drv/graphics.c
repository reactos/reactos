/*
 * PROJECT:         ReactOS
 * LICENSE:         GNU LGPL by FSF v2.1 or any later
 * FILE:            dll/win32/winent.drv/graphics.c
 * PURPOSE:         GDI high-level driver for ReactOS/Windows
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 *                  Some code is taken from winex11.drv (c) Wine project
 */

/* INCLUDES ***************************************************************/

#include "winent.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(graphics);

/* FUNCTIONS **************************************************************/

/* get the rectangle in device coordinates, with optional mirroring */
static RECT get_device_rect( HDC hdc, int left, int top, int right, int bottom )
{
    RECT rect;

    rect.left   = left;
    rect.top    = top;
    rect.right  = right;
    rect.bottom = bottom;
    if (GetLayout( hdc ) & LAYOUT_RTL)
    {
        /* shift the rectangle so that the right border is included after mirroring */
        /* it would be more correct to do this after LPtoDP but that's not what Windows does */
        rect.left--;
        rect.right--;
    }
    LPtoDP( hdc, (POINT *)&rect, 2 );
    if (rect.left > rect.right)
    {
        int tmp = rect.left;
        rect.left = rect.right;
        rect.right = tmp;
    }
    if (rect.top > rect.bottom)
    {
        int tmp = rect.top;
        rect.top = rect.bottom;
        rect.bottom = tmp;
    }
    return rect;
}

void CDECL RosDrv_SetDeviceClipping( PDC_ATTR pdcattr, HRGN vis_rgn, HRGN clip_rgn )
{
    RGNDATA *data;
    DWORD size, i;
    RECT *pRects;

    //FIXME("SetDeviceClipping hdc %x\n", pdcattr->hdc);

    /* Update dc region to become a combined region */
    CombineRgn( pdcattr->region, vis_rgn, clip_rgn, clip_rgn ? RGN_AND : RGN_COPY );

    /* Get region data size */
    if (!(size = GetRegionData( pdcattr->region, 0, NULL )))
        return;

    /* Allocate memory for it */
    if (!(data = HeapAlloc( GetProcessHeap(), 0, size )))
        return;

    /* Get region data */
    if (!GetRegionData( pdcattr->region, size, data ))
    {
        HeapFree( GetProcessHeap(), 0, data );
        return;
    }

    /* Offset all rects */
    pRects = (RECT *)data->Buffer;
    for (i=0; i<data->rdh.nCount; i++)
        OffsetRect(&pRects[i], pdcattr->dc_rect.left, pdcattr->dc_rect.top);

    /* Offset bounding rect */
    OffsetRect(&data->rdh.rcBound, pdcattr->dc_rect.left, pdcattr->dc_rect.top);

    /* Set clipping */
    RosGdiSetDeviceClipping(pdcattr->hKernelDC, data->rdh.nCount, (RECTL *)data->Buffer, (RECTL *)&data->rdh.rcBound);

    /* Free memory and delete clipping region */
    HeapFree( GetProcessHeap(), 0, data );
}


/***********************************************************************
 *           RosDrv_XWStoDS
 *
 * Performs a world-to-viewport transformation on the specified width.
 */
INT RosDrv_XWStoDS( PDC_ATTR pdcattr, INT width )
{
    POINT pt[2];

    pt[0].x = 0;
    pt[0].y = 0;
    pt[1].x = width;
    pt[1].y = 0;
    LPtoDP( pdcattr->hdc, pt, 2 );
    return pt[1].x - pt[0].x;
}

/***********************************************************************
 *           RosDrv_YWStoDS
 *
 * Performs a world-to-viewport transformation on the specified height.
 */
INT RosDrv_YWStoDS( PDC_ATTR pdcattr, INT height )
{
    POINT pt[2];

    pt[0].x = 0;
    pt[0].y = 0;
    pt[1].x = 0;
    pt[1].y = height;
    LPtoDP( pdcattr->hdc, pt, 2 );
    return pt[1].y - pt[0].y;
}

BOOL CDECL RosDrv_LineTo( PDC_ATTR pdcattr, INT x, INT y )
{
    POINT pt[2];

    /* Get current cursor position */
    GetCurrentPositionEx( pdcattr->hdc, &pt[0] );

    /* Convert both points coordinates to device */
    pt[1].x = x;
    pt[1].y = y;
    LPtoDP( pdcattr->hdc, pt, 2 );

    /* Draw the line */
    return RosGdiLineTo(pdcattr->hKernelDC, pdcattr->dc_rect.left + pt[0].x, pdcattr->dc_rect.top + pt[0].y,
        pdcattr->dc_rect.left + pt[1].x,  pdcattr->dc_rect.top + pt[1].y);
}

BOOL CDECL RosDrv_Arc( PDC_ATTR pdcattr, INT left, INT top, INT right, INT bottom,
            INT xstart, INT ystart, INT xend, INT yend )
{
    POINT pts[4];
    DWORD i;

    /* Map coordinates */
    pts[0].x = left;
    pts[0].y = top;
    pts[1].x = right;
    pts[1].y = bottom;
    pts[2].x = xstart;
    pts[2].y = ystart;
    pts[3].x = xend;
    pts[3].y = yend;

    LPtoDP(pdcattr->hdc, pts, 4);

    for (i=0; i<4; i++)
    {
        pts[i].x += pdcattr->dc_rect.left;
        pts[i].y += pdcattr->dc_rect.top;
    }

    return RosGdiArc(pdcattr->hKernelDC, pts[0].x, pts[0].y, pts[1].x, pts[1].y,
        pts[2].x, pts[2].y, pts[3].x, pts[3].y, GdiTypeArc);
}

BOOL CDECL RosDrv_Pie( PDC_ATTR pdcattr, INT left, INT top, INT right, INT bottom,
            INT xstart, INT ystart, INT xend, INT yend )
{
    POINT pts[4];
    DWORD i;

    /* Map coordinates */
    pts[0].x = left;
    pts[0].y = top;
    pts[1].x = right;
    pts[1].y = bottom;
    pts[2].x = xstart;
    pts[2].y = ystart;
    pts[3].x = xend;
    pts[3].y = yend;

    LPtoDP(pdcattr->hdc, pts, 4);

    for (i=0; i<4; i++)
    {
        pts[i].x += pdcattr->dc_rect.left;
        pts[i].y += pdcattr->dc_rect.top;
    }

    return RosGdiArc(pdcattr->hKernelDC, pts[0].x, pts[0].y, pts[1].x, pts[1].y,
        pts[2].x, pts[2].y, pts[3].x, pts[3].y, GdiTypePie);
}

BOOL CDECL RosDrv_Chord( PDC_ATTR pdcattr, INT left, INT top, INT right, INT bottom,
              INT xstart, INT ystart, INT xend, INT yend )
{
    POINT pts[4];
    DWORD i;

    /* Map coordinates */
    pts[0].x = left;
    pts[0].y = top;
    pts[1].x = right;
    pts[1].y = bottom;
    pts[2].x = xstart;
    pts[2].y = ystart;
    pts[3].x = xend;
    pts[3].y = yend;

    LPtoDP(pdcattr->hdc, pts, 4);

    for (i=0; i<4; i++)
    {
        pts[i].x += pdcattr->dc_rect.left;
        pts[i].y += pdcattr->dc_rect.top;
    }

    return RosGdiArc(pdcattr->hKernelDC, pts[0].x, pts[0].y, pts[1].x, pts[1].y,
        pts[2].x, pts[2].y, pts[3].x, pts[3].y, GdiTypeChord );
}

BOOL CDECL RosDrv_Ellipse( PDC_ATTR pdcattr, INT left, INT top, INT right, INT bottom )
{
    POINT pts[2];
    DWORD i;

    /* Map coordinates */
    pts[0].x = left;
    pts[0].y = top;
    pts[1].x = right;
    pts[1].y = bottom;

    LPtoDP(pdcattr->hdc, pts, 2);

    for (i=0; i<2; i++)
    {
        pts[i].x += pdcattr->dc_rect.left;
        pts[i].y += pdcattr->dc_rect.top;
    }

    return RosGdiEllipse(pdcattr->hKernelDC, pts[0].x, pts[0].y, pts[1].x, pts[1].y);
}

BOOL CDECL RosDrv_Rectangle(PDC_ATTR pdcattr, INT left, INT top, INT right, INT bottom)
{
    POINT ptBrush;
    RECT rc = get_device_rect( pdcattr->hdc, left, top, right, bottom );

    if ((rc.left == rc.right) || (rc.top == rc.bottom)) return TRUE;

    if (rc.right < rc.left) { INT tmp = rc.right; rc.right = rc.left; rc.left = tmp; }
    if (rc.bottom < rc.top) { INT tmp = rc.bottom; rc.bottom = rc.top; rc.top = tmp; }

    /* Update brush origin */
    GetBrushOrgEx(pdcattr->hdc, &ptBrush);
    ptBrush.x += pdcattr->dc_rect.left;
    ptBrush.y += pdcattr->dc_rect.top;
    RosGdiSetBrushOrg(pdcattr->hKernelDC, ptBrush.x, ptBrush.y);

    OffsetRect(&rc, pdcattr->dc_rect.left, pdcattr->dc_rect.top);
    RosGdiRectangle(pdcattr->hKernelDC, &rc);

    return TRUE;
}

BOOL CDECL RosDrv_RoundRect( PDC_ATTR pdcattr, INT left, INT top, INT right,
                  INT bottom, INT ell_width, INT ell_height )
{
    UNIMPLEMENTED;
    return FALSE;
}

COLORREF CDECL RosDrv_SetPixel( PDC_ATTR pdcattr, INT x, INT y, COLORREF color )
{
    POINT ptPixel;

    /* Transform to device coordinates */
    ptPixel.x = x; ptPixel.y = y;
    LPtoDP(pdcattr->hdc, &ptPixel, 1);

    return RosGdiSetPixel(pdcattr->hKernelDC, pdcattr->dc_rect.left + ptPixel.x, pdcattr->dc_rect.top + ptPixel.y, color);
}

COLORREF CDECL RosDrv_GetPixel( PDC_ATTR pdcattr, INT x, INT y )
{
    POINT ptPixel;

    /* Transform to device coordinates */
    ptPixel.x = x; ptPixel.y = y;
    LPtoDP(pdcattr->hdc, &ptPixel, 1);

    return RosGdiGetPixel(pdcattr->hKernelDC, pdcattr->dc_rect.left + ptPixel.x, pdcattr->dc_rect.top + ptPixel.y);
}

BOOL CDECL RosDrv_PaintRgn( PDC_ATTR pdcattr, HRGN hrgn )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL CDECL RosDrv_Polyline( PDC_ATTR pdcattr, const POINT* pt, INT count )
{
    register int i;
    POINT *points;

    if (!(points = HeapAlloc( GetProcessHeap(), 0, sizeof(POINT) * count )))
    {
        WARN("No memory to convert POINTs!\n");
        return FALSE;
    }
    for (i = 0; i < count; i++)
    {
        POINT tmp = pt[i];
        LPtoDP(pdcattr->hdc, &tmp, 1);
        points[i].x = pdcattr->dc_rect.left + tmp.x;
        points[i].y = pdcattr->dc_rect.top + tmp.y;
    }

    /* Call kernel mode */
    RosGdiPolyline(pdcattr->hKernelDC, points, count);

    HeapFree( GetProcessHeap(), 0, points );
    return TRUE;
}

BOOL CDECL RosDrv_Polygon( PDC_ATTR pdcattr, const POINT* pt, INT count )
{
    register int i;
    POINT *points;
    POINT ptBrush;

    if (!(points = HeapAlloc( GetProcessHeap(), 0, sizeof(POINT) * (count+1) )))
    {
        WARN("No memory to convert POINTs!\n");
        return FALSE;
    }
    for (i = 0; i < count; i++)
    {
        POINT tmp = pt[i];
        LPtoDP(pdcattr->hdc, &tmp, 1);
        points[i].x = pdcattr->dc_rect.left + tmp.x;
        points[i].y = pdcattr->dc_rect.top + tmp.y;
    }
    points[count] = points[0];

    /* Update brush origin */
    GetBrushOrgEx(pdcattr->hdc, &ptBrush);
    ptBrush.x += pdcattr->dc_rect.left;
    ptBrush.y += pdcattr->dc_rect.top;
    RosGdiSetBrushOrg(pdcattr->hKernelDC, ptBrush.x, ptBrush.y);

    /* Call kernel mode */
    RosGdiPolygon(pdcattr->hKernelDC, points, count+1);

    HeapFree( GetProcessHeap(), 0, points );
    return TRUE;
}

BOOL CDECL RosDrv_PolyPolygon( PDC_ATTR pdcattr, const POINT* pt, const INT* counts, UINT polygons)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL CDECL RosDrv_PolyPolyline( PDC_ATTR pdcattr, const POINT* pt, const DWORD* counts, DWORD polylines )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL CDECL RosDrv_ExtFloodFill( PDC_ATTR pdcattr, INT x, INT y, COLORREF color,
                     UINT fillType )
{
    POINT ptPixel;

    /* Transform to device coordinates */
    ptPixel.x = x; ptPixel.y = y;

    LPtoDP(pdcattr->hdc, &ptPixel, 1);

    return RosGdiExtFloodFill(pdcattr->hKernelDC, pdcattr->dc_rect.left + ptPixel.x, pdcattr->dc_rect.top + ptPixel.y, color, fillType);
}

COLORREF CDECL RosDrv_SetBkColor( PDC_ATTR pdcattr, COLORREF color )
{
    return RosGdiSetBkColor(pdcattr->hKernelDC, color);
}

COLORREF CDECL RosDrv_SetTextColor( PDC_ATTR pdcattr, COLORREF color )
{
    return RosGdiSetTextColor(pdcattr->hKernelDC, color);
}

BOOL CDECL RosDrv_GetICMProfile( PDC_ATTR pdcattr, LPDWORD size, LPWSTR filename )
{
    UNIMPLEMENTED;
    return FALSE;
}

INT CDECL RosDrv_EnumICMProfiles( PDC_ATTR pdcattr, ICMENUMPROCW proc, LPARAM lparam )
{
    UNIMPLEMENTED;
    return 0;
}

/* EOF */
