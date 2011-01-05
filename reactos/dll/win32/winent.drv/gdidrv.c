/*
 * PROJECT:         ReactOS
 * LICENSE:         GNU LGPL by FSF v2.1 or any later
 * FILE:            dll/win32/winent.drv/gdidrv.c
 * PURPOSE:         GDI driver stub for ReactOS/Windows
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include "winent.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rosgdidrv);

/* GLOBALS ****************************************************************/
HANDLE hStockBitmap;

/* FUNCTIONS **************************************************************/

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

BOOL CDECL RosDrv_EnumDeviceFonts( NTDRV_PDEVICE *physDev, LPLOGFONTW plf,
                                   FONTENUMPROCW proc, LPARAM lp )
{
    /* We're always using client-side fonts. */
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

                    RosGdiSetDcRects(physDev->hKernelDC, (RECT*)&data->dc_rect, (RECT*)&data->drawable_rect);

                    if (!data->release)
                        RosGdiGetDC(physDev->hKernelDC, data->hwnd, data->clip_children);
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

INT CDECL RosDrv_GetDeviceCaps( NTDRV_PDEVICE *physDev, INT cap )
{
    return RosGdiGetDeviceCaps(physDev->hKernelDC, cap);
}

BOOL CDECL RosDrv_GetDeviceGammaRamp(NTDRV_PDEVICE *physDev, LPVOID ramp)
{
    UNIMPLEMENTED;
    return FALSE;
}

COLORREF CDECL RosDrv_GetNearestColor( NTDRV_PDEVICE *physDev, COLORREF color )
{
    UNIMPLEMENTED;
    return 0;
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

BOOL CDECL RosDrv_SetDeviceGammaRamp(NTDRV_PDEVICE *physDev, LPVOID ramp)
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
