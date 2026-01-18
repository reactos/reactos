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
   VERIFIER FLAGS
   ============================================================ */

#define VF_FLAG_SPECIAL_POOL          0x00000001
#define VF_FLAG_DMA_FAULT_INJECTION   0x00000001
#define VF_FLAG_IRQL_CHECKING         0x00000002
#define VF_FLAG_POOL_TRACKING         0x00000004

#define TAG_VFDRV  'DrFV'
#define TAG_VFALL  'AlFV'
#define TAG_VFSP   'pfSV'

/* ============================================================
   BUGCHECK CODES (MATCH NT)
   ============================================================ */

#define VF_BUGCHECK_DRIVER_VIOLATION   0xC9
#define VF_BUGCHECK_SPECIAL_POOL       0xF0
#define VF_BUGCHECK_MEMORY_LEAK        0xC4
#define VF_BUGCHECK_INVALID_FREE       0xF7

/* ============================================================
   VERIFIER VIOLATION CODES (BUGCHECK PARAM 1)
   ============================================================ */

#define VF_VIOLATION_IRQL_MISUSE              0xC9
#define VF_VIOLATION_INVALID_IRP_STATE        0xC8
#define VF_VIOLATION_CANCEL_ROUTINE_MISSING   0xC7
#define VF_VIOLATION_IRP_NOT_MARKED_PENDING   0xC6
#define VF_VIOLATION_DOUBLE_COMPLETE          0xC5
#define VF_VIOLATION_LEAKED_RESOURCES         0xC4
#define VF_VIOLATION_REUSED_IRP               0xC3
#define VF_VIOLATION_COMPLETING_UNKNOWN_IRP   0xC2
#define VF_VIOLATION_INVALID_DMA_ADAPTER      0xCA
#define VF_VIOLATION_RECURSIVE_LOCK           0xCD
#define VF_VIOLATION_LOCK_ORDER_INVERSION     0xCE
#define VF_VIOLATION_DEADLOCK                 0xD0
#define VF_VIOLATION_SPINLOCK_RELEASE         0xCB
#define VF_VIOLATION_SPINLOCK_NOT_HELD        0xCF
#define VF_VIOLATION_PAGEABLE_CODE            0xCC
#define VF_VIOLATION_DEVICE_NODE              0xD1
#define VF_VIOLATION_FIRMWARE_BIOS            0xE0
#define VF_VIOLATION_SPINLOCK_DEPENDENCY      0x30C
#define VF_VIOLATION_SPINLOCK_TRACK           0x30D

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

extern BOOLEAN VfGlobalEnabled;
extern VF_GLOBAL_STATE VfGlobal;
extern BOOLEAN VfGlobalEnabled;
extern VF_DMA_FAULT_STATE VfDmaFaultState;

/* LIST ENTRIES */
LIST_ENTRY VfDriverList;
LIST_ENTRY VfIrpTrackList;
LIST_ENTRY VfIrpHookList;
LIST_ENTRY VfSpinlockList;
LIST_ENTRY VfDmaAdapterList;
LIST_ENTRY VfThreadLockList;
LIST_ENTRY VfSpinlockDependencyList;

/* LOCKS */
KSPIN_LOCK VfSpinlockLock;
KSPIN_LOCK VfDmaLock;
KSPIN_LOCK VfDriverListLock;
KSPIN_LOCK VfIrpTrackLock;
KSPIN_LOCK VfIrpHookLock;
KSPIN_LOCK VfThreadLockListLock;
KSPIN_LOCK VfSpinlockDepLock;

/* ============================================================
   STRUCTURES
   ============================================================ */

typedef struct _VF_POOL_ALLOCATION
{
    LIST_ENTRY ListEntry;
    PVOID Address;
    SIZE_T Size;
    ULONG Tag;
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
} VF_DRIVER_ENTRY;

typedef struct _VF_IRP_TRACK
{
    LIST_ENTRY ListEntry;
    LONG ReferenceCount;
    BOOLEAN CancelRoutineSet;
    PIRP Irp;
    PDRIVER_OBJECT DriverObject;
    UCHAR MajorFunction;
    KIRQL DispatchIrql;
    BOOLEAN PendingReturned;
    BOOLEAN Completed;
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

    DMA_OPERATIONS  OriginalOps;   // <- copy of real ops

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

typedef struct _VF_IRP_TRACK_EXT
{
    LIST_ENTRY ListEntry;
    PIRP Irp;
    LONG ReferenceCount;
    BOOLEAN CancelRoutineSet;
    BOOLEAN Completed;
} VF_IRP_TRACK_EXT;

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

static
VOID
VfCheckPageableCode(
  PVOID Address UNUSED,
  PDRIVER_OBJECT DriverObject UNUSED
);

VOID
VfValidateDmaAdapter(
  PDMA_ADAPTER Adapter
);

static 
VF_DMA_ADAPTER_TRACK* 
VfLookupDmaAdapter(
  PDMA_ADAPTER Adapter
);

static 
BOOLEAN 
UNUSED 
VfShouldInjectDmaFault(
  VF_DMA_FAULT_TYPE Type
);

static 
VF_DMA_ADAPTER_TRACK*
VfFindAdapter(
    PDMA_ADAPTER Adapter
);

static
VF_IRP_TRACK*
VfLookupIrp(
    _In_ PIRP Irp
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

VOID VfFailDeviceNode(
    PDEVICE_OBJECT PhysicalDeviceObject,
    ULONG BugCheckMajorCode,
    ULONG BugCheckMinorCode,
    VF_FAILURE_CLASS FailureClass,
    PULONG AssertionControl,
    PSTR DebuggerMessageText,
    PSTR ParameterFormatString,
    ...
);

VOID
__cdecl
VfFailSystemBIOS(
    PCSTR Message
);

#ifdef __cplusplus
}
#endif