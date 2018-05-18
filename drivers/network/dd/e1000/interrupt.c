/*
 * PROJECT:     ReactOS Intel PRO/1000 Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Interrupt handlers
 * COPYRIGHT:   Copyright 2013 Cameron Gutman (cameron.gutman@reactos.org)
 *              Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include "nic.h"

#include <debug.h>

VOID
NTAPI
MiniportISR(
    OUT PBOOLEAN InterruptRecognized,
    OUT PBOOLEAN QueueMiniportHandleInterrupt,
    IN NDIS_HANDLE MiniportAdapterContext)
{
    PE1000_ADAPTER Adapter = (PE1000_ADAPTER)MiniportAdapterContext;

    //
    // FIXME: We need to synchronize with this ISR for changes to InterruptPending,
    // LinkChange, MediaState, and LinkSpeedMbps. We can get away with IRQL
    // synchronization on non-SMP machines because we run a DIRQL here.
    //

    Adapter->InterruptPending |= NICInterruptRecognized(Adapter, InterruptRecognized);
    if (!(*InterruptRecognized))
    {
        /* This is not ours. */
        *QueueMiniportHandleInterrupt = FALSE;
        return;
    }

    UNIMPLEMENTED;

    /* Acknowledge the interrupt and mark the events pending service */
    NICAcknowledgeInterrupts(Adapter);
    *QueueMiniportHandleInterrupt = TRUE;
}

VOID
NTAPI
MiniportHandleInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
}
