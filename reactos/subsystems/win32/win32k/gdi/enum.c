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
    pDC = DC_Lock(physDev);

    /* Get device caps from the DC */
    iCaps = GreGetDeviceCaps(pDC, cap);

    /* Release the object */
    DC_Unlock(pDC);

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

UINT APIENTRY RosGdiGetSystemPaletteEntries( HDC hDC, UINT uStart, UINT uCount,
                                     LPPALETTEENTRY lpEntries )
{
    UINT uRes = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Call GRE function fully wrapped into SEH */
    _SEH2_TRY
    {
        uRes = GreGetSystemPaletteEntries(hDC, uStart, uCount, lpEntries);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status)) return 0;

    return uRes;
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
