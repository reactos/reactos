/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdirgn.c
 * PURPOSE:         GDI Region Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

INT
APIENTRY
NtGdiCombineRgn(IN HRGN hrgnDst,
                IN HRGN hrgnSrc1,
                IN HRGN hrgnSrc2,
                IN INT iMode)
{
    UNIMPLEMENTED;
    return 0;
}

HRGN
APIENTRY
NtGdiCreateEllipticRgn(IN INT xLeft,
                       IN INT yTop,
                       IN INT xRight,
                       IN INT yBottom)
{
    UNIMPLEMENTED;
    return NULL;
}

HRGN
APIENTRY
NtGdiCreateRectRgn(IN INT xLeft,
                   IN INT yTop,
                   IN INT xRight,
                   IN INT yBottom)
{
    UNIMPLEMENTED;
    return NULL;
}

HRGN
APIENTRY
NtGdiCreateRoundRectRgn(IN INT xLeft,
                        IN INT yTop,
                        IN INT xRight,
                        IN INT yBottom,
                        IN INT xWidth,
                        IN INT yHeight)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtGdiEqualRgn(IN HRGN hrgn1,
              IN HRGN hrgn2)
{
    UNIMPLEMENTED;
    return FALSE;
}

INT
APIENTRY
NtGdiExtSelectClipRgn(IN HDC hdc,
                      IN HRGN hrgn,
                      IN INT iMode)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiFillRgn(IN HDC hdc,
             IN HRGN hrgn,
             IN HBRUSH hbrush)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiFrameRgn(IN HDC hdc,
              IN HRGN hrgn,
              IN HBRUSH hbrush,
              IN INT xWidth,
              IN INT yHeight)
{
    UNIMPLEMENTED;
    return FALSE;
}

INT
APIENTRY
NtGdiGetRandomRgn(IN HDC hDC,
                  OUT HRGN hDest,
                  IN INT iCode)
{
    UNIMPLEMENTED;
    return 0;
}

INT
APIENTRY
NtGdiGetRgnBox(IN HRGN hrgn,
               OUT LPRECT prcOut)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiInvertRgn(IN HDC hdc,
               IN HRGN hrgn)
{
    UNIMPLEMENTED;
    return FALSE;
}

INT
APIENTRY
NtGdiOffsetClipRgn(IN HDC hdc,
                   IN INT x,
                   IN INT y)
{
    UNIMPLEMENTED;
    return 0;
}

INT
APIENTRY
NtGdiOffsetRgn(IN HRGN hrgn,
               IN INT cx,
               IN INT cy)
{
    UNIMPLEMENTED;
    return 0;
}

INT
APIENTRY
NtGdiSetMetaRgn(IN HDC hdc)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiSetRectRgn(IN HRGN hrgn,
                IN INT xLeft,
                IN INT yTop,
                IN INT xRight,
                IN INT yBottom)
{
    UNIMPLEMENTED;
    return FALSE;
}

HRGN
APIENTRY
NtGdiExtCreateRegion(IN OPTIONAL LPXFORM px,
                     IN DWORD cj,
                     IN LPRGNDATA prgn)
{
    UNIMPLEMENTED;
    return NULL;
}

DWORD
APIENTRY
NtGdiGetRegionData(IN HRGN hrgn,
                   IN DWORD nCount,
                   OUT OPTIONAL LPRGNDATA lpRgnData)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiPtInRegion(IN HRGN hrgn,
                IN INT x,
                IN INT y)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiRectInRegion(IN HRGN hrgn,
                  IN OUT LPRECT prcl)
{
    UNIMPLEMENTED;
    return FALSE;
}
