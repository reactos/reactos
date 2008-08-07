/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdistock.c
 * PURPOSE:         GDI Stock Object Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

HANDLE
APIENTRY
NtGdiGetStockObject(IN INT Object)
{
    DPRINT1("NtGdiGetStockObject() index %d\n", Object);

    /* TODO: Return the requested stock object*/
    return NULL;
}
