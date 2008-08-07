/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdifill.c
 * PURPOSE:         GDI Filled Shape Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtGdiEllipse(IN HDC hdc,
             IN INT xLeft,
             IN INT yTop,
             IN INT xRight,
             IN INT yBottom)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiExtFloodFill(IN HDC hdc,
                  IN INT x,
                  IN INT y,
                  IN COLORREF crColor,
                  IN UINT iFillType)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiGradientFill(IN HDC hdc,
                  IN PTRIVERTEX pVertex,
                  IN ULONG nVertex,
                  IN PVOID pMesh,
                  IN ULONG nMesh,
                  IN ULONG ulMode)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiRectangle(IN HDC hdc,
               IN INT xLeft,
               IN INT yTop,
               IN INT xRight,
               IN INT yBottom)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiRoundRect(IN HDC hdc,
               IN INT x1,
               IN INT y1,
               IN INT x2,
               IN INT y2,
               IN INT x3,
               IN INT y3)
{
    UNIMPLEMENTED;
    return FALSE;
}
