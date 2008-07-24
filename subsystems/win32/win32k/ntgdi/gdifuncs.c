/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdistubs.c
 * PURPOSE:         Syscall stubs
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

W32KAPI
BOOL
APIENTRY
NtGdiInit()
{
    DPRINT1("NtGdiInit() called\n");
    return TRUE;
}

