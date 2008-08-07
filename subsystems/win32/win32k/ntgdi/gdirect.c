/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdirect.c
 * PURPOSE:         GDI Rectangle Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/* TODO: Move rectangle functions here from gdirgn.c */

DWORD
APIENTRY
NtGdiGetBoundsRect(IN HDC hdc,
                   OUT LPRECT prc,
                   IN DWORD f)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtGdiSetBoundsRect(IN HDC hdc,
                   IN LPRECT prc,
                   IN DWORD f)
{
    UNIMPLEMENTED;
    return 0;
}
