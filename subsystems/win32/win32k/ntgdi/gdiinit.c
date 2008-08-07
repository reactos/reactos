/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdiinit.c
 * PURPOSE:         GDI Inittialization Routine
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtGdiInit(VOID)
{
    DPRINT1("NtGdiInit() called\n");
    return TRUE;
}

BOOL
APIENTRY
NtGdiInitSpool()
{
    UNIMPLEMENTED;
    return FALSE;
}
