/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
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
LIST_ENTRY PopIrpThreadList;
LIST_ENTRY PopIrpDataList;
KSPIN_LOCK PopIrpLock;
PKTHREAD PopIrpOwnerLockThread;
KEVENT PopIrpDispatchPendingEvent;
ULONG PopPendingIrpDispatcWorkerCount;
ULONG PopIrpDispatchWorkerCount;
KSEMAPHORE PopIrpDispatchMasterSemaphore;
BOOLEAN PopIrpDispatchWorkerPending;
ULONG PopIrpWatchdogTickIntervalInSeconds = 1;

/* COMPLETION ROUTINE ********************************************************/

static IO_COMPLETION_ROUTINE PopRequestPowerIrpCompletion;

/* PRIVATE FUNCTIONS *********************************************************/

/**
 * @brief
 * The main watchdog counttdown routine. Each power IRP keeps
 * a routine of which it decrements the watchdog time. If a
 * power IRP stalls down a system for too long than it should,
 * the system will be killed.
 */
_Function_class_(KDEFERRED_ROUTINE)
VOID
NTAPI
PopIrpWatchdogDpcRoutine(
    _In_ PKDPC Dpc,
    _In_ PVOID DeferredContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2)
{
    ULONGLONG TimeRemaining;
    TRIAGE_9F_POWER Triage;
    KLOCK_QUEUE_HANDLE IrpLockHandle;
    PPOP_IRP_DATA IrpData = (PPOP_IRP_DATA)DeferredContext;

    /* We do not care for these parameters */
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    /* Toy around with this watchdog with the IRP lock held */
    PopAcquireIrpLock(&IrpLockHandle);

    /*
     * Decrease the watchdog time by one second. Log to the debugger information
     * about this watchdog if it has reached specific intervals to warn the
     * user of an impending death of the system.
     */
    TimeRemaining = InterlockedDecrementUL(&IrpData->WatchdogStart);
#if DBG
    PopReportWatchdogTime(IrpData);
#endif

    /* Give this system a final breath if the watchdog has expired */
    if (TimeRemaining == 0)
    {
        /* Setup the triage dump info for debugging purposes */
        RtlZeroMemory(&Triage, sizeof(TRIAGE_9F_POWER));
        Triage.Signature = POP_9F_TRIAGE_SIGNATURE;
        Triage.Revision = POP_9F_TRIAGE_REVISION_V1;
        Triage.IrpList = PopDispatchWorkerIrpList;
        Triage.ThreadList = PopIrpThreadList;
        Triage.DelayedWorkerQueue = NULL; // FIXME: We must fill in data once we support KPRIQUEUEs

        /*
         * This device has failed to complete its power IRP in time.
         * Any major delays on power operations from devices indicate a
         * malfunction from their end, which could further badly impact
         * system's startup or shutdown. It is the final countdown.
         */
        KeBugCheckEx(DRIVER_POWER_STATE_FAILURE,
                     0x3,
                     (ULONG_PTR)IrpData->Pdo,
                     (ULONG_PTR)&Triage,
                     (ULONG_PTR)IrpData->Irp);
    }

    PopReleaseIrpLock(&IrpLockHandle);
}

/**
 * @brief
 * Translates the power state of a system into a suitable
 * power action to be taken for the power IRP.
 *
 * @param[in] PowerAction
 * A pointer to a power action structure of which the current
 * given power action is checked against the system power state.
 *
 * @param[in] EjectDevice
 * If set to TRUE, the current device that requests a power IRP
 * is an eject-able device. If set to FALSE, it is just a normal
 * device otherwise.
 *
 * @return
 * Returns the suitable power action to be taken for the IRP.
 *
 * @remarks
 * Generally the power action chosen by the Power Manager is the one
 * that's returned to the power IRP that's requested by a driver.
 * However the system state can override the said action if the
 * system is about to undergo hibernation, or the current device
 * attempts to warm-eject itself.
 */
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

/**
 * @brief
 * Builds a system power state context for the power IRP.
 * Usually a driver reads this information to understand the
 * current state of the machine and what are the effective
 * and target power states going to be. This is so that a driver
 * can take appropriate action when, say, the system is going
 * to sleep very soon.
 *
 * @param[in] Type
 * The type of power state, provided by the caller. This can be
 * either a device state or system state.
 *
 * @return
 * Returns the system power state context which describes the
 * context of the system's current state.
 *
 * @remarks
 * See the FIXME comment below.
 */
static
SYSTEM_POWER_STATE_CONTEXT
PopBuildSystemPowerStateContext(
    _In_ POWER_STATE_TYPE Type)
{
    SYSTEM_POWER_STATE_CONTEXT StateContext;

    /*
     * For drivers, we will always tell the system is at working stage.
     * Whereas for system power IRPs we must tell the user whether
     * a wake-from-hibernation or fast startup was instantiated.
     *
     * FIXME: These values are hardcoded, the system state must be obtained
     * from the global power actions.
     *
     * Specifically, SYSTEM_POWER_STATE_CONTEXT should be declared as a global kernel
     * variable such as PopSystemStateContext where the Power Manager uses such global
     * variable across its subsystem to change the context state of the system.
     */
    RtlZeroMemory(&StateContext, sizeof(SYSTEM_POWER_STATE_CONTEXT));
    if (Type == DevicePowerState)
    {
        StateContext.TargetSystemState = PowerSystemWorking;
        StateContext.EffectiveSystemState = PowerSystemWorking;
        StateContext.CurrentSystemState = PowerSystemWorking;
    }
    else
    {
        /* I do not implement that yet */
        ASSERT(FALSE);
    }

    return StateContext;
}

/**
 * @brief
 * Inserts a power IRP into the dispatcher queue list for
 * handling.
 *
 * @param[in] Irp
 * A pointer to a power IRP of which it is to be enqueued
 * for dispatching.
 *
 * @param[in] CurrentStack
 * A pointer to the current I/O stack location of the device
 * object that it should receive this power IRP.
 *
 * @remarks
 * Power IRPs are enqueued for dispatching when the condiitons
 * to forward them at PASSIVE_LEVEL couldn't be met. The power
 * IRP master dispatcher will allocate a worker thread to dispatch
 * this IRP accordingly. See the PoHandlePowerIrp function for
 * more details.
 */
static
VOID
PopInsertIrpForDispatch(
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION CurrentStack)
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
    DeviceObject = CurrentStack->DeviceObject;
    CurrentStack->Control |= SL_PENDING_RETURNED;
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

/**
 * @brief
 * The worker thread that dispatches the target power
 * IRP to the driver.
 *
 * @param[in] StartContext
 * A pointer to a start parameter context. This is unused.
 *
 * @remarks
 * The power IRP master dispatcher creates each worker thread
 * for each power IRP that must be dispatched. The worker
 * will forward the IRP at a raised IRQL as per our own liking
 * if the driver is not pageable.
 *
 * Note that power IRPs are watchdog enabled, so a driver mustn't
 * take long to handle the IRP. For each power IRP that is pending
 * for dispatch, a worker thread notifies the master dispatcher that
 * a new worker can be created to serve the IRP.
 */
_Function_class_(KSTART_ROUTINE)
VOID
NTAPI
PopIrpDispatchWorker(
    _In_ PVOID StartContext)
{
    PIRP Irp;
    KIRQL OldIrql;
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_OBJECT DeviceObject;
    KLOCK_QUEUE_HANDLE LockHandle;
    POP_IRP_THREAD_ENTRY IrpThreadEntry;
    PLIST_ENTRY Entry;

    PAGED_CODE();
    UNREFERENCED_PARAMETER(StartContext);

    /* Grab the IRP from the list */
    PopAcquireIrpLock(&LockHandle);
    Entry = RemoveHeadList(&PopDispatchWorkerIrpList);
    Irp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);

    /* Setup the IRP thread entry */
    RtlZeroMemory(&IrpThreadEntry, sizeof(POP_IRP_THREAD_ENTRY));
    InitializeListHead(&IrpThreadEntry.Link);
    IrpThreadEntry.Thread = KeGetCurrentThread();
    IrpThreadEntry.Irp = Irp;

    /* And insert this worker thread entry into the list, for debugging purposes */
    InsertTailList(&PopIrpThreadList, &IrpThreadEntry.Link);
    PopReleaseIrpLock(&LockHandle);

    /* Obtain the stack and device object for dispatching */
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
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
    RemoveEntryList(&IrpThreadEntry.Link);
    PopReleaseIrpLock(&LockHandle);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

/**
 * @brief
 * The master dispatcher power IRPs. This is the core worker
 * thread that services worker threads to serve power IRPs.
 *
 * @param[in] StartContext
 * A pointer to a start parameter context. This is unused.
 */
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
            Status = PopCreateWorkerThread(PopIrpDispatchWorker,
                                           NULL,
                                           POP_IRP_WORKER_THREAD_PRIORITY);
            if (!NT_SUCCESS(Status))
            {
                /* Failing to service the IRP on our end is fatal */
                KeBugCheckEx(INTERNAL_POWER_ERROR, 0x1, (ULONG_PTR)Status, 0, 0);
            }

            /* And occupy a slot space for this newly created worker */
            PopIrpDispatchWorkerCount++;

            /* If we have been woken up by the pending event, reset it */
            if (KeReadStateEvent(&PopIrpDispatchPendingEvent) != 0)
            {
                KeClearEvent(&PopIrpDispatchPendingEvent);
            }
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

/**
 * @brief
 * Checks whether power IRPs should be forwarded at the lowest
 * IRQL (PASSIVE_LEVEL).
 *
 * @param[in] Irp
 * A pointer to power IRP to be forwarded at PASSIVE_LEVEL.
 *
 * @return
 * Returns TRUE if the power IRP can be forwarded at PASSIVE_LEVEL,
 * FALSE otherwise.
 */
static
BOOLEAN
PopShouldForwardIrpAtPassive(
    _In_ PIRP Irp)
{
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION IrpStack;

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
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
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

/**
 * @brief
 * Checks if the passed device object has outstanding power
 * IRPs that still need to be processed.
 *
 * @param[in] DeviceObject
 * A pointer to a device object to check for outstanding IRPs.
 *
 * @return
 * Returns TRUE if the following device object is still processing
 * power IRPs, FALSE otherwise.
 */
static
BOOLEAN
PopIsDoProcessingPowerIrp(
    _In_ PDEVICE_OBJECT DeviceObject)
{
    PEXTENDED_DEVOBJ_EXTENSION DevObjExts;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Is this device processing any power IRPs? */
    DevObjExts = IoGetDevObjExtension(DeviceObject);
    if ((DevObjExts->PowerFlags & POP_DOE_DEVICE_IRP_ACTIVE) ||
        (DevObjExts->PowerFlags & POP_DOE_SYSTEM_IRP_ACTIVE))
    {
        return TRUE;
    }

    /* This DO is not processing anything */
    return FALSE;
}

/**
 * @brief
 * Computes the length of a power IRP list.
 *
 * @param[in] IrpListHead
 * A pointer to a list that contains entries of enqueued
 * power IRPs.
 *
 * @return
 * Returns the count of entries that exist in a power IRP list.
 */
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

/**
 * @brief
 * Enqueues an inrush power IRP to the list.
 *
 * @param[in] IrpData
 * A pointer to a power IRP data that contains critical information
 * about the inrush IRP that is to be enqueued.
 *
 * @remarks
 * The Power Manager can only enqueue up to a limited number of inrush
 * IRPs before the system starts to consume so much power.
 */
static
VOID
PopAddInrushIrpToQueue(
    _In_ PPOP_IRP_DATA IrpData)
{
    PPOP_IRP_QUEUE_ENTRY QueueEntry;
    ULONG QueuedIrpsCount;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* The thread who enqueues an IRP must own the IRP lock */
    POP_ASSERT_IRP_LOCK_THREAD_OWNERSHIP();

    /*
     * Allocate a IRP queue entry to hold it up into the inrush
     * IRP list. We are already in the middle of processing the
     * power IRP so this operation must not fail.
     */
    QueueEntry = PopAllocatePool(sizeof(*QueueEntry),
                                 FALSE,
                                 TAG_PO_IRP_QUEUE_ENTRY);
    NT_ASSERT(QueueEntry != NULL);

    /* Insert the queue entry into the inrush queue list */
    InitializeListHead(&QueueEntry->Link);
    QueueEntry->IrpData = IrpData;
    InsertTailList(&PopQueuedInrushIrpList, &QueueEntry->Link);

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
        KeBugCheckEx(INTERNAL_POWER_ERROR,
                     0x1,
                     2,
                     POP_MAX_INRUSH_IRP_QUEUE_LIST,
                     0);
    }
}

/**
 * @brief
 * Enqueues a power IRP to the list.
 *
 * @param[in] IrpData
 * A pointer to a power IRP data that contains critical information
 * about the IRP that is to be enqueued.
 *
 * @remarks
 * The Power Manager can only enqueue up to a limited number of
 * IRPs before the system starts to consume so much power.
 */
static
VOID
PopAddIrpToQueue(
    _In_ PPOP_IRP_DATA IrpData)
{
    PPOP_IRP_QUEUE_ENTRY QueueEntry;
    ULONG QueuedIrpsCount;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* The thread who enqueues an IRP must own the IRP lock */
    POP_ASSERT_IRP_LOCK_THREAD_OWNERSHIP();

    /*
     * Allocate a IRP queue entry to hold it up into the IRP list.
     * We are already in the middle of processing the power IRP so
     * this operation must not fail.
     */
    QueueEntry = PopAllocatePool(sizeof(*QueueEntry),
                                 FALSE,
                                 TAG_PO_IRP_QUEUE_ENTRY);
    NT_ASSERT(QueueEntry != NULL);

    /* Insert the queue entry into the queue list */
    InitializeListHead(&QueueEntry->Link);
    QueueEntry->IrpData = IrpData;
    InsertTailList(&PopQueuedIrpList, &QueueEntry->Link);

    /*
     * Same condition case like above. However for normal power
     * IRPs, if drivers request too many of them but do not complete
     * them in an expected time manner, then one or some drivers
     * are buggy. Crash the system.
     */
    QueuedIrpsCount = PopComputeIrpListLength(&PopQueuedIrpList);
    if (QueuedIrpsCount > POP_MAX_INRUSH_IRP_QUEUE_LIST)
    {
        KeBugCheckEx(INTERNAL_POWER_ERROR,
                     0x1,
                     3,
                     POP_MAX_IRP_QUEUE_LIST,
                     0);
    }
}

/**
 * @brief
 * Dequeues an inrush power IRP that is to be handled by
 * the Power Manager. The enqueue/dequeue mechanism of power IRPs
 * is responsible to forward these IRPs to the master dispatcher
 * accordingly.
 *
 * @param[in] DeviceObject
 * A pointer to a device object that is owning the inrush power IRP.
 * This parameter cannot be NULL if SearchByDevice is set to TRUE.
 *
 * @param[in] SearchByDevice
 * If set to TRUE, this function will look for the inrush IRP
 * to be dequeued that is owned by the pointed device object.
 *
 * @return
 * Returns a pointer to the power IRP data of the inrush IRP.
 * If none is found to be removed from the queue, then NULL is returned.
 */
static
PPOP_IRP_DATA
PopRemoveInrushPowerIrpFromQueue(
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_ BOOLEAN SearchByDevice)
{
    PLIST_ENTRY Entry;
    PPOP_IRP_DATA IrpData;
    PPOP_IRP_QUEUE_ENTRY QueueEntry;
    PEXTENDED_DEVOBJ_EXTENSION DevObjExts;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* The thread who dequeues an IRP must own the IRP lock */
    POP_ASSERT_IRP_LOCK_THREAD_OWNERSHIP();

    /* The list is empty, consider our job as done */
    if (IsListEmpty(&PopQueuedInrushIrpList))
    {
        return NULL;
    }

    /* Delist the previous IRP at the top of the head list */
    if (!SearchByDevice)
    {
        Entry = RemoveHeadList(&PopQueuedInrushIrpList);
        QueueEntry = CONTAINING_RECORD(Entry, POP_IRP_QUEUE_ENTRY, Link);
        IrpData = QueueEntry->IrpData;
        PopFreePool(QueueEntry, TAG_PO_IRP_QUEUE_ENTRY);
        return IrpData;
    }

    /* Cache the DO extensions, make sure this is really an inrush device */
    ASSERT(DeviceObject != NULL);
    DevObjExts = IoGetDevObjExtension(DeviceObject);
    ASSERT(DevObjExts->PowerFlags & POP_DOE_HAS_INRUSH_DEVICE);

    /* Iterate over the list of inrush IRPs and look for the appropriate one */
    for (Entry = PopQueuedInrushIrpList.Flink;
         Entry != &PopQueuedInrushIrpList;
         Entry = Entry->Flink)
    {
        QueueEntry = CONTAINING_RECORD(Entry, POP_IRP_QUEUE_ENTRY, Link);
        IrpData = QueueEntry->IrpData;
        if ((IrpData->Pdo == DeviceObject) &&
            (DevObjExts->PowerFlags & POP_DOE_PENDING_PROCESS))
        {
            /* This is the interested IRP, dequeue it and stop looking */
            RemoveEntryList(&QueueEntry->Link);
            PopFreePool(QueueEntry, TAG_PO_IRP_QUEUE_ENTRY);
            return IrpData;
        }
    }

    return NULL;
}

/**
 * @brief
 * Dequeues a power IRP that is to be handled by the Power Manager.
 * The enqueue/dequeue mechanism of power IRPs is responsible to forward
 * these IRPs to the master dispatcher accordingly.
 *
 * @param[in] DeviceObject
 * A pointer to a device object that is owning the power IRP.
 *
 * @param[in] StateType
 * The type of power state, provided by the caller. The state type must
 * match with that of the power IRP the device is owning. A device can
 * only handle device or system power IRPs, not in-between.
 *
 * @return
 * Returns a pointer to the power IRP data of the IRP.
 * If none is found to be removed from the queue, then NULL is returned.
 */
static
PPOP_IRP_DATA
PopRemovePowerIrpFromQueue(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ POWER_STATE_TYPE StateType)
{
    PPOP_IRP_DATA IrpData;
    PLIST_ENTRY Entry;
    PPOP_IRP_QUEUE_ENTRY QueueEntry;
    PEXTENDED_DEVOBJ_EXTENSION DevObjExts;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* The thread who dequeues an IRP must own the IRP lock */
    POP_ASSERT_IRP_LOCK_THREAD_OWNERSHIP();

    /* The list is empty, consider our job as done */
    if (IsListEmpty(&PopQueuedIrpList))
    {
        return NULL;
    }

    /* Iterate over the list of IRPs and look for the appropriate one */
    DevObjExts = IoGetDevObjExtension(DeviceObject);
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
        QueueEntry = CONTAINING_RECORD(Entry, POP_IRP_QUEUE_ENTRY, Link);
        IrpData = QueueEntry->IrpData;
        if ((IrpData->Pdo == DeviceObject) &&
            (IrpData->PowerStateType == StateType) &&
            (DevObjExts->PowerFlags & POP_DOE_PENDING_PROCESS))
        {
            /* This is the interested IRP, dequeue it and stop looking */
            RemoveEntryList(&QueueEntry->Link);
            PopFreePool(QueueEntry, TAG_PO_IRP_QUEUE_ENTRY);
            return IrpData;
        }
    }

    return NULL;
}

/**
 * @brief
 * Clears the power DOE flags of a device.
 *
 * @param[in] StateType
 * A power state type, this can either be device or system.
 *
 * @param[in,out] DevObjExts
 * A pointer to an extended device object extension of which
 * power flags are cleared.
 */
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
        /* It was processing a normal device IRP */
        DevObjExts->PowerFlags &= ~POP_DOE_DEVICE_IRP_ACTIVE;
        return;
    }

    /* If we reach here then this device was processing a system IRP */
    DevObjExts->PowerFlags &= ~POP_DOE_SYSTEM_IRP_ACTIVE;
}

/**
 * @brief
 * Sets the power DOE flags of a device.
 *
 * @param[in] StateType
 * A power state type, this can either be device or system.
 *
 * @param[in,out] DevObjExts
 * A pointer to an extended device object extension of which
 * power flags are cleared.
 */
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
        /* It is about to process a normal device IRP */
        DevObjExts->PowerFlags |= POP_DOE_DEVICE_IRP_ACTIVE;
        return;
    }

    /* If we reach here then this device is about to process a system IRP */
    DevObjExts->PowerFlags |= POP_DOE_SYSTEM_IRP_ACTIVE;
}

/**
 * @brief
 * Sets a watchdog countdown to a power IRP.
 *
 * @param[in] IrpData
 * A pointer to a power IRP data of which a watchdog
 * is to be set.
 */
static
VOID
PopSetupIrpWatchdog(
    _In_ PPOP_IRP_DATA IrpData)
{
    /* We must be held under a IRP lock when setting up watchdogs */
    POP_ASSERT_IRP_LOCK_THREAD_OWNERSHIP();

    /* Setup the watchdog DPC and timer */
    KeInitializeTimerEx(&IrpData->WatchdogTimer, NotificationTimer);
    KeInitializeDpc(&IrpData->WatchdogDpc, PopIrpWatchdogDpcRoutine, IrpData);
}

/**
 * @brief
 * Enables a watchdog countdown to a power IRP.
 *
 * @param[in] IrpData
 * A pointer to a power IRP data of which a watchdog
 * is to be enabled.
 *
 * @remarks
 * Once a watchdog is enabled for an IRP, the Power Manager
 * starts a timer that ticks by a second. During that point
 * the IRP must be processed as soon as possible.
 */
static
VOID
PopEnableIrpWatchdog(
    _In_ PPOP_IRP_DATA IrpData)
{
    LARGE_INTEGER TickInterval;

    /* We must be held under a IRP lock when enabling watchdogs */
    POP_ASSERT_IRP_LOCK_THREAD_OWNERSHIP();

    /* Set the tick interval for the DPC routine to fire up in 1 second */
    TickInterval.QuadPart = Int32x32To64(PopIrpWatchdogTickIntervalInSeconds,
                                         -10 * 1000 * 1000);

    /* Now enable the watchdog and fire up the timer */
    IrpData->WatchdogEnabled = TRUE;
    KeSetTimerEx(&IrpData->WatchdogTimer,
                 TickInterval,
                 1000, // Recurring interval in 1 second (1000 ms = 1 s)
                 &IrpData->WatchdogDpc);
}

/**
 * @brief
 * Cancels a power IRP watchdog. This is usually done when a power
 * IRP has been completed by the driver.
 *
 * @param[in] IrpData
 * A pointer to a power IRP data of which a watchdog
 * is to be cancelled.
 */
static
VOID
PopCancelIrpWatchdog(
    _In_ PPOP_IRP_DATA IrpData)
{
    /* We must be held under a IRP lock when cancelling watchdogs */
    POP_ASSERT_IRP_LOCK_THREAD_OWNERSHIP();

    /* Cancel the watchdog now */
    IrpData->WatchdogEnabled = FALSE;
    KeCancelTimer(&IrpData->WatchdogTimer);
}

/**
 * @brief
 * Enqueues a power IRP if the target device is still
 * processing IRPs. The IRP will be forwarded immediately
 * to the target top device if the bus driver isn't processing
 * any IRPs.
 *
 * @param[in] IrpData
 * A pointer to a power IRP data that is to be enqueued.
 *
 * @remarks
 * The enqueueing of power IRPs is done on an inrush-normal type
 * basis. For further details, check the code below.
 */
static
VOID
PopQueuePowerIrp(
    _In_ PPOP_IRP_DATA IrpData)
{
    PIRP Irp;
    POWER_STATE_TYPE StateType;
    SYSTEM_POWER_STATE SystemState;
    DEVICE_POWER_STATE DeviceState;
    BOOLEAN HasInrushDevice;
    PIO_STACK_LOCATION Stack;
    KLOCK_QUEUE_HANDLE IrpLockHandle;
    PEXTENDED_DEVOBJ_EXTENSION DevObjExts, PdoDevObjExts;
    PDEVICE_OBJECT TopTargetDevice, PolicyDeviceOwner, LowestDevice;

    /* Cache the associated DOs and IRP stack for later use */
    TopTargetDevice = IrpData->TargetDevice;
    PolicyDeviceOwner = IrpData->Pdo;
    Irp = IrpData->Irp;
    Stack = IoGetCurrentIrpStackLocation(Irp);

    /*
     * Precaution check, make sure the caller did not pass a bogus power state.
     * Passing PowerSystemUnspecified or PowerDeviceUnspecified only makes things
     * confusing for power management.
     */
    if (Stack->Parameters.Power.SystemContext == POP_SYS_CONTEXT_DEVICE_PWR_REQUEST)
    {
        DeviceState = Stack->Parameters.Power.State.DeviceState;
        ASSERT(DeviceState != PowerDeviceUnspecified);
    }
    else // POP_SYS_CONTEXT_WAKE_REQUEST
    {
        SystemState = Stack->Parameters.WaitWake.PowerState;
        ASSERT(SystemState != PowerSystemUnspecified);
    }

    /*
     * Go the faster path, if this is a wake request then dispatch the IRP
     * to the target top device immediately. We do not enable watchdog here
     * because a wake request does not change the power state of a device
     * or system.
     */
    if (IrpData->MinorFunction == IRP_MN_WAIT_WAKE)
    {
        IoCallDriver(TopTargetDevice, Irp);
        return;
    }

    /* This is a Query/Set minor function IRP, we gotta handle the lock */
    PopAcquireIrpLock(&IrpLockHandle);

    /*
     * Loop within the device stack and look for devices that are inrush.
     * No matter if there are more than one inrush devices because at least
     * one is required to consider the whole device stack as inrush.
     */
    HasInrushDevice = FALSE;
    DevObjExts = IoGetDevObjExtension(TopTargetDevice);
    PdoDevObjExts = IoGetDevObjExtension(PolicyDeviceOwner);
    while (DevObjExts->AttachedTo)
    {
        LowestDevice = DevObjExts->AttachedTo;
        DevObjExts = IoGetDevObjExtension(LowestDevice);
        if (LowestDevice->Flags & DO_POWER_INRUSH)
        {
            /*
             * This device is inrush so we have to mark the entire device
             * stack as inrush. We assign the inrush flag to the policy
             * device owner as it is responsible for the device stack chain.
             */
            HasInrushDevice = TRUE;
            PdoDevObjExts->PowerFlags |= POP_DOE_HAS_INRUSH_DEVICE;
            break;
        }
    }

    /* Setup the IRP watchdog but do not enable it yet */
    PopSetupIrpWatchdog(IrpData);

    /*
     * RULE A -- Handling power inrush IRPs.
     *
     * This device wants a surge amount of electricity to power itself up.
     * For that we must assert ourselves the following conditions are met
     * for this to occur.
     *
     * - The caller explicitly requests a change in power with IRP_MN_SET_POWER;
     * - The caller did submit the correct D-state value to power up the device;
     * - The device is not in D0 power state already.
     *
     * Powering up inrush devices is a serialized process. Only the Power Manager
     * can handle an inrush IRP one at a time.
     */
    StateType = IrpData->PowerStateType;
    if (HasInrushDevice &&
        IrpData->MinorFunction == IRP_MN_SET_POWER &&
        DeviceState == PowerDeviceD0 &&
        PopGetDoePowerState(DevObjExts, FALSE) != PowerDeviceD0)
    {
        if (PopInrushIrp == NULL)
        {
            /*
             * We have a perfect candidate of an inrush IRP, let the Power Manager
             * handle it to the device stack now.
             */
            PopSelectInrushIrp(Irp);
            goto DispatchNow;
        }
        else
        {
            /*
             * There is already an inrush IRP of which the Power Manager is currently
             * handling for a device. Either it is this device that inquires another
             * inrush IRP or it is another device, either way, enqueue this IRP.
             */
            PopAddInrushIrpToQueue(IrpData);
            PdoDevObjExts->PowerFlags |= POP_DOE_PENDING_PROCESS;
            PopReleaseIrpLock(&IrpLockHandle);
            return;
        }
    }

    /*
     * RULE B - Handling normal Query/Set power IRPs.
     *
     * The process of handling these power IRPs is done on a per-request serialization
     * basis, unlike inrush devices, the Power Manager can power up multiple devices
     * in parallel. Of course a device can only submit one power request at a time so
     * if it still has to complete a power IRP and it inquires another power IRP, the
     * newly request IRP will be put in queue.
     */
    if (PopIsDoProcessingPowerIrp(PolicyDeviceOwner))
    {
        PopAddIrpToQueue(IrpData);
        PdoDevObjExts->PowerFlags |= POP_DOE_PENDING_PROCESS;
        PopReleaseIrpLock(&IrpLockHandle);
        return;
    }

    /* Time to dispatch this power IRP finally */
DispatchNow:
    PopSetDoeActiveFlags(StateType, PdoDevObjExts);
    PopEnableIrpWatchdog(IrpData);
    PopReleaseIrpLock(&IrpLockHandle);
    IoCallDriver(TopTargetDevice, Irp);
}

/**
 * @brief
 * Dequeues a power IRP if the target device is still
 * processing IRPs. The IRP will be forwarded immediately
 * to the target top device if the bus driver isn't processing
 * any IRPs.
 *
 * @param[in] IrpData
 * A pointer to a power IRP data that is to be dequeued.
 *
 * @remarks
 * The dequeueing of power IRPs is done on an inrush-normal
 * basis basis. For further details, check the code below.
 */
static
VOID
PopDequeuePowerIrp(
    _In_ PPOP_IRP_DATA IrpData)
{
    PIRP Irp;
    POWER_STATE_TYPE StateType;
    KLOCK_QUEUE_HANDLE IrpLockHandle;
    PEXTENDED_DEVOBJ_EXTENSION PdoDevObjExts;
    BOOLEAN IrpFound = FALSE;
    PDEVICE_OBJECT PolicyDeviceOwner;
    PPOP_IRP_DATA DequeuedIrpData, DequeuedInrushIrpData, IrpDataToDispatch = NULL;

    /* Cache the IRP and the associated policy DO */
    Irp = IrpData->Irp;
    PolicyDeviceOwner = IrpData->Pdo;

    /* This was a wake request, return immediately */
    if (IrpData->MinorFunction == IRP_MN_WAIT_WAKE)
    {
        return;
    }

    /* This is a Query/Set power IRP, acquire the lock now */
    PopAcquireIrpLock(&IrpLockHandle);

    /* Cache the policy DO extensions and power state type */
    PdoDevObjExts = IoGetDevObjExtension(PolicyDeviceOwner);
    StateType = IrpData->PowerStateType;

    /* This IRP is in our hands now, disable the watchdog and clear active flags */
    PopCancelIrpWatchdog(IrpData);
    PopClearDoeActiveFlags(StateType, PdoDevObjExts);

    /*
     * Check if this IRP is an inrush power IRP that the Power Manager was
     * currently processing it. We have to look for any queued inrush IRPs
     * this device has inquired. Deploy any other inrush IRPs of other devices
     * so that they can get a chance for their request to be satisfied.
     */
    if (PopInrushIrp == Irp)
    {
        /*
         * RULE A - Dismiss the currently completed inrush IRP and check if this
         * device inquired more than one inrush power request.
         */
        PopReleaseInrushIrp(Irp);
        DequeuedInrushIrpData = PopRemoveInrushPowerIrpFromQueue(PolicyDeviceOwner, TRUE);
        if (!DequeuedInrushIrpData)
        {
            /*
             * RULE B - This device no longer has any queued inrush IRPs to be completed.
             * It is worth noting this device might no longer be an inrush one so we would
             * have to look for any queued normal IRPs this device is still processing but
             * we must first give chance to other inrush devices to complete their IRPs.
             */
            PdoDevObjExts->PowerFlags &= ~POP_DOE_HAS_INRUSH_DEVICE;
            DequeuedInrushIrpData = PopRemoveInrushPowerIrpFromQueue(NULL, FALSE);
            if (DequeuedInrushIrpData)
            {
                /*
                 * We have an inrush IRP that awaits processing (RULE B is in effect). We must gather
                 * new data from this IRP as previous device object no longer applies because it does
                 * not own this IRP.
                 */
                IrpFound = TRUE;
                IrpDataToDispatch = DequeuedInrushIrpData;
                PopSelectInrushIrp(DequeuedInrushIrpData->Irp);
                StateType = DequeuedInrushIrpData->PowerStateType;

                /*
                 * And as this policy device owner does not own this IRP but another policy owner owns
                 * the IRP, we must get the policy DO extensions from that PDO.
                 */
                PdoDevObjExts = IoGetDevObjExtension(DequeuedInrushIrpData->Pdo);
            }
        }
        else
        {
            /* This device still has inrush IRPs to complete (RULE A is in effect), select it for processing */
            IrpFound = TRUE;
            IrpDataToDispatch = DequeuedInrushIrpData;
            PopSelectInrushIrp(DequeuedInrushIrpData->Irp);
        }
    }


    /*
     * RULE C - If we ever get up to this point there could be that the current
     * device was inrush before and we have exhausted all the inrush IRPs of
     * all devices (see RULE B). Or it is just a normal device that may have
     * inquired multiple power requests.
     */
    if (!IrpFound)
    {
        DequeuedIrpData = PopRemovePowerIrpFromQueue(PolicyDeviceOwner, StateType);
        if (!DequeuedIrpData)
        {
            /* This device does not have any queued IRPs, take away the pending flag */
            PdoDevObjExts->PowerFlags &= ~POP_DOE_PENDING_PROCESS;
            PopReleaseIrpLock(&IrpLockHandle);
            return;
        }

        /* This device still has power IRPs to complete (RULE C is in effect) */
        IrpDataToDispatch = DequeuedIrpData;
    }

    /* Deploy the dequeued IRP to the device stack */
    ASSERT(IrpDataToDispatch != NULL);
    PopSetDoeActiveFlags(StateType, PdoDevObjExts);
    PopEnableIrpWatchdog(IrpDataToDispatch);
    PopReleaseIrpLock(&IrpLockHandle);
    IoCallDriver(IrpDataToDispatch->TargetDevice, IrpDataToDispatch->Irp);
}


/**
 * @brief
 * Frees a power IRP data allocated in memory that is
 * no longer used.
 *
 * @param[in] IrpData
 * A pointer to a power IRP data that is to be freed.
 */
static
VOID
PopFreePowerIrp(
    _In_ PPOP_IRP_DATA IrpData)
{
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    KLOCK_QUEUE_HANDLE IrpLockHandle;

    /* Delist the following IRP power data from IRP data list */
    PopAcquireIrpLock(&IrpLockHandle);
    RemoveEntryList(&IrpData->Link);
    PopReleaseIrpLock(&IrpLockHandle);

    /* Cache the referenced top device object before we are going to free the IRP */
    DeviceObject = IrpData->TargetDevice;

    /* Mark the location of this IRP as completed and free it */
    Irp = IrpData->Irp;
    IoSkipCurrentIrpStackLocation(Irp);
    PopMarkIrpCurrentLocationComplete(Irp);
    IoFreeIrp(Irp);

    /*
     * Take one reference away. Note that this device might have more than one
     * reference depending on how many power requests the policy device owner
     * has issued.
     */
    ObDereferenceObject(DeviceObject);

    /* And finally free the IRP power data */
    PopFreePool(IrpData, TAG_PO_IRP_DATA);
}

/**
 * @brief
 * Allocates a power IRP, requested by a bus driver to power
 * up or down a device.
 *
 * @param[in] DeviceObject
 * A pointer to a physical device object (PDO) or a functional
 * device object (FDO) that represents the bus driver of which
 * it wants a power IRP to be allocated.
 *
 * @param[in] IsFxDevice
 * If set to TRUE, the bus driver is registered with Power Framework
 * (PoFx) and it issues a power IRP to change the state of its device
 * components. This condition is set to TRUE if the device registered
 * with Power Framework with a call to PoFxRegisterDevice. If set to
 * FALSE, the bus driver is just for a simple device.
 *
 * @param[in] MinorFunction
 * The power function code, provided by the caller. This can be IRP_MN_QUERY_POWER,
 * IRP_MN_SET_POWER or IRP_MN_WAIT_WAKE that represents the purpose of
 * what is this power IRP attempting to do.
 *
 * @param[in] PowerState
 * The power state that represents the power IRP is powering, either
 * for a device or system state.
 *
 * @param[out] IrpData
 * A pointer to allocated power IRP data, returned to the caller.
 *
 * @return
 * Returns STATUS_SUCCESS if the power IRP has been allocated successfully.
 * Otherwise STATUS_INSUFFICIENT_RESOURCES is returned if there's no enough
 * memory for allocation.
 */
static
NTSTATUS
PopAllocatePowerIrp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ BOOLEAN IsFxDevice,
    _In_ UCHAR MinorFunction,
    _In_ POWER_STATE PowerState,
    _Out_ PPOP_IRP_DATA *IrpData)
{
    PIRP Irp;
    PPOP_IRP_DATA LocalIrpData;
    PDEVICE_OBJECT TopDeviceObject;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Get the top device object of the device stack and allocate the power IRP */
    TopDeviceObject = IoGetAttachedDeviceReference(DeviceObject);
    Irp = IoAllocateIrp(TopDeviceObject->StackSize + 2, FALSE);
    if (!Irp)
    {
        DPRINT1("Failed to allocate memory for the power IRP\n");
        ObDereferenceObject(TopDeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate memory for the power IRP data */
    LocalIrpData = PopAllocatePool(sizeof(*LocalIrpData),
                                   FALSE,
                                   TAG_PO_IRP_DATA);
    if (LocalIrpData == NULL)
    {
        DPRINT1("Failed to allocate memory for the power IRP data\n");
        IoFreeIrp(Irp);
        ObDereferenceObject(TopDeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Is this IRP belonging to a framework device? */
    if (IsFxDevice)
    {
        /* We do not support PoFx yet */
        ASSERT(IsFxDevice == FALSE);
    }

    /* Initialize the basic data */
    InitializeListHead(&LocalIrpData->Link);
    LocalIrpData->Irp = Irp;
    LocalIrpData->Pdo = DeviceObject;
    LocalIrpData->TargetDevice = TopDeviceObject;
    LocalIrpData->CurrentDevice = TopDeviceObject;
    LocalIrpData->WatchdogStart = POP_IRP_WATCHDOG_DUETIME;
    LocalIrpData->PowerState = PowerState;
    LocalIrpData->MinorFunction = MinorFunction;
    LocalIrpData->WatchdogEnabled = FALSE;
    LocalIrpData->FxDevice = NULL;

    /* Initialize device and system related IRP data */
    LocalIrpData->Device.CallerCompletion = NULL;
    LocalIrpData->Device.CallerContext = NULL;
    LocalIrpData->Device.CallerDevice = DeviceObject;
    LocalIrpData->Device.SystemWake = FALSE;

    LocalIrpData->System.NotifyDevice = NULL;
    LocalIrpData->System.FxDeviceActivated = FALSE;

    /* Insert it into the list and give it to caller */
    ExInterlockedInsertTailList(&PopIrpDataList,
                                &LocalIrpData->Link,
                                &PopIrpLock);
    *IrpData = LocalIrpData;
    return STATUS_SUCCESS;
}

/**
 * @brief
 * The core power IRP completion routine of Power Manager.
 * The I/O manager calls this routine whenever a driver completes
 * a power IRP.
 *
 * @param[in] DeviceObject
 * A pointer to a physical device object (PDO) or a functional
 * device object (FDO) that represents the bus driver. This
 * parameter is unused.
 *
 * @param[in] Irp
 * A pointer to a power IRP that's been completed by the driver.
 *
 * @param[in] Context
 * A pointer to a parameter context. This is unused.
 *
 * @return
 * Returns STATUS_MORE_PROCESSING_REQUIRED to acknowledge the I/O manager
 * there's more processing required.
 *
 * @remarks
 * Upon power IRP completion the Power Manager looks for queued power
 * IRPs based on the ownership of this IRP by a device driver and dequeues
 * them for further processing.
 */
static
NTSTATUS
NTAPI
PopRequestPowerIrpCompletion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
    PPOP_IRP_DATA IrpData;
    KLOCK_QUEUE_HANDLE IrpLockHandle;
    PREQUEST_POWER_COMPLETE CompletionRoutine;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /*
     * We do not care about the completion context and DO passed by this
     * routine as we already have the IRP power data that has them.
     */
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Context);

    /* Get the IRP power data, make sure this is not NULL */
    PopAcquireIrpLock(&IrpLockHandle);
    IrpData = PopFindIrpData(Irp, NULL, SearchByIrp);
    ASSERT(IrpData);
    PopReleaseIrpLock(&IrpLockHandle);

    /* Dispatch the function completion with the desired power state to the device */
    CompletionRoutine = IrpData->Device.CallerCompletion;
    if (CompletionRoutine)
    {
        CompletionRoutine(IrpData->Device.CallerDevice,
                          IrpData->MinorFunction,
                          IrpData->PowerState,
                          IrpData->Device.CallerContext,
                          &Irp->IoStatus);
    }

    /*
     * Dequeue any pending power IRPs that this device has requested or
     * from any other device that awaits their power request to be satisfied.
     */
    PopDequeuePowerIrp(IrpData);
    PopFreePowerIrp(IrpData);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

/* PUBLIC FUNCTIONS **********************************************************/

/**
 * @brief
 * Searches for a power IRP data by giving hints such as
 * a device object that owns the IRP or by the IRP itself.
 *
 * @param[in] Irp
 * A pointer to a power IRP, given by the caller.
 *
 * @param[in] DeviceObject
 * A pointer to a device object that owns the IRP. This
 * parameter cannot be NULL if the search method is by device.
 *
 * @param[in] SearchBy
 * The search hint the function uses to understand how to look
 * for the power IRP data. If set to SearchByIrp, the function
 * will search for the power IRP data by the pointed IRP, otherwise
 * if set to SearchByDevice, it'll look for by the pointed device.
 *
 * @return
 * Returns a pointer to the power IRP data that the caller is looking
 * for. NULL is returned if it couldn't find anything.
 */
PPOP_IRP_DATA
NTAPI
PopFindIrpData(
    _In_opt_ PIRP Irp,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_ POP_SEARCH_BY SearchBy)
{
    PLIST_ENTRY Entry;
    PPOP_IRP_DATA IrpData = NULL;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* We must be held under a IRP lock when looking for IRPs data */
    POP_ASSERT_IRP_LOCK_THREAD_OWNERSHIP();

    /* Iterate over the IRP data list and look for the appropriate IRP data */
    for (Entry = PopIrpDataList.Flink;
         Entry != &PopIrpDataList;
         Entry = Entry->Flink)
    {
        IrpData = CONTAINING_RECORD(Entry, POP_IRP_DATA, Link);
        if (SearchBy == SearchByIrp)
        {
            ASSERT(Irp);
            if (IrpData->Irp == Irp)
            {
                break;
            }
        }
        else // SearchByDevice
        {
            ASSERT(DeviceObject);
            if (IrpData->Pdo == DeviceObject)
            {
                break;
            }
        }
    }

    return IrpData;
}

/**
 * @brief
 * Checks if a device has pending power IRPs that still need
 * to be processed.
 *
 * @param[in] DeviceObject
 * A pointer to a device object to look for outstanding power
 * IRPs.
 *
 * @return
 * Returns TRUE if the device still has to process IRPs,
 * FALSE otherwise.
 */
BOOLEAN
NTAPI
PopHasDoOutstandingIrp(
    _In_ PDEVICE_OBJECT DeviceObject)
{
    /* Simply call the private helper */
    return PopIsDoProcessingPowerIrp(DeviceObject);
}

/**
 * @brief
 * Creates a power IRP for the bus driver that is requesting
 * to power up or down devices.
 *
 * @param[in] DeviceObject
 * A pointer to a device object representing a PDO or FDO that
 * requests a power IRP.
 *
 * @param[in] MinorFunction
 * The power function code, given by the caller. This can either
 * be IRP_MN_QUERY_POWER, IRP_MN_SET_POWER or IRP_MN_WAIT_WAKE.
 *
 * @param[in] PowerState
 * Specifies the power state that can either be a device or
 * system state.
 *
 * @param[in] IsFxDevice
 * If set to TRUE, the device in question that requests a power
 * IRP is registered with Power Framework (PoFx). Otherwise if
 * set to FALSE, this acknowledges the Power Manager that it's
 * just a simple device.
 *
 * @param[in] NotifyPEP
 * If set to TRUE, PoFx will notify every Power Extension Plug-ins
 * (PEPs) of upcoming power transitions of involved devices at
 * completion of power IRPs. Set this to FALSE if PEPs mustn't
 * be notified.
 *
 * @param[in] CompletionFunction
 * A pointer to a completion callback routine. The Power Manager
 * calls this callback upon power IRP completion so that the driver
 * can do additional power related tasks. This parameter can be optional.
 *
 * @param[in] Context
 * A pointer to a parameter context that is passed to the completion
 * routine. This parameter can be optional.
 *
 * @param[out] Irp
 * A pointer to a power IRP that's been created and ready to be
 * forwarded to the top device. The Power Manager does that.
 *
 * @return
 * See PoRequestPowerIrp for more details.
 */
NTSTATUS
NTAPI
PopRequestPowerIrp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ UCHAR MinorFunction,
    _In_ POWER_STATE PowerState,
    _In_ BOOLEAN IsFxDevice,
    _In_ BOOLEAN NotifyPEP,
    _In_opt_ PREQUEST_POWER_COMPLETE CompletionFunction,
    _In_opt_ __drv_aliasesMem PVOID Context,
    _Outptr_opt_ PIRP *Irp)
{
    NTSTATUS Status;
    PIRP PwrIrp;
    PPOP_IRP_DATA IrpData;
    PIO_STACK_LOCATION Stack;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Bail out on unrelated minor functions */
    if (MinorFunction != IRP_MN_QUERY_POWER
        && MinorFunction != IRP_MN_SET_POWER
        && MinorFunction != IRP_MN_WAIT_WAKE)
    {
        DPRINT1("Invalid power minor function (%u)\n", MinorFunction);
        return STATUS_INVALID_PARAMETER_2;
    }

    /* Initialize power IRP data for this power request */
    Status = PopAllocatePowerIrp(DeviceObject,
                                 IsFxDevice,
                                 MinorFunction,
                                 PowerState,
                                 &IrpData);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to process the power request for DO (0x%p), IRP data allocation failed\n", DeviceObject);
        return Status;
    }

    /*
     * Assign the notify PEP toggle as this IRP travels within
     * the device stack a PEP gets notified at IRP completion.
     */
    IrpData->NotifyPEP = NotifyPEP;

    /*
     * Usually power requests are inquired by bus drivers, the system power
     * state is not transitioned, the Power Manager is responsible to do that.
     */
    IrpData->SystemTransition = FALSE;

    /* Fill in power request caller context data */
    IrpData->Device.CallerCompletion = CompletionFunction;
    IrpData->Device.CallerContext = Context;
    IrpData->Device.CallerDevice = DeviceObject;

    /*
     * Set an initial status information for the newly created IRP.
     * It is just so that the driver does not see bogus data upon
     * receiving the IRP.
     */
    PwrIrp = IrpData->Irp;
    PwrIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    PwrIrp->IoStatus.Information = 0;
    IoSetNextIrpStackLocation(PwrIrp);

    /* Fill in power data to the IRP stack */
    Stack = IoGetNextIrpStackLocation(PwrIrp);
    Stack->Parameters.Others.Argument1 = DeviceObject;
    Stack->Parameters.Others.Argument2 = (PVOID)(ULONG_PTR)MinorFunction;
    Stack->Parameters.Others.Argument3 = (PVOID)(ULONG_PTR)PowerState.DeviceState;
    Stack->Parameters.Others.Argument4 = Context;
    Stack->DeviceObject = IrpData->TargetDevice;
    IoSetNextIrpStackLocation(PwrIrp);

    /*
     * Check what kind of minor function this is and fill specific power
     * data depending on that function.
     */
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
         * set the "system wake" toggle within IRP data accordingly.
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
        IrpData->PowerStateType = DevicePowerState;
        Stack->Parameters.Power.Type = DevicePowerState;
        Stack->Parameters.Power.State = PowerState;
        Stack->Parameters.Power.SystemContext = POP_SYS_CONTEXT_DEVICE_PWR_REQUEST;
        Stack->Parameters.Power.ShutdownType = PopTranslateDriverIrpPowerAction(&PopAction, FALSE);

        /* Fill in system power state context for the IRP */
        Stack->Parameters.Power.SystemPowerStateContext = PopBuildSystemPowerStateContext(DevicePowerState);
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
     * Now enqueue this power request IRP for later processing. The function
     * will dispatch the IRP immediately if it is a wake request or if it was
     * not processing any IRPs before.
     */
    PopQueuePowerIrp(IrpData);
    return STATUS_PENDING;
}

/**
 * @brief
 * Forwards a power IRP to the device. It's called by the I/O manager
 * when it finds that the IRP in question is a power one that needs
 * to be handled by the Power Manager subsystem.
 *
 * @param[in] Irp
 * A pointer to a power IRP that is to be forwarded to the device.
 *
 * @return
 * Returns STATUS_SUCCESS to indicate the power IRP has been handled
 * successfully. Returns STATUS_PENDING to indicate the power IRP
 * is being handled asynchronously.
 */
NTSTATUS
FASTCALL
PoHandlePowerIrp(
    _In_ PIRP Irp)
{
    PPOP_IRP_DATA IrpData;
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_OBJECT DeviceObject;
    KLOCK_QUEUE_HANDLE IrpLockHandle;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Make sure this is a Power IRP as we expected */
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
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

    /* Update the current device as it currently holds hands on this IRP */
    PopAcquireIrpLock(&IrpLockHandle);
    IrpData = PopFindIrpData(Irp, NULL, SearchByIrp);
    ASSERT(IrpData != NULL);
    IrpData->CurrentDevice = DeviceObject;
    PopReleaseIrpLock(&IrpLockHandle);

    /* Check if we should send this IRP at PASSIVE_LEVEL */
    if (PopShouldForwardIrpAtPassive(Irp))
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
