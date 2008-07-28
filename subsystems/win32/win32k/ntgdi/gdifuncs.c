/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdifuncs.c
 * PURPOSE:         GDI functions
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

W32KAPI
ULONG
APIENTRY
NtGdiQueryFontAssocInfo(IN HDC hdc)
{
    UNIMPLEMENTED;
    return 0;
}

W32KAPI
HANDLE
APIENTRY
NtGdiGetStockObject(IN INT Object)
{
    DPRINT1("NtGdiGetStockObject() index %d\n", Object);

    /* TODO: Return the requested stock object*/
    return NULL;
}

