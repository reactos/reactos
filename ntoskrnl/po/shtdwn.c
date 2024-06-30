/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power shutdown mechanism routines
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#include "inbv/logo.h"

/* GLOBALS ********************************************************************/

KEVENT PopShutdownEvent;
PPOP_SHUTDOWN_WAIT_ENTRY PopShutdownThreadList;
LIST_ENTRY PopShutdownQueue;
KGUARDED_MUTEX PopShutdownListMutex;
BOOLEAN PopShutdownListAvailable;

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
NTAPI
PopShutdownHandler(VOID)
{
    /* FIXME */
    UNIMPLEMENTED;
    return;
}

NTSTATUS
NTAPI
PoQueueShutdownWorkItem(
    _In_ PWORK_QUEUE_ITEM WorkItem)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
PoRequestShutdownEvent(
    _Out_ PVOID *Event)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
