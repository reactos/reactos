/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power Manager IRP management routines
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

PIRP PopInrushIrp;
LIST_ENTRY PopDispatchWorkerIrpList;
LIST_ENTRY PopQueuedIrpList;
LIST_ENTRY PopQueuedInrushIrpList;
LIST_ENTRY PopIrpWatchdogList;
LIST_ENTRY PopOutstandingIrpList;
LIST_ENTRY PopIrpThreadWorkerList;
KSPIN_LOCK PopIrpLock;
PKTHREAD PopIrpOwnerLockThread;
KEVENT PopIrpDispatchPendingEvent;
ULONG PopPendingIrpDispatcWorkerCount;
ULONG PopIrpDispatchWorkerCount;
KSEMAPHORE PopIrpDispatchMasterSemaphore;
BOOLEAN PopIrpDispatchWorkerPending;

/* COMPLETION ROUTINES *******************************************************/

static IO_COMPLETION_ROUTINE PopRequestPowerIrpCompletion;

#if 0
static IO_COMPLETION_ROUTINE PopThermalIrpCompletion;
static IO_COMPLETION_ROUTINE PopFanIrpCompletion;
static IO_COMPLETION_ROUTINE PopBatteryIrpCompletion;
#endif

/* PRIVATE FUNCTIONS *********************************************************/

_Function_class_(KDEFERRED_ROUTINE)
VOID
NTAPI
PopIrpWatchdogDpcRoutine(
    _In_ PKDPC Dpc,
    _In_ PVOID DeferredContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2)
{
    PIRP Irp;
    BOOLEAN Found;
    PLIST_ENTRY Entry;
    TRIAGE_9F_POWER Triage;
    KLOCK_QUEUE_HANDLE IrpLockHandle;
    PPOP_IRP_WATCHDOG Watchdog;

    /* Loop over the IRP watchdog list to look for the appropriate IRP */
    PopAcquireIrpLock(&IrpLockHandle);
    Irp = (PIRP)DeferredContext;
    Found = FALSE;
    for (Entry = PopIrpWatchdogList.Flink;
         Entry != &PopIrpWatchdogList;
         Entry = Entry->Flink)
    {
        Watchdog = CONTAINING_RECORD(Entry, POP_IRP_WATCHDOG, Link);
        if (Watchdog->Irp == Irp)
        {
            /* We found it */
            Found = TRUE;
            break;
        }
    }

    /* We are done iterating over the IRP watchdogs */
    PopReleaseIrpLock(&IrpLockHandle);

    /*
     * We cannot simply bait an eye in the scenario where we have not
     * found the IRP from the watchdog list. The DPC has been fired
     * therefore whoever has setup a watchdog must have brought the
     * IRP in the list. If not, that is a serious nasty bug.
     */
    NT_ASSERT(Found == TRUE);

    /* Setup the triage dump info for debugging purposes */
    RtlZeroMemory(&Triage, sizeof(TRIAGE_9F_POWER));
    Triage.Signature = POP_9F_TRIAGE_SIGNATURE;
    Triage.Revision = POP_9F_TRIAGE_REVISION_V1;
    Triage.IrpList = PopOutstandingIrpList;
    Triage.ThreadList = PopIrpThreadWorkerList;
    Triage.DelayedWorkerQueue = NULL; // FIXME: We must fill in data once we support KPRIQUEUEs

    /*
     * This device has failed to complete its power IRP in time.
     * Any major delays on power operations from devices indicate a
     * malfunction from their end, which could further badly impact
     * system's startup or shutdown. It is the final countdown.
     */
    KeBugCheckEx(DRIVER_POWER_STATE_FAILURE,
                 0x3,
                 (ULONG_PTR)Watchdog->DeviceObject,
                 (ULONG_PTR)&Triage,
                 (ULONG_PTR)Irp);
}

static
VOID
PopRemoveOutstandingIrp(
    _In_ PIRP Irp)
{
    PLIST_ENTRY Entry;
    PIRP OutstandingIrp;

    /* Removing outstanding power IRPs must be done within IRP lock held */
    POP_ASSERT_IRP_LOCK_THREAD_OWNERSHIP();

    /* Iterate over the list to look for the appropriate IRP */
    for (Entry = PopOutstandingIrpList.Flink;
         Entry != &PopOutstandingIrpList;
         Entry = Entry->Flink)
    {
        /* This is the IRP we are looking for, remove it */
        OutstandingIrp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);
        if (Irp == OutstandingIrp)
        {
            RemoveEntryList(Entry);
            break;
        }
    }
}

static
POWER_ACTION
PopTranslateDriverIrpPowerAction(
    _In_ PPOP_POWER_ACTION PowerAction,
    _In_ BOOLEAN EjectDevice)
{
    POWER_ACTION Action;
    SYSTEM_POWER_STATE SystemState;

    /*
     * The current power action must not be within a hibernation
     * path (aka PowerActionHibernate) when a driver is doing a
     * power request. It is at our discretion whether the device
     * IRP can get into the hibernation path.
     */
    Action = PowerAction->Action;
    SystemState = PowerAction->SystemState;
    ASSERT(Action != PowerActionHibernate);

    /*
     * Check if we are incurring in a warm eject operation, and
     * specifically for this device it is requesting a power operation.
     */
    if (Action == PowerActionWarmEject)
    {
        /*
         * We are doing a warm eject operation. The system can only be
         * within appropriate sleep states (S1-S4) at this rate. This is
         * to ensure the power transition of the system is consistent.
         */
        ASSERT((SystemState >= PowerSystemSleeping1) &&
               (SystemState <= PowerSystemHibernate));

        if (EjectDevice)
        {
            /* The device is the one wanting to do a warm eject operation */
            return PowerActionWarmEject;
        }

        /*
         * We are doing a warm eject operation, however it is not for the
         * current device so we cannot simply translate the power action into
         * a warm eject one. In this scenario translate the action into a sleep
         * case if the system is about to sleep, or hibernate.
         */
        if (SystemState == PowerSystemHibernate)
        {
            return PowerActionHibernate;
        }
        else
        {
            return PowerActionSleep;
        }
    }

    /*
     * No warm eject operation is in progress, simply return the action
     * that comes from the global power actions. If the system incurs in
     * hibernation however, then translate to hibernate.
     */
    if (SystemState == PowerSystemHibernate)
    {
        return PowerActionHibernate;
    }

    return Action;
}

static
VOID
PopInsertIrpForDispatch(
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION NextStack)
{
    PDEVICE_OBJECT DeviceObject;

    /*
     * The same thread who wants to insert an IRP worker
     * entry must own the lock. Otherwise it is a bug.
     */
    POP_ASSERT_IRP_LOCK_THREAD_OWNERSHIP();

    /*
     * Mark this IRP as pending as it will be dispatched
     * as soon as a worker thread will process the IRP and
     * add an extra reference count to the device so that it
     * does not randomly die out on us.
     */
    DeviceObject = NextStack->DeviceObject;
    NextStack->Control |= SL_PENDING_RETURNED;
    ObReferenceObject(DeviceObject);

    /*
     * This is the same inrush IRP that the system is
     * processing. Put it in the front of the list, as
     * inrush IRPs have the utmost priority.
     */
    if (Irp == PopInrushIrp)
    {
        InsertHeadList(&PopDispatchWorkerIrpList, &Irp->Tail.Overlay.ListEntry);
    }
    else
    {
        /* This is a normal IRP, insert it for processing */
        InsertTailList(&PopDispatchWorkerIrpList, &Irp->Tail.Overlay.ListEntry);
    }

    /*
     * Signal the semaphore of an impeding IRP that is queued for a worker.
     * The IRP master dispatcher has to deal with this.
     */
    KeReleaseSemaphore(&PopIrpDispatchMasterSemaphore,
                       IO_NO_INCREMENT,
                       1,
                       FALSE);
}

_Function_class_(KSTART_ROUTINE)
VOID
NTAPI
PopIrpDispatchWorker(
    _In_ PVOID StartContext)
{
    PIRP Irp;
    KIRQL OldIrql;
    PKTHREAD Thread;
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_OBJECT DeviceObject;
    KLOCK_QUEUE_HANDLE LockHandle;
    PLIST_ENTRY Entry;

    PAGED_CODE();
    UNREFERENCED_PARAMETER(StartContext);

    /* Grab the IRP from the list */
    PopAcquireIrpLock(&LockHandle);
    Entry = RemoveHeadList(&PopDispatchWorkerIrpList);
    Irp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);

    /* And insert this worker thread into the list, for debugging purposes */
    Thread = KeGetCurrentThread();
    InsertTailList(&PopIrpThreadWorkerList, &Thread->ThreadListEntry);
    PopReleaseIrpLock(&LockHandle);

    /* Obtain the stack and device object for dispatching */
    IrpStack = IoGetNextIrpStackLocation(Irp);
    DeviceObject = IrpStack->DeviceObject;

    /*
     * Dispatch the IRP to a raised IRQL if we are safe that
     * the device driver is not pageable. If it is, then
     * do not raise the IRQL, this is a blocking request.
     */
    PopRaiseIrqlToDpc(DeviceObject, &OldIrql);
    PopDispatchPowerIrp(Irp, DeviceObject, IrpStack);
    PopLowerIrqlBack(OldIrql);

    /* We are done, save up a worker space */
    PopIrpDispatchWorkerCount--;

    /* Dereference the device object that we hold a reference back */
    ObDereferenceObject(DeviceObject);

    /*
     * Check if we have any pending IRPs that are waiting
     * for a worker to be available. With a worker slot that
     * we liberated, signal the master dispatcher to put the
     * IRP in service.
     */
    if (PopIrpDispatchWorkerPending)
    {
        PopPendingIrpDispatcWorkerCount--;
        KeSetEvent(&PopIrpDispatchPendingEvent,
                   IO_NO_INCREMENT,
                   FALSE);

        /* This is the last IRP pending for a worker so we are done */
        if (PopPendingIrpDispatcWorkerCount == 0)
        {
            PopIrpDispatchWorkerPending = FALSE;
        }
    }

    /* This worker is about to terminate, remove it from the list */
    PopAcquireIrpLock(&LockHandle);
    RemoveTailList(&PopIrpThreadWorkerList);
    PopReleaseIrpLock(&LockHandle);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

_Function_class_(KSTART_ROUTINE)
VOID
NTAPI
PopMasterDispatchIrp(
    _In_ PVOID StartContext)
{
    NTSTATUS Status;
    PVOID WaitObjects[2];

    PAGED_CODE();
    UNREFERENCED_PARAMETER(StartContext);

    WaitObjects[0] = &PopIrpDispatchMasterSemaphore;
    WaitObjects[1] = &PopIrpDispatchPendingEvent;

    for (;;)
    {
        /*
         * Wait for the IRP master dispatcher semaphore or for the
         * dispatch pending event to get signaled. In the former case,
         * we have an impeding IRP that is queued for a worker. In the
         * latter case, an IRP is pending for a worker space to be liberated.
         */
        KeWaitForMultipleObjects(2,
                                 WaitObjects,
                                 WaitAny,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 NULL,
                                 NULL);
        /*
         * Determine if we have exhausted every IRP worker currently
         * available to service this IRP.
         */
        if (PopIrpDispatchWorkerCount < POP_MAX_IRP_WORKERS_COUNT)
        {
            /* Service it with a power thread */
            Status = PopCreateIrpWorkerThread(PopIrpDispatchWorker);
            if (!NT_SUCCESS(Status))
            {
                /* Failing to service the IRP on our end is fatal */
                KeBugCheckEx(INTERNAL_POWER_ERROR, 0x1, (ULONG_PTR)Status, 0, 0);
            }

            /* And occupy a slot space for this newly created worker */
            PopIrpDispatchWorkerCount++;
        }
        else
        {
            /*
             * We do not have any space left to setup a worker for this IRP,
             * we have to put it in queue until an existing worker thread
             * finishes its job and liberates a space for us.
             */
            PopIrpDispatchWorkerPending = TRUE;
            PopPendingIrpDispatcWorkerCount++;
        }
    }
}

static
BOOLEAN
PopShouldForwardIrpAtPassive(
    _In_ PIO_STACK_LOCATION IrpStack)
{
    PDEVICE_OBJECT DeviceObject;

    /*
     * Forwarding a power IRP at passive level can only happen
     * within the following conditions:
     *
     * - The DO has set DO_POWER_PAGABLE indicating the device driver
     *   is pageable and not inrush;
     *
     * - The device or system is not in a fully working state.
     *
     * Either the system or device that is not fully ON it is not guaranteed
     * that the IRP can be dispatched safely at a higher interrupt level.
     */
    DeviceObject = IrpStack->DeviceObject;
    if ((IrpStack->MinorFunction == IRP_MN_SET_POWER) &&
        !(DeviceObject->Flags & DO_POWER_PAGABLE))
    {
        if (IrpStack->Parameters.Power.Type == DevicePowerState &&
            IrpStack->Parameters.Power.State.DeviceState == PowerDeviceD0)
        {
            return FALSE;
        }

        if (IrpStack->Parameters.Power.Type == SystemPowerState &&
            IrpStack->Parameters.Power.State.SystemState == PowerSystemWorking)
        {
            return FALSE;
        }
    }

    return TRUE;
}

static
BOOLEAN
PopIsDoProcessingPowerIrp(
    _In_ PDEVICE_OBJECT DeviceObject)
{
    PEXTENDED_DEVOBJ_EXTENSION DevObjExts;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Is this IRP belonging to a device? */
    DevObjExts = IoGetDevObjExtension(DeviceObject);
    if ((DevObjExts->PowerFlags & POP_DOE_DEVICE_IRP_ACTIVE) ||
        (DevObjExts->PowerFlags & POP_DOE_INRUSH_IRP_ACTIVE) ||
        (DevObjExts->PowerFlags & POP_DOE_SYSTEM_IRP_ACTIVE))
    {
        return TRUE;
    }

    /* This DO is not processing anything */
    return FALSE;
}

static
ULONG
PopComputeIrpListLength(
    _In_ PLIST_ENTRY IrpListHead)
{
    PLIST_ENTRY Entry;
    ULONG Count = 0;

    /* The naughty caller passed a NULL list */
    ASSERT(IrpListHead != NULL);

    /* This list is empty */
    if (IsListEmpty(IrpListHead))
    {
        return 0;
    }

    /* Compute the exact number of list entries this list has */
    for (Entry = IrpListHead->Flink;
         Entry != IrpListHead;
         Entry = Entry->Flink)
    {
        Count++;
    }

    return Count;
}

static
VOID
PopAddPowerIrpToQueue(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    ULONG QueuedIrpsCount;
    PEXTENDED_DEVOBJ_EXTENSION DevObjExts;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* The thread who enqueues an IRP must own the IRP lock */
    POP_ASSERT_IRP_LOCK_THREAD_OWNERSHIP();

    /*
     * Check if the device is inrush. Inrush devices have their
     * IRPs queued in another dedicated list.
     */
    DevObjExts = IoGetDevObjExtension(DeviceObject);
    if (DevObjExts->PowerFlags & POP_DOE_INRUSH_DEVICE)
    {
        InsertTailList(&PopQueuedInrushIrpList, &Irp->Tail.Overlay.ListEntry);

        /*
         * We must not enqueue too many inrush IRPs in the list.
         * Unlike normal power IRPs the maximum threshold is lower
         * because, while the system can only process one inrush IRP
         * at a time, all the bloat of inrush IRPs would cause too
         * much power consumption.
         */
        QueuedIrpsCount = PopComputeIrpListLength(&PopQueuedInrushIrpList);
        if (QueuedIrpsCount > POP_MAX_INRUSH_IRP_QUEUE_LIST)
        {
            /* This is it, crash the system */
            KeBugCheckEx(INTERNAL_POWER_ERROR, 0x1, 2, POP_MAX_INRUSH_IRP_QUEUE_LIST, 0);
        }
    }
    else
    {
        /* This is a normal power IRP, enqueue it in another list */
        InsertTailList(&PopQueuedIrpList, &Irp->Tail.Overlay.ListEntry);

        /*
         * Same condition case like above. However for normal power
         * IRPs, if drivers request too many of them but do not complete
         * them in an expected time manner, then one or some drivers
         * are buggy. Crash the system.
         */
        QueuedIrpsCount = PopComputeIrpListLength(&PopQueuedIrpList);
        if (QueuedIrpsCount > POP_MAX_INRUSH_IRP_QUEUE_LIST)
        {
            KeBugCheckEx(INTERNAL_POWER_ERROR, 0x1, 3, POP_MAX_IRP_QUEUE_LIST, 0);
        }
    }
}

static
PIRP
PopRemovePowerIrpFromQueue(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ POWER_STATE_TYPE StateType)
{
    PIRP Irp;
    PLIST_ENTRY Entry;
    PIO_STACK_LOCATION IrpStack;
    PEXTENDED_DEVOBJ_EXTENSION DevObjExts;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* The thread who dequeues an IRP must own the IRP lock */
    POP_ASSERT_IRP_LOCK_THREAD_OWNERSHIP();

    /*
     * Dequeue the appropriate IRP based on the properties
     * of the device (aka, is it an inrush device)?
     */
    DevObjExts = IoGetDevObjExtension(DeviceObject);
    if (DevObjExts->PowerFlags & POP_DOE_INRUSH_DEVICE)
    {
        /*
         * This is an inrush device. Loop the queued inrush IRPs
         * list and look for the appropriate IRP the device is
         * interested in.
         */
        for (Entry = PopQueuedInrushIrpList.Flink;
             Entry != &PopQueuedInrushIrpList;
             Entry = Entry->Flink)
        {
            /*
             * Check if this IRP from the queued list is the one the
             * device is currently interested in. We do not care about the
             * power type of the IRP, inrush IRPs are always delivered
             * by devices.
             */
            Irp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);
            IrpStack = IoGetNextIrpStackLocation(Irp);
            if (IrpStack->DeviceObject == DeviceObject)
            {
                /* This is the interested IRP, dequeue it and stop looking */
                RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
                return Irp;
            }
        }
    }
    else
    {
        /* This is a normal power IRP, look within the queued IRPs */
        for (Entry = PopQueuedIrpList.Flink;
             Entry != &PopQueuedIrpList;
             Entry = Entry->Flink)
        {
            /*
             * Look for the appropriate IRP and power type the device is
             * interested in. Typically the power state type is the one
             * coming from the previous IRP that is about to be completed.
             * A device driver cannot request a mix of device and system IRPs
             * simultaneously.
             */
            Irp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);
            IrpStack = IoGetNextIrpStackLocation(Irp);
            if ((IrpStack->DeviceObject == DeviceObject) &&
                (IrpStack->Parameters.Power.Type == StateType))
            {
                /* This is the interested IRP, dequeue it and stop looking */
                RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
                return Irp;
            }
        }
    }

    return NULL;
}

static
VOID
PopClearDoeActiveFlags(
    _In_ POWER_STATE_TYPE StateType,
    _Inout_ PEXTENDED_DEVOBJ_EXTENSION DevObjExts)
{
    /* NULL DOEs are illegal here */
    ASSERT(DevObjExts != NULL);

    /* Check if this is a device type of IRP */
    if (StateType == DevicePowerState)
    {
        /* It is, is this an inrush device? */
        if (DevObjExts->PowerFlags & POP_DOE_INRUSH_DEVICE)
        {
            /* This device was processing an inrush IRP, clear the active flag */
            DevObjExts->PowerFlags &= ~POP_DOE_INRUSH_IRP_ACTIVE;
            return;
        }

        /* Otherwise it was processing a normal device IRP */
        DevObjExts->PowerFlags &= ~POP_DOE_DEVICE_IRP_ACTIVE;
        return;
    }

    /* If we reach here then this device was processing a system IRP */
    DevObjExts->PowerFlags &= ~POP_DOE_SYSTEM_IRP_ACTIVE;
}

static
VOID
PopSetDoeActiveFlags(
    _In_ POWER_STATE_TYPE StateType,
    _Inout_ PEXTENDED_DEVOBJ_EXTENSION DevObjExts)
{
    /* NULL DOEs are illegal here */
    ASSERT(DevObjExts != NULL);

    /* Check if this is a device type of IRP */
    if (StateType == DevicePowerState)
    {
        /* It is, is this an inrush device? */
        if (DevObjExts->PowerFlags & POP_DOE_INRUSH_DEVICE)
        {
            /* This device is about to process an inrush IRP */
            DevObjExts->PowerFlags |= POP_DOE_INRUSH_IRP_ACTIVE;
            return;
        }

        /* Otherwise it is about to process a normal device IRP */
        DevObjExts->PowerFlags |= POP_DOE_DEVICE_IRP_ACTIVE;
        return;
    }

    /* If we reach here then this device is about to process a system IRP */
    DevObjExts->PowerFlags |= POP_DOE_SYSTEM_IRP_ACTIVE;
}

static
VOID
PopSetupIrpWatchdog(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PPOP_IRP_WATCHDOG Watchdog;
    KDPC WatchdogDpc;
    KTIMER Timer;
    LARGE_INTEGER ExpirationTrigger;

    /* We must be held under a IRP lock when setting up watchdogs */
    POP_ASSERT_IRP_LOCK_THREAD_OWNERSHIP();

    /*
     * Allocate memory for the watchdog database. It has to be
     * always resident as power operations can occur at different
     * interrupt levels, usually passive or dispatch. Failing this
     * allocation is fatal.
     */
    Watchdog = PopAllocatePool(sizeof(POP_IRP_WATCHDOG),
                               FALSE,
                               TAG_IRP_WATCHDOG);
    NT_ASSERT(Watchdog != NULL);

    /* Setup the watchdog DPC and timer */
    KeInitializeTimerEx(&Timer, NotificationTimer);
    KeInitializeDpc(&WatchdogDpc, PopIrpWatchdogDpcRoutine, Irp);

    /*
     * Set an expiration duetime to 10 minutes (600 seconds). This should
     * give to the device object plenty of time to finish its current active
     * IRP and whatever IRPs it has queued further.
     */
    ExpirationTrigger.QuadPart = Int32x32To64(POP_IRP_WATCHDOG_DUETIME,
                                              -10 * 1000 * 1000);
    KeSetTimerEx(&Timer, ExpirationTrigger, 0, &WatchdogDpc);

    /* Fill in data, it will be used later once this watchdog is cancelled */
    InitializeListHead(&Watchdog->Link);
    Watchdog->WatchdogTimer = Timer;
    Watchdog->DeviceObject = DeviceObject;
    Watchdog->Irp = Irp;

    /* Finally register this watchdog */
    InsertTailList(&PopIrpWatchdogList, &Watchdog->Link);
}

static
VOID
PopCancelIrpWatchdog(
    _In_ PIRP Irp)
{
    PLIST_ENTRY Entry;
    PPOP_IRP_WATCHDOG Watchdog;

    /* We must be held under a IRP lock when cancelling watchdogs */
    POP_ASSERT_IRP_LOCK_THREAD_OWNERSHIP();

    /* Iterate over the watchdog IRP database list to look for the appropriate IRP */
    for (Entry = PopIrpWatchdogList.Flink;
         Entry != &PopIrpWatchdogList;
         Entry = Entry->Flink)
    {
        Watchdog = CONTAINING_RECORD(Entry, POP_IRP_WATCHDOG, Link);
        if (Watchdog->Irp == Irp)
        {
            /* This is the IRP we were looking for, cancel its watchdog */
            RemoveEntryList(Entry);
            KeCancelTimer(&Watchdog->WatchdogTimer);
            PopFreePool(Watchdog, TAG_IRP_WATCHDOG);
            break;
        }
    }
}

static
NTSTATUS
NTAPI
PopRequestPowerIrpCompletion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
    PIRP DequeuedIrp;
    PIO_STACK_LOCATION Stack;
    PREQUEST_POWER_COMPLETE CompletionRoutine;
    POWER_STATE PowerState;
    POWER_STATE_TYPE Type;
    KLOCK_QUEUE_HANDLE IrpLockHandle;
    PEXTENDED_DEVOBJ_EXTENSION DevObjExts;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    Stack = IoGetCurrentIrpStackLocation(Irp);
    DevObjExts = IoGetDevObjExtension(DeviceObject);
    Type = Stack->Parameters.Power.Type;
    CompletionRoutine = Context;

    /* Dispatch the function completion with the desired power state to the device */
    PowerState.DeviceState = (ULONG_PTR)Stack->Parameters.Others.Argument3;
    if (CompletionRoutine)
    {
        CompletionRoutine(Stack->Parameters.Others.Argument1,
                          (UCHAR)(ULONG_PTR)Stack->Parameters.Others.Argument2,
                          PowerState,
                          Stack->Parameters.Others.Argument4,
                          &Irp->IoStatus);
    }

    /*
     * If the system was processing this exact inrush IRP, release it.
     * Also cancel the watchdog of this IRP as it is completed.
     */
    PopAcquireIrpLock(&IrpLockHandle);
    PopReleaseInrushIrp(Irp);
    PopRemoveOutstandingIrp(Irp);
    PopCancelIrpWatchdog(Irp);

    /* Clear the active flag as this IRP is done */
    PopClearDoeActiveFlags(Type, DevObjExts);

    /*
     * Cleanup this IRP and mark the location as completed. We will
     * have to check soon whether this device has any queued pending
     * IRPs for processing.
     */
    IoSkipCurrentIrpStackLocation(Irp);
    PopMarkIrpCurrentLocationComplete(Irp);
    IoFreeIrp(Irp);
    ObDereferenceObject(DeviceObject);

    /*
     * Now it is time to look for any queued IRPs this device
     * still needs to process them.
     */
    DequeuedIrp = PopRemovePowerIrpFromQueue(DeviceObject, Type);
    if (DequeuedIrp == NULL)
    {
        /* No queued IRP found for this device, take away the pending flag */
        DevObjExts->PowerFlags &= ~POP_DOE_PENDING_PROCESS;
        PopReleaseIrpLock(&IrpLockHandle);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    /*
     * This device has not done processing all the queued IRPs it
     * has requested. If this is an inrush device then this IRP is
     * very special for us.
     */
    if (DevObjExts->PowerFlags & POP_DOE_INRUSH_DEVICE)
    {
        PopSelectInrushIrp(DequeuedIrp);
    }

    /* Set the appropriate active flags for this device based on the IRP */
    PopSetDoeActiveFlags(Type, DevObjExts);

    /* Dispatch the dequeued IRP to the next driver stack */
    PopReleaseIrpLock(&IrpLockHandle);
    IoCallDriver(DeviceObject, DequeuedIrp);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

/* PUBLIC FUNCTIONS **********************************************************/

BOOLEAN
NTAPI
PopHasDoOutstandingIrp(
    _In_ PDEVICE_OBJECT DeviceObject)
{
    /* Simply call the private helper */
    return PopIsDoProcessingPowerIrp(DeviceObject);
}

NTSTATUS
NTAPI
PopRequestPowerIrp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ UCHAR MinorFunction,
    _In_ POWER_STATE PowerState,
    _In_ BOOLEAN IsFxDevice,
    _In_opt_ PREQUEST_POWER_COMPLETE CompletionFunction,
    _In_opt_ __drv_aliasesMem PVOID Context,
    _Outptr_opt_ PIRP *Irp)
{
    PIRP PwrIrp;
    PIO_STACK_LOCATION Stack;
    BOOLEAN IsInrushDevice;
    KLOCK_QUEUE_HANDLE IrpLockHandle;
    PEXTENDED_DEVOBJ_EXTENSION DevObjExts;
    PDEVICE_OBJECT TopDeviceObject;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* FIXME: PoFx isn't implemented yet */
    UNREFERENCED_PARAMETER(IsFxDevice);

    /* Bail out on unrelated minor functions */
    if (MinorFunction != IRP_MN_QUERY_POWER
        && MinorFunction != IRP_MN_SET_POWER
        && MinorFunction != IRP_MN_WAIT_WAKE)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    /* If this is an inrush device then let ourselves acknowledged */
    IsInrushDevice = FALSE;
    DevObjExts = IoGetDevObjExtension(DeviceObject);
    if (DeviceObject->Flags & DO_POWER_INRUSH)
    {
        IsInrushDevice = TRUE;
        DevObjExts->PowerFlags |= POP_DOE_INRUSH_DEVICE;
    }

    /* Allocate the power IRP, we must send it to the top device */
    TopDeviceObject = IoGetAttachedDeviceReference(DeviceObject);
    PwrIrp = IoAllocateIrp(TopDeviceObject->StackSize + 2, FALSE);
    if (!PwrIrp)
    {
        ObDereferenceObject(TopDeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /*
     * Set an initial status information for the newly created IRP.
     * It is just so that the driver does not see bogus data upon
     * receiving the IRP.
     */
    PwrIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    PwrIrp->IoStatus.Information = 0;
    IoSetNextIrpStackLocation(PwrIrp);

    /* Fill in power data to the IRP stack */
    Stack = IoGetNextIrpStackLocation(PwrIrp);
    Stack->Parameters.Others.Argument1 = DeviceObject;
    Stack->Parameters.Others.Argument2 = (PVOID)(ULONG_PTR)MinorFunction;
    Stack->Parameters.Others.Argument3 = (PVOID)(ULONG_PTR)PowerState.DeviceState;
    Stack->Parameters.Others.Argument4 = Context;
    Stack->DeviceObject = TopDeviceObject;
    IoSetNextIrpStackLocation(PwrIrp);

    /* Insert this IRP into outstanding power IRPs list, for debugging purposes */
    ExInterlockedInsertTailList(&PopOutstandingIrpList,
                                &PwrIrp->Tail.Overlay.ListEntry,
                                &PopIrpLock);

    /* Setup major and minor power information for this IRP */
    Stack = IoGetNextIrpStackLocation(PwrIrp);
    Stack->MajorFunction = IRP_MJ_POWER;
    Stack->MinorFunction = MinorFunction;
    if (MinorFunction == IRP_MN_WAIT_WAKE)
    {
        /*
         * You would think that we will set POP_SYS_CONTEXT_SYSTEM_IRP or
         * POP_SYS_CONTEXT_DEVICE_PWR_REQUEST here but consider the following:
         * this is a wake request, whether it is for the system or a device.
         * Setting the wake context here would come in handy, if a bus driver
         * wants to set this IRP as responsible for waking the system, we can
         * adjust the system context flag to woke flag as accordingly.
         */
        Stack->Parameters.WaitWake.PowerState = PowerState.SystemState;
        Stack->Parameters.Power.SystemContext = POP_SYS_CONTEXT_WAKE_REQUEST;
    }
    else
    {
        /*
         * This is a query/set power request by the device. For the shutdown type
         * we fill in the appropriate power action depending on the current system
         * state and current power request operation.
         */
        Stack->Parameters.Power.Type = DevicePowerState;
        Stack->Parameters.Power.State = PowerState;
        Stack->Parameters.Power.SystemContext = POP_SYS_CONTEXT_DEVICE_PWR_REQUEST;
        Stack->Parameters.Power.ShutdownType = PopTranslateDriverIrpPowerAction(&PopAction, FALSE);

        /* FIXME: We must notify the device of this call request and munch with SYSTEM_POWER_STATE_CONTEXT here */
    }

    /* Give the power IRP to the caller if asked */
    if (Irp != NULL)
    {
        *Irp = PwrIrp;
    }

    /* And setup the dedicated power IRP completion routine */
    IoSetCompletionRoutine(PwrIrp,
                           PopRequestPowerIrpCompletion,
                           CompletionFunction,
                           TRUE,
                           TRUE,
                           TRUE);

    /*
     * Now check whether this device has processed a power IRP before
     * and it still does. We must enqueue the new IRP if that is the case.
     * The responsibility is on the driver to finish off all of its power IRPs
     * before the watchdog tears the system apart, or the list will become too clobbed.
     */
    PopAcquireIrpLock(&IrpLockHandle);
    DevObjExts = IoGetDevObjExtension(DeviceObject);
    if (PopIsDoProcessingPowerIrp(DeviceObject))
    {
        /* Enqueue it then */
        DevObjExts->PowerFlags |= POP_DOE_PENDING_PROCESS;
        PopAddPowerIrpToQueue(DeviceObject, PwrIrp);
        PopSetupIrpWatchdog(DeviceObject, PwrIrp);
        PopReleaseIrpLock(&IrpLockHandle);
        return STATUS_PENDING;
    }

    /* This device isn't processing any IRP, check if this is inrush */
    if (IsInrushDevice)
    {
        /*
         * Somebody else may still process an inrush IRP, the system only
         * allows one inrush IRP at a time. Enqueue it if need be but do not
         * set the pending flag, because it is not this device that processes
         * the current inrush IRP.
         */
        if (PopInrushIrp != NULL)
        {
            PopAddPowerIrpToQueue(DeviceObject, PwrIrp);
            PopSetupIrpWatchdog(DeviceObject, PwrIrp);
            PopReleaseIrpLock(&IrpLockHandle);
            return STATUS_PENDING;
        }
        else
        {
            /* Select this as the candidate inrush IRP for process */
            PopSelectInrushIrp(PwrIrp);
        }
    }

    /* Send the IRP to the top device finally */
    PopSetupIrpWatchdog(DeviceObject, PwrIrp);
    PopReleaseIrpLock(&IrpLockHandle);
    PopSetDoeActiveFlags(Stack->Parameters.Power.Type, DevObjExts);
    IoCallDriver(DeviceObject, PwrIrp);
    return STATUS_PENDING;
}

NTSTATUS
NTAPI
PopCreateIrpWorkerThread(
    _In_ PKSTART_ROUTINE WorkerRoutine)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ThreadHandle;
    KPRIORITY Priority = POP_WORKER_THREAD_PRIORITY;

    PAGED_CODE();

    /* Setup the object attributes for the worker thread */
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Give birth to this worker thread */
    Status = PsCreateSystemThread(&ThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  &ObjectAttributes,
                                  NULL,
                                  NULL,
                                  WorkerRoutine,
                                  NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create an IRP power worker thread (Status 0x%lx)", Status);
        return Status;
    }

    /*
     * The worker thread is alive and sound. We have to re-adjust the thread
     * base priority because we are processing power IRPs so we do not want
     * to get delayed that much.
     */
    Status = ZwSetInformationThread(ThreadHandle,
                                    ThreadBasePriority,
                                    &Priority,
                                    sizeof(KPRIORITY));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to set the thread base priority of IRP power worker thread (Status 0x%lx)\n", Status);
        return Status;
    }

    /* The thread servicing the worker routine is already up, close the handle */
    ZwClose(ThreadHandle);
    return STATUS_SUCCESS;
}

NTSTATUS
FASTCALL
PoHandlePowerIrp(
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_OBJECT DeviceObject;
    KLOCK_QUEUE_HANDLE IrpLockHandle;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Make sure this is a Power IRP as we expected */
    IrpStack = IoGetNextIrpStackLocation(Irp);
    POP_ASSERT_IRP_IS_POWER(IrpStack);

    /*
     * The specific driver does not participate in power management,
     * or at least it does not want to. Consider this IRP of this
     * device as done, because there is nothing the Power Manager can do.
     *
     * !!!NOTE!!! -- DO_POWER_NOOP existed in DDK in the past but it got later
     * removed. During that time, there is always a naughty driver who used it
     * and still uses it. The general rule is that a driver must always participate
     * in power management, even if the driver does not do much in terms of power
     * handling.
     *
     * -- https://www-user.tu-chemnitz.de/~heha/oney_wdm/ch08d.htm
     * -- https://www.digiater.nl/openvms/decus/vmslt01b/nt/wdmerr.htm
     * -- Programming the Windows Drivel Model book by Walter Oney
     */
    DeviceObject = IrpStack->DeviceObject;
    if (DeviceObject->Flags & DO_POWER_NOOP)
    {
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    /* Check if we should send this IRP at PASSIVE_LEVEL */
    if (PopShouldForwardIrpAtPassive(IrpStack))
    {
        /*
         * Blocking (synchronous) request. The driver is pageable and we must
         * dispatch the IRP at PASSIVE_LEVEL as ordered. Only if we're at this
         * IRQL that is.
         */
        if (KeGetCurrentIrql() == PASSIVE_LEVEL)
        {
            return PopDispatchPowerIrp(Irp, DeviceObject, IrpStack);
        }
    }

    /*
     * What we have gotten here is either a non-blocking request (we can forward
     * the IRP to the device in the stack at DISPATCH_LEVEL as per our own liking)
     * or the driver is pageable but we are not at the passive interrupt level.
     * Insert the IRP so that an IRP worker thread will handle the request asynchronously.
     */
    PopAcquireIrpLock(&IrpLockHandle);
    PopInsertIrpForDispatch(Irp, IrpStack);
    PopReleaseIrpLock(&IrpLockHandle);
    return STATUS_PENDING;
}

/* EOF */
