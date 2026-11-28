/*
 * PROJECT:     ReactOS TCP/IP protocol driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     FNET kernel port: heap, timer, worker and upcall threads
 * COPYRIGHT:   Copyright 2026 Muhammad Yahya Bin Ehsan Qureshi <yahyauni54@gmail.com>
 */

#include <ntifs.h>
#include <stdio.h>
#include <fnet.h>
#include <stack/fnet_mempool.h>
#include <stack/fnet_netbuf_prv.h>
#include "fnet_ros_port.h"

#define NDEBUG
#include <debug.h>

/* The allocator puts user data one unit after a block header */
C_ASSERT(FNET_CFG_HEAP_ALIGN >= sizeof(fnet_mempool_unit_header_t));
#if defined(_WIN64)
C_ASSERT(FNET_CFG_HEAP_ALIGN >= 16);
#else
C_ASSERT(FNET_CFG_HEAP_ALIGN >= 8);
#endif

/* FNET is not reentrant: one thread at a time runs it, holding FnetMutex */
static KGUARDED_MUTEX FnetMutex;
static PKTHREAD FnetOwner;
static LIST_ENTRY LocalList;

static PKTHREAD WorkerThread;
static KEVENT WorkEvent;
static KEVENT StopEvent;
static KEVENT ReadyEvent;
static LIST_ENTRY WorkList;
static KSPIN_LOCK WorkLock;
static NTSTATUS WorkerInitStatus;
static KPRIORITY WorkerBasePriority;

/* Protected by WorkLock. Set while a waiting caller's priority is lent to the worker. */
static BOOLEAN WorkerPriorityLent;

/* Upcalls take tcpip locks so never run while FnetMutex is held; UpcallMutex keeps them ordered across draining threads */
static PKTHREAD UpcallThread;
static KEVENT UpcallEvent;
static KEVENT UpcallStopEvent;
static LIST_ENTRY UpcallList;
static KSPIN_LOCK UpcallLock;
static KGUARDED_MUTEX UpcallMutex;
static PKTHREAD UpcallOwner;

static PVOID FnetHeap;
static SIZE_T FnetHeapSize;

static
fnet_time_t
FnetPortGetMs(void)
{
    return (fnet_time_t)(KeQueryInterruptTime() / 10000);
}

static const fnet_timer_api_t FnetPortTimerApi =
{
    FnetPortGetMs,
    NULL
};

static
ULONGLONG
FnetPortPhysicalMemoryBytes(VOID)
{
    PPHYSICAL_MEMORY_RANGE Ranges, Range;
    ULONGLONG Total = 0;

    Ranges = MmGetPhysicalMemoryRanges();
    if (!Ranges)
        return 0;

    for (Range = Ranges; Range->BaseAddress.QuadPart || Range->NumberOfBytes.QuadPart; Range++)
        Total += Range->NumberOfBytes.QuadPart;

    ExFreePool(Ranges);
    return Total;
}

static
BOOLEAN
FnetPortReadHeapOverride(
    _Out_ PULONG Value)
{
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters");
    UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"FnetHeapSize");
    OBJECT_ATTRIBUTES Attributes;
    UCHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION Info = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;
    HANDLE Key;
    ULONG Length;
    NTSTATUS Status;

    InitializeObjectAttributes(&Attributes, &KeyName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    Status = ZwOpenKey(&Key, KEY_QUERY_VALUE, &Attributes);
    if (!NT_SUCCESS(Status))
        return FALSE;

    Status = ZwQueryValueKey(Key, &ValueName, KeyValuePartialInformation, Info, sizeof(Buffer), &Length);
    ZwClose(Key);

    if (!NT_SUCCESS(Status) || Info->Type != REG_DWORD || Info->DataLength != sizeof(ULONG))
        return FALSE;

    *Value = *(PULONG)Info->Data;
    return TRUE;
}

static
SIZE_T
FnetPortChooseHeapSize(VOID)
{
    ULONGLONG Size;
    ULONG Override;

    if (FnetPortReadHeapOverride(&Override))
    {
        Size = min(max(Override, FNET_PORT_HEAP_OVERRIDE_MIN), FNET_PORT_HEAP_OVERRIDE_MAX);
        DPRINT1("FNET heap size %lu bytes from the registry\n", (ULONG)Size);
        return (SIZE_T)Size;
    }

    Size = FnetPortPhysicalMemoryBytes() / FNET_PORT_HEAP_RAM_DIVISOR;
    Size = min(max(Size, FNET_PORT_HEAP_MIN), FNET_PORT_HEAP_MAX);
    return (SIZE_T)(Size & ~(ULONGLONG)(PAGE_SIZE - 1));
}

/* Catches a unit size that does not match the block header layout of this architecture */
static
BOOLEAN
FnetPortCheckHeapAlignment(VOID)
{
    static const fnet_size_t Sizes[] = { 1, 3, 13, 17, 100, 1500 };
    void *Blocks[RTL_NUMBER_OF(Sizes)];
    BOOLEAN Ok = TRUE;
    ULONG i;

    for (i = 0; i < RTL_NUMBER_OF(Sizes); i++)
    {
        Blocks[i] = _fnet_malloc(Sizes[i]);
        if (!Blocks[i] || ((ULONG_PTR)Blocks[i] & (FNET_CFG_HEAP_ALIGN - 1)))
        {
            DPRINT1("FNET heap block %p for %lu bytes is misaligned\n", Blocks[i], (ULONG)Sizes[i]);
            Ok = FALSE;
        }
    }
    for (i = 0; i < RTL_NUMBER_OF(Sizes); i++)
    {
        if (Blocks[i])
            _fnet_free(Blocks[i]);
    }
    return Ok;
}

static
BOOLEAN
FnetPortIsWorkerThread(VOID)
{
    return (WorkerThread != NULL) && (KeGetCurrentThread() == WorkerThread);
}

static
BOOLEAN
FnetPortOwnsFnet(VOID)
{
    return FnetOwner == KeGetCurrentThread();
}

static
VOID
FnetPortEnter(VOID)
{
    PKTHREAD Owner;
    KPRIORITY Priority;
    KIRQL OldIrql;

    if (!KeTryToAcquireGuardedMutex(&FnetMutex))
    {
        /* A caller waiting on the worker must not wait behind lower priority work */
        Owner = FnetOwner;
        Priority = KeQueryPriorityThread(KeGetCurrentThread());
        if (Owner && Owner == WorkerThread && Priority > KeQueryPriorityThread(WorkerThread))
        {
            KeAcquireSpinLock(&WorkLock, &OldIrql);
            WorkerPriorityLent = TRUE;
            KeSetPriorityThread(WorkerThread, Priority);
            KeReleaseSpinLock(&WorkLock, OldIrql);
        }
        KeAcquireGuardedMutex(&FnetMutex);
    }
    FnetOwner = KeGetCurrentThread();
}

static
VOID
FnetPortLeave(VOID)
{
    KIRQL OldIrql;

    FnetOwner = NULL;
    KeReleaseGuardedMutex(&FnetMutex);

    if (WorkerPriorityLent && FnetPortIsWorkerThread())
    {
        KeAcquireSpinLock(&WorkLock, &OldIrql);
        WorkerPriorityLent = FALSE;
        KeSetPriorityThread(KeGetCurrentThread(), WorkerBasePriority);
        KeReleaseSpinLock(&WorkLock, OldIrql);
    }
}

static
VOID
FnetPortRunUpcalls(VOID)
{
    PLIST_ENTRY Entry;
    PFNET_PORT_WORK_ITEM Item;

    KeAcquireGuardedMutex(&UpcallMutex);
    UpcallOwner = KeGetCurrentThread();

    while ((Entry = ExInterlockedRemoveHeadList(&UpcallList, &UpcallLock)) != NULL)
    {
        Item = CONTAINING_RECORD(Entry, FNET_PORT_WORK_ITEM, ListEntry);
        Item->Routine(Item->Context);
    }

    UpcallOwner = NULL;
    KeReleaseGuardedMutex(&UpcallMutex);
}

static
VOID
NTAPI
FnetPortUpcallMain(
    _In_ PVOID StartContext)
{
    PVOID WaitObjects[2] = { &UpcallEvent, &UpcallStopEvent };
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(StartContext);

    for (;;)
    {
        Status = KeWaitForMultipleObjects(2, WaitObjects, WaitAny, Executive,
                                          KernelMode, FALSE, NULL, NULL);
        FnetPortRunUpcalls();
        if (Status == STATUS_WAIT_1)
            break;
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}

static
VOID
FnetPortPoll(
    _In_opt_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
    fnet_poll();
}

static
VOID
FnetPortRunLocal(VOID)
{
    PLIST_ENTRY Entry;
    PFNET_PORT_WORK_ITEM Item;

    /* A loopback packet can produce another one; run until none is left */
    while (!IsListEmpty(&LocalList))
    {
        Entry = RemoveHeadList(&LocalList);
        Item = CONTAINING_RECORD(Entry, FNET_PORT_WORK_ITEM, ListEntry);
        Item->Routine(Item->Context);
    }
}

/* One item per hold of the mutex, so synchronous callers get in between packets */
static
VOID
FnetPortDrainWork(VOID)
{
    PLIST_ENTRY Entry;
    PFNET_PORT_WORK_ITEM Item;

    while ((Entry = ExInterlockedRemoveHeadList(&WorkList, &WorkLock)) != NULL)
    {
        Item = CONTAINING_RECORD(Entry, FNET_PORT_WORK_ITEM, ListEntry);
        FnetPortEnter();
        Item->Routine(Item->Context);
        FnetPortRunLocal();
        FnetPortLeave();

        /* Received data completes here instead of waking the upcall thread */
        if (!IsListEmpty(&UpcallList))
            FnetPortRunUpcalls();
    }
}

static
VOID
NTAPI
FnetPortWorkerMain(
    _In_ PVOID StartContext)
{
    struct fnet_init_params Params;
    PVOID WaitObjects[2] = { &WorkEvent, &StopEvent };
    LARGE_INTEGER Timeout;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(StartContext);

    WorkerBasePriority = KeQueryPriorityThread(KeGetCurrentThread());

    RtlZeroMemory(&Params, sizeof(Params));
    Params.netheap_ptr = FnetHeap;
    Params.netheap_size = (fnet_size_t)FnetHeapSize;
    Params.timer_api = &FnetPortTimerApi;

    WorkerInitStatus = (fnet_init(&Params) == FNET_OK) ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
    if (NT_SUCCESS(WorkerInitStatus) && !FnetPortCheckHeapAlignment())
    {
        fnet_release();
        WorkerInitStatus = STATUS_DATATYPE_MISALIGNMENT;
    }
    KeSetEvent(&ReadyEvent, IO_NO_INCREMENT, FALSE);
    if (!NT_SUCCESS(WorkerInitStatus))
        PsTerminateSystemThread(WorkerInitStatus);

    Timeout.QuadPart = -10000LL * FNET_PORT_POLL_MS;

    for (;;)
    {
        Status = KeWaitForMultipleObjects(2, WaitObjects, WaitAny, Executive,
                                          KernelMode, FALSE, &Timeout, NULL);
        if (Status == STATUS_WAIT_1)
            break;

        FnetPortDrainWork();
        FnetPortEnter();
        FnetPortPoll(NULL);
        FnetPortRunLocal();
        FnetPortLeave();
        if (!IsListEmpty(&UpcallList))
            FnetPortRunUpcalls();
    }

    FnetPortDrainWork();
    FnetPortEnter();
    fnet_release();
    FnetPortLeave();

    PsTerminateSystemThread(STATUS_SUCCESS);
}

static
NTSTATUS
FnetPortStartThread(
    _In_ PKSTART_ROUTINE StartRoutine,
    _In_ PKEVENT ThreadStopEvent,
    _Out_ PKTHREAD *Thread)
{
    HANDLE ThreadHandle;
    NTSTATUS Status;

    Status = PsCreateSystemThread(&ThreadHandle, THREAD_ALL_ACCESS, NULL, NULL,
                                  NULL, StartRoutine, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = ObReferenceObjectByHandle(ThreadHandle, THREAD_ALL_ACCESS, *PsThreadType,
                                       KernelMode, (PVOID *)Thread, NULL);
    if (!NT_SUCCESS(Status))
    {
        *Thread = NULL;
        KeSetEvent(ThreadStopEvent, IO_NO_INCREMENT, FALSE);
        ZwWaitForSingleObject(ThreadHandle, FALSE, NULL);
    }
    ZwClose(ThreadHandle);
    return Status;
}

static
VOID
FnetPortStopThread(
    _In_ PKEVENT ThreadStopEvent,
    _Inout_ PKTHREAD *Thread)
{
    if (!*Thread)
        return;

    KeSetEvent(ThreadStopEvent, IO_NO_INCREMENT, FALSE);
    KeWaitForSingleObject(*Thread, Executive, KernelMode, FALSE, NULL);
    ObDereferenceObject(*Thread);
    *Thread = NULL;
}

NTSTATUS
FnetPortInitialize(VOID)
{
    NTSTATUS Status;

    /* Large nonpaged blocks can fail on small or fragmented systems */
    for (FnetHeapSize = FnetPortChooseHeapSize(); ; FnetHeapSize /= 2)
    {
        FnetHeap = ExAllocatePoolWithTag(NonPagedPool, FnetHeapSize, FNET_PORT_TAG);
        if (FnetHeap || FnetHeapSize / 2 < FNET_PORT_HEAP_OVERRIDE_MIN)
            break;
    }
    if (!FnetHeap)
    {
        DPRINT1("Cannot allocate the FNET heap\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    InitializeListHead(&WorkList);
    KeInitializeGuardedMutex(&FnetMutex);
    FnetOwner = NULL;
    InitializeListHead(&LocalList);
    InitializeListHead(&UpcallList);
    KeInitializeSpinLock(&WorkLock);
    KeInitializeSpinLock(&UpcallLock);
    KeInitializeGuardedMutex(&UpcallMutex);
    KeInitializeEvent(&WorkEvent, SynchronizationEvent, FALSE);
    KeInitializeEvent(&StopEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&ReadyEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&UpcallEvent, SynchronizationEvent, FALSE);
    KeInitializeEvent(&UpcallStopEvent, NotificationEvent, FALSE);
    WorkerPriorityLent = FALSE;
    UpcallOwner = NULL;

    /* The worker sets ReadyEvent before it can exit, even when fnet_init fails */
    Status = FnetPortStartThread(FnetPortWorkerMain, &StopEvent, &WorkerThread);
    if (!NT_SUCCESS(Status))
        goto Fail;

    KeWaitForSingleObject(&ReadyEvent, Executive, KernelMode, FALSE, NULL);
    if (!NT_SUCCESS(WorkerInitStatus))
    {
        DPRINT1("FNET initialization failed: 0x%lx\n", WorkerInitStatus);
        KeWaitForSingleObject(WorkerThread, Executive, KernelMode, FALSE, NULL);
        ObDereferenceObject(WorkerThread);
        WorkerThread = NULL;
        Status = WorkerInitStatus;
        goto Fail;
    }

    Status = FnetPortStartThread(FnetPortUpcallMain, &UpcallStopEvent, &UpcallThread);
    if (!NT_SUCCESS(Status))
    {
        FnetPortStopThread(&StopEvent, &WorkerThread);
        goto Fail;
    }

    return STATUS_SUCCESS;

Fail:
    ExFreePoolWithTag(FnetHeap, FNET_PORT_TAG);
    FnetHeap = NULL;
    return Status;
}

VOID
FnetPortShutdown(VOID)
{
    if (!WorkerThread)
        return;

    /* Upcalls call into FNET, so they stop first */
    FnetPortStopThread(&UpcallStopEvent, &UpcallThread);
    FnetPortStopThread(&StopEvent, &WorkerThread);

    ExFreePoolWithTag(FnetHeap, FNET_PORT_TAG);
    FnetHeap = NULL;
}

VOID
FnetPortQueueWork(
    _Inout_ PFNET_PORT_WORK_ITEM Item,
    _In_ PFNET_PORT_WORK_ROUTINE Routine,
    _In_opt_ PVOID Context)
{
    Item->Routine = Routine;
    Item->Context = Context;
    ExInterlockedInsertTailList(&WorkList, &Item->ListEntry, &WorkLock);
    KeSetEvent(&WorkEvent, IO_NO_INCREMENT, FALSE);
}

/* Runs on the calling thread; the caller must not hold any lock that FNET output can need */
VOID
FnetPortRunSync(
    _In_ PFNET_PORT_WORK_ROUTINE Routine,
    _In_opt_ PVOID Context)
{
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    if (FnetPortOwnsFnet())
    {
        Routine(Context);
        return;
    }

    FnetPortEnter();
    Routine(Context);

    /* Loopback traffic caused by the call (a FIN, a whole handshake) reaches FNET before the caller continues */
    FnetPortRunLocal();
    FnetPortLeave();
}

VOID
FnetPortQueueLocal(
    _Inout_ PFNET_PORT_WORK_ITEM Item,
    _In_ PFNET_PORT_WORK_ROUTINE Routine,
    _In_opt_ PVOID Context)
{
    ASSERT(FnetPortOwnsFnet());

    Item->Routine = Routine;
    Item->Context = Context;
    InsertTailList(&LocalList, &Item->ListEntry);
}

VOID
FnetPortDeferUpcall(
    _Inout_ PFNET_PORT_WORK_ITEM Item,
    _In_ PFNET_PORT_WORK_ROUTINE Routine,
    _In_opt_ PVOID Context)
{
    ASSERT(FnetPortOwnsFnet());

    Item->Routine = Routine;
    Item->Context = Context;
    ExInterlockedInsertTailList(&UpcallList, &Item->ListEntry, &UpcallLock);
    if (!FnetPortIsWorkerThread())
        KeSetEvent(&UpcallEvent, IO_NO_INCREMENT, FALSE);
}

VOID
FnetPortDrainUpcalls(VOID)
{
    /* Completions may call back into tcpip, so never drain while holding FNET */
    if (KeGetCurrentIrql() != PASSIVE_LEVEL ||
        !WorkerThread ||
        FnetPortIsWorkerThread() ||
        FnetPortOwnsFnet() ||
        UpcallOwner == KeGetCurrentThread() ||
        IsListEmpty(&UpcallList))
    {
        return;
    }

    FnetPortRunUpcalls();
}

BOOLEAN
FnetPortInsideFnet(VOID)
{
    return FnetPortOwnsFnet();
}

/* FNET debug output; the FNET serial service is not built */

fnet_size_t
fnet_printf(const fnet_char_t *format, ...)
{
#if FNET_CFG_DEBUG
    va_list Args;

    va_start(Args, format);
    vDbgPrintEx(DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, format, Args);
    va_end(Args);
#else
    UNREFERENCED_PARAMETER(format);
#endif
    return 0;
}

fnet_size_t
fnet_println(const fnet_char_t *format, ...)
{
#if FNET_CFG_DEBUG
    va_list Args;

    va_start(Args, format);
    vDbgPrintEx(DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, format, Args);
    va_end(Args);
    DbgPrintEx(DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "\n");
#else
    UNREFERENCED_PARAMETER(format);
#endif
    return 0;
}

void
fnet_putchar(fnet_char_t character)
{
#if FNET_CFG_DEBUG
    DbgPrintEx(DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "%c", character);
#else
    UNREFERENCED_PARAMETER(character);
#endif
}

fnet_size_t
fnet_snprintf(fnet_char_t *str, fnet_size_t size, const fnet_char_t *format, ...)
{
    va_list Args;
    int Length;

    if (size == 0)
        return 0;

    va_start(Args, format);
    Length = _vsnprintf(str, size, format, Args);
    va_end(Args);

    /* _vsnprintf does not terminate on truncation */
    if (Length < 0 || (fnet_size_t)Length >= size)
    {
        str[size - 1] = '\0';
        Length = (int)size - 1;
    }
    return (fnet_size_t)Length;
}

fnet_size_t
fnet_sprintf(fnet_char_t *str, const fnet_char_t *format, ...)
{
    va_list Args;
    int Length;

    va_start(Args, format);
    Length = vsprintf(str, format, Args);
    va_end(Args);

    return (Length < 0) ? 0 : (fnet_size_t)Length;
}

/* Referenced by fnet_isr.c; there are no hardware interrupts to install */
fnet_return_t
fnet_cpu_isr_install(fnet_uint32_t vector_number, fnet_uint32_t priority)
{
    UNREFERENCED_PARAMETER(vector_number);
    UNREFERENCED_PARAMETER(priority);
    return FNET_ERR;
}
