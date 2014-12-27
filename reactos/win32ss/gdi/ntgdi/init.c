/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Initialization of GDI
 * FILE:             win32ss/gdi/ntgdi/init.c
 * PROGRAMER:
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>
#include <kdros.h>

/*
 * @implemented
 */
BOOL
APIENTRY
NtGdiInit(VOID)
{
    return TRUE;
}

/* EOF */
