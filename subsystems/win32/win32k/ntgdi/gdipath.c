/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdipath.c
 * PURPOSE:         GDI Path Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtGdiAbortPath(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiBeginPath(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiCloseFigure(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEndPath(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiFillPath(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiFlattenPath(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}

INT
APIENTRY
NtGdiGetPath(IN HDC hdc,
             OUT OPTIONAL LPPOINT pptlBuf,
             OUT OPTIONAL LPBYTE pjTypes,
             IN INT cptBuf)
{
    UNIMPLEMENTED;
    return 0;
}

HRGN
APIENTRY
NtGdiPathToRegion(IN HDC hdc)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtGdiSelectClipPath(IN HDC hdc,
                    IN INT iMode)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiStrokeAndFillPath(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiStrokePath(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiWidenPath(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiGetMiterLimit(IN HDC hdc,
                   OUT PDWORD pdwOut)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiSetMiterLimit(IN HDC hdc,
                   IN DWORD dwNew,
                   IN OUT OPTIONAL PDWORD pdwOut)
{
    UNIMPLEMENTED;
    return FALSE;
}
