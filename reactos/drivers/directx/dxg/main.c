
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native driver for dxg implementation
 * FILE:             drivers/directx/dxg/main.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       15/10-2007   Magnus Olsen
 */

#include <ntddk.h>

NTSTATUS
GsDriverEntry(IN PVOID Context1,
              IN PVOID Context2)
{
    /* NOTE : in windows it does a secure cookies check */
    return DriverEntry(Context1, Context2);
}


NTSTATUS
DriverEntry(IN PVOID Context1,
            IN PVOID Context2)
{
    return 0;
}
