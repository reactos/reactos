/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/stream/dll.c
 * PURPOSE:         kernel mode driver initialization
 * PROGRAMMER:      Johannes Anderwald
 */


#include "stream.h"

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
