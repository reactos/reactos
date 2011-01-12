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

/* FUNCTIONS **************************************************************/

BOOL CDECL RosDrv_CreateBitmap( PDC_ATTR pdcattr, HBITMAP hbitmap, LPVOID bmBits )
{
    BITMAP bitmap;
    HBITMAP hKbitmap;

    /* Get the usermode object */
    if (!GetObjectW(hbitmap, sizeof(bitmap), &bitmap)) return FALSE;

    /* Check parameters */
    if (bitmap.bmPlanes != 1) return FALSE;

    /* Create the kernelmode bitmap object */
    hKbitmap = RosGdiCreateBitmap(pdcattr->hKernelDC, &bitmap, bmBits);

    if(!hKbitmap)
        return FALSE;

    AddHandleMapping(hKbitmap, hbitmap);
    return TRUE;
}

BOOL CDECL RosDrv_CreateDC( HDC hdc, PDC_ATTR *ppdcattr, LPCWSTR driver, LPCWSTR device,
                            LPCWSTR output, const DEVMODEW* initData )
{
    BOOL bRet;
    DWORD dcType;
    PDC_ATTR pdcattr;
    HDC hKernelDC;

    /* Get DC type */
    dcType = GetObjectType(hdc);
    
    /* Call the win32 kernel */
    bRet = RosGdiCreateDC(&hKernelDC, driver, device, output, initData, dcType);
    if(!bRet)
    {
        ERR("RosGdiCreateDC failed!!!\n");
        return FALSE;
    }

    /* Retrieve the user mode part of the new handle */
    pdcattr = GdiGetDcAttr(hKernelDC);
    if(!pdcattr)
    {
        ERR("RosGdiCreateDC failed!!!\n");
        return TRUE;
    }

    /* Save newly created DC */
    pdcattr->hKernelDC = hKernelDC;
    pdcattr->hdc = hdc;

    /* No font is selected */
    pdcattr->cache_index = -1;

    /* Create a usermode clipping region (same as kernelmode one, just to reduce
       amount of syscalls) */
    pdcattr->region = CreateRectRgn( 0, 0, 0, 0 );

    /* Return allocated physical DC to the caller */
    *ppdcattr = pdcattr;

    return bRet;
}

BOOL CDECL RosDrv_DeleteBitmap( HBITMAP hbitmap )
{
    HBITMAP hKbitmap = (HBITMAP)MapUserHandle(hbitmap);

    if(hKbitmap == NULL)
        return FALSE;

    RemoveHandleMapping(hbitmap);

    return RosGdiDeleteBitmap(hKbitmap);
}

BOOL CDECL RosDrv_DeleteDC( PDC_ATTR pdcattr )
{
    /* Delete usermode copy of a clipping region */
    DeleteObject( pdcattr->region );

    /* Delete kernel DC */
    return RosGdiDeleteDC(pdcattr->hKernelDC);
}

INT CDECL RosDrv_ExtEscape( PDC_ATTR pdcattr, INT escape, INT in_count, LPCVOID in_data,
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

                    pdcattr->dc_rect = data->dc_rect;
                    RosGdiSetDcRects(pdcattr->hKernelDC, NULL, (RECT*)&data->drawable_rect);

                    RosGdiGetDC(pdcattr->hKernelDC, data->drawable, data->clip_children);

                    TRACE( "SET_DRAWABLE hdc %p dc_rect %s drawable_rect %s\n",
                           pdcattr->hdc, wine_dbgstr_rect(&data->dc_rect), wine_dbgstr_rect(&data->drawable_rect) );
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

LONG CDECL RosDrv_GetBitmapBits( HBITMAP hbitmap, void *buffer, LONG count )
{
    hbitmap = (HBITMAP)MapUserHandle(hbitmap);

    return RosGdiGetBitmapBits(hbitmap, buffer, count);
}

BOOL CDECL RosDrv_GetCharWidth( PDC_ATTR pdcattr, UINT firstChar, UINT lastChar,
                                  LPINT buffer )
{
    UNIMPLEMENTED;
    return FALSE;
}

INT CDECL RosDrv_GetDeviceCaps( PDC_ATTR pdcattr, INT cap )
{
    return RosGdiGetDeviceCaps(pdcattr->hKernelDC, cap);
}

BOOL CDECL RosDrv_GetDeviceGammaRamp(PDC_ATTR pdcattr, LPVOID ramp)
{
    UNIMPLEMENTED;
    return FALSE;
}

COLORREF CDECL RosDrv_GetNearestColor( PDC_ATTR pdcattr, COLORREF color )
{
    UNIMPLEMENTED;
    return 0;
}

UINT CDECL RosDrv_GetSystemPaletteEntries( PDC_ATTR pdcattr, UINT start, UINT count,
                                           LPPALETTEENTRY entries )
{
    return RosGdiGetSystemPaletteEntries(pdcattr->hKernelDC, start, count, entries);
}

UINT CDECL RosDrv_RealizeDefaultPalette( PDC_ATTR pdcattr )
{
    //UNIMPLEMENTED;
    return FALSE;
}

UINT CDECL RosDrv_RealizePalette( PDC_ATTR pdcattr, HPALETTE hpal, BOOL primary )
{
    UNIMPLEMENTED;
    return FALSE;
}

HBITMAP CDECL RosDrv_SelectBitmap( PDC_ATTR pdcattr, HBITMAP hbitmap )
{
    BOOL bRes;

    /* Check if it's a stock bitmap */
    hbitmap = (HBITMAP)MapUserHandle(hbitmap);

    /* Select the bitmap into the DC */
    bRes = RosGdiSelectBitmap(pdcattr->hKernelDC, hbitmap);

    /* If there was an error, return 0 */
    if (!bRes) return 0;

    /* Return handle of selected bitmap as a success*/
    return hbitmap;
}

HBRUSH CDECL RosDrv_SelectBrush( PDC_ATTR pdcattr, HBRUSH hbrush )
{
    LOGBRUSH logbrush;

    if (!GetObjectA( hbrush, sizeof(logbrush), &logbrush )) return 0;

    if(logbrush.lbStyle == BS_PATTERN)
        logbrush.lbHatch = (ULONG_PTR)MapUserHandle((HBITMAP)logbrush.lbHatch);

    RosGdiSelectBrush(pdcattr->hKernelDC, &logbrush);

    return hbrush;
}

HPEN CDECL RosDrv_SelectPen( PDC_ATTR pdcattr, HPEN hpen )
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
        logpen.lopnColor = GetDCPenColor(pdcattr->hdc);

    /* Call kernelmode */
    RosGdiSelectPen(pdcattr->hKernelDC, &logpen, elogpen);

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

COLORREF CDECL RosDrv_SetDCBrushColor( PDC_ATTR pdcattr, COLORREF crColor )
{
    UNIMPLEMENTED;
    return 0;
}

COLORREF CDECL RosDrv_SetDCPenColor( PDC_ATTR pdcattr, COLORREF crColor )
{
    UNIMPLEMENTED;
    return 0;
}

BOOL CDECL RosDrv_SetDeviceGammaRamp(PDC_ATTR pdcattr, LPVOID ramp)
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
