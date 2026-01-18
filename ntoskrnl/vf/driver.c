/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD
 * FILE:            ntoskrnl/vf/driver.c
 * PROGRAMMER:      Alex Mendoza
 * PURPOSE:         Core Driver Verifier
 */

#include <stdarg.h>
#include <stdio.h>
#include <ntoskrnl.h>
#include <ntddk.h>
#include <ntdef.h>
#include <ntifs.h>
#include <debug.h>
#include <wdm.h>
#include <ndk/vftypes.h>
#include "vf.h"

static
VF_IRP_TRACK*
VfLookupIrp(
    _In_ PIRP Irp
);

static
VF_DMA_ADAPTER_TRACK*
VfLookupDmaAdapter(
    PDMA_ADAPTER Adapter
);


/* ============================================================ 
   INITIALIZE HEADER EXTERN GLOBALS
   ============================================================ */
VF_GLOBAL_STATE VfGlobal = { 0 };
BOOLEAN VfGlobalEnabled = FALSE;  
VF_SETTINGS VfSettings;    
VF_DMA_FAULT_STATE VfDmaFaultState = {
    FALSE,
    0,
    0,
    VfDmaFaultNone
};

/* ============================================================
   INITIALIZATION
   ============================================================ */

VOID NTAPI VfInitialize(VOID) { 
    InitializeListHead(&VfDriverList); 
    KeInitializeSpinLock(&VfDriverListLock); 

    InitializeListHead(&VfIrpTrackList); 
    KeInitializeSpinLock(&VfIrpTrackLock);

    InitializeListHead(&VfIrpHookList); 
    KeInitializeSpinLock(&VfIrpHookLock); 

    InitializeListHead(&VfSpinlockList); 
    KeInitializeSpinLock(&VfSpinlockLock);

    InitializeListHead(&VfDmaAdapterList); 
    KeInitializeSpinLock(&VfDmaLock); 

    InitializeListHead(&VfThreadLockList); 
    KeInitializeSpinLock(&VfThreadLockListLock);

    InitializeListHead(&VfSpinlockDependencyList); 
    KeInitializeSpinLock(&VfSpinlockDepLock); 

    /* TODO: read registry HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Verifier in future commit / when ROS adds it. */ 
    VfGlobalEnabled = TRUE; 
    DPRINT1("VF: Driver Verifier initialized\n"); }

/* ============================================================
   INTERNAL BUGCHECK
   ============================================================ */

UNUSED static VOID VfFailInternal(
    IN PSTR ObjectType,
    IN VF_FAILURE_CLASS FailureClass,
    IN PULONG AssertionControl,
    IN PSTR Message,
    IN PSTR ParamFormat,
    va_list args
)
{
    char buffer[512];

    if (Message)
        DbgPrint("%s\n", Message);

    if (ParamFormat)
    {
        vsnprintf(buffer, sizeof(buffer), ParamFormat, args);
        DbgPrint("%s\n", buffer);
    }

    if (AssertionControl && *AssertionControl)
    {
        switch (FailureClass)
        {
            case VF_BUGCHECK_POOL_OVERFLOW:
                KeBugCheckEx(VF_BUGCHECK_POOL_OVERFLOW,
                             (ULONG_PTR)ParamFormat,
                             (ULONG_PTR)ObjectType,
                             0, 0);
                break;

            case VF_BUGCHECK_IRQL_VIOLATION:
                KeBugCheckEx(VF_BUGCHECK_IRQL_VIOLATION,
                             (ULONG_PTR)ObjectType,
                             0, 0, 0);
                break;

            case VF_FLAG_SPECIAL_POOL:
                KeBugCheckEx(VF_BUGCHECK_SPECIAL_POOL,
                             (ULONG_PTR)ObjectType,
                             (ULONG_PTR)ParamFormat,
                             0, 0);
                break;

            default:
                KeBugCheckEx(VF_BUGCHECK_DRIVER_VIOLATION,
                             (ULONG_PTR)FailureClass,
                             0, 0, (ULONG_PTR)ObjectType);
                break;
        }
    }
}

/* ============================================================
   DRIVER TRACKING
   ============================================================ */

static VF_DRIVER_ENTRY* VfFindDriver(PDRIVER_OBJECT DriverObject)
{
    PLIST_ENTRY Link;
    VF_DRIVER_ENTRY* Entry;

    for (Link = VfDriverList.Flink; Link != &VfDriverList; Link = Link->Flink)
    {
        Entry = CONTAINING_RECORD(Link, VF_DRIVER_ENTRY, ListEntry);
        if (Entry->DriverObject == DriverObject)
            return Entry;
    }
    return NULL;
}

VOID NTAPI VfDriverUnload(PDRIVER_OBJECT DriverObject);

VOID NTAPI VfRegisterDriver(PDRIVER_OBJECT DriverObject)
{
    VF_DRIVER_ENTRY* Entry;
    KIRQL OldIrql;

    if (!VfGlobal.Enabled)
        return;

    Entry = ExAllocatePoolWithTag(NonPagedPool,
                                  sizeof(VF_DRIVER_ENTRY),
                                  TAG_VFDRV);
    if (!Entry)
        return;

    RtlZeroMemory(Entry, sizeof(*Entry));
    Entry->DriverObject = DriverObject;
    Entry->VerifierFlags = VF_FLAG_POOL_TRACKING | VF_FLAG_IRQL_CHECKING | VF_FLAG_SPECIAL_POOL;
    InitializeListHead(&Entry->PoolList);
    KeInitializeSpinLock(&Entry->PoolLock);
    Entry->OriginalUnload = DriverObject->DriverUnload;

    KeAcquireSpinLock(&VfDriverListLock, &OldIrql);
    InsertTailList(&VfDriverList, &Entry->ListEntry);
    KeReleaseSpinLock(&VfDriverListLock, OldIrql);

    DriverObject->DriverUnload = VfDriverUnload;

    DPRINT1("VF: Registered driver %p for verification flags=0x%lx\n",
            DriverObject,
            Entry->VerifierFlags);
}

/* ============================================================
   ADAPTER CHANNEL
   ============================================================ */
VOID
VfAllocateAdapterChannel(
    PDMA_ADAPTER Adapter,
    PDEVICE_OBJECT DeviceObject,
    ULONG NumberOfMapRegisters,
    PDRIVER_CONTROL ExecutionRoutine,
    PVOID Context
)
{
    KIRQL OldIrql;
    VF_DMA_ADAPTER_TRACK* T;

    KeAcquireSpinLock(&VfDmaLock, &OldIrql);

    T = VfFindAdapter(Adapter);
    if (!T)
        KeBugCheckEx(VF_BUGCHECK_DRIVER_VIOLATION, 0, (ULONG_PTR)Adapter, 0, 0);

    if (T->MapRegisterCount != 0)
        KeBugCheckEx(
            VF_BUGCHECK_DRIVER_VIOLATION,
            VfDmaFaultAllocateChannel,
            (ULONG_PTR)Adapter,
            T->MapRegisterCount,
            0
        );

    T->MapRegisterCount = NumberOfMapRegisters;
    T->AdapterReleased = FALSE;

    KeReleaseSpinLock(&VfDmaLock, OldIrql);

    T->OriginalOps.AllocateAdapterChannel(
        Adapter, DeviceObject, NumberOfMapRegisters, ExecutionRoutine, Context
    );
}

PVOID
VfAllocateCommonBuffer(
    PDMA_ADAPTER Adapter,
    ULONG Length,
    PPHYSICAL_ADDRESS LogicalAddress,
    BOOLEAN CacheEnabled
)
{
    VF_DMA_ADAPTER_TRACK* Track;

    VfValidateDmaAdapter(Adapter);
    Track = VfLookupDmaAdapter(Adapter);

    if (VfShouldInjectDmaFault(VfDmaFaultAllocateCommonBuffer))
        return NULL;

    return Track->OriginalOps.AllocateCommonBuffer(
        Adapter,
        Length,
        LogicalAddress,
        CacheEnabled
    );
}

VOID
VfFreeAdapterChannel(
    PDMA_ADAPTER Adapter
)
{
    VfValidateDmaAdapter(Adapter);

    VF_DMA_ADAPTER_TRACK* Track;
    KIRQL OldIrql;

    KeAcquireSpinLock(&VfDmaLock, &OldIrql);
    VfValidateDmaAdapter(Adapter);
    Track = VfLookupDmaAdapter(Adapter);

    if (!Track || Track->MapRegisterCount <= 0)
    {
        KeBugCheckEx(VF_BUGCHECK_DRIVER_VIOLATION,
                     VF_VIOLATION_INVALID_DMA_ADAPTER,
                     (ULONG_PTR)Adapter,
                     Track ? Track->MapRegisterCount : 0,
                     0);
    }

    Track->MapRegisterCount--;
    KeReleaseSpinLock(&VfDmaLock, OldIrql);

    Track->OriginalOps.FreeAdapterChannel(Adapter);
}

PHYSICAL_ADDRESS
NTAPI
VfMapTransfer(
    PDMA_ADAPTER Adapter,
    PMDL Mdl,
    PVOID MapRegisterBase,
    PVOID CurrentVa,
    PULONG Length,
    BOOLEAN WriteToDevice
)
{
    VF_DMA_ADAPTER_TRACK* T;
    PHYSICAL_ADDRESS Pa;

    T = VfLookupDmaAdapter(Adapter);

    if (T)
    {
        if (T->AdapterReleased)
        {
            KeBugCheckEx(
                VF_BUGCHECK_DRIVER_VIOLATION,
                VF_VIOLATION_INVALID_DMA_ADAPTER,
                (ULONG_PTR)Adapter,
                0,
                0
            );
        }

        /* Increment BEFORE mapping (matches Windows behavior) */
        InterlockedIncrement(&T->MapRegisterCount);

        Pa = T->OriginalOps.MapTransfer(
            Adapter,
            Mdl,
            MapRegisterBase,
            CurrentVa,
            Length,
            WriteToDevice
        );
    }
    else
    {
        Pa = Adapter->DmaOperations->MapTransfer(
            Adapter,
            Mdl,
            MapRegisterBase,
            CurrentVa,
            Length,
            WriteToDevice
        );
    }

    return Pa;
}

BOOLEAN
NTAPI
VfFlushAdapterBuffers(
    PDMA_ADAPTER Adapter,
    PMDL Mdl,
    PVOID MapRegisterBase,
    PVOID CurrentVa,
    ULONG Length,
    BOOLEAN WriteToDevice
)
{
    VF_DMA_ADAPTER_TRACK* T;
    LONG Count;

    T = VfLookupDmaAdapter(Adapter);
    if (!T)
        goto CallOriginal;

    Count = InterlockedDecrement(&T->MapRegisterCount);

    if (Count < 0)
    {
        KeBugCheckEx(
            VF_BUGCHECK_DRIVER_VIOLATION,
            VfDmaFaultFlushBuffers,
            (ULONG_PTR)Adapter,
            Count,
            Length
        );
    }

CallOriginal:
    return T
        ? T->OriginalOps.FlushAdapterBuffers(
              Adapter,
              Mdl,
              MapRegisterBase,
              CurrentVa,
              Length,
              WriteToDevice)
        : Adapter->DmaOperations->FlushAdapterBuffers(
              Adapter,
              Mdl,
              MapRegisterBase,
              CurrentVa,
              Length,
              WriteToDevice);
}

VOID
VfEnableDmaFaultInjection(
    VF_DMA_FAULT_TYPE Type,
    ULONG Probability
)
{
    VfDmaFaultState.Enabled     = TRUE;
    VfDmaFaultState.FaultType   = Type;
    VfDmaFaultState.Probability = Probability;
    VfDmaFaultState.Counter     = 0;
}
/* ============================================================
   POOL TRACKING
   ============================================================ */

static VOID VfCheckIrqlForPool(POOL_TYPE PoolType, BOOLEAN IsFree)
{
    KIRQL CurrentIrql = KeGetCurrentIrql();

    if ((PoolType & PagedPool) && (CurrentIrql > APC_LEVEL))
    {
        KeBugCheckEx(VF_BUGCHECK_IRQL_VIOLATION,
                     CurrentIrql,
                     APC_LEVEL,
                     IsFree,
                     0);
    }
    else if (CurrentIrql > DISPATCH_LEVEL)
    {
        KeBugCheckEx(VF_BUGCHECK_IRQL_VIOLATION,
                     CurrentIrql,
                     DISPATCH_LEVEL,
                     IsFree,
                     0);
    }
}

static PVOID
VfAllocateSpecialPool(
    SIZE_T Size,
    PMDL* OutMdl
)
{
    PMDL Mdl;
    PVOID Mapping;
    PHYSICAL_ADDRESS Low = { 0 };
    PHYSICAL_ADDRESS High;
    PHYSICAL_ADDRESS Skip = { 0 };
    PUCHAR Base;
    SIZE_T TotalSize;
    ULONG Pages;
    NTSTATUS Status;

    High.QuadPart = MAXULONG_PTR;

    // We need: 1 guard + data + 1 guard
    TotalSize = Size + (2 * PAGE_SIZE);
    Pages = BYTES_TO_PAGES(TotalSize);

    Mdl = MmAllocatePagesForMdl(Low, High, Skip, Pages * PAGE_SIZE);
    if (!Mdl)
        return NULL;

    Mapping = MmMapLockedPagesSpecifyCache(
        Mdl,
        KernelMode,
        MmCached,
        NULL,
        FALSE,
        NormalPagePriority
    );

    if (!Mapping)
    {
        MmFreePagesFromMdl(Mdl);
        ExFreePool(Mdl);
        return NULL;
    }

    Base = (PUCHAR)Mapping;

    Status = MmProtectMdlSystemAddress(Mdl, PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
        goto Fail;

    Status = MmProtectMdlSystemAddress(
        (PMDL)((PUCHAR)Mdl + 0 * sizeof(MDL)),
        PAGE_NOACCESS
    );
    ASSERT(NT_SUCCESS(Status));

        // ReactOS does not support special pool alloc
        *OutMdl = NULL;
        return ExAllocatePoolWithTag(NonPagedPool, Size, TAG_VFSP);
    ASSERT(NT_SUCCESS(Status));

    *OutMdl = Mdl;

    return Base + (Pages * PAGE_SIZE) - PAGE_SIZE - Size;

Fail:
    MmUnmapLockedPages(Mapping, Mdl);
    MmFreePagesFromMdl(Mdl);
    ExFreePool(Mdl);
    return NULL;
}

PVOID NTAPI VfAllocatePool(
    PDRIVER_OBJECT DriverObject,
    POOL_TYPE PoolType,
    SIZE_T Size,
    ULONG Tag
)
{
    VF_DRIVER_ENTRY* Driver = VfFindDriver(DriverObject);
    VF_POOL_ALLOCATION* Alloc = NULL;
    PVOID Address = NULL;
    PMDL Mdl = NULL;
    BOOLEAN Special = FALSE;
    KIRQL OldIrql;

    // only accept IRQL rules (paged pool cannot be allocated at dispatch level OR higher)
    if (Driver && (Driver->VerifierFlags & VF_FLAG_IRQL_CHECKING))
        VfCheckIrqlForPool(PoolType, FALSE);

    // zerosize allocations
    if (Size == 0)
    {
        // windows returns a dummy non-null pointer for zerosize allocs
        static UCHAR DummyZero;
        Address = &DummyZero;
        goto TrackAllocation; 
    }

    if (Driver && (Driver->VerifierFlags & VF_FLAG_SPECIAL_POOL))
    {
        Address = VfAllocateSpecialPool(Size, &Mdl);
        Special = TRUE;

        if (!Address)
        {
            VfFailDriver(DriverObject, "Special pool allocation failed");
            return NULL;
        }
    }
    else
    {
        Address = ExAllocatePoolWithTag(PoolType, Size, Tag);
        if (!Address)
        {
            if (Driver)
                VfFailDriver(DriverObject, "Pool allocation failed");
            return NULL;
        }
    }

TrackAllocation:
    if (!Address || !Driver)
        return Address;

    Alloc = ExAllocatePoolWithTag(NonPagedPool, sizeof(VF_POOL_ALLOCATION), TAG_VFALL);
    if (!Alloc)
    {
        return Address;
    }

    Alloc->Address      = Address;
    Alloc->Size         = Size;
    Alloc->Tag          = Tag;
    Alloc->SpecialPool  = Special;
    Alloc->Mdl          = Mdl;
    Alloc->AllocateIrql = KeGetCurrentIrql();

    KeAcquireSpinLock(&Driver->PoolLock, &OldIrql);
    InsertTailList(&Driver->PoolList, &Alloc->ListEntry);
    KeReleaseSpinLock(&Driver->PoolLock, OldIrql);

    return Address;
}

VOID NTAPI VfFreePool(PDRIVER_OBJECT DriverObject, PVOID Address)
{
    VF_DRIVER_ENTRY* Driver = VfFindDriver(DriverObject);
    VF_POOL_ALLOCATION* Alloc;
    PLIST_ENTRY Link;
    KIRQL OldIrql;

    if (!Driver)
        KeBugCheckEx(VF_BUGCHECK_INVALID_FREE, (ULONG_PTR)Address, 0, 0, 0);

    KeAcquireSpinLock(&Driver->PoolLock, &OldIrql);

    for (Link = Driver->PoolList.Flink; Link != &Driver->PoolList; Link = Link->Flink)
    {
        Alloc = CONTAINING_RECORD(Link, VF_POOL_ALLOCATION, ListEntry);
        if (Alloc->Address == Address)
        {
            RemoveEntryList(&Alloc->ListEntry);
            KeReleaseSpinLock(&Driver->PoolLock, OldIrql);

            if (Driver->VerifierFlags & VF_FLAG_IRQL_CHECKING)
            {
                if (KeGetCurrentIrql() > Alloc->AllocateIrql)
                    KeBugCheckEx(VF_BUGCHECK_IRQL_VIOLATION,
                                 KeGetCurrentIrql(),
                                 Alloc->AllocateIrql,
                                 (ULONG_PTR)DriverObject,
                                 (ULONG_PTR)Address);
            }

            if (Alloc->SpecialPool)
            {
                MmUnmapLockedPages(Alloc->Address, Alloc->Mdl);
                MmFreePagesFromMdl(Alloc->Mdl);
                ExFreePool(Alloc->Mdl);
            }
            else
            {
                ExFreePool(Address);
            }

            ExFreePool(Alloc);
            return;
        }
    }

    KeReleaseSpinLock(&Driver->PoolLock, OldIrql);
    VfCheckPageableCode((PVOID)VfFreePool, DriverObject);
    KeBugCheckEx(VF_BUGCHECK_INVALID_FREE, (ULONG_PTR)Address, 0, 0, 0);
}

/* ============================================================
   IRP
   ============================================================ */
static VOID
VfTrackIrpDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    VF_IRP_TRACK* Track;
    KIRQL OldIrql;
    PLIST_ENTRY L;

    KeAcquireSpinLock(&VfIrpTrackLock, &OldIrql);

    // IRP reuse detection
    for (L = VfIrpTrackList.Flink; L != &VfIrpTrackList; L = L->Flink)
    {
        Track = CONTAINING_RECORD(L, VF_IRP_TRACK, ListEntry);
        if (Track->Irp == Irp)
        {
            KeBugCheckEx(
                VF_BUGCHECK_DRIVER_VIOLATION,
                VF_VIOLATION_REUSED_IRP,
                (ULONG_PTR)Irp,
                (ULONG_PTR)Track->DriverObject,
                (ULONG_PTR)DeviceObject->DriverObject
            );
        }
    }

    Track = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(VF_IRP_TRACK),
        'rIrV'
    );

    if (!Track)
        KeBugCheckEx(VF_BUGCHECK_DRIVER_VIOLATION, 0, 0, 0, 0);

    Track->Irp            = Irp;
    Track->DriverObject   = DeviceObject->DriverObject;
    Track->MajorFunction  = IoGetCurrentIrpStackLocation(Irp)->MajorFunction;
    Track->DispatchIrql   = KeGetCurrentIrql();
    Track->PendingReturned = FALSE;
    Track->Completed      = FALSE;

    InsertTailList(&VfIrpTrackList, &Track->ListEntry);

    KeReleaseSpinLock(&VfIrpTrackLock, OldIrql);
}

UNUSED static
NTSTATUS
NTAPI
VfIrpDispatchHook(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION Stack;
    PDRIVER_DISPATCH Original = NULL;
    VF_IRP_HOOK* Hook;
    KIRQL OldIrql;
    NTSTATUS Status;
    VF_IRP_TRACK* Track;
    PLIST_ENTRY L;

    /* sanity check pointers first */
    if (!DeviceObject || !DeviceObject->DriverObject || !Irp)
    {
#if DBG
        DbgPrint("VF: Invalid IRP or device object passed to hook!\n");
#endif
        return STATUS_INVALID_PARAMETER;
    }

    if (KeGetCurrentIrql() > DISPATCH_LEVEL)
    {
        KeBugCheckEx(
            IRQL_NOT_LESS_OR_EQUAL,
            KeGetCurrentIrql(),
            DISPATCH_LEVEL,
            (ULONG_PTR)DeviceObject,
            (ULONG_PTR)Irp
        );
    }

    VfTrackIrpDispatch(DeviceObject, Irp);

    Stack = IoGetCurrentIrpStackLocation(Irp);

    if (Irp->CurrentLocation == 0 ||
        Irp->CurrentLocation > Irp->StackCount)
    {
        KeBugCheckEx(
            VF_BUGCHECK_DRIVER_VIOLATION,
            VF_VIOLATION_INVALID_IRP_STATE,
            (ULONG_PTR)Irp,
            Irp->CurrentLocation,
            Irp->StackCount
        );
    }

    /* major function sanity */
    if (Stack->MajorFunction > IRP_MJ_MAXIMUM_FUNCTION)
    {
        KeBugCheckEx(VF_BUGCHECK_DRIVER_VIOLATION,
                     VF_VIOLATION_IRQL_MISUSE,
                     Stack->MajorFunction,
                     0,
                     (ULONG_PTR)Irp);
    }

    /* IRQL check */
    if (KeGetCurrentIrql() > DISPATCH_LEVEL)
    {
        KeBugCheckEx(IRQL_NOT_LESS_OR_EQUAL,
                     KeGetCurrentIrql(),
                     DISPATCH_LEVEL,
                     (ULONG_PTR)DeviceObject,
                     (ULONG_PTR)Irp);
    }

    /* Increment reference for tracking */
    VfIoIncrementRef(Irp);

    /* Find original dispatch */
    KeAcquireSpinLock(&VfIrpHookLock, &OldIrql);
    for (L = VfIrpHookList.Flink;
         L != &VfIrpHookList;
         L = L->Flink)
    {
        Hook = CONTAINING_RECORD(L, VF_IRP_HOOK, ListEntry);
        if (Hook->DriverObject == DeviceObject->DriverObject)
        {
            Original = Hook->OriginalMajor[Stack->MajorFunction];
            break;
        }
    }
    KeReleaseSpinLock(&VfIrpHookLock, OldIrql);

    if (!Original)
    {
        // no dispatch? complete immediately then
        VfIoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NOT_SUPPORTED;
    }

    /* irp reuse detection */
    if (Irp->IoStatus.Status != STATUS_PENDING &&
        Irp->IoStatus.Status != 0)
    {
        KeBugCheckEx(
            VF_BUGCHECK_DRIVER_VIOLATION,
            VF_VIOLATION_INVALID_IRP_STATE,
            (ULONG_PTR)Irp,
            Irp->IoStatus.Status,
            0
        );
    }

    if (Irp->Cancel && !Irp->CancelRoutine)
    {
        KeBugCheckEx(
            VF_BUGCHECK_DRIVER_VIOLATION,
            VF_VIOLATION_CANCEL_ROUTINE_MISSING,
            (ULONG_PTR)Irp,
            0,
            0
        );
    }

    /* Call original driver */
    Status = Original(DeviceObject, Irp);

    KeAcquireSpinLock(&VfIrpTrackLock, &OldIrql);

    Track = VfLookupIrp(Irp);

    if (!Track)
        KeBugCheckEx(VF_BUGCHECK_DRIVER_VIOLATION, 0, (ULONG_PTR)Irp, 0, 0);

    if (Status == STATUS_PENDING)
    {
        if (!Irp->PendingReturned)
            KeBugCheckEx(
                VF_BUGCHECK_DRIVER_VIOLATION,
                VF_VIOLATION_IRP_NOT_MARKED_PENDING,
                (ULONG_PTR)Irp,
                0,
                0
            );

        Track->PendingReturned = TRUE;
    }
    else
    {
        if (Irp->PendingReturned)
            KeBugCheckEx(
                VF_BUGCHECK_DRIVER_VIOLATION,
                VF_VIOLATION_INVALID_IRP_STATE,
                (ULONG_PTR)Irp,
                Status,
                0
            );
    }

    KeReleaseSpinLock(&VfIrpTrackLock, OldIrql);

    /* SUCCESS but IoStatus.Status not set */
    if (NT_SUCCESS(Status) && Irp->IoStatus.Status == STATUS_PENDING)
    {
        KeBugCheckEx(
            VF_BUGCHECK_DRIVER_VIOLATION,
            VF_VIOLATION_INVALID_IRP_STATE,
            (ULONG_PTR)Irp,
            Status,
            0
        );
    }

    if (Status == STATUS_PENDING && !Irp->PendingReturned)
    {
        KeBugCheckEx(
            VF_BUGCHECK_DRIVER_VIOLATION,
            VF_VIOLATION_IRP_NOT_MARKED_PENDING,
            (ULONG_PTR)Irp,
            0,
            0
        );
    }

    /* If driver returned success but IRP is still marked pending, this is a violation */
    if (Status != STATUS_PENDING && Irp->PendingReturned)
    {
        KeBugCheckEx(VF_BUGCHECK_DRIVER_VIOLATION,
                     VF_VIOLATION_IRQL_MISUSE,
                     (ULONG_PTR)Irp,
                     Status,
                     0);
    }

    return Status;
}

UNUSED static VOID VfHookDriverIrps(PDRIVER_OBJECT DriverObject)
{
    VF_IRP_HOOK* Hook;
    KIRQL OldIrql;

    // alloc hook struct
    Hook = ExAllocatePoolWithTag(NonPagedPool,
                                 sizeof(VF_IRP_HOOK),
                                 'hIrV');
    if (!Hook)
        KeBugCheckEx(VF_BUGCHECK_DRIVER_VIOLATION, VF_VIOLATION_LEAKED_RESOURCES, (ULONG_PTR)DriverObject, 0, 0);

    // zero the structure to avoid uninitialized mem
    RtlZeroMemory(Hook, sizeof(VF_IRP_HOOK));

    Hook->DriverObject = DriverObject;

    // hook EVERY major function
    for (ULONG i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        // save the original dispatch
        Hook->OriginalMajor[i] = DriverObject->MajorFunction[i];

        // wrap with our verifier hook
        DriverObject->MajorFunction[i] = VfIrpDispatchHook;
    }

    // insert hook into the global list
    KeAcquireSpinLock(&VfIrpHookLock, &OldIrql);
    InsertTailList(&VfIrpHookList, &Hook->ListEntry);
    KeReleaseSpinLock(&VfIrpHookLock, OldIrql);

    DPRINT1("VF: IRP dispatch hooks installed for driver %p\n", DriverObject);
}

static
VF_IRP_TRACK*
VfLookupIrp(
    _In_ PIRP Irp
)
{
    PLIST_ENTRY Entry;
    VF_IRP_TRACK* Track;
    KIRQL OldIrql;

    KeAcquireSpinLock(&VfIrpTrackLock, &OldIrql);

    for (Entry = VfIrpTrackList.Flink;
         Entry != &VfIrpTrackList;
         Entry = Entry->Flink)
    {
        Track = CONTAINING_RECORD(Entry, VF_IRP_TRACK, ListEntry);
        if (Track->Irp == Irp)
        {
            KeReleaseSpinLock(&VfIrpTrackLock, OldIrql);
            return Track;
        }
    }

    KeReleaseSpinLock(&VfIrpTrackLock, OldIrql);
    return NULL;
}

/* ============================================================
   PAGEABLE STUFF
   ============================================================ */
static UNUSED VOID VfCheckPageableCode(PVOID Address, PDRIVER_OBJECT DriverObject)
{
    KIRQL CurrentIrql = KeGetCurrentIrql();

    /* if address is in pageable memory.. */
    if (!MmIsNonPagedSystemAddressValid(Address)
)
    {
        /* ..if code executes above apc level, bugcheck */
        if (CurrentIrql > APC_LEVEL)
        {
            KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                         VF_VIOLATION_PAGEABLE_CODE,                          // VF_PAGEABLE_CODE
                         (ULONG_PTR)Address,             // address
                         CurrentIrql,                    // current IRQL
                         (ULONG_PTR)DriverObject);       // driver
        }
    }
}

/* ============================================================
   IO
   ============================================================ */
VOID
VfIoCompleteRequest(
    PIRP Irp,
    CCHAR PriorityBoost
)
{
    VF_IRP_TRACK* Track;
    KIRQL OldIrql;

    if (KeGetCurrentIrql() > DISPATCH_LEVEL)
    {
        KeBugCheckEx(
            VF_BUGCHECK_DRIVER_VIOLATION,
            VF_VIOLATION_IRQL_MISUSE,
            KeGetCurrentIrql(),
            DISPATCH_LEVEL,
            (ULONG_PTR)Irp
        );
    }

    KeAcquireSpinLock(&VfIrpTrackLock, &OldIrql);

    Track = VfLookupIrp(Irp);
    if (!Track)
        KeBugCheckEx(
            VF_BUGCHECK_DRIVER_VIOLATION,
            VF_VIOLATION_COMPLETING_UNKNOWN_IRP,
            (ULONG_PTR)Irp,
            0,
            0
        );

    if (Track->Completed)
        KeBugCheckEx(
            VF_BUGCHECK_DRIVER_VIOLATION,
            VF_VIOLATION_DOUBLE_COMPLETE,
            (ULONG_PTR)Irp,
            0,
            0
        );

    Track->Completed = TRUE;

    RemoveEntryList(&Track->ListEntry);
    ExFreePool(Track);

    KeReleaseSpinLock(&VfIrpTrackLock, OldIrql);

    IoCompleteRequest(Irp, PriorityBoost);
}

NTSTATUS
VfIoIncrementRef(
    PIRP Irp
)
{
    LIST_ENTRY VfIrpRefList;
    PVF_IRP_TRACK Track = NULL;   // ← pointer, initialized to NULL

    UNREFERENCED_PARAMETER(VfIrpRefList);

    if (!Track)
    {
        Track = (PVF_IRP_TRACK)ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(VF_IRP_TRACK),
            TAG_VFSP);

        Track->Irp = Irp;
        Track->ReferenceCount = 1;
        Track->CancelRoutineSet = FALSE;
    }

    InterlockedIncrement(&Track->ReferenceCount);

    Irp->Tail.Overlay.DriverContext[0] = Track;
    InsertTailList(&VfIrpTrackList, &Track->ListEntry);

    return STATUS_SUCCESS;
}

VOID VfIoDecrementRef(PIRP Irp)
{
    VF_IRP_TRACK_EXT* Track = Irp->Tail.Overlay.DriverContext[0];
    if (!Track)
        KeBugCheckEx(VF_BUGCHECK_DRIVER_VIOLATION, VF_VIOLATION_IRQL_MISUSE, (ULONG_PTR)Irp, 0, 0);

    if (InterlockedDecrement(&Track->ReferenceCount) < 0)
    {
        KeBugCheckEx(VF_BUGCHECK_DRIVER_VIOLATION, VF_VIOLATION_IRQL_MISUSE, (ULONG_PTR)Irp, 0, 0);
    }

    if (Track->ReferenceCount == 0)
    {
        RemoveEntryList(&Track->ListEntry);
        ExFreePool(Track);
        Irp->Tail.Overlay.DriverContext[0] = NULL;
    }

    if (Track->Completed)
    {
        KeBugCheckEx(
            VF_BUGCHECK_DRIVER_VIOLATION,
            VF_VIOLATION_DOUBLE_COMPLETE,
            (ULONG_PTR)Irp,
            0,
            0
        );
    }
}

/* ============================================================
   SPINLOCK MISUSE DETECTION
   ============================================================ */
static VF_THREAD_LOCK_STACK* VfGetThreadLockStack(VOID)
{
    KIRQL OldIrql;
    PETHREAD Thread = PsGetCurrentThread();
    VF_THREAD_LOCK_STACK* Stack = NULL;
    PLIST_ENTRY L;

    KeAcquireSpinLock(&VfThreadLockListLock, &OldIrql);

    for (L = VfThreadLockList.Flink; L != &VfThreadLockList; L = L->Flink)
    {
        Stack = CONTAINING_RECORD(L, VF_THREAD_LOCK_STACK, ListEntry);
        if (Stack->Thread == Thread)
        {
            KeReleaseSpinLock(&VfThreadLockListLock, OldIrql);
            return Stack;
        }
    }

    // not found? -> create new
    Stack = ExAllocatePoolWithTag(NonPagedPool, sizeof(VF_THREAD_LOCK_STACK), 'tLkV');
    RtlZeroMemory(Stack, sizeof(VF_THREAD_LOCK_STACK));
    Stack->Thread = Thread;
    Stack->Count = 0;
    InitializeListHead(&Stack->ListEntry);
    InsertTailList(&VfThreadLockList, &Stack->ListEntry);

    KeReleaseSpinLock(&VfThreadLockListLock, OldIrql);
    return Stack;
}

static VOID VfEnsureLockStackCapacity(VF_THREAD_LOCK_STACK* Stack)
{
    if (Stack->Count >= Stack->Capacity)
    {
        ULONG NewCapacity = Stack->Capacity ? Stack->Capacity * 2 : 16;
        PKSPIN_LOCK* NewArray = ExAllocatePoolWithTag(NonPagedPool, sizeof(PKSPIN_LOCK) * NewCapacity, 'lStV');
        if (!NewArray)
            KeBugCheckEx(VF_BUGCHECK_DRIVER_VIOLATION, VF_VIOLATION_RECURSIVE_LOCK, (ULONG_PTR)Stack, 0, 0);

        if (Stack->HeldLocks)
        {
            RtlCopyMemory(NewArray, Stack->HeldLocks, sizeof(PKSPIN_LOCK) * Stack->Count);
            ExFreePool(Stack->HeldLocks);
        }

        Stack->HeldLocks = NewArray;
        Stack->Capacity = NewCapacity;
    }
}

BOOLEAN VfCheckDeadlock(PKSPIN_LOCK NewLock, VF_THREAD_LOCK_STACK* Stack)
{
    for (ULONG i = 0; i < Stack->Count; i++)
    {
        PKSPIN_LOCK Held = Stack->HeldLocks[i];
        KIRQL OldIrql;
        PLIST_ENTRY L;
    
        KeAcquireSpinLock(&VfSpinlockDepLock, &OldIrql);

        for (L = VfSpinlockDependencyList.Flink; L != &VfSpinlockDependencyList; L = L->Flink)
        {
            VF_SPINLOCK_DEPENDENCY* Edge = CONTAINING_RECORD(L, VF_SPINLOCK_DEPENDENCY, ListEntry);
            if (Edge->HeldLock == NewLock && Edge->AcquiredLock == Held)
            {
                // if it found a back-edge -> cycle detected
                KeReleaseSpinLock(&VfSpinlockDepLock, OldIrql);
                return TRUE;
            }
        }

        KeReleaseSpinLock(&VfSpinlockDepLock, OldIrql);
    }

    return FALSE;
}

VOID VfAcquireSpinLock(PKSPIN_LOCK SpinLock, KIRQL* OldIrql)
{
    KIRQL CurrentIrql = KeGetCurrentIrql();
    VF_THREAD_LOCK_STACK* Stack = VfGetThreadLockStack();
    KIRQL DepOldIrql;
    KIRQL TrackOldIrql;

    // --- IRQL check ---
    if (CurrentIrql > DISPATCH_LEVEL)
    {
        KeBugCheckEx(VF_BUGCHECK_IRQL_VIOLATION,
                     CurrentIrql,
                     DISPATCH_LEVEL,
                     (ULONG_PTR)SpinLock,
                     0);
    }

    // --- check for recursive acquisition ---
    for (ULONG i = 0; i < Stack->Count; i++)
    {
        if (Stack->HeldLocks[i] == SpinLock)
        {
            KeBugCheckEx(VF_BUGCHECK_DRIVER_VIOLATION,
                         VF_VIOLATION_RECURSIVE_LOCK,
                         (ULONG_PTR)SpinLock,
                         0,
                         0);
        }
    }

    // --- lock order inversion detection ---
    if (Stack->Count > 0)
    {
        PKSPIN_LOCK LastLock = Stack->HeldLocks[Stack->Count - 1];
        if (SpinLock < LastLock)
        {
            KeBugCheckEx(VF_BUGCHECK_DRIVER_VIOLATION,
                         VF_VIOLATION_LOCK_ORDER_INVERSION,
                         (ULONG_PTR)SpinLock,
                         (ULONG_PTR)LastLock,
                         0);
        }
    }

    // --- ensure stack capacity ---
    VfEnsureLockStackCapacity(Stack);

    // --- deadlock detection via global dependency graph ---
    if (VfCheckDeadlock(SpinLock, Stack))
    {
        KeBugCheckEx(VF_BUGCHECK_DRIVER_VIOLATION,
                     VF_VIOLATION_DEADLOCK,
                     (ULONG_PTR)SpinLock,
                     (ULONG_PTR)PsGetCurrentThread(),
                     0);
    }

    KeAcquireSpinLock(&VfSpinlockDepLock, &DepOldIrql);

    for (ULONG i = 0; i < Stack->Count; i++)
    {
        VF_SPINLOCK_DEPENDENCY* Edge = ExAllocatePoolWithTag(NonPagedPool,
                                                             sizeof(VF_SPINLOCK_DEPENDENCY),
                                                             'lDvF');
        if (!Edge)
        {
            KeReleaseSpinLock(&VfSpinlockDepLock, DepOldIrql);
            KeBugCheckEx(VF_BUGCHECK_DRIVER_VIOLATION,
                         VF_VIOLATION_SPINLOCK_DEPENDENCY,
                         (ULONG_PTR)SpinLock,
                         0,
                         0);
        }

        Edge->HeldLock = Stack->HeldLocks[i];
        Edge->AcquiredLock = SpinLock;
        InsertTailList(&VfSpinlockDependencyList, &Edge->ListEntry);
    }

    KeReleaseSpinLock(&VfSpinlockDepLock, DepOldIrql);

    // --- acquire the actual spinlock ---
    KeAcquireSpinLock(SpinLock, OldIrql);

    // --- track in per-thread stack ---
    Stack->HeldLocks[Stack->Count++] = SpinLock;

    // --- global spinlock tracking ---
    VF_SPINLOCK_TRACK* Track = ExAllocatePoolWithTag(NonPagedPool,
                                                     sizeof(VF_SPINLOCK_TRACK),
                                                     'sIrV');
    if (!Track)
    {
        KeBugCheckEx(VF_BUGCHECK_DRIVER_VIOLATION,
                     VF_VIOLATION_SPINLOCK_TRACK,
                     (ULONG_PTR)SpinLock,
                     0,
                     0);
    }

    Track->SpinLock = SpinLock;
    Track->AcquireIrql = *OldIrql;
    Track->OwnerThread = PsGetCurrentThread();

    KeAcquireSpinLock(&VfSpinlockLock, &TrackOldIrql);
    InsertTailList(&VfSpinlockList, &Track->ListEntry);
    KeReleaseSpinLock(&VfSpinlockLock, TrackOldIrql);
}

VOID VfReleaseSpinLock(PKSPIN_LOCK SpinLock, KIRQL OldIrql)
{
    KIRQL CurrentIrql = KeGetCurrentIrql();
    KIRQL TrackOldIrql;
    PLIST_ENTRY L;
    VF_THREAD_LOCK_STACK* Stack = VfGetThreadLockStack();
    BOOLEAN Found = FALSE;

    // --- IRQL check ---
    if (CurrentIrql > DISPATCH_LEVEL)
    {
        KeBugCheckEx(VF_BUGCHECK_IRQL_VIOLATION,
                     CurrentIrql,
                     DISPATCH_LEVEL,
                     (ULONG_PTR)SpinLock,
                     0);
    }

    // --- find the lock in stack (must be held!) ---
    for (LONG i = (LONG)Stack->Count - 1; i >= 0; i--)
    {
        if (Stack->HeldLocks[i] == SpinLock)
        {
            // remove it by shifting others down
            for (LONG j = i; j < (LONG)Stack->Count - 1; j++)
            {
                Stack->HeldLocks[j] = Stack->HeldLocks[j + 1];
            }
            Stack->Count--;
            Found = TRUE;
            break;
        }
    }

    if (!Found)
    {
        KeBugCheckEx(VF_BUGCHECK_DRIVER_VIOLATION,
                     VF_VIOLATION_SPINLOCK_RELEASE,
                     (ULONG_PTR)SpinLock,
                     0,
                     0);
    }

    // --- remove global spinlock tracking ---
    KeAcquireSpinLock(&VfSpinlockLock, &TrackOldIrql);

    for (L = VfSpinlockList.Flink; L != &VfSpinlockList; L = L->Flink)
    {
        VF_SPINLOCK_TRACK* Track = CONTAINING_RECORD(L, VF_SPINLOCK_TRACK, ListEntry);
        if (Track->SpinLock == SpinLock && Track->OwnerThread == PsGetCurrentThread())
        {
            RemoveEntryList(&Track->ListEntry);
            ExFreePoolWithTag(Track, 'sIrV');
            break;
        }
    }

    KeReleaseSpinLock(&VfSpinlockLock, TrackOldIrql);

    // --- release the actual spinlock ---
    KeReleaseSpinLock(SpinLock, OldIrql);
}

/* ============================================================
   DMA ADAPATER VERIFICATION
   ============================================================ */
VOID
NTAPI
VfPutDmaAdapter(
    PDMA_ADAPTER Adapter
)
{
    VF_DMA_ADAPTER_TRACK* T;

    T = VfLookupDmaAdapter(Adapter);
    if (T)
    {
        if (T->MapRegisterCount != 0)
        {
            KeBugCheckEx(
                VF_BUGCHECK_DRIVER_VIOLATION,
                VF_VIOLATION_LEAKED_RESOURCES,
                (ULONG_PTR)Adapter,
                T->MapRegisterCount,
                0
            );
        }

        T->AdapterReleased = TRUE;
    }

    Adapter->DmaOperations->PutDmaAdapter(Adapter);
}

static
VF_DMA_ADAPTER_TRACK*
VfLookupDmaAdapter(
    PDMA_ADAPTER Adapter
)
{
    PLIST_ENTRY L;
    VF_DMA_ADAPTER_TRACK* Track;

    if (!Adapter)
        return NULL;

    for (L = VfDmaAdapterList.Flink;
         L != &VfDmaAdapterList;
         L = L->Flink)
    {
        Track =
            CONTAINING_RECORD(L, VF_DMA_ADAPTER_TRACK, ListEntry);

        if (Track->Adapter == Adapter)
            return Track;
    }

    return NULL;
}

PDMA_ADAPTER
VfGetDmaAdapter(
    PDEVICE_OBJECT DeviceObject,
    PVOID Context,
    PVOID DmaOperations
)
{
    PDMA_ADAPTER Adapter;
    VF_DMA_ADAPTER_TRACK* Track;
    KIRQL OldIrql;

    Adapter = IoGetDmaAdapter(DeviceObject, Context, DmaOperations);
    if (!Adapter)
        return NULL;

    // alloc tracker
    Track = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(VF_DMA_ADAPTER_TRACK),
        'dIrV'
    );

    if (!Track)
    {
        KeBugCheckEx(
            VF_BUGCHECK_DRIVER_VIOLATION,
            VF_VIOLATION_INVALID_DMA_ADAPTER,
            (ULONG_PTR)Adapter,
            0,
            0
        );
        return NULL; // unreachable, but required
    }

    RtlZeroMemory(Track, sizeof(*Track));

    // copy the original dma operations
    Track->OriginalOps = *Adapter->DmaOperations;

    Track->Adapter = Adapter;
    Track->DriverObject = DeviceObject->DriverObject;

    // add to the global list
    KeAcquireSpinLock(&VfDmaLock, &OldIrql);
    InsertTailList(&VfDmaAdapterList, &Track->ListEntry);
    KeReleaseSpinLock(&VfDmaLock, OldIrql);

    return Adapter;
}

VOID
VfValidateDmaAdapter(
    PDMA_ADAPTER Adapter
)
{
    KIRQL OldIrql;
    VF_DMA_ADAPTER_TRACK* Track;

    if (!Adapter)
    {
        KeBugCheckEx(
            VF_BUGCHECK_DRIVER_VIOLATION,
            VF_VIOLATION_INVALID_DMA_ADAPTER,
            0,
            0,
            0
        );
        return;
    }

    KeAcquireSpinLock(&VfDmaLock, &OldIrql);
    Track = VfLookupDmaAdapter(Adapter);
    KeReleaseSpinLock(&VfDmaLock, OldIrql);

    if (!Track || Track->AdapterReleased)
    {
        KeBugCheckEx(
            VF_BUGCHECK_DRIVER_VIOLATION,
            VF_VIOLATION_INVALID_DMA_ADAPTER,
            (ULONG_PTR)Adapter,
            0,
            0
        );
        return;
    }
}

static
BOOLEAN
UNUSED
VfShouldInjectDmaFault(
    VF_DMA_FAULT_TYPE Type
)
{
    if (!VfDmaFaultState.Enabled)
        return FALSE;

    if (VfDmaFaultState.FaultType != Type)
        return FALSE;

    VfDmaFaultState.Counter++;

    if ((VfDmaFaultState.Counter % 100) < VfDmaFaultState.Probability)
        return TRUE;

    return FALSE;
}

static VF_DMA_ADAPTER_TRACK*
VfFindAdapter(PDMA_ADAPTER Adapter)
{
    PLIST_ENTRY L;
    VF_DMA_ADAPTER_TRACK* T;

    for (L = VfDmaAdapterList.Flink;
         L != &VfDmaAdapterList;
         L = L->Flink)
    {
        T =
            CONTAINING_RECORD(L, VF_DMA_ADAPTER_TRACK, ListEntry);
        if (T->Adapter == Adapter)
            return T;
    }
    return NULL;
}
/* ============================================================
   DRIVER UNLOAD VERIFICATION
   ============================================================ */

VOID NTAPI VfDriverUnload(PDRIVER_OBJECT DriverObject)
{
    VF_DRIVER_ENTRY* Driver = VfFindDriver(DriverObject);
    KIRQL OldIrql;

    if (!Driver)
        return;

    if (!IsListEmpty(&Driver->PoolList))
    {
        /* detected memory leak! */
        KeBugCheckEx(VF_BUGCHECK_MEMORY_LEAK,
                     (ULONG_PTR)DriverObject,
                     0, 0, 0);
    }

    if (Driver->OriginalUnload)
        Driver->OriginalUnload(DriverObject);

    KeAcquireSpinLock(&VfDriverListLock, &OldIrql);
    RemoveEntryList(&Driver->ListEntry);
    KeReleaseSpinLock(&VfDriverListLock, OldIrql);

    ExFreePool(Driver);
}

/* ============================================================
   .SPEC FUNCTIONS
   ============================================================ */
VOID
__cdecl
VfFailDeviceNode(
    _In_opt_ PDEVICE_OBJECT PhysicalDeviceObject,
    _In_ ULONG BugCheckMajorCode,
    _In_ ULONG BugCheckMinorCode,
    _In_ VF_FAILURE_CLASS FailureClass,
    _Inout_opt_ PULONG AssertionControl,
    _In_opt_ PSTR DebuggerMessageText,
    _In_opt_ PSTR ParameterFormatString,
    ...
)
{
    PDRIVER_OBJECT DriverObject = NULL;
    va_list VaList;

    if (PhysicalDeviceObject)
    {
        DriverObject = PhysicalDeviceObject->DriverObject;
    }

    DbgPrintEx(
        DPFLTR_VERIFIER_ID,
        DPFLTR_ERROR_LEVEL,
        "VERIFIER: VfFailDeviceNode\n"
    );

    DbgPrintEx(
        DPFLTR_VERIFIER_ID,
        DPFLTR_ERROR_LEVEL,
        "VERIFIER: PhysicalDeviceObject %p\n"
        "VERIFIER: DriverObject %p\n",
        PhysicalDeviceObject,
        DriverObject
    );

    if (DebuggerMessageText)
    {
        DbgPrintEx(
            DPFLTR_VERIFIER_ID,
            DPFLTR_ERROR_LEVEL,
            "VERIFIER: %s\n",
            DebuggerMessageText
        );
    }

    if (ParameterFormatString)
    {
        va_start(VaList, ParameterFormatString);

        vDbgPrintEx(
            DPFLTR_VERIFIER_ID,
            DPFLTR_ERROR_LEVEL,
            ParameterFormatString,
            VaList
        );

        va_end(VaList);

        DbgPrintEx(
            DPFLTR_VERIFIER_ID,
            DPFLTR_ERROR_LEVEL,
            "\n"
        );
    }

    if (AssertionControl && (*AssertionControl == 0))
    {
        return;
    }

#if DBG
    if (FailureClass == VfFatalFailure)
    {
        DbgBreakPoint();
    }
#endif

    KeBugCheckEx(
        BugCheckMajorCode,
        BugCheckMinorCode,
        (ULONG_PTR)PhysicalDeviceObject,
        (ULONG_PTR)DriverObject,
        (ULONG_PTR)FailureClass
    );
}

VOID
__cdecl
VfFailDriver(
    PDRIVER_OBJECT DriverObject,
    PCSTR Message
)
{
    DbgPrint(
        "VERIFIER: VfFailDriver\n"
        "VERIFIER: DriverObject = %p\n"
        "VERIFIER: Message = %s\n",
        DriverObject,
        Message ? Message : "(null)"
    );

#if DBG
    DbgBreakPoint();
#endif

    KeBugCheckEx(
        VF_BUGCHECK_DRIVER_VIOLATION,
        VF_VIOLATION_LEAKED_RESOURCES,                      
        (ULONG_PTR)DriverObject,    // offending driver
        (ULONG_PTR)_ReturnAddress(),
        0
    );
}

VOID
__cdecl
VfFailSystemBIOS(
    PCSTR Message
)
{
    DbgPrint(
        "VERIFIER: VfFailSystemBIOS\n"
        "VERIFIER: Message = %s\n",
        Message ? Message : "(null)"
    );

#if DBG
    DbgBreakPoint();
#endif

    KeBugCheckEx(
        VF_BUGCHECK_DRIVER_VIOLATION,
        VF_VIOLATION_FIRMWARE_BIOS,                       // firmware / bios violation
        (ULONG_PTR)Message,
        (ULONG_PTR)_ReturnAddress(),
        0
    );
}

#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllexport)
BOOLEAN
__stdcall
VfIsVerificationEnabled(
    ULONG Flags,
    ULONG Reserved
)
{
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(Reserved);

#if DBG
    DbgPrint("VF: VfIsVerificationEnabled called. Returning %d\n", VfGlobalEnabled);
#endif

    return VfGlobalEnabled;
}

#ifdef __cplusplus
}
#endif

/* ============================================================
   EOF
   ============================================================ */
