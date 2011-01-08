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
static struct list handle_mapping_list = LIST_INIT( handle_mapping_list );
static CRITICAL_SECTION handle_mapping_cs;

typedef struct _HMAPPING
{
    HGDIOBJ hUser;
    HGDIOBJ hKernel;
    struct list entry;
} HMAPPING, *PHMAPPING;

/* FUNCTIONS **************************************************************/

VOID InitHandleMapping()
{
    InitializeCriticalSection(&handle_mapping_cs);
}

VOID AddHandleMapping(HGDIOBJ hKernel, HGDIOBJ hUser)
{
    PHMAPPING mapping = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HMAPPING));
    if(!mapping)
        return;

    mapping->hKernel = hKernel;
    mapping->hUser = hUser;

    EnterCriticalSection(&handle_mapping_cs);
    list_add_tail(&handle_mapping_list, &mapping->entry);
    LeaveCriticalSection(&handle_mapping_cs);
}

static PHMAPPING FindHandleMapping(HGDIOBJ hUser)
{
    PHMAPPING item;

    LIST_FOR_EACH_ENTRY( item, &handle_mapping_list, HMAPPING, entry )
    {
        if (item->hUser == hUser)
        {
            return item;
        }
    }

    return NULL;
}

HGDIOBJ MapUserHandle(HGDIOBJ hUser)
{
    PHMAPPING mapping;

    mapping = FindHandleMapping(hUser);

    return mapping ? mapping->hKernel : NULL;
}

VOID RemoveHandleMapping(HGDIOBJ hUser)
{
    PHMAPPING mapping;

    mapping = FindHandleMapping(hUser);
    if(mapping == NULL)
        return;

    EnterCriticalSection(&handle_mapping_cs);
    list_remove(&mapping->entry);
    LeaveCriticalSection(&handle_mapping_cs);
}

VOID CleanupHandleMapping()
{
    PHMAPPING mapping;

    while(!list_empty(&handle_mapping_list))
    {
        mapping = LIST_ENTRY(list_head(&handle_mapping_list), HMAPPING, entry);
        RemoveHandleMapping(mapping->hUser);
    }
}

BOOL CDECL RosDrv_CreateBitmap( NTDRV_PDEVICE *physDev, HBITMAP hbitmap, LPVOID bmBits )
{
    BITMAP bitmap;
    HBITMAP hKbitmap;

    /* Get the usermode object */
    if (!GetObjectW(hbitmap, sizeof(bitmap), &bitmap)) return FALSE;

    /* Check parameters */
    if (bitmap.bmPlanes != 1) return FALSE;

    /* Create the kernelmode bitmap object */
    hKbitmap = RosGdiCreateBitmap(physDev->hKernelDC, &bitmap, bmBits);

    if(!hKbitmap)
        return FALSE;

    AddHandleMapping(hKbitmap, hbitmap);
    return TRUE;
}

BOOL CDECL RosDrv_CreateDC( HDC hdc, NTDRV_PDEVICE **pdev, LPCWSTR driver, LPCWSTR device,
                            LPCWSTR output, const DEVMODEW* initData )
{
    BOOL bRet;
    DWORD dcType;
    NTDRV_PDEVICE *physDev;
    HDC hKernelDC;

    /* Allocate memory for two handles */
    physDev = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*physDev) );
    if (!physDev) return FALSE;

    /* Get DC type */
    dcType = GetObjectType(hdc);

    /* Save stock bitmap's handle */
    if (dcType == OBJ_MEMDC && !hStockBitmap)
        hStockBitmap = GetCurrentObject( hdc, OBJ_BITMAP );

    /* Call the win32 kernel */
    bRet = RosGdiCreateDC(&hKernelDC, driver, device, output, initData, dcType);

    /* Save newly created DC */
    physDev->hKernelDC = hKernelDC;
    physDev->hdc = hdc;

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
    HBITMAP hKbitmap = (HBITMAP)MapUserHandle(hbitmap);

    if(hKbitmap == NULL)
        return FALSE;

    RemoveHandleMapping(hbitmap);

    return RosGdiDeleteBitmap(hKbitmap);
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

                    physDev->dc_rect = data->dc_rect;
                    RosGdiSetDcRects(physDev->hKernelDC, NULL, (RECT*)&data->drawable_rect);

                    if (!data->release)
                        RosGdiGetDC(physDev->hKernelDC, data->hwnd, data->clip_children);
                    else
                        RosGdiReleaseDC(physDev->hKernelDC);

                    TRACE( "SET_DRAWABLE hdc %p dc_rect %s drawable_rect %s\n",
                           physDev->hdc, wine_dbgstr_rect(&data->dc_rect), wine_dbgstr_rect(&data->drawable_rect) );
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
    if (hbitmap == hStockBitmap)
        bStock = TRUE;
    else
        hbitmap = (HBITMAP)MapUserHandle(hbitmap);

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

    if(logbrush.lbStyle == BS_PATTERN)
        logbrush.lbHatch = (ULONG_PTR)MapUserHandle((HBITMAP)logbrush.lbHatch);

    RosGdiSelectBrush(physDev->hKernelDC, &logbrush);

    return hbrush;
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
        logpen.lopnColor = GetDCPenColor(physDev->hdc);

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
