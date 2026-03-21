/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Driver Verifier internal header
 * COPYRIGHT:   Copyright 2026 ReactOS Contributors
 */

#pragma once

#include <ntoskrnl.h>
#include <ntddk.h>
#include <ntdef.h>
#include <ntifs.h>
#include <wdm.h>
#include <debug.h>
#include <ndk/vftypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _VF_SETTINGS {
    ULONG Reserved;
} VF_SETTINGS;

/* ============================================================
   VERIFIER FLAGS & TAGS
   ============================================================ */

#define VF_FLAG_SPECIAL_POOL          0x00000001
#define VF_FLAG_DMA_FAULT_INJECTION   0x00000002
#define VF_FLAG_IRQL_CHECKING         0x00000004
#define VF_FLAG_POOL_TRACKING         0x00000008

#define TAG_VFDRV  'DrFV'
#define TAG_VFALL  'AlFV'
#define TAG_VFSP   'pfSV'

/* ============================================================
   BUGCHECK CODES — use Windows-compatible names directly
   ============================================================ */

/* Use DRIVER_VERIFIER_DETECTED_VIOLATION (0xC4) directly in KeBugCheckEx calls */
/* Use SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION (0xC1) for special pool issues  */
/* Use DRIVER_VERIFIER_IOMANAGER_VIOLATION (0xC9) for I/O manager violations   */

/* ============================================================
   VERIFIER VIOLATION CODES (BUGCHECK PARAM 1) - ReactOS-specific
   These use high-order bits to avoid collision with MSFT-defined codes.
   ============================================================ */

#define VF_VIOLATION_COMPLETING_UNKNOWN_IRP   0x80000001
#define VF_VIOLATION_REUSED_IRP               0x80000002
#define VF_VIOLATION_LEAKED_RESOURCES         0x80000003
#define VF_VIOLATION_DOUBLE_COMPLETE          0x80000004
#define VF_VIOLATION_IRP_NOT_MARKED_PENDING   0x80000005
#define VF_VIOLATION_CANCEL_ROUTINE_MISSING   0x80000006
#define VF_VIOLATION_INVALID_IRP_STATE        0x80000007
#define VF_VIOLATION_IRQL_MISUSE              0x80000008
#define VF_VIOLATION_INVALID_DMA_ADAPTER      0x80000009
#define VF_VIOLATION_SPINLOCK_RELEASE         0x8000000A
#define VF_VIOLATION_SPINLOCK_DEPENDENCY      0x8000000B
#define VF_VIOLATION_SPINLOCK_TRACK           0x8000000C
#define VF_VIOLATION_SPINLOCK_NOT_HELD        0x8000000D
#define VF_VIOLATION_RECURSIVE_LOCK           0x8000000E
#define VF_VIOLATION_LOCK_ORDER_INVERSION     0x8000000F
#define VF_VIOLATION_DEADLOCK                 0x80000010
#define VF_VIOLATION_PAGEABLE_CODE            0x80000011
#define VF_VIOLATION_DEVICE_NODE              0x80000012
#define VF_VIOLATION_FIRMWARE_BIOS            0x80000013

/* ============================================================ 
   SUBCODES (win-compatible 0xC4 subcodes)
   ============================================================ */
#define VF_SUBCODE_POOL_NOT_FREED_ON_UNLOAD 0x62
#define VF_SUBCODE_WRONG_POOL_TAG           0x16
#define VF_SUBCODE_WRONG_POOL_TYPE          0x17
#define VF_SUBCODE_INVALID_FREE             0x1A

/* ============================================================ 
   KEY AND ZW
   ============================================================ */
#define VF_REG_KEY L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\" \
                   L"Control\\Session Manager\\Memory Management"
#define VF_DEFAULT_LEVEL (VF_FLAG_POOL_TRACKING | VF_FLAG_IRQL_CHECKING | \
                          VF_FLAG_SPECIAL_POOL)
#define VF_DEFAULT_DRIVERS L"*"

#if defined(_MSC_VER)
#define UNUSED
#else
#define UNUSED __attribute__((unused))
#endif

/* ============================================================
   GLOBAL STATE
   ============================================================ */
typedef struct _VF_GLOBAL_STATE
{
    BOOLEAN Enabled;
    ULONG GlobalFlags;
    UNICODE_STRING VerifyDriverList;
} VF_GLOBAL_STATE;

typedef enum _VF_DMA_FAULT_TYPE
{
    VfDmaFaultNone = 0,
    VfDmaFaultAllocateChannel,
    VfDmaFaultMapTransfer,
    VfDmaFaultAllocateCommonBuffer,
    VfDmaFaultFlushBuffers
} VF_DMA_FAULT_TYPE;

typedef struct _VF_DMA_FAULT_STATE
{
    BOOLEAN Enabled;
    ULONG Probability;
    ULONG Counter;
    VF_DMA_FAULT_TYPE FaultType;
} VF_DMA_FAULT_STATE;

extern VF_GLOBAL_STATE VfGlobal;
extern BOOLEAN VfGlobalEnabled;
extern VF_DMA_FAULT_STATE VfDmaFaultState;

/* LIST ENTRIES */
extern LIST_ENTRY VfDriverList;
extern LIST_ENTRY VfIrpTrackList;
extern LIST_ENTRY VfIrpHookList;
extern LIST_ENTRY VfSpinlockList;
extern LIST_ENTRY VfDmaAdapterList;
extern LIST_ENTRY VfThreadLockList;
extern LIST_ENTRY VfSpinlockDependencyList;

/* LOCKS */
extern KSPIN_LOCK VfSpinlockLock;
extern KSPIN_LOCK VfDmaLock;
extern KSPIN_LOCK VfDriverListLock;
extern KSPIN_LOCK VfIrpTrackLock;
extern KSPIN_LOCK VfIrpHookLock;
extern KSPIN_LOCK VfThreadLockListLock;
extern KSPIN_LOCK VfSpinlockDepLock;

/* ============================================================
   STRUCTURES
   ============================================================ */

typedef struct _VF_POOL_ALLOCATION
{
    LIST_ENTRY ListEntry;
    PVOID Address;
    SIZE_T Size;
    ULONG Tag;
    POOL_TYPE PoolType;
    BOOLEAN SpecialPool;
    PMDL Mdl;
    KIRQL AllocateIrql;
} VF_POOL_ALLOCATION;

typedef struct _VF_DRIVER_ENTRY
{
    LIST_ENTRY ListEntry;
    PDRIVER_OBJECT DriverObject;
    ULONG VerifierFlags;
    LIST_ENTRY PoolList;
    KSPIN_LOCK PoolLock;
    PDRIVER_UNLOAD OriginalUnload;
    SIZE_T PoolUsage;
    SIZE_T PoolQuota;

    /* stats for SystemVerifierInformation */
    ULONG AllocationsAttempted;
    ULONG AllocationsSucceeded;
    ULONG AllocationsSucceededSpecialPool;
    ULONG AllocationsWithNoTag;
    ULONG AllocationsFailed;
    ULONG AllocationsFailedDeliberately;
    ULONG Loads;
    ULONG Unloads;
    ULONG CurrentPagedPoolAllocations;
    ULONG CurrentNonPagedPoolAllocations;
    ULONG PeakPagedPoolAllocations;
    ULONG PeakNonPagedPoolAllocations;
    SIZE_T PagedPoolUsageInBytes;
    SIZE_T NonPagedPoolUsageInBytes;
    SIZE_T PeakPagedPoolUsageInBytes;
    SIZE_T PeakNonPagedPoolUsageInBytes;
} VF_DRIVER_ENTRY;

typedef struct _VF_IRP_TRACK
{
    LIST_ENTRY  ListEntry;
    PIRP        Irp;
    PDRIVER_OBJECT DriverObject;
    LONG        ReferenceCount;
    UCHAR       MajorFunction;
    KIRQL       DispatchIrql;
    BOOLEAN     PendingReturned;
    BOOLEAN     Completed;
    BOOLEAN     CancelRoutineSet;
} VF_IRP_TRACK, *PVF_IRP_TRACK;

typedef struct _VF_SPINLOCK_TRACK
{
    LIST_ENTRY ListEntry;
    PKSPIN_LOCK SpinLock;
    KIRQL AcquireIrql;
    PETHREAD OwnerThread;
} VF_SPINLOCK_TRACK;

typedef struct _VF_DMA_ADAPTER_TRACK
{
    LIST_ENTRY      ListEntry;
    PDMA_ADAPTER    Adapter;
    PDRIVER_OBJECT  DriverObject;
    DMA_OPERATIONS  OriginalOps;
    LONG            MapRegisterCount;
    BOOLEAN         AdapterReleased;
} VF_DMA_ADAPTER_TRACK;

typedef struct _VF_DMA_OPS
{
    DMA_OPERATIONS Original;
} VF_DMA_OPS;

typedef struct _VF_IRP_HOOK
{
    LIST_ENTRY ListEntry;
    PDRIVER_OBJECT DriverObject;
    PDRIVER_DISPATCH OriginalMajor[IRP_MJ_MAXIMUM_FUNCTION + 1];
} VF_IRP_HOOK;

typedef struct _VF_THREAD_LOCK_STACK
{
    LIST_ENTRY ListEntry;
    PETHREAD Thread;
    PKSPIN_LOCK* HeldLocks;
    ULONG Count;
    ULONG Capacity;
} VF_THREAD_LOCK_STACK;

typedef struct _VF_SPINLOCK_DEPENDENCY
{
    LIST_ENTRY ListEntry;
    PKSPIN_LOCK HeldLock;
    PKSPIN_LOCK AcquiredLock;
} VF_SPINLOCK_DEPENDENCY;

/* ============================================================
   FUNCTION PROTOTYPES
   ============================================================ */

NTSTATUS
VfIoIncrementRef(
    PIRP Irp
);

VOID
VfIoDecrementRef(
    PIRP Irp
);

VOID
VfIoCompleteRequest(
    PIRP Irp,
    CCHAR PriorityBoost
);

VOID
VfValidateDmaAdapter(
    PDMA_ADAPTER Adapter
);

__declspec(dllexport)
BOOLEAN
__stdcall
VfIsVerificationEnabled(
    ULONG Flags,
    ULONG Reserved
);

VOID
__cdecl
VfFailDriver(
    PDRIVER_OBJECT DriverObject,
    PCSTR Message
);

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
);

VOID NTAPI VfRegisterDriver(
    PDRIVER_OBJECT DriverObject
);

VOID VfUnregisterDriver(
    PDRIVER_OBJECT DriverObject
);

VOID NTAPI VfInitialize(
    VOID
);

PDRIVER_OBJECT
VfGetDriverByAddress(
    PVOID Address
);

PVOID NTAPI
VfAllocatePool(
    PDRIVER_OBJECT DriverObject,
    POOL_TYPE PoolType,
    SIZE_T Size,
    ULONG Tag
);

VOID VfHookDriverUnload(
    PDRIVER_OBJECT DriverObject
);

VOID NTAPI VfFreePool(
    PDRIVER_OBJECT DriverObject,
    PVOID Address,
    ULONG PoolTag,
    POOL_TYPE PoolType
);

VOID
__cdecl
VfFailSystemBIOS(
    PCSTR Message
);

#ifdef __cplusplus
}
#endif