/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdiclip.c
 * PURPOSE:         GDI Clipping Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/* TODO: Move clipping functions here from gdirgn.c */

INT
APIENTRY
NtGdiGetAppClipBox(IN HDC hdc,
                  OUT LPRECT prc)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiPtVisible(IN HDC hdc,
               IN INT x,
               IN INT y)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiRectVisible(IN HDC hdc,
                IN LPRECT prc)
{
    UNIMPLEMENTED;
    return FALSE;
}

INT
APIENTRY
NtGdiExcludeClipRect(IN HDC hdc,
                     IN INT xLeft,
                     IN INT yTop,
                     IN INT xRight,
                     IN INT yBottom)
{
    UNIMPLEMENTED;
    return 0;
}

INT
APIENTRY
NtGdiIntersectClipRect(IN HDC hdc,
                       IN INT xLeft,
                       IN INT yTop,
                       IN INT xRight,
                       IN INT yBottom)
{
    UNIMPLEMENTED;
    return 0;
}
