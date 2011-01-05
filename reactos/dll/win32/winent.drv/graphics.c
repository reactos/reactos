/*
 * PROJECT:         ReactOS
 * LICENSE:         GNU LGPL by FSF v2.1 or any later
 * FILE:            dll/win32/winent.drv/graphics.c
 * PURPOSE:         GDI high-level driver for ReactOS/Windows
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
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

void CDECL RosDrv_SetDeviceClipping( NTDRV_PDEVICE *physDev, HRGN vis_rgn, HRGN clip_rgn )
{
    RGNDATA *data;
    DWORD size;

    //FIXME("SetDeviceClipping hdc %x\n", physDev->hUserDC);

    /* Update dc region to become a combined region */
    CombineRgn( physDev->region, vis_rgn, clip_rgn, clip_rgn ? RGN_AND : RGN_COPY );

    /* Get region data size */
    if (!(size = GetRegionData( physDev->region, 0, NULL )))
        return;

    /* Allocate memory for it */
    if (!(data = HeapAlloc( GetProcessHeap(), 0, size )))
        return;

    /* Get region data */
    if (!GetRegionData( physDev->region, size, data ))
    {
        HeapFree( GetProcessHeap(), 0, data );
        return;
    }

    /* Set clipping */
    RosGdiSetDeviceClipping(physDev->hKernelDC, data->rdh.nCount, (RECTL *)data->Buffer, (RECTL *)&data->rdh.rcBound);

    /* Free memory and delete clipping region */
    HeapFree( GetProcessHeap(), 0, data );
}


/***********************************************************************
 *           RosDrv_XWStoDS
 *
 * Performs a world-to-viewport transformation on the specified width.
 */
INT RosDrv_XWStoDS( NTDRV_PDEVICE *physDev, INT width )
{
    POINT pt[2];

    pt[0].x = 0;
    pt[0].y = 0;
    pt[1].x = width;
    pt[1].y = 0;
    LPtoDP( physDev->hUserDC, pt, 2 );
    return pt[1].x - pt[0].x;
}

/***********************************************************************
 *           RosDrv_YWStoDS
 *
 * Performs a world-to-viewport transformation on the specified height.
 */
INT RosDrv_YWStoDS( NTDRV_PDEVICE *physDev, INT height )
{
    POINT pt[2];

    pt[0].x = 0;
    pt[0].y = 0;
    pt[1].x = 0;
    pt[1].y = height;
    LPtoDP( physDev->hUserDC, pt, 2 );
    return pt[1].y - pt[0].y;
}

BOOL CDECL RosDrv_LineTo( NTDRV_PDEVICE *physDev, INT x, INT y )
{
    POINT pt[2];

    /* Get current cursor position */
    GetCurrentPositionEx( physDev->hUserDC, &pt[0] );

    /* Convert both points coordinates to device */
    pt[1].x = x;
    pt[1].y = y;
    LPtoDP( physDev->hUserDC, pt, 2 );

    /* Draw the line */
    return RosGdiLineTo(physDev->hKernelDC, pt[0].x, pt[0].y, pt[1].x, pt[1].y);
}

BOOL CDECL RosDrv_Arc( NTDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom,
            INT xstart, INT ystart, INT xend, INT yend )
{
    POINT pts[4];

    /* Map coordinates */
    pts[0].x = left;
    pts[0].y = top;
    pts[1].x = right;
    pts[1].y = bottom;
    pts[2].x = xstart;
    pts[2].y = ystart;
    pts[3].x = xend;
    pts[3].y = yend;

    LPtoDP(physDev->hUserDC, pts, 4);

    return RosGdiArc(physDev->hKernelDC, pts[0].x, pts[0].y, pts[1].x, pts[1].y,
        pts[2].x, pts[2].y, pts[3].x, pts[3].y, GdiTypeArc);
}

BOOL CDECL RosDrv_Pie( NTDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom,
            INT xstart, INT ystart, INT xend, INT yend )
{
    POINT pts[4];

    /* Map coordinates */
    pts[0].x = left;
    pts[0].y = top;
    pts[1].x = right;
    pts[1].y = bottom;
    pts[2].x = xstart;
    pts[2].y = ystart;
    pts[3].x = xend;
    pts[3].y = yend;

    LPtoDP(physDev->hUserDC, pts, 4);

    return RosGdiArc(physDev->hKernelDC, pts[0].x, pts[0].y, pts[1].x, pts[1].y,
        pts[2].x, pts[2].y, pts[3].x, pts[3].y, GdiTypePie);
}

BOOL CDECL RosDrv_Chord( NTDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom,
              INT xstart, INT ystart, INT xend, INT yend )
{
    POINT pts[4];

    /* Map coordinates */
    pts[0].x = left;
    pts[0].y = top;
    pts[1].x = right;
    pts[1].y = bottom;
    pts[2].x = xstart;
    pts[2].y = ystart;
    pts[3].x = xend;
    pts[3].y = yend;

    LPtoDP(physDev->hUserDC, pts, 4);

    return RosGdiArc(physDev->hKernelDC, pts[0].x, pts[0].y, pts[1].x, pts[1].y,
        pts[2].x, pts[2].y, pts[3].x, pts[3].y, GdiTypeChord );
}

BOOL CDECL RosDrv_Ellipse( NTDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom )
{
    POINT pts[2];

    /* Map coordinates */
    pts[0].x = left;
    pts[0].y = top;
    pts[1].x = right;
    pts[1].y = bottom;

    LPtoDP(physDev->hUserDC, pts, 2);

    return RosGdiEllipse(physDev->hKernelDC, pts[0].x, pts[0].y, pts[1].x, pts[1].y);
}

BOOL CDECL RosDrv_Rectangle(NTDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom)
{
    POINT ptBrush;
    RECT rc = get_device_rect( physDev->hUserDC, left, top, right, bottom );

    if ((rc.left == rc.right) || (rc.top == rc.bottom)) return TRUE;

    if (rc.right < rc.left) { INT tmp = rc.right; rc.right = rc.left; rc.left = tmp; }
    if (rc.bottom < rc.top) { INT tmp = rc.bottom; rc.bottom = rc.top; rc.top = tmp; }

    /* Update brush origin */
    GetBrushOrgEx(physDev->hUserDC, &ptBrush);
    RosGdiSetBrushOrg(physDev->hKernelDC, ptBrush.x, ptBrush.y);

    RosGdiRectangle(physDev->hKernelDC, &rc);

    return TRUE;
}

BOOL CDECL RosDrv_RoundRect( NTDRV_PDEVICE *physDev, INT left, INT top, INT right,
                  INT bottom, INT ell_width, INT ell_height )
{
    UNIMPLEMENTED;
    return FALSE;
}

COLORREF CDECL RosDrv_SetPixel( NTDRV_PDEVICE *physDev, INT x, INT y, COLORREF color )
{
    POINT ptPixel;

    /* Transform to device coordinates */
    ptPixel.x = x; ptPixel.y = y;
    LPtoDP(physDev->hUserDC, &ptPixel, 1);

    return RosGdiSetPixel(physDev->hKernelDC, ptPixel.x, ptPixel.y, color);
}

COLORREF CDECL RosDrv_GetPixel( NTDRV_PDEVICE *physDev, INT x, INT y )
{
    POINT ptPixel;

    /* Transform to device coordinates */
    ptPixel.x = x; ptPixel.y = y;
    LPtoDP(physDev->hUserDC, &ptPixel, 1);

    return RosGdiGetPixel(physDev->hKernelDC, ptPixel.x, ptPixel.y);
}

BOOL CDECL RosDrv_PaintRgn( NTDRV_PDEVICE *physDev, HRGN hrgn )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL CDECL RosDrv_Polyline( NTDRV_PDEVICE *physDev, const POINT* pt, INT count )
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
        LPtoDP(physDev->hUserDC, &tmp, 1);
        points[i].x = tmp.x;
        points[i].y = tmp.y;
    }

    /* Call kernel mode */
    RosGdiPolyline(physDev->hKernelDC, points, count);

    HeapFree( GetProcessHeap(), 0, points );
    return TRUE;
}

BOOL CDECL RosDrv_Polygon( NTDRV_PDEVICE *physDev, const POINT* pt, INT count )
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
        LPtoDP(physDev->hUserDC, &tmp, 1);
        points[i].x = tmp.x;
        points[i].y = tmp.y;
    }
    points[count] = points[0];

    /* Update brush origin */
    GetBrushOrgEx(physDev->hUserDC, &ptBrush);
    RosGdiSetBrushOrg(physDev->hKernelDC, ptBrush.x, ptBrush.y);

    /* Call kernel mode */
    RosGdiPolygon(physDev->hKernelDC, points, count+1);

    HeapFree( GetProcessHeap(), 0, points );
    return TRUE;
}

BOOL CDECL RosDrv_PolyPolygon( NTDRV_PDEVICE *physDev, const POINT* pt, const INT* counts, UINT polygons)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL CDECL RosDrv_PolyPolyline( NTDRV_PDEVICE *physDev, const POINT* pt, const DWORD* counts, DWORD polylines )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL CDECL RosDrv_ExtFloodFill( NTDRV_PDEVICE *physDev, INT x, INT y, COLORREF color,
                     UINT fillType )
{
    POINT ptPixel;

    /* Transform to device coordinates */
    ptPixel.x = x; ptPixel.y = y;

    LPtoDP(physDev->hUserDC, &ptPixel, 1);

    return RosGdiExtFloodFill(physDev->hKernelDC, ptPixel.x, ptPixel.y, color, fillType);
}

COLORREF CDECL RosDrv_SetBkColor( NTDRV_PDEVICE *physDev, COLORREF color )
{
    return RosGdiSetBkColor(physDev->hKernelDC, color);
}

COLORREF CDECL RosDrv_SetTextColor( NTDRV_PDEVICE *physDev, COLORREF color )
{
    return RosGdiSetTextColor(physDev->hKernelDC, color);
}

BOOL CDECL RosDrv_GetICMProfile( NTDRV_PDEVICE *physDev, LPDWORD size, LPWSTR filename )
{
    UNIMPLEMENTED;
    return FALSE;
}

INT CDECL RosDrv_EnumICMProfiles( NTDRV_PDEVICE *physDev, ICMENUMPROCW proc, LPARAM lparam )
{
    UNIMPLEMENTED;
    return 0;
}

/* EOF */
