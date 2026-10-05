/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Forward progress work items
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ucx01000.h"

#define NDEBUG
#include <debug.h>

static IO_WORKITEM_ROUTINE_EX UcxWorkItemIoRoutine;

PUCXHUB_WORKITEM
UcxWorkItem::Allocate(
    _In_ UcxController* Controller,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG Flags)
{
    UcxWorkItem* Item;

    Item = (UcxWorkItem*)ExAllocatePoolZero(NonPagedPool, sizeof(*Item), UCX_POOL_TAG);
    if (Item == NULL)
    {
        DPRINT1("Controller %p work item allocation failed\n", Controller);
        return NULL;
    }

    Item->IoWorkItem = IoAllocateWorkItem(DeviceObject);
    if (Item->IoWorkItem == NULL)
    {
        DPRINT1("Controller %p IO work item allocation failed for %p\n", Controller, DeviceObject);
        ExFreePoolWithTag(Item, UCX_POOL_TAG);
        return NULL;
    }

    Item->Controller = Controller;
    Item->DeviceObject = DeviceObject;
    KeInitializeSpinLock(&Item->Lock);
    KeInitializeEvent(&Item->Idle, NotificationEvent, TRUE);
    Item->NeedsFlush = (Flags & UCXHUB_WORKITEM_FLAG_NEEDS_FLUSH) != 0;

    return Item;
}

/** With flush support the caller must have flushed first. */
VOID
UcxWorkItem::Free()
{
    IoFreeWorkItem(IoWorkItem);
    ExFreePoolWithTag(this, UCX_POOL_TAG);
}

VOID
UcxWorkItem::Enqueue(
    _In_ PUCXHUB_WORKITEM_ROUTINE NewRoutine,
    _In_opt_ PVOID NewContext,
    _In_ UCXHUB_WORKITEM_ENQUEUE_OPTIONS Options)
{
    BOOLEAN Queued = FALSE;

    NT_ASSERT(!this->Queued && Routine == NULL);

    Routine = NewRoutine;
    RoutineContext = NewContext;
    this->Queued = TRUE;

    if (NeedsFlush)
    {
        SpinLockGuard Guard(&Lock);

        RunningCount++;
        KeClearEvent(&Idle);
    }

    if (UcxDriver.IoTryQueueWorkItem != NULL)
        Queued = UcxDriver.IoTryQueueWorkItem(IoWorkItem, UcxWorkItemIoRoutine, DelayedWorkQueue, this);

    if (Queued)
        return;

    /* The worker pool is out of threads; fall back to the controller's own thread */
    if (Options == UcxHubWorkItemForwardProgressNotRequired || !Controller->HasWorkerThread())
    {
        IoQueueWorkItemEx(IoWorkItem, UcxWorkItemIoRoutine, DelayedWorkQueue, this);
    }
    else
    {
        DPRINT("Work item %p sent to controller %p system thread\n", this, Controller);
        Controller->QueueOnWorkerThread(this);
    }
}

/** Waits until the last queued run finished touching the item. */
VOID
UcxWorkItem::Flush()
{
    PAGED_CODE();

    if (!NeedsFlush)
    {
        DPRINT1("Flush on work item %p allocated without flush support\n", this);
        NT_ASSERT(FALSE);
        return;
    }

    KeWaitForSingleObject(&Idle, Executive, KernelMode, FALSE, NULL);

    /* The runner signals Idle while holding the lock; let it leave */
    SpinLockGuard Guard(&Lock);
}

VOID
UcxWorkItem::Run()
{
    PUCXHUB_WORKITEM_ROUTINE RunRoutine;
    PDEVICE_OBJECT Object;
    PVOID RunContext;
    BOOLEAN Flush = NeedsFlush;

    Queued = FALSE;
    RunRoutine = Routine;
    RunContext = RoutineContext;
    Routine = NULL;
    RoutineContext = NULL;

    /* Keeps the owner of the device object loaded across its own callback */
    Object = DeviceObject;
    ObReferenceObject(Object);
    RunRoutine(Object, RunContext, this);
    ObDereferenceObject(Object);

    /* Without flush support the routine may already have freed the item */
    if (Flush)
    {
        SpinLockGuard Guard(&Lock);

        if (--RunningCount == 0)
            KeSetEvent(&Idle, IO_NO_INCREMENT, FALSE);
    }
}

static
VOID
NTAPI
UcxWorkItemIoRoutine(
    _In_ PVOID IoObject,
    _In_opt_ PVOID Context,
    _In_ PIO_WORKITEM IoWorkItem)
{
    UNREFERENCED_PARAMETER(IoObject);
    UNREFERENCED_PARAMETER(IoWorkItem);

    ((UcxWorkItem*)Context)->Run();
}
