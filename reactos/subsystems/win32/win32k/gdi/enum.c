/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gdi/bitmap.c
 * PURPOSE:         ReactOS GDI enumeration and info quering syscalls
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

int APIENTRY RosGdiChoosePixelFormat(HDC physDev,
                                   const PIXELFORMATDESCRIPTOR *ppfd)
{
    UNIMPLEMENTED;
    return 0;
}

int APIENTRY RosGdiDescribePixelFormat(HDC physDev,
			       int iPixelFormat,
			       UINT nBytes,
			       PIXELFORMATDESCRIPTOR *ppfd)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL APIENTRY RosGdiEnumDeviceFonts( HDC physDev, LPLOGFONTW plf,
                                   FONTENUMPROCW proc, LPARAM lp )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiGetCharWidth( HDC physDev, UINT firstChar, UINT lastChar,
                                  LPINT buffer )
{
    UNIMPLEMENTED;
    return FALSE;
}

INT APIENTRY RosGdiGetDeviceCaps( HDC physDev, INT cap )
{
    PDC pDC;
    INT iCaps;

    /* Get a pointer to the DC */
    pDC = GDI_GetObjPtr(physDev, (SHORT)GDI_OBJECT_TYPE_DC);

    /* Get device caps from the DC */
    iCaps = GreGetDeviceCaps(pDC, cap);

    /* Release the object */
    GDI_ReleaseObj(physDev);

    /* Return result */
    return iCaps;
}

BOOL APIENTRY RosGdiGetDeviceGammaRamp(HDC physDev, LPVOID ramp)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiGetICMProfile( HDC physDev, LPDWORD size, LPWSTR filename )
{
    UNIMPLEMENTED;
    return FALSE;
}

COLORREF APIENTRY RosGdiGetNearestColor( HDC physDev, COLORREF color )
{
    UNIMPLEMENTED;
    return 0;
}

int APIENTRY RosGdiGetPixelFormat(HDC physDev)
{
    UNIMPLEMENTED;
    return 0;
}

UINT APIENTRY RosGdiGetSystemPaletteEntries( HDC physDev, UINT start, UINT count,
                                     LPPALETTEENTRY entries )
{
    UNIMPLEMENTED;
    return 0;
}

BOOL APIENTRY RosGdiGetTextExtentExPoint( HDC physDev, LPCWSTR str, INT count,
                                        INT maxExt, LPINT lpnFit, LPINT alpDx, LPSIZE size )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiGetTextMetrics(HDC physDev, TEXTMETRICW *metrics)
{
    UNIMPLEMENTED;
    return FALSE;
}


/* EOF */
