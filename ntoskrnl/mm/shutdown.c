/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/mm/shutdown.c
 * PURPOSE:         Memory Manager Shutdown
 * PROGRAMMERS:
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include "ARM3/miarm.h"

/* PRIVATE FUNCTIONS *********************************************************/

VOID
MiShutdownSystem(VOID)
{
    ULONG i;

    /* Loop through all the paging files */
    for (i = 0; i < MmNumberOfPagingFiles; i++)
    {
        /* And close them */
        ZwClose(MmPagingFile[i]->FileHandle);
    }

    UNIMPLEMENTED;
}

VOID
MmShutdownSystem(IN ULONG Phase)
{
    if (Phase == 0)
    {
        MiShutdownSystem();
    }
    else
    {
        UNIMPLEMENTED;
    }
}
