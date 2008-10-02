/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdibrush.c
 * PURPOSE:         GDI Brush Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

HBRUSH
APIENTRY
NtGdiClearBrushAttributes(IN HBRUSH hbm,
                          IN DWORD dwFlags)
{
    UNIMPLEMENTED;
    return NULL;
}

HBRUSH
APIENTRY
NtGdiCreateDIBBrush(IN PVOID pv,
                    IN FLONG fl,
                    IN UINT  cj,
                    IN BOOL b8X8,
                    IN BOOL bPen,
                    IN PVOID pClient)
{
    UNIMPLEMENTED;
    return NULL;
}

HBRUSH
APIENTRY
NtGdiCreateHatchBrushInternal(IN ULONG ulStyle,
                              IN COLORREF clrr,
                              IN BOOL bPen)
{
    UNIMPLEMENTED;
    return NULL;
}

HBRUSH
APIENTRY
NtGdiCreatePatternBrushInternal(IN HBITMAP hbm,
                                IN BOOL bPen,
                                IN BOOL b8X8)
{
    UNIMPLEMENTED;
    return NULL;
}

HBRUSH
APIENTRY
NtGdiCreateSolidBrush(IN COLORREF cr,
                      IN OPTIONAL HBRUSH hbr)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtGdiIcmBrushInfo(IN HDC hdc,
                  IN HBRUSH hbrush,
                  IN OUT PBITMAPINFO pbmiDIB,
                  IN OUT PVOID pvBits,
                  IN OUT ULONG *pulBits,
                  OUT OPTIONAL DWORD *piUsage,
                  OUT OPTIONAL BOOL *pbAlreadyTran,
                  IN ULONG Command)
{
    UNIMPLEMENTED;
    return FALSE;
}

HBRUSH
APIENTRY
NtGdiSelectBrush(IN HDC hdc,
                 IN HBRUSH hbrush)
{
    UNIMPLEMENTED;
    return NULL;
}

HBRUSH
APIENTRY
NtGdiSetBrushAttributes(IN HBRUSH hbm,
                        IN DWORD dwFlags)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtGdiSetBrushOrg(IN HDC hdc,
                 IN INT x,
                 IN INT y,
                 OUT LPPOINT pptOut)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiBRUSHOBJ_DeleteRbrush(IN BRUSHOBJ *pbo,
                           IN BRUSHOBJ *pboB)
{
    UNIMPLEMENTED;
    return FALSE;
}
