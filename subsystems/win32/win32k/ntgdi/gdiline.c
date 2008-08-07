/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdiline.c
 * PURPOSE:         GDI Line Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtGdiAngleArc(IN HDC hdc,
              IN INT x,
              IN INT y,
              IN DWORD dwRadius,
              IN DWORD dwStartAngle,
              IN DWORD dwSweepAngle)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiArcInternal(IN ARCTYPE arctype,
                 IN HDC hdc,
                 IN INT x1,
                 IN INT y1,
                 IN INT x2,
                 IN INT y2,
                 IN INT x3,
                 IN INT y3,
                 IN INT x4,
                 IN INT y4)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiLineTo(IN HDC hdc,
            IN INT x,
            IN INT y)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiMoveTo(IN HDC hdc,
            IN INT x,
            IN INT y,
            OUT OPTIONAL LPPOINT pptOut)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiPolyDraw(IN HDC hdc,
              IN LPPOINT ppt,
              IN LPBYTE pjAttr,
              IN ULONG cpt)
{
    UNIMPLEMENTED;
    return FALSE;
}

ULONG_PTR
APIENTRY
NtGdiPolyPolyDraw(IN HDC hdc,
                  IN PPOINT ppt,
                  IN PULONG pcpt,
                  IN ULONG ccpt,
                  IN INT iFunc)
{
    UNIMPLEMENTED;
    return 0;
}
