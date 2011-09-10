/*
 * PROJECT:         ReactOS TDI driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/network/tdi/cte/events.c
 * PURPOSE:         CTE events support
 * PROGRAMMERS:     Oleg Baikalow (obaikalow@gmail.com)
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

typedef struct _CTEBLOCK_EVENT
{
    NTSTATUS Status;
    KEVENT Event;
} CTEBLOCK_EVENT, *PCTEBLOCK_EVENT;

struct _CTE_DELAYED_EVENT;
typedef void (*CTE_WORKER_ROUTINE)(struct _CTE_DELAYED_EVENT *, void *Context);

typedef struct _CTE_DELAYED_EVENT
{
    BOOLEAN Queued;
    KSPIN_LOCK Lock;
    CTE_WORKER_ROUTINE WorkerRoutine;
    PVOID Context;
    WORK_QUEUE_ITEM WorkItem;
} CTE_DELAYED_EVENT, *PCTE_DELAYED_EVENT;

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
CTEBlock(PCTEBLOCK_EVENT Block)
{
    NTSTATUS Status;

    /* Perform the wait */
    Status = KeWaitForSingleObject(&Block->Event, UserRequest, KernelMode, FALSE, NULL);

    /* Update event status if wait was not successful */
    if (!NT_SUCCESS(Status)) Block->Status = Status;

    return Block->Status;
}


VOID
NTAPI
InternalWorker(IN PVOID Parameter)
{
    PCTE_DELAYED_EVENT Event = (PCTE_DELAYED_EVENT)Parameter;
    KIRQL OldIrql;

    /* Acquire the lock */
    KeAcquireSpinLock(&Event->Lock, &OldIrql);

    /* Make sure it is queued */
    ASSERT(Event->Queued);
    Event->Queued = FALSE;

    /* Release the lock */
    KeReleaseSpinLock(&Event->Lock, OldIrql);

    /* Call the real worker routine */
    (*Event->WorkerRoutine)(Event, Event->Context);
}


/*
 * @implemented
 */
VOID
NTAPI
CTEInitEvent(PCTE_DELAYED_EVENT Event,
             CTE_WORKER_ROUTINE Routine)
{
    /* Init the structure, lock and a work item */
    Event->Queued = FALSE;
    KeInitializeSpinLock(&Event->Lock);
    Event->WorkerRoutine = Routine;
    ExInitializeWorkItem(&Event->WorkItem, InternalWorker, Event);
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
CTELogEvent (
	ULONG	Unknown0,
	ULONG	Unknown1,
	ULONG	Unknown2,
	ULONG	Unknown3,
	ULONG	Unknown4,
	ULONG	Unknown5,
	ULONG	Unknown6
	)
{
	/* Probably call
	 * IoAllocateErrorLogEntry and
	 * IoWriteErrorLogEntry
	 */
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */
BOOLEAN
NTAPI
CTEScheduleEvent(PCTE_DELAYED_EVENT Event,
                 PVOID Context)
{
    KIRQL OldIrql;

    /* Acquire the lock */
    KeAcquireSpinLock(&Event->Lock, &OldIrql);

    /* Make sure it is queued */
    if (!Event->Queued)
    {
        /* Mark it as queued and set optional context pointer */
        Event->Queued = TRUE;
        Event->Context = Context;

        /* Actually queue it */
        ExQueueWorkItem(&Event->WorkItem, CriticalWorkQueue);
    }

    /* Release the lock */
    KeReleaseSpinLock(&Event->Lock, OldIrql);

    return TRUE;
}


/*
 * @implemented
 */
LONG
NTAPI
CTESignal(PCTEBLOCK_EVENT Block, NTSTATUS Status)
{
    /* Set status right away */
    Block->Status = Status;

    /* Set the event */
    return KeSetEvent(&Block->Event, IO_NO_INCREMENT, FALSE);
}

/* EOF */
