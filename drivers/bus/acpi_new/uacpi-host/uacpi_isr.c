/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     SCI delivery and deferred work for the uACPI host
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <uacpi/kernel_api.h>
#include "uacpintsci.h"
#include <debug.h>

/* Seconds between reports while a work drain is stuck */
#define UACPINT_HOST_WORK_REPORT_SECONDS    5

/* Created by uACPI ahead of time, so scheduling never allocates */
typedef struct _UACPINT_HOST_WORK_ITEM
{
    UACPINT_HOST_DEFER Defer;
    WORK_QUEUE_ITEM    Item;
    uacpi_work_handler Handler;
    uacpi_handle       Context;
    uacpi_work_type    Type;
} UACPINT_HOST_WORK_ITEM, *PUACPINT_HOST_WORK_ITEM;

/* Translated SCI resource from START_DEVICE */
static UACPINT_SCI_RESOURCE UacpiNtHostSci;
static BOOLEAN UacpiNtHostSciValid;

/* Recorded while the namespace loads, connected by UacpiNtHostConnectSci */
static PUACPINT_HOST_INTERRUPT UacpiNtHostSciBlock;
static ULONG UacpiNtHostSciGsi;

/*
 * DIRQL of the SCI once connected, DISPATCH_LEVEL before that. The ISR takes
 * the same uACPI locks, so a lock holder must not be preemptible by it.
 */
static KIRQL UacpiNtHostSciLockIrql = DISPATCH_LEVEL;

static SLIST_HEADER UacpiNtHostDeferPending;
static KDPC UacpiNtHostDeferDpc;

static volatile LONG UacpiNtHostWorkOutstanding;
static KEVENT UacpiNtHostWorkDrained;
static BOOLEAN UacpiNtHostWorkReady;

VOID
NTAPI
UacpiNtHostSetSciResource(
    _In_ PUACPINT_SCI_RESOURCE Resource)
{
    UacpiNtHostSci = *Resource;
    UacpiNtHostSciValid = TRUE;
}

/* Raise to the SCI level, uACPI expects these locks to mask its interrupt */
uacpi_cpu_flags
uacpi_kernel_lock_spinlock(
    _In_ uacpi_handle Handle)
{
    KIRQL TargetIrql = UacpiNtHostSciLockIrql;
    KIRQL OldIrql;

    if (KeGetCurrentIrql() > TargetIrql)
        TargetIrql = KeGetCurrentIrql();

    KeRaiseIrql(TargetIrql, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(Handle);
    return (uacpi_cpu_flags)OldIrql;
}

void
uacpi_kernel_unlock_spinlock(
    _In_ uacpi_handle Handle,
    _In_ uacpi_cpu_flags Flags)
{
    KeReleaseSpinLockFromDpcLevel(Handle);
    KeLowerIrql((KIRQL)Flags);
}

static
VOID
NTAPI
UacpiNtHostWorkDone(VOID)
{
    if (InterlockedDecrement(&UacpiNtHostWorkOutstanding) == 0)
        KeSetEvent(&UacpiNtHostWorkDrained, IO_NO_INCREMENT, FALSE);
}

static
VOID
NTAPI
UacpiNtHostWorkRoutine(
    _In_ PVOID Parameter)
{
    PUACPINT_HOST_WORK_ITEM WorkItem = Parameter;
    uacpi_work_handler Handler = WorkItem->Handler;
    uacpi_handle Context = WorkItem->Context;
#if (NTDDI_VERSION >= NTDDI_VISTA)
    KAFFINITY OldAffinity = 0;
#endif
    BOOLEAN Pinned = FALSE;

    /* Some firmware expects GPE methods on CPU0 because of SMI handling */
    if (WorkItem->Type == UACPI_WORK_GPE_EXECUTION)
    {
#if (NTDDI_VERSION >= NTDDI_VISTA)
        OldAffinity = KeSetSystemAffinityThreadEx((KAFFINITY)1);
#else
        KeSetSystemAffinityThread((KAFFINITY)1);
#endif
        Pinned = TRUE;
    }

    /* The handler may reschedule or free the item, it is not ours past here */
    Handler(Context);

    if (Pinned)
    {
#if (NTDDI_VERSION >= NTDDI_VISTA)
        KeRevertToUserAffinityThreadEx(OldAffinity);
#else
        KeRevertToUserAffinityThread();
#endif
    }

    UacpiNtHostWorkDone();
}

/* DISPATCH_LEVEL or below */
static
VOID
NTAPI
UacpiNtHostWorkQueue(
    _In_ PUACPINT_HOST_WORK_ITEM WorkItem)
{
    ExInitializeWorkItem(&WorkItem->Item, UacpiNtHostWorkRoutine, WorkItem);
    ExQueueWorkItem(&WorkItem->Item, DelayedWorkQueue);
}

static
VOID
NTAPI
UacpiNtHostDeferQueue(
    _In_ PUACPINT_HOST_DEFER Entry)
{
    InterlockedPushEntrySList(&UacpiNtHostDeferPending, &Entry->Link);
    KeInsertQueueDpc(&UacpiNtHostDeferDpc, NULL, NULL);
}

/* Releases the units given to a semaphore above DISPATCH_LEVEL */
static
VOID
NTAPI
UacpiNtHostSemaphoreFlush(
    _In_ PUACPINT_HOST_SEMAPHORE Semaphore)
{
    LONG Units;

    Units = InterlockedCompareExchange(&Semaphore->DeferredUnits, 0, 0);
    KeReleaseSemaphore(&Semaphore->Semaphore, IO_NO_INCREMENT, Units, FALSE);

    /* Units given meanwhile found the count nonzero and did not queue it */
    if (InterlockedExchangeAdd(&Semaphore->DeferredUnits, -Units) != Units)
        UacpiNtHostDeferQueue(&Semaphore->Defer);
}

/* Runs at DISPATCH_LEVEL, hands off what was queued above it */
static
VOID
NTAPI
UacpiNtHostDeferRoutine(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    PUACPINT_HOST_DEFER Entry;
    PSLIST_ENTRY ListEntry;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    for (;;)
    {
        ListEntry = ExInterlockedPopEntrySList(&UacpiNtHostDeferPending, NULL);
        if (!ListEntry)
            break;

        Entry = CONTAINING_RECORD(ListEntry, UACPINT_HOST_DEFER, Link);

        if (Entry->Kind == UacpiNtHostDeferSignal)
            UacpiNtHostSemaphoreFlush(CONTAINING_RECORD(Entry, UACPINT_HOST_SEMAPHORE, Defer));
        else
            UacpiNtHostWorkQueue(CONTAINING_RECORD(Entry, UACPINT_HOST_WORK_ITEM, Defer));
    }
}

/* PASSIVE_LEVEL, must run before uACPI is initialized */
VOID
NTAPI
UacpiNtHostDeferInitialize(VOID)
{
    PAGED_CODE();

    if (UacpiNtHostWorkReady)
        return;

    InitializeSListHead(&UacpiNtHostDeferPending);
    KeInitializeDpc(&UacpiNtHostDeferDpc, UacpiNtHostDeferRoutine, NULL);
    KeInitializeEvent(&UacpiNtHostWorkDrained, NotificationEvent, TRUE);
    UacpiNtHostWorkReady = TRUE;
}

uacpi_handle
uacpi_kernel_create_work_item(void)
{
    PUACPINT_HOST_WORK_ITEM WorkItem;

    WorkItem = ExAllocatePoolWithTag(NonPagedPool, sizeof(*WorkItem), UACPINT_HOST_POOL_TAG);
    if (!WorkItem)
        return NULL;

    RtlZeroMemory(WorkItem, sizeof(*WorkItem));
    WorkItem->Defer.Kind = UacpiNtHostDeferWork;
    return WorkItem;
}

void
uacpi_kernel_free_work_item(
    _In_opt_ uacpi_handle Handle)
{
    if (Handle)
        ExFreePoolWithTag(Handle, UACPINT_HOST_POOL_TAG);
}

void
uacpi_kernel_schedule_work(
    _In_ uacpi_work_type Type,
    _In_ uacpi_handle Handle,
    _In_ uacpi_work_handler Handler,
    _In_opt_ uacpi_handle Context)
{
    PUACPINT_HOST_WORK_ITEM WorkItem = Handle;

    WorkItem->Handler = Handler;
    WorkItem->Context = Context;
    WorkItem->Type = Type;

    /* Counted before it can run, so a drain also covers work scheduled by work */
    InterlockedIncrement(&UacpiNtHostWorkOutstanding);

    if (KeGetCurrentIrql() <= DISPATCH_LEVEL)
    {
        UacpiNtHostWorkQueue(WorkItem);
        return;
    }

    /* Work items cannot be queued from DIRQL, let the DPC do it */
    UacpiNtHostDeferQueue(&WorkItem->Defer);
}

void
uacpi_kernel_signal_semaphore(
    _In_ uacpi_handle Handle)
{
    PUACPINT_HOST_SEMAPHORE Semaphore = Handle;

    Semaphore->Owner = NULL;

    if (KeGetCurrentIrql() <= DISPATCH_LEVEL)
    {
        KeReleaseSemaphore(&Semaphore->Semaphore, IO_NO_INCREMENT, 1, FALSE);
        return;
    }

    /* Semaphores cannot be released from DIRQL, the first pending unit queues the DPC */
    if (InterlockedIncrement(&Semaphore->DeferredUnits) == 1)
        UacpiNtHostDeferQueue(&Semaphore->Defer);
}

uacpi_status
uacpi_kernel_wait_for_work_completion(void)
{
    LARGE_INTEGER Interval;
    ULONG Waited = 0;

    /* The deferral DPC first, then the work it queued */
    KeFlushQueuedDpcs();

    if (!UacpiNtHostWorkReady)
        return UACPI_STATUS_OK;

    /* A drain that never finishes is a deadlock, keep reporting it */
    for (;;)
    {
        /* Cleared here and not when scheduling, which may happen at DIRQL */
        KeClearEvent(&UacpiNtHostWorkDrained);
        if (InterlockedCompareExchange(&UacpiNtHostWorkOutstanding, 0, 0) == 0)
            break;

        Interval.QuadPart = -((LONGLONG)UACPINT_HOST_WORK_REPORT_SECONDS * 10 * 1000 * 1000);
        if (KeWaitForSingleObject(&UacpiNtHostWorkDrained,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  &Interval) != STATUS_TIMEOUT)
        {
            continue;
        }

        Waited += UACPINT_HOST_WORK_REPORT_SECONDS;
        DPRINT1("uACPI-NT: %ld work item(s) still pending after %lu s (waiter %p, IRQL %u)\n",
                InterlockedCompareExchange(&UacpiNtHostWorkOutstanding, 0, 0),
                Waited,
                PsGetCurrentThread(),
                (ULONG)KeGetCurrentIrql());
    }

    return UACPI_STATUS_OK;
}

static
BOOLEAN
NTAPI
UacpiNtHostIsr(
    _In_ PKINTERRUPT Interrupt,
    _In_opt_ PVOID ServiceContext)
{
    PUACPINT_HOST_INTERRUPT Block = ServiceContext;

    UNREFERENCED_PARAMETER(Interrupt);

    /* The handler acknowledges PM1 status and masks fired GPEs itself */
    if (Block->Handler(Block->Context) == UACPI_INTERRUPT_HANDLED)
    {
        Block->UnclaimedCount = 0;
        return TRUE;
    }

    /* Not ours, let a shared vector reach its owner */
    Block->UnclaimedCount++;
    return FALSE;
}

uacpi_status
uacpi_kernel_install_interrupt_handler(
    _In_ uacpi_u32 Irq,
    _In_ uacpi_interrupt_handler Handler,
    _In_opt_ uacpi_handle Context,
    _Out_ uacpi_handle *OutIrqHandle)
{
    PUACPINT_HOST_INTERRUPT Block;

    Block = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Block), UACPINT_HOST_POOL_TAG);
    if (!Block)
        return UACPI_STATUS_OUT_OF_MEMORY;

    RtlZeroMemory(Block, sizeof(*Block));
    Block->Handler = Handler;
    Block->Context = Context;

    /* Only recorded here, the vector is not known until IrqLib is up */
    UacpiNtHostSciBlock = Block;
    UacpiNtHostSciGsi = Irq;
    *OutIrqHandle = Block;

    DPRINT1("uACPI-NT: SCI handler recorded for GSI %u, connect deferred\n", Irq);
    return UACPI_STATUS_OK;
}

NTSTATUS
NTAPI
UacpiNtHostConnectSci(VOID)
{
    PUACPINT_HOST_INTERRUPT Block = UacpiNtHostSciBlock;
    KAFFINITY Affinity = 0;
    ULONG Polarity = 0;
    ULONG Vector = 0;
    ULONG Mode = 0;
    KIRQL Irql = 0;
    NTSTATUS Status;

    /* No SCI is not fatal */
    if (!Block)
    {
        DPRINT1("uACPI-NT: no SCI handler recorded\n");
        return STATUS_SUCCESS;
    }

    if (Block->InterruptObject)
        return STATUS_SUCCESS;

    Status = UacpiNtIrqLibResolveVector(UacpiNtHostSciGsi,
                                        &Vector,
                                        &Irql,
                                        &Affinity,
                                        &Polarity,
                                        &Mode);
    if (!NT_SUCCESS(Status) || Vector == 0)
    {
        DPRINT1("uACPI-NT: SCI GSI %lu did not resolve, status 0x%lx vector 0x%lx\n",
                UacpiNtHostSciGsi,
                Status,
                Vector);
        return NT_SUCCESS(Status) ? STATUS_UNSUCCESSFUL : Status;
    }

    Block->Vector = Vector;
    Block->Irql = Irql;
    Block->LevelTriggered = (BOOLEAN)(Mode == LevelSensitive);

    /* The ISR can run from here on, so spinlocks have to mask it */
    UacpiNtHostSciLockIrql = (Irql > DISPATCH_LEVEL) ? Irql : DISPATCH_LEVEL;

    /* uACPI does its own locking, no service spinlock */
    Status = IoConnectInterrupt(&Block->InterruptObject,
                                UacpiNtHostIsr,
                                Block,
                                NULL,
                                Vector,
                                Irql,
                                Irql,
                                (Mode == LevelSensitive) ? LevelSensitive : Latched,
                                TRUE,
                                Affinity,
                                FALSE);

    DPRINT1("uACPI-NT: SCI GSI %lu connected to vector 0x%lx IRQL %u, status 0x%lx\n",
            UacpiNtHostSciGsi,
            Vector,
            (ULONG)Irql,
            Status);
    return Status;
}

uacpi_status
uacpi_kernel_uninstall_interrupt_handler(
    _In_ uacpi_interrupt_handler Handler,
    _In_opt_ uacpi_handle IrqHandle)
{
#if (NTDDI_VERSION >= NTDDI_VISTA)
    IO_DISCONNECT_INTERRUPT_PARAMETERS Parameters;
#endif
    PUACPINT_HOST_INTERRUPT Block = IrqHandle;

    UNREFERENCED_PARAMETER(Handler);

    if (!Block)
        return UACPI_STATUS_INVALID_ARGUMENT;

    if (Block->InterruptObject)
    {
#if (NTDDI_VERSION >= NTDDI_VISTA)
        RtlZeroMemory(&Parameters, sizeof(Parameters));
        Parameters.Version = CONNECT_FULLY_SPECIFIED;
        Parameters.ConnectionContext.InterruptObject = Block->InterruptObject;
        IoDisconnectInterruptEx(&Parameters);
#else
        IoDisconnectInterrupt(Block->InterruptObject);
#endif

        /* Drain the deferral DPC before the block goes away */
        KeFlushQueuedDpcs();
    }

    if (UacpiNtHostSciBlock == Block)
        UacpiNtHostSciBlock = NULL;

    ExFreePoolWithTag(Block, UACPINT_HOST_POOL_TAG);
    return UACPI_STATUS_OK;
}
