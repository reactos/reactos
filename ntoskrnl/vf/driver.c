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
#include <mm/ARM3/miarm.h>
#include "vf.h"

/*
 * internal forward declarations
 * (these are NOT in the header to keep them fully internal)
 */
static UNUSED VF_IRP_TRACK*  VfLookupIrp(_In_ PIRP Irp);
static VF_IRP_TRACK*         VfLookupIrpLocked(_In_ PIRP Irp);
static VF_DMA_ADAPTER_TRACK* VfLookupDmaAdapter(PDMA_ADAPTER Adapter);
static VF_DMA_ADAPTER_TRACK* VfFindAdapter(PDMA_ADAPTER Adapter);
static BOOLEAN UNUSED        VfShouldInjectDmaFault(VF_DMA_FAULT_TYPE Type);
static VOID UNUSED           VfCheckPageableCode(PVOID Address, PDRIVER_OBJECT DriverObject);


/* ============================================================
   initialize header extern globals
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

LIST_ENTRY VfDriverList;
LIST_ENTRY VfIrpTrackList;
LIST_ENTRY VfIrpHookList;
LIST_ENTRY VfSpinlockList;
LIST_ENTRY VfDmaAdapterList;
LIST_ENTRY VfThreadLockList;
LIST_ENTRY VfSpinlockDependencyList;

KSPIN_LOCK VfSpinlockLock;
KSPIN_LOCK VfDmaLock;
KSPIN_LOCK VfDriverListLock;
KSPIN_LOCK VfIrpTrackLock;
KSPIN_LOCK VfIrpHookLock;
KSPIN_LOCK VfThreadLockListLock;
KSPIN_LOCK VfSpinlockDepLock;

/* ============================================================
   HELPER(S)
   ============================================================ */
PDRIVER_OBJECT
VfGetDriverByAddress(PVOID Address)
{
    PLIST_ENTRY Entry;
    PLIST_ENTRY VfLink;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    VF_DRIVER_ENTRY* VfEntry;
    KIRQL OldIrql;

    if (KeGetCurrentIrql() > DISPATCH_LEVEL)
        return NULL;

    KeAcquireSpinLock(&PsLoadedModuleSpinLock, &OldIrql);

    for (Entry = PsLoadedModuleList.Flink;
         Entry != &PsLoadedModuleList;
         Entry = Entry->Flink)
    {
        LdrEntry = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if (Address >= LdrEntry->DllBase &&
            Address < (PVOID)((ULONG_PTR)LdrEntry->DllBase + LdrEntry->SizeOfImage))
        {
            for (VfLink = VfDriverList.Flink;
                 VfLink != &VfDriverList;
                 VfLink = VfLink->Flink)
            {
                VfEntry = CONTAINING_RECORD(VfLink, VF_DRIVER_ENTRY, ListEntry);
                if (VfEntry->DriverObject->DriverSection == LdrEntry)
                {
                    DPRINT1("VF: Match! Driver %p owns address %p\n", VfEntry->DriverObject, Address);
                    KeReleaseSpinLock(&PsLoadedModuleSpinLock, OldIrql);
                    return VfEntry->DriverObject;
                }
            }
        }
    }

    KeReleaseSpinLock(&PsLoadedModuleSpinLock, OldIrql);
    return NULL;
}

static BOOLEAN VfShouldVerifyDriver(PDRIVER_OBJECT DriverObject)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    UNICODE_STRING DriverName;
    PWCHAR List, Token, End;

    /* NULL or empty buffer means verify all */
    if (!VfGlobal.VerifyDriverList.Buffer ||
        VfGlobal.VerifyDriverList.Length == 0)
        return TRUE;

    /* * means verify all */
    if (VfGlobal.VerifyDriverList.Length == sizeof(WCHAR) &&
        VfGlobal.VerifyDriverList.Buffer[0] == L'*')
        return TRUE;

    /* get driver filename from LdrEntry */
    LdrEntry = (PLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection;
    if (!LdrEntry || !LdrEntry->BaseDllName.Buffer || !LdrEntry->BaseDllName.Length)
        return TRUE;

    DriverName = LdrEntry->BaseDllName;
    if (DriverName.Length > DriverName.MaximumLength)
        DriverName.Length = DriverName.MaximumLength;

    DPRINT1("VF: ShouldVerify: list='%wZ' driver='%.*ws'\n",
            &VfGlobal.VerifyDriverList,
            DriverName.Length / sizeof(WCHAR),
            DriverName.Buffer);

    /* walk space-separated list */
    List = VfGlobal.VerifyDriverList.Buffer;
    while (*List)
    {
        while (*List == L' ') List++;
        if (!*List) break;

        Token = List;
        End = List;
        while (*End && *End != L' ') End++;

        UNICODE_STRING Entry;
        Entry.Buffer = Token;
        Entry.Length = (USHORT)((End - Token) * sizeof(WCHAR));
        Entry.MaximumLength = Entry.Length;

        if (RtlEqualUnicodeString(&DriverName, &Entry, TRUE))
            return TRUE;

        List = End;
    }

    return FALSE;
}

/* ============================================================
   INIT
   ============================================================ */
static VOID VfInitializeRegistry(VOID)
{
    OBJECT_ATTRIBUTES ObjAttrs;
    UNICODE_STRING KeyPath;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    NTSTATUS Status;
    ULONG Disposition;
    ULONG Level = VF_DEFAULT_LEVEL;

    /* 8 byte buffer for header + ULONG data */
    UCHAR LevelBuf[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    /* buffer for VerifyDrivers string */
    UCHAR DriversBuf[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 512 * sizeof(WCHAR)];
    PKEY_VALUE_PARTIAL_INFORMATION Info;
    ULONG ResultLen;

    RtlInitUnicodeString(&KeyPath, VF_REG_KEY);
    InitializeObjectAttributes(&ObjAttrs,
                                &KeyPath,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL);

    Status = ZwCreateKey(&KeyHandle,
                         KEY_READ | KEY_WRITE,
                         &ObjAttrs,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         &Disposition);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("VF: Failed to create/open registry key (0x%lx), using defaults\n", Status);
        VfGlobal.GlobalFlags = VF_DEFAULT_LEVEL;
        return;
    }

    /* ---- VerifyDriverLevel ---- */
    RtlInitUnicodeString(&ValueName, L"VerifyDriverLevel");
    Info = (PKEY_VALUE_PARTIAL_INFORMATION)LevelBuf;
    Status = ZwQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             Info,
                             sizeof(LevelBuf),
                             &ResultLen);

    if (NT_SUCCESS(Status) && Info->Type == REG_DWORD && Info->DataLength == sizeof(ULONG))
    {
        /* key existed, read it */
        Level = *(PULONG)Info->Data;
        DPRINT1("VF: VerifyDriverLevel = 0x%lx\n", Level);
    }
    else
    {
        /* key missing, create with default */
        Status = ZwSetValueKey(KeyHandle,
                               &ValueName,
                               0,
                               REG_DWORD,
                               &Level,
                               sizeof(ULONG));
        if (!NT_SUCCESS(Status))
            DPRINT1("VF: Failed to write VerifyDriverLevel (0x%lx)\n", Status);
        DPRINT1("VF: VerifyDriverLevel defaulting to 0x%lx\n", Level);
    }

    VfGlobal.GlobalFlags = Level;

    /* ---- VerifyDrivers ---- */
    RtlInitUnicodeString(&ValueName, L"VerifyDrivers");
    Info = (PKEY_VALUE_PARTIAL_INFORMATION)DriversBuf;
    Status = ZwQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             Info,
                             sizeof(DriversBuf),
                             &ResultLen);

    if (NT_SUCCESS(Status) && Info->Type == REG_SZ && Info->DataLength > 0)
    {
        /* copy into VfGlobal.VerifyDriverList */
        ULONG ByteLen = min(Info->DataLength, 511 * sizeof(WCHAR));
        PWCHAR Buf = ExAllocatePoolWithTag(NonPagedPool,
                                           ByteLen + sizeof(WCHAR),
                                           TAG_VFDRV);
        if (Buf)
        {
            RtlCopyMemory(Buf, Info->Data, ByteLen);
            Buf[ByteLen / sizeof(WCHAR)] = UNICODE_NULL;
            RtlInitUnicodeString(&VfGlobal.VerifyDriverList, Buf);
            DPRINT1("VF: VerifyDrivers = %wZ\n", &VfGlobal.VerifyDriverList);
        }
    }
    else
    {
        /* key missing, create with default "*" */
        Status = ZwSetValueKey(KeyHandle,
                               &ValueName,
                               0,
                               REG_SZ,
                               VF_DEFAULT_DRIVERS,
                               sizeof(VF_DEFAULT_DRIVERS));
        if (!NT_SUCCESS(Status))
            DPRINT1("VF: Failed to write VerifyDrivers (0x%lx)\n", Status);
        DPRINT1("VF: VerifyDrivers defaulting to *\n");
        /* NULL buffer means verify all */
        VfGlobal.VerifyDriverList.Buffer = NULL;
        VfGlobal.VerifyDriverList.Length = 0;
    }

    ZwClose(KeyHandle);
}

VOID NTAPI VfInitialize(VOID)
{
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

    VfGlobal.Enabled = TRUE;
    VfGlobalEnabled  = TRUE;

    ExpPoolFlags |= POOL_FLAG_VERIFIER;

    VfInitializeRegistry();

    DPRINT1("VF: Driver Verifier initialized\n");
}

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
        if (FailureClass == VfPoolOverflow)
        {
            KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                         VF_SUBCODE_WRONG_POOL_TAG,
                         (ULONG_PTR)ParamFormat,
                         (ULONG_PTR)ObjectType,
                         0);
        }
        else if (FailureClass == VfIrqlViolation)
        {
            KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                         (ULONG_PTR)ObjectType,
                         0, 0, 0);
        }
        else if (FailureClass == VfSpecialPool)
        {
            KeBugCheckEx(SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION,
                         (ULONG_PTR)ObjectType,
                         (ULONG_PTR)ParamFormat,
                         0, 0);
        }
        else
        {
            KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                         (ULONG_PTR)FailureClass,
                         0, 0, (ULONG_PTR)ObjectType);
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

    if (!VfShouldVerifyDriver(DriverObject))
    {
        DPRINT1("VF: Skipping driver %p (not in verify list)\n", DriverObject);
        return;
    }

    if (!VfGlobal.Enabled)
        return;

    Entry = ExAllocatePoolWithTag(NonPagedPool,
                                  sizeof(VF_DRIVER_ENTRY),
                                  TAG_VFDRV);
    if (!Entry)
        return;
    
    RtlZeroMemory(Entry, sizeof(VF_DRIVER_ENTRY));
    Entry->DriverObject = DriverObject;
    InitializeListHead(&Entry->PoolList);
    KeInitializeSpinLock(&Entry->PoolLock);
    Entry->OriginalUnload = DriverObject->DriverUnload;
    Entry->VerifierFlags = VfGlobal.GlobalFlags ? VfGlobal.GlobalFlags :
                           (VF_FLAG_POOL_TRACKING | VF_FLAG_IRQL_CHECKING | VF_FLAG_SPECIAL_POOL);
    Entry->Loads = 1;

    KeAcquireSpinLock(&VfDriverListLock, &OldIrql);
    InsertTailList(&VfDriverList, &Entry->ListEntry);
    KeReleaseSpinLock(&VfDriverListLock, OldIrql);

    DriverObject->DriverUnload = VfDriverUnload;

    DPRINT1("VF: Registered driver %p for verification flags=0x%lx\n",
            DriverObject,
            Entry->VerifierFlags);
}

VOID VfUnregisterDriver(PDRIVER_OBJECT DriverObject) 
{
    VF_DRIVER_ENTRY* Entry;
    KIRQL OldIrql;

    DPRINT1("VF: VfUnregisterDriver called for %p\n", DriverObject);

    KeAcquireSpinLock(&VfDriverListLock, &OldIrql);
    Entry = VfFindDriver(DriverObject);
    if (Entry)
    {
        RemoveEntryList(&Entry->ListEntry);
    }
    KeReleaseSpinLock(&VfDriverListLock, OldIrql);

    if (Entry)
    {
        /* restore original unload (IF we hooked it) */
        if (DriverObject->DriverUnload == VfDriverUnload)
            DriverObject->DriverUnload = Entry->OriginalUnload;

        ExFreePool(Entry);
    }
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
    {
        KeReleaseSpinLock(&VfDmaLock, OldIrql);
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION, 0, (ULONG_PTR)Adapter, 0, 0);
    }

    if (T->MapRegisterCount != 0)
    {
        KeReleaseSpinLock(&VfDmaLock, OldIrql);
        KeBugCheckEx(
            DRIVER_VERIFIER_DETECTED_VIOLATION,
            VfDmaFaultAllocateChannel,
            (ULONG_PTR)Adapter,
            T->MapRegisterCount,
            0
        );
    }

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
    VF_DMA_ADAPTER_TRACK* Track;
    KIRQL OldIrql;

    VfValidateDmaAdapter(Adapter);

    KeAcquireSpinLock(&VfDmaLock, &OldIrql);
    Track = VfLookupDmaAdapter(Adapter);

    if (!Track || Track->MapRegisterCount <= 0)
    {
        KeReleaseSpinLock(&VfDmaLock, OldIrql);
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
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
                DRIVER_VERIFIER_DETECTED_VIOLATION,
                VF_VIOLATION_INVALID_DMA_ADAPTER,
                (ULONG_PTR)Adapter,
                0,
                0
            );
        }

        /* increment BEFORE map */
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
            DRIVER_VERIFIER_DETECTED_VIOLATION,
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
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     CurrentIrql,
                     APC_LEVEL,
                     IsFree,
                     0);
    }
    else if (CurrentIrql > DISPATCH_LEVEL)
    {
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
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

    /*
     * reactos doesn't support MmProtectMdlSystemAddress guard pages. :(
    */
    UNREFERENCED_PARAMETER(Mdl);
    UNREFERENCED_PARAMETER(Mapping);
    UNREFERENCED_PARAMETER(Low);
    UNREFERENCED_PARAMETER(High);
    UNREFERENCED_PARAMETER(Skip);
    UNREFERENCED_PARAMETER(Base);
    UNREFERENCED_PARAMETER(TotalSize);
    UNREFERENCED_PARAMETER(Pages);
    UNREFERENCED_PARAMETER(Status);

    *OutMdl = NULL;
    return ExAllocatePoolWithTag(NonPagedPool, Size, TAG_VFSP);
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

    if (Driver)
        Driver->AllocationsAttempted++;

    if (Driver && !Tag)
        Driver->AllocationsWithNoTag++;

    if (Driver && (Driver->VerifierFlags & VF_FLAG_IRQL_CHECKING))
        VfCheckIrqlForPool(PoolType, FALSE);

    if (Driver && Driver->PoolQuota > 0)
    {
        KeAcquireSpinLock(&Driver->PoolLock, &OldIrql);
        if ((Driver->PoolUsage + Size) > Driver->PoolQuota)
        {
            KeReleaseSpinLock(&Driver->PoolLock, OldIrql);
            Driver->AllocationsFailedDeliberately++;
            Driver->AllocationsFailed++;
            VfFailDriver(DriverObject, "Pool quota exceeded");
            return NULL;
        }
        Driver->PoolUsage += Size;
        KeReleaseSpinLock(&Driver->PoolLock, OldIrql);
    }

    if (Size == 0)
    {
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
            if (Driver) Driver->AllocationsFailed++;
            VfFailDriver(DriverObject, "Special pool allocation failed");
            return NULL;
        }
    }
    else
    {
        Address = ExAllocatePoolWithTag(PoolType, Size, Tag);
        if (!Address)
        {
            if (Driver) Driver->AllocationsFailed++;
            VfFailDriver(DriverObject, "Pool allocation failed");
            return NULL;
        }
    }

TrackAllocation:
    if (!Address || !Driver)
        return Address;

    Driver->AllocationsSucceeded++;
    if (Special)
        Driver->AllocationsSucceededSpecialPool++;

    if (PoolType == PagedPool)
    {
        Driver->CurrentPagedPoolAllocations++;
        Driver->PagedPoolUsageInBytes += Size;
        if (Driver->CurrentPagedPoolAllocations > Driver->PeakPagedPoolAllocations)
            Driver->PeakPagedPoolAllocations = Driver->CurrentPagedPoolAllocations;
        if (Driver->PagedPoolUsageInBytes > Driver->PeakPagedPoolUsageInBytes)
            Driver->PeakPagedPoolUsageInBytes = Driver->PagedPoolUsageInBytes;
    }
    else
    {
        Driver->CurrentNonPagedPoolAllocations++;
        Driver->NonPagedPoolUsageInBytes += Size;
        if (Driver->CurrentNonPagedPoolAllocations > Driver->PeakNonPagedPoolAllocations)
            Driver->PeakNonPagedPoolAllocations = Driver->CurrentNonPagedPoolAllocations;
        if (Driver->NonPagedPoolUsageInBytes > Driver->PeakNonPagedPoolUsageInBytes)
            Driver->PeakNonPagedPoolUsageInBytes = Driver->NonPagedPoolUsageInBytes;
    }

    Alloc = ExAllocatePoolWithTag(NonPagedPool, sizeof(VF_POOL_ALLOCATION), TAG_VFALL);
    if (!Alloc)
        return Address;

    Alloc->Address      = Address;
    Alloc->Size         = Size;
    Alloc->Tag          = Tag;
    Alloc->PoolType     = PoolType;
    Alloc->SpecialPool  = Special;
    Alloc->Mdl          = Mdl;
    Alloc->AllocateIrql = KeGetCurrentIrql();

    KeAcquireSpinLock(&Driver->PoolLock, &OldIrql);
    InsertTailList(&Driver->PoolList, &Alloc->ListEntry);
    KeReleaseSpinLock(&Driver->PoolLock, OldIrql);

    return Address;
}

VOID NTAPI VfFreePool(PDRIVER_OBJECT DriverObject, PVOID Address, ULONG PoolTag, POOL_TYPE PoolType)
{
    VF_DRIVER_ENTRY* Driver = VfFindDriver(DriverObject);
    VF_POOL_ALLOCATION* Alloc;
    PLIST_ENTRY Link;
    KIRQL OldIrql;

    if (!Driver)
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION, VF_SUBCODE_INVALID_FREE,
                     (ULONG_PTR)Address, 0, 0);

    KeAcquireSpinLock(&Driver->PoolLock, &OldIrql);

    for (Link = Driver->PoolList.Flink; Link != &Driver->PoolList; Link = Link->Flink)
    {
        Alloc = CONTAINING_RECORD(Link, VF_POOL_ALLOCATION, ListEntry);
        if (Alloc->Address == Address)
        {
            if (Alloc->Tag != PoolTag)
            {
                KeReleaseSpinLock(&Driver->PoolLock, OldIrql);
                KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                             VF_SUBCODE_WRONG_POOL_TAG,
                             (ULONG_PTR)Address,
                             (ULONG_PTR)Alloc->Tag,
                             (ULONG_PTR)PoolTag);
            }

            RemoveEntryList(&Alloc->ListEntry);
            Driver->PoolUsage -= Alloc->Size;

            if (Alloc->PoolType == PagedPool)
            {
                Driver->CurrentPagedPoolAllocations--;
                Driver->PagedPoolUsageInBytes -= Alloc->Size;
            }
            else
            {
                Driver->CurrentNonPagedPoolAllocations--;
                Driver->NonPagedPoolUsageInBytes -= Alloc->Size;
            }

            KeReleaseSpinLock(&Driver->PoolLock, OldIrql);

            if (Driver->VerifierFlags & VF_FLAG_IRQL_CHECKING)
            {
                if (KeGetCurrentIrql() > Alloc->AllocateIrql)
                    KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                                 KeGetCurrentIrql(),
                                 Alloc->AllocateIrql,
                                 (ULONG_PTR)DriverObject,
                                 (ULONG_PTR)Address);
            }

            if (Alloc->PoolType != PoolType)
            {
                KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                             VF_SUBCODE_WRONG_POOL_TYPE,
                             (ULONG_PTR)Address,
                             (ULONG_PTR)Alloc->PoolType,
                             (ULONG_PTR)PoolType);
            }

            if (Alloc->SpecialPool && Alloc->Mdl != NULL)
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
    KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION, VF_SUBCODE_INVALID_FREE,
                 (ULONG_PTR)Address, 0, 0);
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

    /* IRP reuse detection */
    for (L = VfIrpTrackList.Flink; L != &VfIrpTrackList; L = L->Flink)
    {
        Track = CONTAINING_RECORD(L, VF_IRP_TRACK, ListEntry);
        if (Track->Irp == Irp)
        {
            KeReleaseSpinLock(&VfIrpTrackLock, OldIrql);
            KeBugCheckEx(
                DRIVER_VERIFIER_DETECTED_VIOLATION,
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
    {
        KeReleaseSpinLock(&VfIrpTrackLock, OldIrql);
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION, 0, 0, 0, 0);
    }

    RtlZeroMemory(Track, sizeof(*Track));
    Track->Irp            = Irp;
    Track->DriverObject   = DeviceObject->DriverObject;
    Track->MajorFunction  = IoGetCurrentIrpStackLocation(Irp)->MajorFunction;
    Track->DispatchIrql   = KeGetCurrentIrql();
    Track->ReferenceCount = 1;
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
            DRIVER_VERIFIER_DETECTED_VIOLATION,
            VF_VIOLATION_INVALID_IRP_STATE,
            (ULONG_PTR)Irp,
            Irp->CurrentLocation,
            Irp->StackCount
        );
    }

    if (Stack->MajorFunction > IRP_MJ_MAXIMUM_FUNCTION)
    {
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     VF_VIOLATION_IRQL_MISUSE,
                     Stack->MajorFunction,
                     0,
                     (ULONG_PTR)Irp);
    }

    /* increment reference for tracking */
    VfIoIncrementRef(Irp);

    /* find original dispatch */
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
        VfIoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NOT_SUPPORTED;
    }

    if (Irp->IoStatus.Status != STATUS_PENDING &&
        Irp->IoStatus.Status != 0)
    {
        KeBugCheckEx(
            DRIVER_VERIFIER_DETECTED_VIOLATION,
            VF_VIOLATION_INVALID_IRP_STATE,
            (ULONG_PTR)Irp,
            Irp->IoStatus.Status,
            0
        );
    }

    if (Irp->Cancel && !Irp->CancelRoutine)
    {
        KeBugCheckEx(
            DRIVER_VERIFIER_DETECTED_VIOLATION,
            VF_VIOLATION_CANCEL_ROUTINE_MISSING,
            (ULONG_PTR)Irp,
            0,
            0
        );
    }

    /* call THE original driver */
    Status = Original(DeviceObject, Irp);

    KeAcquireSpinLock(&VfIrpTrackLock, &OldIrql);
    Track = VfLookupIrpLocked(Irp);

    if (!Track)
    {
        KeReleaseSpinLock(&VfIrpTrackLock, OldIrql);
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION, 0, (ULONG_PTR)Irp, 0, 0);
    }

    if (Status == STATUS_PENDING)
    {
        if (!Irp->PendingReturned)
        {
            KeReleaseSpinLock(&VfIrpTrackLock, OldIrql);
            KeBugCheckEx(
                DRIVER_VERIFIER_DETECTED_VIOLATION,
                VF_VIOLATION_IRP_NOT_MARKED_PENDING,
                (ULONG_PTR)Irp,
                0,
                0
            );
        }
        Track->PendingReturned = TRUE;
    }
    else
    {
        if (Irp->PendingReturned)
        {
            KeReleaseSpinLock(&VfIrpTrackLock, OldIrql);
            KeBugCheckEx(
                DRIVER_VERIFIER_DETECTED_VIOLATION,
                VF_VIOLATION_INVALID_IRP_STATE,
                (ULONG_PTR)Irp,
                Status,
                0
            );
        }
    }

    KeReleaseSpinLock(&VfIrpTrackLock, OldIrql);

    if (NT_SUCCESS(Status) && Irp->IoStatus.Status == STATUS_PENDING)
    {
        KeBugCheckEx(
            DRIVER_VERIFIER_DETECTED_VIOLATION,
            VF_VIOLATION_INVALID_IRP_STATE,
            (ULONG_PTR)Irp,
            Status,
            0
        );
    }

    if (Status == STATUS_PENDING && !Irp->PendingReturned)
    {
        KeBugCheckEx(
            DRIVER_VERIFIER_DETECTED_VIOLATION,
            VF_VIOLATION_IRP_NOT_MARKED_PENDING,
            (ULONG_PTR)Irp,
            0,
            0
        );
    }

    if (Status != STATUS_PENDING && Irp->PendingReturned)
    {
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
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

    Hook = ExAllocatePoolWithTag(NonPagedPool,
                                 sizeof(VF_IRP_HOOK),
                                 'hIrV');
    if (!Hook)
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION, VF_VIOLATION_LEAKED_RESOURCES, (ULONG_PTR)DriverObject, 0, 0);

    RtlZeroMemory(Hook, sizeof(VF_IRP_HOOK));
    Hook->DriverObject = DriverObject;

    for (ULONG i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        Hook->OriginalMajor[i] = DriverObject->MajorFunction[i];
        DriverObject->MajorFunction[i] = VfIrpDispatchHook;
    }

    KeAcquireSpinLock(&VfIrpHookLock, &OldIrql);
    InsertTailList(&VfIrpHookList, &Hook->ListEntry);
    KeReleaseSpinLock(&VfIrpHookLock, OldIrql);

    DPRINT1("VF: IRP dispatch hooks installed for driver %p\n", DriverObject);
}

VOID VfHookDriverUnload(PDRIVER_OBJECT DriverObject)
{
    VF_DRIVER_ENTRY* Entry = VfFindDriver(DriverObject);
    if (!Entry) return;
    Entry->OriginalUnload = DriverObject->DriverUnload;
    DriverObject->DriverUnload = VfDriverUnload;
}

static
VF_IRP_TRACK*
VfLookupIrpLocked(
    _In_ PIRP Irp
)
{
    PLIST_ENTRY Entry;
    VF_IRP_TRACK* Track;

    for (Entry = VfIrpTrackList.Flink;
         Entry != &VfIrpTrackList;
         Entry = Entry->Flink)
    {
        Track = CONTAINING_RECORD(Entry, VF_IRP_TRACK, ListEntry);
        if (Track->Irp == Irp)
            return Track;
    }

    return NULL;
}

static
UNUSED
VF_IRP_TRACK*
VfLookupIrp(
    _In_ PIRP Irp
)
{
    VF_IRP_TRACK* Track;
    KIRQL OldIrql;

    KeAcquireSpinLock(&VfIrpTrackLock, &OldIrql);
    Track = VfLookupIrpLocked(Irp);
    KeReleaseSpinLock(&VfIrpTrackLock, OldIrql);

    return Track;
}

/* ============================================================
   PAGEABLE STUFF
   ============================================================ */
static UNUSED VOID VfCheckPageableCode(PVOID Address, PDRIVER_OBJECT DriverObject)
{
    KIRQL CurrentIrql = KeGetCurrentIrql();

    if (!MmIsNonPagedSystemAddressValid(Address))
    {
        if (CurrentIrql > APC_LEVEL)
        {
            KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                         VF_VIOLATION_PAGEABLE_CODE,
                         (ULONG_PTR)Address,
                         CurrentIrql,
                         (ULONG_PTR)DriverObject);
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
            DRIVER_VERIFIER_DETECTED_VIOLATION,
            VF_VIOLATION_IRQL_MISUSE,
            KeGetCurrentIrql(),
            DISPATCH_LEVEL,
            (ULONG_PTR)Irp
        );
    }

    KeAcquireSpinLock(&VfIrpTrackLock, &OldIrql);

    Track = VfLookupIrpLocked(Irp);
    if (!Track)
    {
        KeReleaseSpinLock(&VfIrpTrackLock, OldIrql);
        KeBugCheckEx(
            DRIVER_VERIFIER_DETECTED_VIOLATION,
            VF_VIOLATION_COMPLETING_UNKNOWN_IRP,
            (ULONG_PTR)Irp,
            0,
            0
        );
    }

    if (Track->Completed)
    {
        KeReleaseSpinLock(&VfIrpTrackLock, OldIrql);
        KeBugCheckEx(
            DRIVER_VERIFIER_DETECTED_VIOLATION,
            VF_VIOLATION_DOUBLE_COMPLETE,
            (ULONG_PTR)Irp,
            0,
            0
        );
    }

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
    KIRQL OldIrql;
    PVF_IRP_TRACK Track;

    KeAcquireSpinLock(&VfIrpTrackLock, &OldIrql);

    Track = VfLookupIrpLocked(Irp);

    if (Track)
    {
        InterlockedIncrement(&Track->ReferenceCount);
        Track->CancelRoutineSet = (Irp->CancelRoutine != NULL);
        Irp->Tail.Overlay.DriverContext[0] = Track;
        KeReleaseSpinLock(&VfIrpTrackLock, OldIrql);
        return STATUS_SUCCESS;
    }

    /*
     * if no existing entry -> allocate a new one
     * since ReferenceCount starts at 1 we do NOT increment again
     */
    Track = (PVF_IRP_TRACK)ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(VF_IRP_TRACK),
        TAG_VFSP);

    if (!Track)
    {
        KeReleaseSpinLock(&VfIrpTrackLock, OldIrql);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Track, sizeof(*Track));
    Track->Irp            = Irp;
    Track->ReferenceCount = 1;
    Track->CancelRoutineSet = (Irp->CancelRoutine != NULL);

    InsertTailList(&VfIrpTrackList, &Track->ListEntry);
    Irp->Tail.Overlay.DriverContext[0] = Track;

    KeReleaseSpinLock(&VfIrpTrackLock, OldIrql);
    return STATUS_SUCCESS;
}

VOID VfIoDecrementRef(PIRP Irp)
{
    PVF_IRP_TRACK Track = (PVF_IRP_TRACK)Irp->Tail.Overlay.DriverContext[0];
    KIRQL OldIrql;
    BOOLEAN ShouldFree;
    BOOLEAN WasCompleted;

    if (!Track)
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION, VF_VIOLATION_IRQL_MISUSE, (ULONG_PTR)Irp, 0, 0);

    KeAcquireSpinLock(&VfIrpTrackLock, &OldIrql);

    if (InterlockedDecrement(&Track->ReferenceCount) < 0)
    {
        KeReleaseSpinLock(&VfIrpTrackLock, OldIrql);
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION, VF_VIOLATION_IRQL_MISUSE, (ULONG_PTR)Irp, 0, 0);
    }

    ShouldFree   = (Track->ReferenceCount == 0);
    WasCompleted = Track->Completed;

    if (ShouldFree)
    {
        RemoveEntryList(&Track->ListEntry);
        Irp->Tail.Overlay.DriverContext[0] = NULL;
    }

    KeReleaseSpinLock(&VfIrpTrackLock, OldIrql);

    /* check completed BEFORE freeing the track */
    if (WasCompleted)
    {
        if (ShouldFree)
            ExFreePool(Track);

        KeBugCheckEx(
            DRIVER_VERIFIER_DETECTED_VIOLATION,
            VF_VIOLATION_DOUBLE_COMPLETE,
            (ULONG_PTR)Irp,
            0,
            0
        );
    }

    if (ShouldFree)
        ExFreePool(Track);
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

    Stack = ExAllocatePoolWithTag(NonPagedPool, sizeof(VF_THREAD_LOCK_STACK), 'tLkV');
    if (!Stack)
    {
        KeReleaseSpinLock(&VfThreadLockListLock, OldIrql);
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION, VF_VIOLATION_SPINLOCK_TRACK, 0, 0, 0);
    }

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
            KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION, VF_VIOLATION_RECURSIVE_LOCK, (ULONG_PTR)Stack, 0, 0);

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
                /* back-edge found -> cycle */
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

    if (CurrentIrql > DISPATCH_LEVEL)
    {
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     CurrentIrql,
                     DISPATCH_LEVEL,
                     (ULONG_PTR)SpinLock,
                     0);
    }

    for (ULONG i = 0; i < Stack->Count; i++)
    {
        if (Stack->HeldLocks[i] == SpinLock)
        {
            KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                         VF_VIOLATION_RECURSIVE_LOCK,
                         (ULONG_PTR)SpinLock,
                         0,
                         0);
        }
    }

    if (Stack->Count > 0)
    {
        PKSPIN_LOCK LastLock = Stack->HeldLocks[Stack->Count - 1];
        if (SpinLock < LastLock)
        {
            KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                         VF_VIOLATION_LOCK_ORDER_INVERSION,
                         (ULONG_PTR)SpinLock,
                         (ULONG_PTR)LastLock,
                         0);
        }
    }

    VfEnsureLockStackCapacity(Stack);

    if (VfCheckDeadlock(SpinLock, Stack))
    {
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
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
            KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
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

    /* acquire the ACTUAL spinlock */
    KeAcquireSpinLock(SpinLock, OldIrql);

    /* track in per-thread stack */
    Stack->HeldLocks[Stack->Count++] = SpinLock;

    /* global spinlock tracking */
    VF_SPINLOCK_TRACK* Track = ExAllocatePoolWithTag(NonPagedPool,
                                                     sizeof(VF_SPINLOCK_TRACK),
                                                     'sIrV');
    if (!Track)
    {
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     VF_VIOLATION_SPINLOCK_TRACK,
                     (ULONG_PTR)SpinLock,
                     0,
                     0);
    }

    Track->SpinLock    = SpinLock;
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

    if (CurrentIrql > DISPATCH_LEVEL)
    {
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     CurrentIrql,
                     DISPATCH_LEVEL,
                     (ULONG_PTR)SpinLock,
                     0);
    }

    for (LONG i = (LONG)Stack->Count - 1; i >= 0; i--)
    {
        if (Stack->HeldLocks[i] == SpinLock)
        {
            for (LONG j = i; j < (LONG)Stack->Count - 1; j++)
                Stack->HeldLocks[j] = Stack->HeldLocks[j + 1];
            Stack->Count--;
            Found = TRUE;
            break;
        }
    }

    if (!Found)
    {
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     VF_VIOLATION_SPINLOCK_RELEASE,
                     (ULONG_PTR)SpinLock,
                     0,
                     0);
    }

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

    KeReleaseSpinLock(SpinLock, OldIrql);
}

/* ============================================================
   DMA ADAPTER VERIFICATION
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
                DRIVER_VERIFIER_DETECTED_VIOLATION,
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
        Track = CONTAINING_RECORD(L, VF_DMA_ADAPTER_TRACK, ListEntry);
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

    Track = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(VF_DMA_ADAPTER_TRACK),
        'dIrV'
    );

    if (!Track)
    {
        KeBugCheckEx(
            DRIVER_VERIFIER_DETECTED_VIOLATION,
            VF_VIOLATION_INVALID_DMA_ADAPTER,
            (ULONG_PTR)Adapter,
            0,
            0
        );
        return NULL; /* unreachable */
    }

    RtlZeroMemory(Track, sizeof(*Track));
    Track->OriginalOps  = *Adapter->DmaOperations;
    Track->Adapter      = Adapter;
    Track->DriverObject = DeviceObject->DriverObject;

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
            DRIVER_VERIFIER_DETECTED_VIOLATION,
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
            DRIVER_VERIFIER_DETECTED_VIOLATION,
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
        T = CONTAINING_RECORD(L, VF_DMA_ADAPTER_TRACK, ListEntry);
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

    Driver->Unloads++;

    if (!IsListEmpty(&Driver->PoolList))
    {
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     VF_SUBCODE_POOL_NOT_FREED_ON_UNLOAD,
                     (ULONG_PTR)DriverObject,
                     0, 0);
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
        DriverObject = PhysicalDeviceObject->DriverObject;

    DbgPrintEx(DPFLTR_VERIFIER_ID, DPFLTR_ERROR_LEVEL,
               "VERIFIER: VfFailDeviceNode\n");

    DbgPrintEx(DPFLTR_VERIFIER_ID, DPFLTR_ERROR_LEVEL,
               "VERIFIER: PhysicalDeviceObject %p\n"
               "VERIFIER: DriverObject %p\n",
               PhysicalDeviceObject,
               DriverObject);

    if (DebuggerMessageText)
    {
        DbgPrintEx(DPFLTR_VERIFIER_ID, DPFLTR_ERROR_LEVEL,
                   "VERIFIER: %s\n", DebuggerMessageText);
    }

    if (ParameterFormatString)
    {
        va_start(VaList, ParameterFormatString);
        vDbgPrintEx(DPFLTR_VERIFIER_ID, DPFLTR_ERROR_LEVEL,
                    ParameterFormatString, VaList);
        va_end(VaList);
        DbgPrintEx(DPFLTR_VERIFIER_ID, DPFLTR_ERROR_LEVEL, "\n");
    }

    if (AssertionControl && (*AssertionControl == 0))
        return;

#if DBG
    if (FailureClass == VfFatalFailure)
        DbgBreakPoint();
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
        DRIVER_VERIFIER_DETECTED_VIOLATION,
        VF_VIOLATION_LEAKED_RESOURCES,
        (ULONG_PTR)DriverObject,
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
        DRIVER_VERIFIER_DETECTED_VIOLATION,
        VF_VIOLATION_FIRMWARE_BIOS,
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
