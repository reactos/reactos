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
#include "ntrosgdi.h"
#include "winent.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rosgdidrv);

/* FUNCTIONS **************************************************************/

BOOL CDECL RosDrv_AlphaBlend(NTDRV_PDEVICE *devDst, INT xDst, INT yDst, INT widthDst, INT heightDst,
                             NTDRV_PDEVICE *devSrc, INT xSrc, INT ySrc, INT widthSrc, INT heightSrc,
                             BLENDFUNCTION blendfn)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL CDECL RosDrv_Arc( NTDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom,
            INT xstart, INT ystart, INT xend, INT yend )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL CDECL RosDrv_BitBlt( NTDRV_PDEVICE *physDevDst, INT xDst, INT yDst,
                    INT width, INT height, NTDRV_PDEVICE *physDevSrc,
                    INT xSrc, INT ySrc, DWORD rop )
{
    return RosGdiBitBlt(physDevDst->hKernelDC, xDst, yDst, width, height,
        physDevSrc->hKernelDC, xSrc, ySrc, rop);
}

int CDECL RosDrv_ChoosePixelFormat(NTDRV_PDEVICE *physDev,
                                   const PIXELFORMATDESCRIPTOR *ppfd)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL CDECL RosDrv_Chord( NTDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom,
              INT xstart, INT ystart, INT xend, INT yend )
{
    UNIMPLEMENTED;
    return FALSE;
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

    /* Call the win32 kernel */
    bRet = RosGdiCreateDC(&dcInfo, &hKernelDC, driver, device, output, initData);

    /* Save newly created DC */
    physDev->hKernelDC = hKernelDC;
    physDev->hUserDC = hdc;

    /* No font is selected */
    physDev->cache_index = -1;

    /* Return allocated physical DC to the caller */
    *pdev = physDev;

    return bRet;
}

HBITMAP CDECL RosDrv_CreateDIBSection( NTDRV_PDEVICE *physDev, HBITMAP hbitmap,
                                       const BITMAPINFO *bmi, UINT usage )
{
    UNIMPLEMENTED;
    return 0;
}

BOOL CDECL RosDrv_DeleteBitmap( HBITMAP hbitmap )
{
    return RosGdiDeleteBitmap(hbitmap);
}

BOOL CDECL RosDrv_DeleteDC( NTDRV_PDEVICE *physDev )
{
    BOOL res;

    /* Delete kernel DC */
    res = RosGdiDeleteDC(physDev->hKernelDC);

    /* Free the um/km handle pair memory */
    HeapFree( GetProcessHeap(), 0, physDev );

    /* Return result */
    return res;
}

int CDECL RosDrv_DescribePixelFormat(NTDRV_PDEVICE *physDev,
			       int iPixelFormat,
			       UINT nBytes,
			       PIXELFORMATDESCRIPTOR *ppfd)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL CDECL RosDrv_Ellipse( NTDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL CDECL RosDrv_EnumDeviceFonts( NTDRV_PDEVICE *physDev, LPLOGFONTW plf,
                                   FONTENUMPROCW proc, LPARAM lp )
{
    UNIMPLEMENTED;
    return FALSE;
}

INT CDECL RosDrv_ExtEscape( NTDRV_PDEVICE *physDev, INT escape, INT in_count, LPCVOID in_data,
                            INT out_count, LPVOID out_data )
{
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
                    RosGdiSetDcRects(physDev->hKernelDC, &data->dc_rect, &data->drawable_rect);
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
    UNIMPLEMENTED;
    return FALSE;
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

BOOL CDECL RosDrv_GetDCOrgEx( NTDRV_PDEVICE *physDev, LPPOINT lpp )
{
    UNIMPLEMENTED;
    return FALSE;
}

INT CDECL RosDrv_GetDIBits( NTDRV_PDEVICE *physDev, HBITMAP hbitmap, UINT startscan, UINT lines,
                            LPVOID bits, BITMAPINFO *info, UINT coloruse )
{
    UNIMPLEMENTED;
    return 0;
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
    UNIMPLEMENTED;
    return 0;
}

int CDECL RosDrv_GetPixelFormat(NTDRV_PDEVICE *physDev)
{
    UNIMPLEMENTED;
    return 0;
}

UINT CDECL RosDrv_GetSystemPaletteEntries( NTDRV_PDEVICE *physDev, UINT start, UINT count,
                                     LPPALETTEENTRY entries )
{
    UNIMPLEMENTED;
    return 0;
}

BOOL CDECL RosDrv_GetTextExtentExPoint( NTDRV_PDEVICE *physDev, LPCWSTR str, INT count,
                                        INT maxExt, LPINT lpnFit, LPINT alpDx, LPSIZE size )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL CDECL RosDrv_GetTextMetrics(NTDRV_PDEVICE *physDev, TEXTMETRICW *metrics)
{
    UNIMPLEMENTED;

    // HACK
    metrics->tmMaxCharWidth = 9;
    metrics->tmHeight = 18;
    metrics->tmExternalLeading = 0;


    return TRUE;
    //return FALSE;
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
    return RosGdiPatBlt(physDev->hKernelDC, left, top, width, height, rop);
}

BOOL CDECL RosDrv_Pie( NTDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom,
            INT xstart, INT ystart, INT xend, INT yend )
{
    UNIMPLEMENTED;
    return FALSE;
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
    UNIMPLEMENTED;
    return FALSE;
}

UINT CDECL RosDrv_RealizePalette( NTDRV_PDEVICE *physDev, HPALETTE hpal, BOOL primary )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL CDECL RosDrv_Rectangle(NTDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom)
{
    RECT rc;

    /* Convert coordinates */
    SetRect(&rc, left, top, right, bottom);
    LPtoDP(physDev->hUserDC, (POINT*)&rc, 2);

    if ((rc.left == rc.right) || (rc.top == rc.bottom)) return TRUE;

    if (rc.right < rc.left) { INT tmp = rc.right; rc.right = rc.left; rc.left = tmp; }
    if (rc.bottom < rc.top) { INT tmp = rc.bottom; rc.bottom = rc.top; rc.top = tmp; }

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
    RosGdiSelectBitmap(physDev->hKernelDC, hbitmap);

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
        UNIMPLEMENTED;
        //RosGdiSelectFont(physDev->hKernelDC, hfont, gdiFont);
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

DWORD CDECL RosDrv_SetDCOrg( NTDRV_PDEVICE *physDev, INT x, INT y )
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
    UNIMPLEMENTED;
    return 0;
}

INT CDECL RosDrv_SetDIBits( NTDRV_PDEVICE *physDev, HBITMAP hbitmap, UINT startscan,
                            UINT lines, LPCVOID bits, const BITMAPINFO *info, UINT coloruse )
{
    return RosGdiSetDIBits(physDev->hKernelDC, hbitmap, startscan, lines, bits, info, coloruse);
}

INT CDECL RosDrv_SetDIBitsToDevice( NTDRV_PDEVICE *physDev, INT xDest, INT yDest, DWORD cx,
                                    DWORD cy, INT xSrc, INT ySrc,
                                    UINT startscan, UINT lines, LPCVOID bits,
                                    const BITMAPINFO *info, UINT coloruse )
{
    UNIMPLEMENTED;
    return 0;
}

void CDECL RosDrv_SetDeviceClipping( NTDRV_PDEVICE *physDev, HRGN vis_rgn, HRGN clip_rgn )
{
    RGNDATA *data;
    HRGN dc_rgn;
    DWORD size;

    /* Create a dummy region (FIXME: create it once!) */
    dc_rgn = CreateRectRgn(0,0,0,0);
    if (!dc_rgn) return;

    /* Update dcRegion to become a combined region */
    CombineRgn( dc_rgn, vis_rgn, clip_rgn, clip_rgn ? RGN_AND : RGN_COPY );

    /* Get region data size */
    if (!(size = GetRegionData( dc_rgn, 0, NULL )))
    {
        DeleteObject(dc_rgn);
        return;
    }

    /* Allocate memory for it */
    if (!(data = HeapAlloc( GetProcessHeap(), 0, size )))
    {
        DeleteObject(dc_rgn);
        return;
    }

    /* Get region data */
    if (!GetRegionData( dc_rgn, size, data ))
    {
        HeapFree( GetProcessHeap(), 0, data );
        DeleteObject(dc_rgn);
        return;
    }

    // FIXME: What to do with origin?
    //XSetClipRectangles( gdi_display, physDev->gc, physDev->dc_rect.left, physDev->dc_rect.top,
    //                    (XRectangle *)data->Buffer, data->rdh.nCount, YXBanded );

    /* Set clipping */
    RosGdiSetDeviceClipping(physDev->hKernelDC, data->rdh.nCount, (RECTL *)data->Buffer, (RECTL *)&data->rdh.rcBound);

    /* Free memory and delete clipping region */
    HeapFree( GetProcessHeap(), 0, data );
    DeleteObject(dc_rgn);
}

BOOL CDECL RosDrv_SetDeviceGammaRamp(NTDRV_PDEVICE *physDev, LPVOID ramp)
{
    UNIMPLEMENTED;
    return FALSE;
}

COLORREF CDECL RosDrv_SetPixel( NTDRV_PDEVICE *physDev, INT x, INT y, COLORREF color )
{
    UNIMPLEMENTED;
    return 0;
}

BOOL CDECL RosDrv_SetPixelFormat(NTDRV_PDEVICE *physDev,
                                   int iPixelFormat,
                                   const PIXELFORMATDESCRIPTOR *ppfd)
{
    UNIMPLEMENTED;
    return FALSE;
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
    return RosGdiStretchBlt(physDevDst->hKernelDC, xDst, yDst, widthDst, heightDst,
        physDevSrc->hKernelDC, xSrc, ySrc, widthSrc, heightSrc, rop);
}

BOOL CDECL RosDrv_SwapBuffers(NTDRV_PDEVICE *physDev)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL CDECL RosDrv_UnrealizePalette( HPALETTE hpal )
{
    UNIMPLEMENTED;
    return FALSE;
}

/* EOF */
