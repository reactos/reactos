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

/* Preallocated entries for work and signals queued from the ISR */
#define UACPINT_HOST_DEFER_ENTRIES          64

typedef enum _UACPINT_HOST_DEFER_KIND
{
    UacpiNtHostDeferWork = 0,
    UacpiNtHostDeferSignal
} UACPINT_HOST_DEFER_KIND;

/*
 * ISR to DPC hand off. Link has to stay first, SLIST entries need
 * 16 byte alignment on 64 bit.
 */
typedef struct _UACPINT_HOST_DEFER
{
    SLIST_ENTRY             Link;
    UACPINT_HOST_DEFER_KIND Kind;
    uacpi_work_handler      Handler;    ///< UacpiNtHostDeferWork
    uacpi_handle            Context;
    uacpi_work_type         Type;
    uacpi_handle            Event;      ///< UacpiNtHostDeferSignal
} UACPINT_HOST_DEFER, *PUACPINT_HOST_DEFER;

typedef struct _UACPINT_HOST_WORK_ITEM
{
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
static SLIST_HEADER UacpiNtHostDeferFree;
static PUACPINT_HOST_DEFER UacpiNtHostDeferPool;
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

    WorkItem->Handler(WorkItem->Context);

    if (Pinned)
    {
#if (NTDDI_VERSION >= NTDDI_VISTA)
        KeRevertToUserAffinityThreadEx(OldAffinity);
#else
        KeRevertToUserAffinityThread();
#endif
    }

    ExFreePoolWithTag(WorkItem, UACPINT_HOST_POOL_TAG);
    UacpiNtHostWorkDone();
}

/* Runs at DISPATCH_LEVEL, turns entries queued by the ISR into work items and releases */
static
VOID
NTAPI
UacpiNtHostDeferRoutine(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    PUACPINT_HOST_WORK_ITEM WorkItem;
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
        {
            KeReleaseSemaphore(Entry->Event, IO_NO_INCREMENT, 1, FALSE);
        }
        else
        {
            WorkItem = ExAllocatePoolWithTag(NonPagedPool, sizeof(*WorkItem), UACPINT_HOST_POOL_TAG);
            if (WorkItem)
            {
                WorkItem->Handler = Entry->Handler;
                WorkItem->Context = Entry->Context;
                WorkItem->Type = Entry->Type;
                ExInitializeWorkItem(&WorkItem->Item, UacpiNtHostWorkRoutine, WorkItem);
                ExQueueWorkItem(&WorkItem->Item, DelayedWorkQueue);
            }
            else
            {
                /* Drop the count too or the completion wait never drains */
                DPRINT1("uACPI-NT: deferred work dropped, out of pool\n");
                UacpiNtHostWorkDone();
            }
        }

        InterlockedPushEntrySList(&UacpiNtHostDeferFree, &Entry->Link);
    }
}

/* PASSIVE_LEVEL, must run before the SCI is connected */
static
NTSTATUS
NTAPI
UacpiNtHostDeferInitialize(VOID)
{
    ULONG Index;

    PAGED_CODE();

    if (UacpiNtHostWorkReady)
        return STATUS_SUCCESS;

    UacpiNtHostDeferPool = ExAllocatePoolWithTag(NonPagedPool,
                                                 sizeof(*UacpiNtHostDeferPool) * UACPINT_HOST_DEFER_ENTRIES,
                                                 UACPINT_HOST_POOL_TAG);
    if (!UacpiNtHostDeferPool)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(UacpiNtHostDeferPool, sizeof(*UacpiNtHostDeferPool) * UACPINT_HOST_DEFER_ENTRIES);

    InitializeSListHead(&UacpiNtHostDeferPending);
    InitializeSListHead(&UacpiNtHostDeferFree);
    for (Index = 0; Index < UACPINT_HOST_DEFER_ENTRIES; Index++)
        InterlockedPushEntrySList(&UacpiNtHostDeferFree, &UacpiNtHostDeferPool[Index].Link);

    KeInitializeDpc(&UacpiNtHostDeferDpc, UacpiNtHostDeferRoutine, NULL);
    KeInitializeEvent(&UacpiNtHostWorkDrained, NotificationEvent, TRUE);
    UacpiNtHostWorkReady = TRUE;

    return STATUS_SUCCESS;
}

/* Any IRQL */
static
PUACPINT_HOST_DEFER
NTAPI
UacpiNtHostDeferAcquire(VOID)
{
    PSLIST_ENTRY ListEntry;

    if (!UacpiNtHostWorkReady)
        return NULL;

    ListEntry = ExInterlockedPopEntrySList(&UacpiNtHostDeferFree, NULL);
    if (!ListEntry)
        return NULL;

    return CONTAINING_RECORD(ListEntry, UACPINT_HOST_DEFER, Link);
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

uacpi_status
uacpi_kernel_schedule_work(
    _In_ uacpi_work_type Type,
    _In_ uacpi_work_handler Handler,
    _In_opt_ uacpi_handle Context)
{
    PUACPINT_HOST_DEFER Entry;

    Entry = UacpiNtHostDeferAcquire();
    if (!Entry)
        return UACPI_STATUS_OUT_OF_MEMORY;

    Entry->Kind = UacpiNtHostDeferWork;
    Entry->Handler = Handler;
    Entry->Context = Context;
    Entry->Type = Type;

    if (InterlockedIncrement(&UacpiNtHostWorkOutstanding) == 1)
        KeClearEvent(&UacpiNtHostWorkDrained);

    UacpiNtHostDeferQueue(Entry);
    return UACPI_STATUS_OK;
}

void
uacpi_kernel_signal_event(
    _In_ uacpi_handle Handle)
{
    PUACPINT_HOST_DEFER Entry;

    if (KeGetCurrentIrql() <= DISPATCH_LEVEL)
    {
        KeReleaseSemaphore(Handle, IO_NO_INCREMENT, 1, FALSE);
        return;
    }

    /* Semaphores cannot be released from DIRQL, let the DPC do it */
    Entry = UacpiNtHostDeferAcquire();
    if (!Entry)
    {
        DPRINT1("uACPI-NT: event signal dropped, deferral pool exhausted\n");
        return;
    }

    Entry->Kind = UacpiNtHostDeferSignal;
    Entry->Event = Handle;
    UacpiNtHostDeferQueue(Entry);
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

    /*
     * uACPI calls this with its own locks held, so a work item needing one of
     * them never drains. Keep reporting instead of hanging silently.
     */
    while (InterlockedCompareExchange(&UacpiNtHostWorkOutstanding, 0, 0) != 0)
    {
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

    /* The deferral pool has to exist before the line can fire */
    Status = UacpiNtHostDeferInitialize();
    if (!NT_SUCCESS(Status))
        return Status;

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
