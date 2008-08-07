/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdistrm.c
 * PURPOSE:         GDI Stream (?) Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtGdiDrawStream(IN HDC hdcDst,
                IN ULONG cjIn,
                IN VOID *pvIn)
{
    UNIMPLEMENTED;
    return FALSE;
}
