/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdipal.c
 * PURPOSE:         GDI Palette Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

ULONG
APIENTRY
NtGdiColorCorrectPalette(IN HDC hdc,
                         IN HPALETTE hpal,
                         IN ULONG FirstEntry,
                         IN ULONG NumberOfEntries,
                         IN OUT PALETTEENTRY *ppalEntry,
                         IN ULONG Command)
{
    UNIMPLEMENTED;
    return 0;
}

HPALETTE
APIENTRY
NtGdiCreateHalftonePalette(IN HDC hdc)
{
    UNIMPLEMENTED;
    return NULL;
}

HPALETTE
APIENTRY
NtGdiCreatePaletteInternal(IN LPLOGPALETTE pLogPal,
                           IN UINT cEntries)
{
    UNIMPLEMENTED;
    return NULL;
}

LONG
APIENTRY
NtGdiDoPalette(IN HPALETTE hObj,
               IN WORD iStart,
               IN WORD cEntries,
               IN PALETTEENTRY* pEntries,
               IN DWORD iFunc,
               IN BOOL bInbound)
{
    /*
     * NOTE: hObj is a DC handle (HDC) for GdiPalGetSystemEntries and GdiPalGet/SetColorTable
     * pEntries is RGBQUAD* for GdiPalGet/SetColorTable
     * We do this to keep compatibility with MS ntgdi.h */
    UNIMPLEMENTED;
    return 0;
}

UINT
APIENTRY
NtGdiGetNearestPaletteIndex(IN HPALETTE hpal,
                            IN COLORREF crColor)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiResizePalette(IN HPALETTE hpal,
                   IN UINT cEntry)
{
    UNIMPLEMENTED;
    return FALSE;
}

UINT
APIENTRY
NtGdiGetSystemPaletteUse(IN HDC hdc)
{
    UNIMPLEMENTED;
    return 0;
}

UINT
APIENTRY
NtGdiSetSystemPaletteUse(IN HDC hdc,
                         IN UINT ui)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiUnrealizeObject(IN HANDLE h)
{
    UNIMPLEMENTED;
    return FALSE;
}
