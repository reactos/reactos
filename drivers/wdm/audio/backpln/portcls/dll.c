/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/dll.c
 * PURPOSE:         portcls generic dispatcher
 * PROGRAMMER:      Andrew Greenwood
 */


#include "private.h"

/*
 * @implemented
 */
ULONG
NTAPI
DllInitialize(ULONG Unknown)
{
    return 0;
}

/*
 * @implemented
 */
ULONG
NTAPI
DllUnload(VOID)
{
    return 0;
}
