/*
 * PROJECT:         ReactOS
 * LICENSE:         LGPL
 * FILE:            dll/win32/winent.drv/gdidrv.c
 * PURPOSE:         GDI driver stub for ReactOS/Windows
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include <winddi.h>
#include <win32k/ntgdityp.h>
#include "ntrosgdi.h"
#include "winent.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rosgdidrv);

/* GLOBALS ****************************************************************/
HANDLE hStockBitmap;

/* FUNCTIONS **************************************************************/

BOOL CDECL RosDrv_AlphaBlend(NTDRV_PDEVICE *physDevDst, INT xDst, INT yDst, INT widthDst, INT heightDst,
                             NTDRV_PDEVICE *physDevSrc, INT xSrc, INT ySrc, INT widthSrc, INT heightSrc,
                             BLENDFUNCTION blendfn)
{
    POINT pts[2], ptBrush;

    /* map source coordinates */
    if (physDevSrc)
    {
        pts[0].x = xSrc;
        pts[0].y = ySrc;
        pts[1].x = xSrc + widthSrc;
        pts[1].y = ySrc + heightSrc;

        LPtoDP(physDevSrc->hUserDC, pts, 2);
        widthSrc = pts[1].x - pts[0].x;
        heightSrc = pts[1].y - pts[0].y;
        xSrc = pts[0].x;
        ySrc = pts[0].y;
    }

    /* map dest coordinates */
    pts[0].x = xDst;
    pts[0].y = yDst;
    pts[1].x = xDst + widthDst;
    pts[1].y = yDst + heightDst;

    LPtoDP(physDevDst->hUserDC, pts, 2);
    widthDst = pts[1].x - pts[0].x;
    heightDst = pts[1].y - pts[0].y;
    xDst = pts[0].x;
    yDst = pts[0].y;

    /* Update brush origin */
    GetBrushOrgEx(physDevDst->hUserDC, &ptBrush);
    RosGdiSetBrushOrg(physDevDst->hKernelDC, ptBrush.x, ptBrush.y);

    return RosGdiAlphaBlend(physDevDst->hKernelDC, xDst, yDst, widthDst, heightDst,
        physDevSrc->hKernelDC, xSrc, ySrc, widthSrc, heightSrc, blendfn);

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

BOOL CDECL RosDrv_BitBlt( NTDRV_PDEVICE *physDevDst, INT xDst, INT yDst,
                    INT width, INT height, NTDRV_PDEVICE *physDevSrc,
                    INT xSrc, INT ySrc, DWORD rop )
{
    POINT pts[2], ptBrush;

    /* map source coordinates */
    if (physDevSrc)
    {
        pts[0].x = xSrc;
        pts[0].y = ySrc;
        pts[1].x = xSrc + width;
        pts[1].y = ySrc + height;

        LPtoDP(physDevSrc->hUserDC, pts, 2);
        width = pts[1].x - pts[0].x;
        height = pts[1].y - pts[0].y;
        xSrc = pts[0].x;
        ySrc = pts[0].y;
    }

    /* map dest coordinates */
    pts[0].x = xDst;
    pts[0].y = yDst;
    LPtoDP(physDevDst->hUserDC, pts, 1);
    xDst = pts[0].x;
    yDst = pts[0].y;

    /* Update brush origin */
    GetBrushOrgEx(physDevDst->hUserDC, &ptBrush);
    RosGdiSetBrushOrg(physDevDst->hKernelDC, ptBrush.x, ptBrush.y);

    //FIXME("xDst %d, yDst %d, widthDst %d, heightDst %d, src x %d y %d\n",
    //    xDst, yDst, width, height, xSrc, ySrc);

    return RosGdiBitBlt(physDevDst->hKernelDC, xDst, yDst, width, height,
        physDevSrc->hKernelDC, xSrc, ySrc, rop);
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

BOOL CDECL RosDrv_CreateBitmap( NTDRV_PDEVICE *physDev, HBITMAP hbitmap, LPVOID bmBits )
{
    BITMAP bitmap;

    /* Get the usermode object */
    if (!GetObjectW(hbitmap, sizeof(bitmap), &bitmap)) return FALSE;

    /* Check parameters */
    if (bitmap.bmPlanes != 1) return FALSE;

    /* Create the kernelmode bitmap object */
    return RosGdiCreateBitmap(physDev->hKernelDC, hbitmap, &bitmap, bmBits);
}

BOOL CDECL RosDrv_CreateDC( HDC hdc, NTDRV_PDEVICE **pdev, LPCWSTR driver, LPCWSTR device,
                            LPCWSTR output, const DEVMODEW* initData )
{
    BOOL bRet;
    double scaleX, scaleY;
    ROS_DCINFO dcInfo = {0};
    NTDRV_PDEVICE *physDev;
    HDC hKernelDC;

    /* Allocate memory for two handles */
    physDev = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*physDev) );
    if (!physDev) return FALSE;

    /* Fill in internal DCINFO structure */
    dcInfo.dwType = GetObjectType(hdc);
    GetWorldTransform(hdc, &dcInfo.xfWorld2Wnd);
    GetViewportExtEx(hdc, &dcInfo.szVportExt);
    GetViewportOrgEx(hdc, &dcInfo.ptVportOrg);
    GetWindowExtEx(hdc, &dcInfo.szWndExt);
    GetWindowOrgEx(hdc, &dcInfo.ptWndOrg);

    /* Calculate xfWnd2Vport */
    scaleX = (double)dcInfo.szVportExt.cx / (double)dcInfo.szWndExt.cx;
    scaleY = (double)dcInfo.szVportExt.cy / (double)dcInfo.szWndExt.cy;
    dcInfo.xfWnd2Vport.eM11 = scaleX;
    dcInfo.xfWnd2Vport.eM12 = 0.0;
    dcInfo.xfWnd2Vport.eM21 = 0.0;
    dcInfo.xfWnd2Vport.eM22 = scaleY;
    dcInfo.xfWnd2Vport.eDx  = (double)dcInfo.ptVportOrg.x -
        scaleX * (double)dcInfo.ptWndOrg.x;
    dcInfo.xfWnd2Vport.eDy  = (double)dcInfo.ptVportOrg.y -
        scaleY * (double)dcInfo.ptWndOrg.y;

     /* The following part is done in kernel mode */
#if 0
    /* Combine with the world transformation */
    CombineTransform( &dc->xformWorld2Vport, &dc->xformWorld2Wnd,
        &xformWnd2Vport );

    /* Create inverse of world-to-viewport transformation */
    dc->vport2WorldValid = DC_InvertXform( &dc->xformWorld2Vport,
        &dc->xformVport2World );
#endif

    /* Save DC handle if it's a compatible one or set it to NULL for
       a display DC */
    if (*pdev)
        hKernelDC = (*pdev)->hKernelDC;
    else
        hKernelDC = 0;

    /* Save stock bitmap's handle */
    if (dcInfo.dwType == OBJ_MEMDC && !hStockBitmap)
        hStockBitmap = GetCurrentObject( hdc, OBJ_BITMAP );

    /* Call the win32 kernel */
    bRet = RosGdiCreateDC(&dcInfo, &hKernelDC, driver, device, output, initData);

    /* Save newly created DC */
    physDev->hKernelDC = hKernelDC;
    physDev->hUserDC = hdc;

    /* No font is selected */
    physDev->cache_index = -1;

    /* Create a usermode clipping region (same as kernelmode one, just to reduce
       amount of syscalls) */
    physDev->region = CreateRectRgn( 0, 0, 0, 0 );

    /* Return allocated physical DC to the caller */
    *pdev = physDev;

    return bRet;
}

HBITMAP CDECL RosDrv_CreateDIBSection( NTDRV_PDEVICE *physDev, HBITMAP hbitmap,
                                       const BITMAPINFO *bmi, UINT usage )
{
    DIBSECTION dib;
    LONG height, width;
    WORD infoBpp, compression;

    GetObjectW( hbitmap, sizeof(dib), &dib );

    /* Get parameters to check if it's topdown or not.
       GetObject doesn't return this info */
    DIB_GetBitmapInfo(&bmi->bmiHeader, &width, &height, &infoBpp, &compression);

    // TODO: Should pass as a flag instead
    if (height < 0) dib.dsBmih.biHeight *= -1;

    return RosGdiCreateDIBSection(physDev->hKernelDC, hbitmap, bmi, usage, &dib);
}

BOOL CDECL RosDrv_DeleteBitmap( HBITMAP hbitmap )
{
    return RosGdiDeleteBitmap(hbitmap);
}

BOOL CDECL RosDrv_DeleteDC( NTDRV_PDEVICE *physDev )
{
    BOOL res;

    /* Delete usermode copy of a clipping region */
    DeleteObject( physDev->region );

    /* Delete kernel DC */
    res = RosGdiDeleteDC(physDev->hKernelDC);

    /* Free the um/km handle pair memory */
    HeapFree( GetProcessHeap(), 0, physDev );

    /* Return result */
    return res;
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

BOOL CDECL RosDrv_EnumDeviceFonts( NTDRV_PDEVICE *physDev, LPLOGFONTW plf,
                                   FONTENUMPROCW proc, LPARAM lp )
{
    /* We're always using client-side fonts. */
    return FALSE;
}

INT CDECL RosDrv_ExtEscape( NTDRV_PDEVICE *physDev, INT escape, INT in_count, LPCVOID in_data,
                            INT out_count, LPVOID out_data )
{
    HWND hwnd;

    switch(escape)
    {
    case NTDRV_ESCAPE:
        if (in_data && in_count >= sizeof(enum ntdrv_escape_codes))
        {
            switch(*(const enum ntdrv_escape_codes *)in_data)
            {
            case NTDRV_SET_DRAWABLE:
                if (in_count >= sizeof(struct ntdrv_escape_set_drawable))
                {
                    const struct ntdrv_escape_set_drawable *data = in_data;

                    RosGdiSetDcRects(physDev->hKernelDC, (RECT*)&data->dc_rect, (RECT*)&data->drawable_rect);

                    hwnd = GetAncestor(data->hwnd, GA_ROOT);

                    if (!data->release)
                        RosGdiGetDC(physDev->hKernelDC, hwnd, data->clip_children);
                    else
                        RosGdiReleaseDC(physDev->hKernelDC);

                    TRACE( "SET_DRAWABLE hdc %p dc_rect %s drawable_rect %s\n",
                           physDev->hUserDC, wine_dbgstr_rect(&data->dc_rect), wine_dbgstr_rect(&data->drawable_rect) );
                    return TRUE;
                }
                break;
            default:
                ERR("ExtEscape NTDRV_ESCAPE case %d is unimplemented!\n", *((DWORD *)in_data));
            }
        }
    default:
        ERR("ExtEscape for escape %d is unimplemented!\n", escape);
    }

    return 0;
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

BOOL CDECL RosDrv_ExtTextOut( NTDRV_PDEVICE *physDev, INT x, INT y, UINT flags,
                   const RECT *lprect, LPCWSTR wstr, UINT count,
                   const INT *lpDx )
{
    //if (physDev->has_gdi_font)
        return FeTextOut(physDev, x, y, flags, lprect, wstr, count, lpDx);

    //UNIMPLEMENTED;
    //return FALSE;
}

LONG CDECL RosDrv_GetBitmapBits( HBITMAP hbitmap, void *buffer, LONG count )
{
    return RosGdiGetBitmapBits(hbitmap, buffer, count);
}

BOOL CDECL RosDrv_GetCharWidth( NTDRV_PDEVICE *physDev, UINT firstChar, UINT lastChar,
                                  LPINT buffer )
{
    UNIMPLEMENTED;
    return FALSE;
}

INT CDECL RosDrv_GetDIBits( NTDRV_PDEVICE *physDev, HBITMAP hbitmap, UINT startscan, UINT lines,
                            LPVOID bits, BITMAPINFO *info, UINT coloruse )
{
    size_t obj_size;
    DIBSECTION dib;

    /* Check if this bitmap has a DIB section */
    if (!(obj_size = GetObjectW( hbitmap, sizeof(dib), &dib ))) return 0;

    /* Perform GetDIBits */
    return RosGdiGetDIBits(physDev->hKernelDC, hbitmap, startscan, lines, bits, info, coloruse, &dib);
}

INT CDECL RosDrv_GetDeviceCaps( NTDRV_PDEVICE *physDev, INT cap )
{
    return RosGdiGetDeviceCaps(physDev->hKernelDC, cap);
}

BOOL CDECL RosDrv_GetDeviceGammaRamp(NTDRV_PDEVICE *physDev, LPVOID ramp)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL CDECL RosDrv_GetICMProfile( NTDRV_PDEVICE *physDev, LPDWORD size, LPWSTR filename )
{
    UNIMPLEMENTED;
    return FALSE;
}

COLORREF CDECL RosDrv_GetNearestColor( NTDRV_PDEVICE *physDev, COLORREF color )
{
    UNIMPLEMENTED;
    return 0;
}

COLORREF CDECL RosDrv_GetPixel( NTDRV_PDEVICE *physDev, INT x, INT y )
{
    POINT ptPixel;

    /* Transform to device coordinates */
    ptPixel.x = x; ptPixel.y = y;
    LPtoDP(physDev->hUserDC, &ptPixel, 1);

    return RosGdiGetPixel(physDev->hKernelDC, ptPixel.x, ptPixel.y);
}

UINT CDECL RosDrv_GetSystemPaletteEntries( NTDRV_PDEVICE *physDev, UINT start, UINT count,
                                           LPPALETTEENTRY entries )
{
    return RosGdiGetSystemPaletteEntries(physDev->hKernelDC, start, count, entries);
}

BOOL CDECL RosDrv_GetTextExtentExPoint( NTDRV_PDEVICE *physDev, LPCWSTR str, INT count,
                                        INT maxExt, LPINT lpnFit, LPINT alpDx, LPSIZE size )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL CDECL RosDrv_GetTextMetrics(NTDRV_PDEVICE *physDev, TEXTMETRICW *metrics)
{
    /* Let GDI font engine do the work */
    return FALSE;
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

BOOL CDECL RosDrv_PaintRgn( NTDRV_PDEVICE *physDev, HRGN hrgn )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL CDECL RosDrv_PatBlt( NTDRV_PDEVICE *physDev, INT left, INT top, INT width, INT height, DWORD rop )
{
    POINT pts[2], ptBrush;

    /* Map coordinates */
    pts[0].x = left;
    pts[0].y = top;
    pts[1].x = left + width;
    pts[1].y = top + height;

    LPtoDP(physDev->hUserDC, pts, 2);
    width = pts[1].x - pts[0].x;
    height = pts[1].y - pts[0].y;
    left = pts[0].x;
    top = pts[0].y;

    /* Update brush origin */
    GetBrushOrgEx(physDev->hUserDC, &ptBrush);
    RosGdiSetBrushOrg(physDev->hKernelDC, ptBrush.x, ptBrush.y);

    return RosGdiPatBlt(physDev->hKernelDC, left, top, width, height, rop);
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

UINT CDECL RosDrv_RealizeDefaultPalette( NTDRV_PDEVICE *physDev )
{
    //UNIMPLEMENTED;
    return FALSE;
}

UINT CDECL RosDrv_RealizePalette( NTDRV_PDEVICE *physDev, HPALETTE hpal, BOOL primary )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL CDECL RosDrv_Rectangle(NTDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom)
{
    POINT ptBrush;
    RECT rc;

    /* Convert coordinates */
    SetRect(&rc, left, top, right, bottom);
    LPtoDP(physDev->hUserDC, (POINT*)&rc, 2);

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

HBITMAP CDECL RosDrv_SelectBitmap( NTDRV_PDEVICE *physDev, HBITMAP hbitmap )
{
    BOOL bRes, bStock = FALSE;

    /* Check if it's a stock bitmap */
    if (hbitmap == hStockBitmap) bStock = TRUE;

    /* Select the bitmap into the DC */
    bRes = RosGdiSelectBitmap(physDev->hKernelDC, hbitmap, bStock);

    /* If there was an error, return 0 */
    if (!bRes) return 0;

    /* Return handle of selected bitmap as a success*/
    return hbitmap;
}

HBRUSH CDECL RosDrv_SelectBrush( NTDRV_PDEVICE *physDev, HBRUSH hbrush )
{
    LOGBRUSH logbrush;

    if (!GetObjectA( hbrush, sizeof(logbrush), &logbrush )) return 0;

    RosGdiSelectBrush(physDev->hKernelDC, &logbrush);

    return hbrush;
}

HFONT CDECL RosDrv_SelectFont( NTDRV_PDEVICE *physDev, HFONT hfont, HANDLE gdiFont )
{
    /* We don't have a kernelmode font engine */
    if (gdiFont == 0)
    {
        /*RosGdiSelectFont(physDev->hKernelDC, hfont, gdiFont);*/
    }
    else
    {
        /* Save information about the selected font */
        FeSelectFont(physDev, hfont);
    }

    /* Indicate that gdiFont is good to use */
    return 0;
}

HPEN CDECL RosDrv_SelectPen( NTDRV_PDEVICE *physDev, HPEN hpen )
{
    LOGPEN logpen;
    EXTLOGPEN *elogpen = NULL;
    INT size;

    /* Try to get LOGPEN */
    if (!GetObjectW( hpen, sizeof(logpen), &logpen ))
    {
        /* It may be an ext pen, get its size */
        size = GetObjectW( hpen, 0, NULL );
        if (!size) return 0;

        elogpen = HeapAlloc( GetProcessHeap(), 0, size );

        GetObjectW( hpen, size, elogpen );
    }

    /* If it's a stock object, then use DC's color */
    if (hpen == GetStockObject( DC_PEN ))
        logpen.lopnColor = GetDCPenColor(physDev->hUserDC);

    /* Call kernelmode */
    RosGdiSelectPen(physDev->hKernelDC, &logpen, elogpen);

    /* Free ext logpen memory if it was allocated */
    if (elogpen) HeapFree( GetProcessHeap(), 0, elogpen );

    /* Return success */
    return hpen;
}

LONG CDECL RosDrv_SetBitmapBits( HBITMAP hbitmap, const void *bits, LONG count )
{
    UNIMPLEMENTED;
    return 0;
}

COLORREF CDECL RosDrv_SetBkColor( NTDRV_PDEVICE *physDev, COLORREF color )
{
    return RosGdiSetBkColor(physDev->hKernelDC, color);
}

COLORREF CDECL RosDrv_SetDCBrushColor( NTDRV_PDEVICE *physDev, COLORREF crColor )
{
    UNIMPLEMENTED;
    return 0;
}

COLORREF CDECL RosDrv_SetDCPenColor( NTDRV_PDEVICE *physDev, COLORREF crColor )
{
    UNIMPLEMENTED;
    return 0;
}

UINT CDECL RosDrv_SetDIBColorTable( NTDRV_PDEVICE *physDev, UINT start, UINT count, const RGBQUAD *colors )
{
    return RosGdiSetDIBColorTable(physDev->hKernelDC, start, count, colors);
}

INT CDECL RosDrv_SetDIBits( NTDRV_PDEVICE *physDev, HBITMAP hbitmap, UINT startscan,
                            UINT lines, LPCVOID bits, const BITMAPINFO *info, UINT coloruse )
{
    LONG height, width, tmpheight;
    WORD infoBpp, compression;

    /* Perform extensive parameter checking */
    if (DIB_GetBitmapInfo( &info->bmiHeader, &width, &height,
        &infoBpp, &compression ) == -1)
        return 0;

    tmpheight = height;
    if (height < 0) height = -height;
    if (!lines || (startscan >= height))
        return 0;

    if (startscan + lines > height) lines = height - startscan;

    return RosGdiSetDIBits(physDev->hKernelDC, hbitmap, startscan,
        tmpheight >= 0 ? lines : -lines, bits, info, coloruse);
}

INT CDECL RosDrv_SetDIBitsToDevice( NTDRV_PDEVICE *physDev, INT xDest, INT yDest, DWORD cx,
                                    DWORD cy, INT xSrc, INT ySrc,
                                    UINT startscan, UINT lines, LPCVOID bits,
                                    const BITMAPINFO *info, UINT coloruse )
{
    BOOL top_down;
    LONG height, width;
    WORD infoBpp, compression;
    POINT pt;

    pt.x = xDest; pt.y = yDest;
    LPtoDP(physDev->hUserDC, &pt, 1);

    /* Perform extensive parameter checking */
    if (DIB_GetBitmapInfo( &info->bmiHeader, &width, &height,
        &infoBpp, &compression ) == -1)
        return 0;

    top_down = (height < 0);
    if (top_down) height = -height;

    if (!lines || (startscan >= height)) return 0;
    if (!top_down && startscan + lines > height) lines = height - startscan;

    /* make xSrc,ySrc point to the upper-left corner, not the lower-left one,
     * and clamp all values to fit inside [startscan,startscan+lines]
     */
    if (ySrc + cy <= startscan + lines)
    {
        UINT y = startscan + lines - (ySrc + cy);
        if (ySrc < startscan) cy -= (startscan - ySrc);
        if (!top_down)
        {
            /* avoid getting unnecessary lines */
            ySrc = 0;
            if (y >= lines) return 0;
            lines -= y;
        }
        else
        {
            if (y >= lines) return lines;
            ySrc = y;  /* need to get all lines in top down mode */
        }
    }
    else
    {
        if (ySrc >= startscan + lines) return lines;
        pt.y += ySrc + cy - (startscan + lines);
        cy = startscan + lines - ySrc;
        ySrc = 0;
        if (cy > lines) cy = lines;
    }
    if (xSrc >= width) return lines;
    if (xSrc + cx >= width) cx = width - xSrc;
    if (!cx || !cy) return lines;

    return RosGdiSetDIBitsToDevice(physDev->hKernelDC, pt.x, pt.y, cx, cy,
        xSrc, ySrc, startscan, top_down ? -lines : lines, bits, info, coloruse);
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

BOOL CDECL RosDrv_SetDeviceGammaRamp(NTDRV_PDEVICE *physDev, LPVOID ramp)
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

COLORREF CDECL RosDrv_SetTextColor( NTDRV_PDEVICE *physDev, COLORREF color )
{
    return RosGdiSetTextColor(physDev->hKernelDC, color);
}

BOOL CDECL RosDrv_StretchBlt( NTDRV_PDEVICE *physDevDst, INT xDst, INT yDst,
                              INT widthDst, INT heightDst,
                              NTDRV_PDEVICE *physDevSrc, INT xSrc, INT ySrc,
                              INT widthSrc, INT heightSrc, DWORD rop )
{
    POINT pts[2], ptBrush;

    /* map source coordinates */
    if (physDevSrc)
    {
        pts[0].x = xSrc;
        pts[0].y = ySrc;
        pts[1].x = xSrc + widthSrc;
        pts[1].y = ySrc + heightSrc;

        LPtoDP(physDevSrc->hUserDC, pts, 2);
        widthSrc = pts[1].x - pts[0].x;
        heightSrc = pts[1].y - pts[0].y;
        xSrc = pts[0].x;
        ySrc = pts[0].y;
    }

    /* map dest coordinates */
    pts[0].x = xDst;
    pts[0].y = yDst;
    pts[1].x = xDst + widthDst;
    pts[1].y = yDst + heightDst;
    LPtoDP(physDevDst->hUserDC, pts, 2);
    widthDst = pts[1].x - pts[0].x;
    heightDst = pts[1].y - pts[0].y;
    xDst = pts[0].x;
    yDst = pts[0].y;

    /* Update brush origin */
    GetBrushOrgEx(physDevDst->hUserDC, &ptBrush);
    RosGdiSetBrushOrg(physDevDst->hKernelDC, ptBrush.x, ptBrush.y);

    return RosGdiStretchBlt(physDevDst->hKernelDC, xDst, yDst, widthDst, heightDst,
        physDevSrc->hKernelDC, xSrc, ySrc, widthSrc, heightSrc, rop);
}

BOOL CDECL RosDrv_UnrealizePalette( HPALETTE hpal )
{
    UNIMPLEMENTED;
    return FALSE;
}

/***********************************************************************
 *           DIB_GetBitmapInfoEx
 *
 * Get the info from a bitmap header.
 * Return 1 for INFOHEADER, 0 for COREHEADER, -1 for error.
 */
int DIB_GetBitmapInfoEx( const BITMAPINFOHEADER *header, LONG *width,
                                LONG *height, WORD *planes, WORD *bpp,
                                WORD *compr, DWORD *size )
{
    if (header->biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *)header;
        *width  = core->bcWidth;
        *height = core->bcHeight;
        *planes = core->bcPlanes;
        *bpp    = core->bcBitCount;
        *compr  = 0;
        *size   = 0;
        return 0;
    }
    if (header->biSize >= sizeof(BITMAPINFOHEADER))
    {
        *width  = header->biWidth;
        *height = header->biHeight;
        *planes = header->biPlanes;
        *bpp    = header->biBitCount;
        *compr  = header->biCompression;
        *size   = header->biSizeImage;
        return 1;
    }
    ERR("(%d): unknown/wrong size for header\n", header->biSize );
    return -1;
}

/***********************************************************************
 *           DIB_GetBitmapInfo
 *
 * Get the info from a bitmap header.
 * Return 1 for INFOHEADER, 0 for COREHEADER, -1 for error.
 */
int DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, LONG *width,
                              LONG *height, WORD *bpp, WORD *compr )
{
    WORD planes;
    DWORD size;

    return DIB_GetBitmapInfoEx( header, width, height, &planes, bpp, compr, &size);    
}

/* EOF */
