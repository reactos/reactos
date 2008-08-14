
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native driver for dxg implementation
 * FILE:             drivers/directx/dxg/main.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       15/10-2007   Magnus Olsen
 */

/* DDK/NDK/SDK Headers */
#include <ddk/ntddk.h>

NTSTATUS NTAPI
DriverEntry(IN PVOID Context1,
            IN PVOID Context2)
{
    /* 
     * NOTE this driver will never be load, it only contain export list
     * to win32k eng functions
     */
    return STATUS_SUCCESS;
}


