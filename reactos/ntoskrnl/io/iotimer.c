/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/iocomp.c
 * PURPOSE:         I/O Wrappers for Executive Timers
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

/* Timer Database */
KSPIN_LOCK IopTimerLock;
LIST_ENTRY IopTimerQueueHead;

/* Timer Firing */
KDPC IopTimerDpc;
KTIMER IopTimer;

/* Keep count of how many timers we have */
ULONG IopTimerCount = 0;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
IopTimerDispatch(IN PKDPC Dpc,
                 IN PVOID DeferredContext,
                 IN PVOID SystemArgument1,
                 IN PVOID SystemArgument2)
{
    KIRQL OldIrql;
    PLIST_ENTRY TimerEntry;
    PIO_TIMER Timer;
    ULONG i;

    /* Check if any Timers are actualyl enabled as of now */
    if (IopTimerCount)
    {
        /* Lock the Timers */
        KeAcquireSpinLock(&IopTimerLock, &OldIrql);

        /* Call the Timer Routine of each enabled Timer */
        for (TimerEntry = IopTimerQueueHead.Flink, i = IopTimerCount;
            (TimerEntry != &IopTimerQueueHead) && i;
            TimerEntry = TimerEntry->Flink)
        {
            /* Get the timer and check if it's enabled */
            Timer = CONTAINING_RECORD(TimerEntry, IO_TIMER, IoTimerList);
            if (Timer->TimerEnabled)
            {
                /* Call the timer routine */
                Timer->TimerRoutine(Timer->DeviceObject, Timer->Context);
                i--;
            }
        }

        /* Unlock the Timers */
        KeReleaseSpinLock(&IopTimerLock, OldIrql);
    }
}

VOID
NTAPI
IopRemoveTimerFromTimerList(IN PIO_TIMER Timer)
{
    KIRQL OldIrql;

    /* Lock Timers */
    KeAcquireSpinLock(&IopTimerLock, &OldIrql);

    /* Remove Timer from the List and Drop the Timer Count if Enabled */
    RemoveEntryList(&Timer->IoTimerList);
    if (Timer->TimerEnabled) IopTimerCount--;

    /* Unlock the Timers */
    KeReleaseSpinLock(&IopTimerLock, OldIrql);
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoInitializeTimer(IN PDEVICE_OBJECT DeviceObject,
                  IN PIO_TIMER_ROUTINE TimerRoutine,
                  IN PVOID Context)
{
    PIO_TIMER IoTimer = DeviceObject->Timer;
    PAGED_CODE();

    /* Check if we don't have a timer yet */
    if (!IoTimer)
    {
        /* Allocate Timer */
        IoTimer = ExAllocatePoolWithTag(NonPagedPool,
                                        sizeof(IO_TIMER),
                                        TAG_IO_TIMER);
        if (!IoTimer) return STATUS_INSUFFICIENT_RESOURCES;

        /* Set up the Timer Structure */
        RtlZeroMemory(IoTimer, sizeof(IO_TIMER));
        IoTimer->Type = IO_TYPE_TIMER;
        IoTimer->DeviceObject = DeviceObject;
        DeviceObject->Timer = IoTimer;
    }

    /* Setup the timer routine and context */
    IoTimer->TimerRoutine = TimerRoutine;
    IoTimer->Context = Context;

    /* Add it to the Timer List */
    ExInterlockedInsertTailList(&IopTimerQueueHead,
                                &IoTimer->IoTimerList,
                                &IopTimerLock);

    /* Return Success */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
NTAPI
IoStartTimer(IN PDEVICE_OBJECT DeviceObject)
{
    KIRQL OldIrql;
    PIO_TIMER IoTimer = DeviceObject->Timer;

    /* Make sure the device isn't unloading */
    if (!(((PEXTENDED_DEVOBJ_EXTENSION)(DeviceObject->DeviceObjectExtension))->
            ExtensionFlags & (DOE_UNLOAD_PENDING |
                              DOE_DELETE_PENDING |
                              DOE_REMOVE_PENDING |
                              DOE_REMOVE_PROCESSED)))
    {
        /* Lock Timers */
        KeAcquireSpinLock(&IopTimerLock, &OldIrql);

        /* Check if the timer isn't already enabled */
        if (!IoTimer->TimerEnabled)
        {
            /* Enable it and increase the timer count */
            IoTimer->TimerEnabled = TRUE;
            IopTimerCount++;
        }

        /* Unlock Timers */
        KeReleaseSpinLock(&IopTimerLock, OldIrql);
    }
}

/*
 * @implemented
 */
VOID
NTAPI
IoStopTimer(PDEVICE_OBJECT DeviceObject)
{
    KIRQL OldIrql;
    PIO_TIMER IoTimer = DeviceObject->Timer;

    /* Lock Timers */
    KeAcquireSpinLock(&IopTimerLock, &OldIrql);

    /* Check if the timer is enabled */
    if (IoTimer->TimerEnabled)
    {
        /* Disable it and decrease the timer count */
        IoTimer->TimerEnabled = FALSE;
        IopTimerCount--;
    }

    /* Unlock Timers */
    KeReleaseSpinLock(&IopTimerLock, OldIrql);
}

/* EOF */
