/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   verifier.c

Abstract:

    This module contains the routines to verify system drivers.

Author:

    Landy Wang (landyw) 3-Sep-1998

Revision History:

--*/
#include "mi.h"

#define THUNKED_API

THUNKED_API
PVOID
VerifierAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    );

THUNKED_API
PVOID
VerifierAllocatePoolWithTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

THUNKED_API
PVOID
VerifierAllocatePoolWithQuotaTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

THUNKED_API
PVOID
VerifierAllocatePoolWithTagPriority(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN EX_POOL_PRIORITY Priority
    );

PVOID
VeAllocatePoolWithTagPriority(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN EX_POOL_PRIORITY Priority,
    IN PVOID CallingAddress
    );

VOID
VerifierFreePool(
    IN PVOID P
    );

THUNKED_API
VOID
VerifierFreePoolWithTag(
    IN PVOID P,
    IN ULONG TagToFree
    );

THUNKED_API
LONG
VerifierSetEvent(
    IN PRKEVENT Event,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait
    );

THUNKED_API
KIRQL
FASTCALL
VerifierKfRaiseIrql (
    IN KIRQL NewIrql
    );

THUNKED_API
VOID
FASTCALL
VerifierKfLowerIrql (
    IN KIRQL NewIrql
    );

THUNKED_API
VOID
VerifierKeRaiseIrql (
    IN KIRQL NewIrql,
    OUT PKIRQL OldIrql
    );

THUNKED_API
VOID
VerifierKeLowerIrql (
    IN KIRQL NewIrql
    );

THUNKED_API
VOID
VerifierKeAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock,
    OUT PKIRQL OldIrql
    );

THUNKED_API
VOID
VerifierKeReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    );

THUNKED_API
KIRQL
FASTCALL
VerifierKfAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

THUNKED_API
VOID
FASTCALL
VerifierKfReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    );

THUNKED_API
VOID
VerifierKeInitializeTimerEx(
    IN PKTIMER Timer,
    IN TIMER_TYPE Type
    );

THUNKED_API
VOID
VerifierKeInitializeTimer(
    IN PKTIMER Timer
    );

THUNKED_API
BOOLEAN
FASTCALL
VerifierExTryToAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    );

THUNKED_API
VOID
FASTCALL
VerifierExAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    );

THUNKED_API
VOID
FASTCALL
VerifierExReleaseFastMutex (
    IN PFAST_MUTEX FastMutex
    );

THUNKED_API
VOID
FASTCALL
VerifierExAcquireFastMutexUnsafe (
    IN PFAST_MUTEX FastMutex
    );

THUNKED_API
VOID
FASTCALL
VerifierExReleaseFastMutexUnsafe (
    IN PFAST_MUTEX FastMutex
    );

THUNKED_API
BOOLEAN
VerifierExAcquireResourceExclusive(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
    );

THUNKED_API
VOID
FASTCALL
VerifierExReleaseResource(
    IN PERESOURCE Resource
    );

THUNKED_API
KIRQL
FASTCALL
VerifierKeAcquireQueuedSpinLock (
    IN KSPIN_LOCK_QUEUE_NUMBER Number
    );

THUNKED_API
VOID
FASTCALL
VerifierKeReleaseQueuedSpinLock (
    IN KSPIN_LOCK_QUEUE_NUMBER Number,
    IN KIRQL OldIrql
    );

THUNKED_API
BOOLEAN
VerifierSynchronizeExecution (
    IN PKINTERRUPT Interrupt,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
    );

THUNKED_API
VOID
VerifierProbeAndLockPages (
    IN OUT PMDL MemoryDescriptorList,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    );

THUNKED_API
VOID
VerifierProbeAndLockProcessPages (
    IN OUT PMDL MemoryDescriptorList,
    IN PEPROCESS Process,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    );

THUNKED_API
VOID
VerifierProbeAndLockSelectedPages (
    IN OUT PMDL MemoryDescriptorList,
    IN PFILE_SEGMENT_ELEMENT SegmentArray,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    );

VOID
VerifierUnlockPages (
     IN OUT PMDL MemoryDescriptorList
     );

VOID
VerifierUnmapLockedPages (
     IN PVOID BaseAddress,
     IN PMDL MemoryDescriptorList
     );

VOID
VerifierUnmapIoSpace (
     IN PVOID BaseAddress,
     IN SIZE_T NumberOfBytes
     );

THUNKED_API
PVOID
VerifierMapIoSpace (
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN SIZE_T NumberOfBytes,
    IN MEMORY_CACHING_TYPE CacheType
    );

THUNKED_API
PVOID
VerifierMapLockedPages (
    IN PMDL MemoryDescriptorList,
    IN KPROCESSOR_MODE AccessMode
    );

THUNKED_API
PVOID
VerifierMapLockedPagesSpecifyCache (
    IN PMDL MemoryDescriptorList,
    IN KPROCESSOR_MODE AccessMode,
    IN MEMORY_CACHING_TYPE CacheType,
    IN PVOID RequestedAddress,
    IN ULONG BugCheckOnFailure,
    IN MM_PAGE_PRIORITY Priority
    );

VOID
VerifierFreeTrackedPool(
    IN PVOID VirtualAddress,
    IN SIZE_T ChargedBytes,
    IN LOGICAL CheckType,
    IN LOGICAL SpecialPool
    );

VOID
ViPrintString (
    IN PUNICODE_STRING DriverName
    );

LOGICAL
ViInjectResourceFailure (
    VOID
    );

VOID
ViTrimAllSystemPagableMemory (
    VOID
    );

VOID
ViInitializeEntry (
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier,
    IN LOGICAL FirstLoad
    );

LOGICAL
ViReservePoolAllocation (
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier
    );

ULONG_PTR
ViInsertPoolAllocation (
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier,
    IN PVOID VirtualAddress,
    IN PVOID CallingAddress,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

VOID
ViCancelPoolAllocation (
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier
    );

VOID
ViReleasePoolAllocation (
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier,
    IN PVOID VirtualAddress,
    IN ULONG_PTR ListIndex,
    IN SIZE_T ChargedBytes
    );

VOID
KfSanityCheckRaiseIrql (
    IN KIRQL NewIrql
    );

VOID
KfSanityCheckLowerIrql (
    IN KIRQL NewIrql
    );

MM_DRIVER_VERIFIER_DATA MmVerifierData;

//
// Any flags which can be modified on the fly without rebooting are set here.
//

ULONG VerifierModifyableOptions;
ULONG VerifierOptionChanges;

LIST_ENTRY MiSuspectDriverList;

LOGICAL MiVerifyAllDrivers;

WCHAR MiVerifyRandomDrivers;

ULONG MiActiveVerifies;

ULONG MiActiveVerifierThunks;

ULONG MiNoPageOnRaiseIrql;

ULONG MiVerifierStackProtectTime;

LOGICAL MmDontVerifyRandomDrivers = TRUE;

LOGICAL VerifierSystemSufficientlyBooted;

LARGE_INTEGER VerifierRequiredTimeSinceBoot = {(ULONG)(40 * 1000 * 1000 * 10), 1};

LOGICAL VerifierIsTrackingPool = FALSE;

KSPIN_LOCK VerifierListLock;

KSPIN_LOCK VerifierPoolLock;

FAST_MUTEX VerifierPoolMutex;

PRTL_BITMAP VerifierLargePagedPoolMap;

LIST_ENTRY MiVerifierDriverAddedThunkListHead;

extern LOGICAL MmSpecialPoolCatchOverruns;

LOGICAL KernelVerifier = FALSE;

ULONG KernelVerifierTickPage = 0x7;

LOGICAL
MiEnableVerifier (
    IN PLDR_DATA_TABLE_ENTRY DataTableEntry
    );

VOID
ViInsertVerifierEntry (
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier
    );

PVOID
ViPostPoolAllocation (
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes,
    IN POOL_TYPE PoolType,
    IN ULONG Tag,
    IN PVOID CallingAddress
    );

PMI_VERIFIER_DRIVER_ENTRY
ViLocateVerifierEntry (
    IN PVOID SystemAddress
    );

VOID
MiVerifierCheckThunks (
    IN PLDR_DATA_TABLE_ENTRY DataTableEntry
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiInitializeDriverVerifierList)
#if defined(_X86_)
#pragma alloc_text(INIT,MiEnableKernelVerifier)
#endif
#pragma alloc_text(PAGE,MiApplyDriverVerifier)
#pragma alloc_text(PAGE,MiEnableVerifier)
#pragma alloc_text(PAGE,ViPrintString)
#pragma alloc_text(PAGE,MmGetVerifierInformation)
#pragma alloc_text(PAGE,MmSetVerifierInformation)
#pragma alloc_text(PAGE,MmAddVerifierThunks)
#pragma alloc_text(PAGELK,MiVerifierCheckThunks)
#pragma alloc_text(PAGELK,MiVerifyingDriverUnloading)

#pragma alloc_text(PAGEVRFY,VerifierProbeAndLockPages)
#pragma alloc_text(PAGEVRFY,VerifierProbeAndLockProcessPages)
#pragma alloc_text(PAGEVRFY,VerifierProbeAndLockSelectedPages)
#pragma alloc_text(PAGEVRFY,VerifierUnlockPages)
#pragma alloc_text(PAGEVRFY,VerifierMapIoSpace)
#pragma alloc_text(PAGEVRFY,VerifierMapLockedPages)
#pragma alloc_text(PAGEVRFY,VerifierMapLockedPagesSpecifyCache)
#pragma alloc_text(PAGEVRFY,VerifierUnmapLockedPages)
#pragma alloc_text(PAGEVRFY,VerifierUnmapIoSpace)
#pragma alloc_text(PAGEVRFY,VerifierAllocatePool)
#pragma alloc_text(PAGEVRFY,VerifierAllocatePoolWithTag)
#pragma alloc_text(PAGEVRFY,VerifierAllocatePoolWithTagPriority)
#pragma alloc_text(PAGEVRFY,VerifierAllocatePoolWithQuotaTag)
#pragma alloc_text(PAGEVRFY,VerifierFreePool)
#pragma alloc_text(PAGEVRFY,VerifierFreePoolWithTag)
#pragma alloc_text(PAGEVRFY,VerifierKfRaiseIrql)
#pragma alloc_text(PAGEVRFY,VerifierKfLowerIrql)
#pragma alloc_text(PAGEVRFY,VerifierKeRaiseIrql)
#pragma alloc_text(PAGEVRFY,VerifierKeLowerIrql)
#pragma alloc_text(PAGEVRFY,VerifierKeAcquireSpinLock)
#pragma alloc_text(PAGEVRFY,VerifierKeReleaseSpinLock)
#pragma alloc_text(PAGEVRFY,VerifierKfAcquireSpinLock)
#pragma alloc_text(PAGEVRFY,VerifierKfReleaseSpinLock)
#pragma alloc_text(PAGEVRFY,VerifierKeInitializeTimer)
#pragma alloc_text(PAGEVRFY,VerifierKeInitializeTimerEx)
#pragma alloc_text(PAGEVRFY,VerifierExTryToAcquireFastMutex)
#pragma alloc_text(PAGEVRFY,VerifierExAcquireFastMutex)
#pragma alloc_text(PAGEVRFY,VerifierExReleaseFastMutex)
#pragma alloc_text(PAGEVRFY,VerifierExAcquireFastMutexUnsafe)
#pragma alloc_text(PAGEVRFY,VerifierExReleaseFastMutexUnsafe)
#pragma alloc_text(PAGEVRFY,VerifierExAcquireResourceExclusive)
#pragma alloc_text(PAGEVRFY,VerifierExReleaseResource)
#pragma alloc_text(PAGEVRFY,VerifierKeAcquireQueuedSpinLock)
#pragma alloc_text(PAGEVRFY,VerifierKeReleaseQueuedSpinLock)
#pragma alloc_text(PAGEVRFY,VerifierSynchronizeExecution)

#pragma alloc_text(PAGEVRFY,VerifierFreeTrackedPool)

#pragma alloc_text(PAGEVRFY,VeAllocatePoolWithTagPriority)
#pragma alloc_text(PAGEVRFY,ViInsertVerifierEntry)
#pragma alloc_text(PAGEVRFY,ViLocateVerifierEntry)
#pragma alloc_text(PAGEVRFY,ViPostPoolAllocation)
#pragma alloc_text(PAGEVRFY,ViInjectResourceFailure)
#pragma alloc_text(PAGEVRFY,ViTrimAllSystemPagableMemory)
#pragma alloc_text(PAGEVRFY,ViInitializeEntry)
#pragma alloc_text(PAGEVRFY,ViReservePoolAllocation)
#pragma alloc_text(PAGEVRFY,ViInsertPoolAllocation)
#pragma alloc_text(PAGEVRFY,ViCancelPoolAllocation)
#pragma alloc_text(PAGEVRFY,ViReleasePoolAllocation)
#pragma alloc_text(PAGEVRFY,KfSanityCheckRaiseIrql)
#pragma alloc_text(PAGEVRFY,KfSanityCheckLowerIrql)
#endif

typedef struct _VERIFIER_THUNKS {
    PDRIVER_VERIFIER_THUNK_ROUTINE  PristineRoutine;
    PDRIVER_VERIFIER_THUNK_ROUTINE  NewRoutine;
    ULONG           Flag;
} VERIFIER_THUNKS, *PVERIFIER_THUNKS;

typedef struct _DRIVER_SPECIFIED_VERIFIER_THUNKS {
    LIST_ENTRY ListEntry;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    ULONG NumberOfThunks;
} DRIVER_SPECIFIED_VERIFIER_THUNKS, *PDRIVER_SPECIFIED_VERIFIER_THUNKS;

#if defined (_X86_)

#define VI_KE_RAISE_IRQL            0
#define VI_KE_LOWER_IRQL            1
#define VI_KE_ACQUIRE_SPINLOCK      2
#define VI_KE_RELEASE_SPINLOCK      3
#define VI_KF_RAISE_IRQL            4
#define VI_KF_LOWER_IRQL            5
#define VI_KF_ACQUIRE_SPINLOCK      6
#define VI_KF_RELEASE_SPINLOCK      7
#define VI_KE_ACQUIRE_QUEUED_SPINLOCK      8
#define VI_KE_RELEASE_QUEUED_SPINLOCK      9

#define VI_HALMAX                   10

PVOID MiKernelVerifierOriginalCalls[VI_HALMAX];

#endif

THUNKED_API
VOID
VerifierProbeAndLockPages (
     IN OUT PMDL MemoryDescriptorList,
     IN KPROCESSOR_MODE AccessMode,
     IN LOCK_OPERATION Operation
     )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();
    if (CurrentIrql > DISPATCH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x70,
                      CurrentIrql,
                      (ULONG_PTR)MemoryDescriptorList,
                      (ULONG_PTR)AccessMode);
    }

    if (ViInjectResourceFailure () == TRUE) {
        ExRaiseStatus (STATUS_WORKING_SET_QUOTA);
    }

    MmProbeAndLockPages (MemoryDescriptorList, AccessMode, Operation);
}

THUNKED_API
VOID
VerifierProbeAndLockProcessPages (
    IN OUT PMDL MemoryDescriptorList,
    IN PEPROCESS Process,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();
    if (CurrentIrql > DISPATCH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x71,
                      CurrentIrql,
                      (ULONG_PTR)MemoryDescriptorList,
                      (ULONG_PTR)Process);
    }

    if (ViInjectResourceFailure () == TRUE) {
        ExRaiseStatus (STATUS_WORKING_SET_QUOTA);
    }

    MmProbeAndLockProcessPages (MemoryDescriptorList,
                                Process,
                                AccessMode,
                                Operation);
}

THUNKED_API
VOID
VerifierProbeAndLockSelectedPages (
    IN OUT PMDL MemoryDescriptorList,
    IN PFILE_SEGMENT_ELEMENT SegmentArray,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();
    if (CurrentIrql > APC_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x72,
                      CurrentIrql,
                      (ULONG_PTR)MemoryDescriptorList,
                      (ULONG_PTR)AccessMode);
    }

    if (ViInjectResourceFailure () == TRUE) {
        ExRaiseStatus (STATUS_WORKING_SET_QUOTA);
    }

    MmProbeAndLockSelectedPages (MemoryDescriptorList,
                                 SegmentArray,
                                 AccessMode,
                                 Operation);
}

//
// It would be great to rip out the ViBadMapper code but some drivers will
// never get fixed if this is done.
//

#define VI_BAD_MAPPERS_MAX  100

KSPIN_LOCK ViBadMapperLock;
PVOID ViBadMappers[VI_BAD_MAPPERS_MAX];

LOGICAL
ViAddBadMapper (
    IN PVOID BadMapper
    )
{
    ULONG i;
    KIRQL OldIrql;
    LOGICAL Added;

    Added = FALSE;
    ExAcquireSpinLock (&ViBadMapperLock, &OldIrql);

    for (i = 0; i < VI_BAD_MAPPERS_MAX; i += 1) {
        if (ViBadMappers[i] == BadMapper) {
            break;
        }

        if (ViBadMappers[i] == NULL) {
            ViBadMappers[i] = BadMapper;
            Added = TRUE;
            break;
        }
    }

    ExReleaseSpinLock (&ViBadMapperLock, OldIrql);

    return Added;
}

THUNKED_API
PVOID
VerifierMapIoSpace (
     IN PHYSICAL_ADDRESS PhysicalAddress,
     IN SIZE_T NumberOfBytes,
     IN MEMORY_CACHING_TYPE CacheType
     )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();
    if (CurrentIrql > DISPATCH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x73,
                      CurrentIrql,
                      (ULONG_PTR)PhysicalAddress.LowPart,
                      NumberOfBytes);
    }

    if (ViInjectResourceFailure () == TRUE) {
        return NULL;
    }

    return MmMapIoSpace (PhysicalAddress, NumberOfBytes, CacheType);
}

THUNKED_API
PVOID
VerifierMapLockedPages (
     IN PMDL MemoryDescriptorList,
     IN KPROCESSOR_MODE AccessMode
     )
{
    PVOID CallingAddress;
    PVOID CallersCaller;
    KIRQL CurrentIrql;

#if defined (_X86_)
    RtlGetCallersAddress(&CallingAddress, &CallersCaller);
#else
    CallingAddress = (PVOID)_ReturnAddress();
#endif

    CurrentIrql = KeGetCurrentIrql();

    if (AccessMode == KernelMode) {
        if (CurrentIrql > DISPATCH_LEVEL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x74,
                          CurrentIrql,
                          (ULONG_PTR)MemoryDescriptorList,
                          (ULONG_PTR)AccessMode);
        }
    }
    else {
        if (CurrentIrql > APC_LEVEL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x75,
                          CurrentIrql,
                          (ULONG_PTR)MemoryDescriptorList,
                          (ULONG_PTR)AccessMode);
        }
    }

    if ((MemoryDescriptorList->MdlFlags & MDL_MAPPING_CAN_FAIL) == 0) {

        if (ViAddBadMapper (CallingAddress) == TRUE) {

            //
            // All drivers must specify can fail.  We'd really like to bugcheck
            // here to get drivers to convert but cut them some slack for now.
            //

            DbgPrint ("*******************************************************************************\n");
            DbgPrint ("*                                                                             *\n");

            DbgPrint ("* The Driver Verifier has detected the driver at address %p\n", CallingAddress);
            DbgPrint ("* is calling MmMapLockedPages instead of MmMapLockedPagesSpecifyCache.\n");
            DbgPrint ("* This can cause the system to needlessly bugcheck whenever system\n");
            DbgPrint ("* resources are low.  This driver needs to be fixed.\n");

            DbgPrint ("*                                                                             *\n");
            DbgPrint ("*******************************************************************************\n");
        }
    }

    return MmMapLockedPages (MemoryDescriptorList, AccessMode);
}

THUNKED_API
PVOID
VerifierMapLockedPagesSpecifyCache (
     IN PMDL MemoryDescriptorList,
     IN KPROCESSOR_MODE AccessMode,
     IN MEMORY_CACHING_TYPE CacheType,
     IN PVOID RequestedAddress,
     IN ULONG BugCheckOnFailure,
     IN MM_PAGE_PRIORITY Priority
     )
{
    PVOID CallingAddress;
    PVOID CallersCaller;
    KIRQL CurrentIrql;

#if defined (_X86_)
    RtlGetCallersAddress(&CallingAddress, &CallersCaller);
#else
    CallingAddress = (PVOID)_ReturnAddress();
#endif

    CurrentIrql = KeGetCurrentIrql();
    if (AccessMode == KernelMode) {
        if (CurrentIrql > DISPATCH_LEVEL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x76,
                          CurrentIrql,
                          (ULONG_PTR)MemoryDescriptorList,
                          (ULONG_PTR)AccessMode);
        }
    }
    else {
        if (CurrentIrql > APC_LEVEL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x77,
                          CurrentIrql,
                          (ULONG_PTR)MemoryDescriptorList,
                          (ULONG_PTR)AccessMode);
        }
    }

    if ((MemoryDescriptorList->MdlFlags & MDL_MAPPING_CAN_FAIL) ||
        (BugCheckOnFailure == 0)) {

        if (ViInjectResourceFailure () == TRUE) {
            return NULL;
        }
    }
    else {

        //
        // All drivers must specify can fail or don't bugcheck.  We'd
        // really like to bugcheck here to get drivers to convert but
        // cut them some slack for now.
        //

        if (ViAddBadMapper (CallingAddress) == TRUE) {

            DbgPrint ("*******************************************************************************\n");
            DbgPrint ("*                                                                             *\n");

            DbgPrint ("* The Driver Verifier has detected the driver at address %p\n", CallingAddress);
            DbgPrint ("* is not calling the safe version of MmMapLockedPagesSpecifyCache.\n");
            DbgPrint ("* This can cause the system to needlessly bugcheck whenever system\n");
            DbgPrint ("* resources are low.  This driver needs to be fixed.\n");

            DbgPrint ("*                                                                             *\n");
            DbgPrint ("*******************************************************************************\n");
        }
    }

    return MmMapLockedPagesSpecifyCache (MemoryDescriptorList,
                                         AccessMode,
                                         CacheType,
                                         RequestedAddress,
                                         BugCheckOnFailure,
                                         Priority);
}

VOID
VerifierUnlockPages (
     IN OUT PMDL MemoryDescriptorList
     )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();
    if (CurrentIrql > DISPATCH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x78,
                      CurrentIrql,
                      (ULONG_PTR)MemoryDescriptorList,
                      0);
    }

    if ((MemoryDescriptorList->MdlFlags & MDL_PAGES_LOCKED) == 0) {

        //
        // The caller is trying to unlock an MDL that was never locked down.
        //

        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x7C,
                      (ULONG_PTR)MemoryDescriptorList,
                      (ULONG_PTR)MemoryDescriptorList->MdlFlags,
                      0);
    }

    if (MemoryDescriptorList->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) {

        //
        // Nonpaged pool should never be locked down.
        //

        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x7D,
                      (ULONG_PTR)MemoryDescriptorList,
                      (ULONG_PTR)MemoryDescriptorList->MdlFlags,
                      0);
    }

    MmUnlockPages (MemoryDescriptorList);
}

VOID
VerifierUnmapLockedPages (
     IN PVOID BaseAddress,
     IN PMDL MemoryDescriptorList
     )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();

    if (BaseAddress > MM_HIGHEST_USER_ADDRESS) {
        if (CurrentIrql > DISPATCH_LEVEL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x79,
                          CurrentIrql,
                          (ULONG_PTR)BaseAddress,
                          (ULONG_PTR)MemoryDescriptorList);
        }
    }
    else {
        if (CurrentIrql > APC_LEVEL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x7A,
                          CurrentIrql,
                          (ULONG_PTR)BaseAddress,
                          (ULONG_PTR)MemoryDescriptorList);
        }
    }

    MmUnmapLockedPages (BaseAddress, MemoryDescriptorList);
}

VOID
VerifierUnmapIoSpace (
     IN PVOID BaseAddress,
     IN SIZE_T NumberOfBytes
     )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();
    if (CurrentIrql > DISPATCH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x7B,
                      CurrentIrql,
                      (ULONG_PTR)BaseAddress,
                      (ULONG_PTR)NumberOfBytes);
    }

    MmUnmapIoSpace (BaseAddress, NumberOfBytes);
}

THUNKED_API
PVOID
VerifierAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    )
{
    PVOID CallingAddress;
    PVOID CallersCaller;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;

#if defined (_X86_)
    RtlGetCallersAddress(&CallingAddress, &CallersCaller);
#else
    CallingAddress = (PVOID)_ReturnAddress();
#endif

    if (KernelVerifier == TRUE) {

        Verifier = ViLocateVerifierEntry (CallingAddress);

        if ((Verifier == NULL) ||
            ((Verifier->Flags & VI_VERIFYING_DIRECTLY) == 0)) {

            return ExAllocatePool (PoolType | POOL_DRIVER_MASK, NumberOfBytes);
        }
        PoolType |= POOL_DRIVER_MASK;
    }

    MmVerifierData.AllocationsWithNoTag += 1;

    return VeAllocatePoolWithTagPriority (PoolType,
                                          NumberOfBytes,
                                          'parW',
                                          HighPoolPriority,
                                          CallingAddress);
}

THUNKED_API
PVOID
VerifierAllocatePoolWithTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )
{
    PVOID CallingAddress;
    PVOID CallersCaller;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;

#if defined (_X86_)
    RtlGetCallersAddress(&CallingAddress, &CallersCaller);
#else
    CallingAddress = (PVOID)_ReturnAddress();
#endif

    if (KernelVerifier == TRUE) {
        Verifier = ViLocateVerifierEntry (CallingAddress);

        if ((Verifier == NULL) ||
            ((Verifier->Flags & VI_VERIFYING_DIRECTLY) == 0)) {

            return ExAllocatePoolWithTag (PoolType | POOL_DRIVER_MASK,
                                          NumberOfBytes,
                                          Tag);
        }
        PoolType |= POOL_DRIVER_MASK;
    }

    return VeAllocatePoolWithTagPriority (PoolType,
                                          NumberOfBytes,
                                          Tag,
                                          HighPoolPriority,
                                          CallingAddress);
}

THUNKED_API
PVOID
VerifierAllocatePoolWithQuota(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    )
{
    PVOID Va;
    LOGICAL RaiseOnQuotaFailure;
    PVOID CallingAddress;
    PVOID CallersCaller;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;

#if defined (_X86_)
    RtlGetCallersAddress(&CallingAddress, &CallersCaller);
#else
    CallingAddress = (PVOID)_ReturnAddress();
#endif

    if (KernelVerifier == TRUE) {
        Verifier = ViLocateVerifierEntry (CallingAddress);

        if ((Verifier == NULL) ||
            ((Verifier->Flags & VI_VERIFYING_DIRECTLY) == 0)) {

            return ExAllocatePoolWithQuota (PoolType | POOL_DRIVER_MASK,
                                            NumberOfBytes);
        }
        PoolType |= POOL_DRIVER_MASK;
    }

    MmVerifierData.AllocationsWithNoTag += 1;

    if (PoolType & POOL_QUOTA_FAIL_INSTEAD_OF_RAISE) {
        RaiseOnQuotaFailure = FALSE;
        PoolType &= ~POOL_QUOTA_FAIL_INSTEAD_OF_RAISE;
    }
    else {
        RaiseOnQuotaFailure = TRUE;
    }

    Va = VeAllocatePoolWithTagPriority (PoolType,
                                        NumberOfBytes,
                                        'parW',
                                        HighPoolPriority,
                                        CallingAddress);

    if (Va == NULL) {
        if (RaiseOnQuotaFailure == TRUE) {
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }
    }

    return Va;
}

THUNKED_API
PVOID
VerifierAllocatePoolWithQuotaTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )
{
    PVOID Va;
    LOGICAL RaiseOnQuotaFailure;
    PVOID CallingAddress;
    PVOID CallersCaller;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;

#if defined (_X86_)
    RtlGetCallersAddress(&CallingAddress, &CallersCaller);
#else
    CallingAddress = (PVOID)_ReturnAddress();
#endif

    if (KernelVerifier == TRUE) {
        Verifier = ViLocateVerifierEntry (CallingAddress);

        if ((Verifier == NULL) ||
            ((Verifier->Flags & VI_VERIFYING_DIRECTLY) == 0)) {

            return ExAllocatePoolWithQuotaTag (PoolType | POOL_DRIVER_MASK,
                                               NumberOfBytes,
                                               Tag);
        }
        PoolType |= POOL_DRIVER_MASK;
    }

    if (PoolType & POOL_QUOTA_FAIL_INSTEAD_OF_RAISE) {
        RaiseOnQuotaFailure = FALSE;
        PoolType &= ~POOL_QUOTA_FAIL_INSTEAD_OF_RAISE;
    }
    else {
        RaiseOnQuotaFailure = TRUE;
    }

    Va = VeAllocatePoolWithTagPriority (PoolType,
                                        NumberOfBytes,
                                        Tag,
                                        HighPoolPriority,
                                        CallingAddress);

    if (Va == NULL) {
        if (RaiseOnQuotaFailure == TRUE) {
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }
    }

    return Va;
}

THUNKED_API
PVOID
VerifierAllocatePoolWithTagPriority(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN EX_POOL_PRIORITY Priority
    )

/*++

Routine Description:

    This thunked-in function:

        - Performs sanity checks on the caller.
        - Can optionally provide allocation failures to the caller.
        - Attempts to provide the allocation from special pool.
        - Tracks pool to ensure callers free everything they allocate.

--*/

{
    PVOID CallingAddress;
    PVOID CallersCaller;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;

#if defined (_X86_)
    RtlGetCallersAddress(&CallingAddress, &CallersCaller);
#else
    CallingAddress = (PVOID)_ReturnAddress();
#endif

    if (KernelVerifier == TRUE) {
        Verifier = ViLocateVerifierEntry (CallingAddress);

        if ((Verifier == NULL) ||
            ((Verifier->Flags & VI_VERIFYING_DIRECTLY) == 0)) {

            return ExAllocatePoolWithTagPriority (PoolType | POOL_DRIVER_MASK,
                                                  NumberOfBytes,
                                                  Tag,
                                                  Priority);
        }
        PoolType |= POOL_DRIVER_MASK;
    }

    return VeAllocatePoolWithTagPriority (PoolType,
                                          NumberOfBytes,
                                          Tag,
                                          Priority,
                                          CallingAddress);
}

LOGICAL
ViInjectResourceFailure (
    VOID
    )

/*++

Routine Description:

    This function determines whether a resource allocation should be
    deliberately failed.  This may be a pool allocation, MDL creation,
    system PTE allocation, etc.

Arguments:

    None.

Return Value:

    TRUE if the allocation should be failed.  FALSE otherwise.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below.

--*/

{
    LARGE_INTEGER CurrentTime;

    if ((MmVerifierData.Level & DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES) == 0) {
        return FALSE;
    }

    //
    // Don't fail any requests in the first 7 or 8 minutes as we want to
    // give the system enough time to boot.
    //

    if (VerifierSystemSufficientlyBooted == FALSE) {
        KeQuerySystemTime (&CurrentTime);
        if (CurrentTime.QuadPart > KeBootTime.QuadPart + VerifierRequiredTimeSinceBoot.QuadPart) {
            VerifierSystemSufficientlyBooted = TRUE;
        }
    }

    if (VerifierSystemSufficientlyBooted == TRUE) {

        KeQueryTickCount(&CurrentTime);

        if ((CurrentTime.LowPart & 0xF) == 0) {

            MmVerifierData.AllocationsFailedDeliberately += 1;

            //
            // Deliberately fail this request.
            //

            return TRUE;
        }
    }

    return FALSE;
}

LOGICAL
ViReservePoolAllocation (
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier
    )
{
    ULONG_PTR OldSize;
    ULONG_PTR NewSize;
    ULONG_PTR NewHashOffset;
    ULONG_PTR Increment;
    ULONG_PTR Entries;
    ULONG_PTR i;
    KIRQL OldIrql;
    PVOID NewHashTable;
    PVI_POOL_ENTRY HashEntry;
    PVI_POOL_ENTRY OldHashTable;

    ExAcquireSpinLock (&Verifier->VerifierPoolLock, &OldIrql);

    while (Verifier->PoolHashSize <= Verifier->CurrentPagedPoolAllocations + Verifier->CurrentNonPagedPoolAllocations + Verifier->PoolHashReserved) {

        //
        // More space is needed.  Try for it now.
        //

#define VI_POOL_ENTRIES_PER_PAGE (PAGE_SIZE / sizeof(VI_POOL_ENTRY))

        OldSize = Verifier->PoolHashSize * sizeof(VI_POOL_ENTRY);

        if (Verifier->PoolHashSize >= VI_POOL_ENTRIES_PER_PAGE) {
            Increment = PAGE_SIZE;
        }
        else {
            Increment = 16 * sizeof (VI_POOL_ENTRY);
        }

        ExReleaseSpinLock (&Verifier->VerifierPoolLock, OldIrql);

        NewSize = OldSize + Increment;

        if (NewSize < OldSize) {
            return FALSE;
        }

        //
        // Note POOL_DRIVER_MASK must be set to stop the recursion loop
        // when using the kernel verifier.
        //

        NewHashTable = ExAllocatePoolWithTagPriority (NonPagedPool | POOL_DRIVER_MASK,
                                                      NewSize,
                                                      'ppeV',
                                                      HighPoolPriority);

        ExAcquireSpinLock (&Verifier->VerifierPoolLock, &OldIrql);

        OldSize = Verifier->PoolHashSize * sizeof(VI_POOL_ENTRY);

        if (NewHashTable == NULL) {
            if (Verifier->PoolHashSize <= Verifier->CurrentPagedPoolAllocations + Verifier->CurrentNonPagedPoolAllocations + Verifier->PoolHashReserved) {
                ExReleaseSpinLock (&Verifier->VerifierPoolLock, OldIrql);
                return FALSE;
            }

            //
            // Another thread got here before us and space is available.
            //

            break;
        }

        if (NewSize != OldSize + Increment) {

            //
            // Another thread got here before us.
            //

            ExReleaseSpinLock (&Verifier->VerifierPoolLock, OldIrql);
            ExFreePool (NewHashTable);
            ExAcquireSpinLock (&Verifier->VerifierPoolLock, &OldIrql);
        }
        else {

            //
            // Rebuild the list into the new table.
            //

            OldHashTable = Verifier->PoolHash;
            if (OldHashTable != NULL) {
                RtlCopyMemory (NewHashTable, OldHashTable, OldSize);
            }

            //
            // Construct the freelist chaining it through any existing
            // list (any free entries must be already reserved).
            //

            HashEntry = (PVI_POOL_ENTRY) ((PCHAR)NewHashTable + OldSize);
            Entries = Increment / sizeof (VI_POOL_ENTRY);
            NewHashOffset = HashEntry - (PVI_POOL_ENTRY)NewHashTable;

            //
            // If list compaction becomes important then chaining it on the
            // end here will need to be revisited.
            //

            for (i = 0; i < Entries; i += 1) {
                HashEntry->FreeListNext = NewHashOffset + i + 1;
                HashEntry += 1;
            }
            HashEntry -= 1;
            HashEntry->FreeListNext = Verifier->PoolHashFree;
            Verifier->PoolHashFree = NewHashOffset;
            Verifier->PoolHash = NewHashTable;
            Verifier->PoolHashSize += Entries;

            //
            // Free the old table.
            //

            if (OldHashTable != NULL) {
                ExReleaseSpinLock (&Verifier->VerifierPoolLock, OldIrql);
                ExFreePool (OldHashTable);
                ExAcquireSpinLock (&Verifier->VerifierPoolLock, &OldIrql);
            }
        }
    }

    Verifier->PoolHashReserved += 1;

    ASSERT (Verifier->PoolHashSize >= Verifier->CurrentPagedPoolAllocations + Verifier->CurrentNonPagedPoolAllocations + Verifier->PoolHashReserved);

    ExReleaseSpinLock (&Verifier->VerifierPoolLock, OldIrql);

    return TRUE;
}

ULONG_PTR
ViInsertPoolAllocation (
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier,
    IN PVOID VirtualAddress,
    IN PVOID CallingAddress,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )

/*++

Routine Description:

    This function inserts the specified virtual address into the verifier
    list for this driver.

Arguments:

    Verifier - Supplies the verifier entry to update.

    VirtualAddress - Supplies the virtual address to insert.

    CallingAddress - Supplies the caller's address.

    NumberOfBytes - Supplies the number of bytes to allocate.

    Tag - Supplies the tag for the pool being allocated.

Return Value:

    The list index this virtual address was inserted at.

Environment:

    Kernel mode, DISPATCH_LEVEL.  The Verifier->VerifierPoolLock must be held.

--*/

{
    ULONG_PTR Index;
    PVI_POOL_ENTRY HashEntry;

    ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    // The list entry must be reserved in advance.
    //

    ASSERT (Verifier->PoolHashReserved != 0);

    ASSERT (Verifier->PoolHashSize >= Verifier->CurrentPagedPoolAllocations + Verifier->CurrentNonPagedPoolAllocations + Verifier->PoolHashReserved);

    //
    // Use the next free list entry.
    //

    Index = Verifier->PoolHashFree;
    ASSERT (Index != VI_POOL_FREELIST_END);

    HashEntry = Verifier->PoolHash + Index;

    Verifier->PoolHashFree = HashEntry->FreeListNext;

    Verifier->PoolHashReserved -= 1;

    ASSERT (((HashEntry->FreeListNext & MINLONG_PTR) == 0) ||
            (HashEntry->FreeListNext == VI_POOL_FREELIST_END));

    HashEntry->InUse.VirtualAddress = VirtualAddress;
    HashEntry->InUse.CallingAddress = CallingAddress;
    HashEntry->InUse.NumberOfBytes = NumberOfBytes;
    HashEntry->InUse.Tag = Tag;

    ASSERT ((HashEntry->FreeListNext & MINLONG_PTR) != 0);

    return Index;
}

VOID
ViReleasePoolAllocation (
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier,
    IN PVOID VirtualAddress,
    IN ULONG_PTR ListIndex,
    IN SIZE_T ChargedBytes
    )

/*++

Routine Description:

    This function removes the specified virtual address from the verifier
    list for this driver.

Arguments:

    Verifier - Supplies the verifier entry to update.

    VirtualAddress - Supplies the virtual address to release.

    ListIndex - Supplies the verifier pool hash index for the address being
                released.

    ChargedBytes - Supplies the bytes charged for this allocation.

Return Value:

    None.

Environment:

    Kernel mode, DISPATCH_LEVEL.  The Verifier->VerifierPoolLock must be held.

--*/

{
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER PageFrameIndex2;
    PVI_POOL_ENTRY HashEntry;
    PMMPTE PointerPte;

    ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (Verifier->PoolHash == NULL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x59,
                      (ULONG_PTR)VirtualAddress,
                      ListIndex,
                      (ULONG_PTR)Verifier);
    }

    //
    // Ensure that the list pointer has not been overrun and still
    // points at something decent.
    //

    HashEntry = Verifier->PoolHash + ListIndex;

    if (ListIndex >= Verifier->PoolHashSize) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x54,
                      (ULONG_PTR)VirtualAddress,
                      Verifier->PoolHashSize,
                      ListIndex);
    }

    if (HashEntry->InUse.VirtualAddress != VirtualAddress) {

        PageFrameIndex = 0;
        PageFrameIndex2 = 1;

        if ((!MI_IS_PHYSICAL_ADDRESS(VirtualAddress)) &&
            (MI_IS_PHYSICAL_ADDRESS(HashEntry->InUse.VirtualAddress))) {

            PointerPte = MiGetPteAddress(VirtualAddress);
            if (PointerPte->u.Hard.Valid == 1) {
                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

                PageFrameIndex2 = MI_CONVERT_PHYSICAL_TO_PFN (HashEntry->InUse.VirtualAddress);
            }
        }

        //
        // Caller overran and corrupted the virtual address - the linked
        // list cannot be counted on either.
        //

        if (PageFrameIndex != PageFrameIndex2) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x52,
                          (ULONG_PTR)VirtualAddress,
                          (ULONG_PTR)HashEntry->InUse.VirtualAddress,
                          ChargedBytes);
        }
    }

    if (HashEntry->InUse.NumberOfBytes != ChargedBytes) {

        //
        // Caller overran and corrupted the byte count - the linked
        // list cannot be counted on either.
        //

        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x51,
                      (ULONG_PTR)VirtualAddress,
                      (ULONG_PTR)HashEntry,
                      ChargedBytes);
    }

    //
    // Put this list entry into the freelist.
    //

    HashEntry->FreeListNext = Verifier->PoolHashFree;
    Verifier->PoolHashFree = HashEntry - Verifier->PoolHash;
}

VOID
ViCancelPoolAllocation (
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier
    )

/*++

Routine Description:

    This function removes a reservation from the verifier list for this driver.
    All reservations must be made in advance.  This routine is used when an
    earlier reservation is not going to be used (ie: the actual pool
    allocation failed so no reservation will be needed after all).

Arguments:

    Verifier - Supplies the verifier entry to update.

Return Value:

    None.

Environment:

    Kernel mode, DISPATCH_LEVEL or below, no verifier mutexes held.

--*/

{
    KIRQL OldIrql;

    ExAcquireSpinLock (&Verifier->VerifierPoolLock, &OldIrql);

    //
    // The hash entry reserved earlier is not going to be used after all.
    //

    ASSERT (Verifier->PoolHashReserved != 0);

    ASSERT (Verifier->PoolHashSize >= Verifier->CurrentPagedPoolAllocations + Verifier->CurrentNonPagedPoolAllocations + Verifier->PoolHashReserved);

    ASSERT (Verifier->PoolHashFree != VI_POOL_FREELIST_END);

    Verifier->PoolHashReserved -= 1;

    ExReleaseSpinLock (&Verifier->VerifierPoolLock, OldIrql);
}

PVOID
ViPostPoolAllocation (
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes,
    IN POOL_TYPE PoolType,
    IN ULONG Tag,
    IN PVOID CallingAddress
    )

/*++

Routine Description:

    This function performs verifier book-keeping on the allocation attempt.

Arguments:

    VirtualAddress - Supplies the virtual address that should be allocated.

    NumberOfBytes - Supplies the number of bytes to allocate.

    PoolType - Supplies the type of pool being allocated.

    Tag - Supplies the tag for the pool being allocated.

    CallingAddress - Supplies the caller's address.

Return Value:

    The virtual address the caller should use.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below.

--*/

{
    KIRQL OldIrql;
    PMI_VERIFIER_POOL_HEADER Header;
    SIZE_T ChargedBytes;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;
    ULONG_PTR InsertedIndex;
    LOGICAL SpecialPoolAllocation;
    PPOOL_HEADER PoolHeader;

    InterlockedIncrement ((PLONG)&MmVerifierData.AllocationsSucceeded);
    ChargedBytes = EX_REAL_POOL_USAGE(NumberOfBytes);
    SpecialPoolAllocation = FALSE;

    if (VirtualAddress >= MmSpecialPoolStart && VirtualAddress < MmSpecialPoolEnd) {
        ChargedBytes = NumberOfBytes;
        InterlockedIncrement ((PLONG)&MmVerifierData.AllocationsSucceededSpecialPool);
        SpecialPoolAllocation = TRUE;
    }
    else if (NumberOfBytes <= POOL_BUDDY_MAX) {
        ChargedBytes -= POOL_OVERHEAD;
    }
    else {

        //
        // This isn't exactly true but it does give the user a way to see
        // if this machine is large enough to support special pool 100%.
        //

        InterlockedIncrement ((PLONG)&MmVerifierData.AllocationsSucceededSpecialPool);
    }

    if ((PoolType & POOL_VERIFIER_MASK) == 0) {
        return VirtualAddress;
    }

    if (NumberOfBytes > POOL_BUDDY_MAX) {
        ASSERT (BYTE_OFFSET(VirtualAddress) == 0);
    }

    Verifier = ViLocateVerifierEntry (CallingAddress);
    ASSERT (Verifier != NULL);
    VerifierIsTrackingPool = TRUE;

    if (SpecialPoolAllocation == TRUE) {

        //
        // Carefully adjust the special pool page to move the verifier tracking
        // header to the front.  This allows the allocation to remain butted
        // against the end of the page so overruns can be detected immediately.
        //

        if (((ULONG_PTR)VirtualAddress & (PAGE_SIZE - 1))) {
            PoolHeader = (PPOOL_HEADER)(PAGE_ALIGN (VirtualAddress));
            Header = (PMI_VERIFIER_POOL_HEADER) (PoolHeader + 1);
            VirtualAddress = (PVOID) ((PCHAR)VirtualAddress + sizeof (MI_VERIFIER_POOL_HEADER));
        }
        else {
            PoolHeader = (PPOOL_HEADER)((PCHAR)PAGE_ALIGN (VirtualAddress) + PAGE_SIZE - POOL_OVERHEAD);
            Header = (PMI_VERIFIER_POOL_HEADER) (PoolHeader - 1);
        }
        // ASSERT (PoolHeader->Ulong1 & MI_SPECIAL_POOL_VERIFIER);
        PoolHeader->Ulong1 -= sizeof (MI_VERIFIER_POOL_HEADER);
        ChargedBytes -= sizeof (MI_VERIFIER_POOL_HEADER);
        PoolHeader->Ulong1 |= MI_SPECIAL_POOL_VERIFIER;
    }
    else {
        Header = (PMI_VERIFIER_POOL_HEADER)((PCHAR)VirtualAddress +
                         ChargedBytes -
                         sizeof(MI_VERIFIER_POOL_HEADER));
    }

    ASSERT (((ULONG_PTR)Header & (sizeof(ULONG) - 1)) == 0);


    Header->Verifier = Verifier;

    //
    // Enqueue the entry and update per-driver counters.
    // Note that paged pool allocations must be chained using nonpaged
    // pool to prevent deadlocks.
    //

    ExAcquireSpinLock (&Verifier->VerifierPoolLock, &OldIrql);

    InsertedIndex = ViInsertPoolAllocation (Verifier,
                                            VirtualAddress,
                                            CallingAddress,
                                            ChargedBytes,
                                            Tag);

    if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {

        Verifier->PagedBytes += ChargedBytes;
        if (Verifier->PagedBytes > Verifier->PeakPagedBytes) {
            Verifier->PeakPagedBytes = Verifier->PagedBytes;
        }

        Verifier->CurrentPagedPoolAllocations += 1;
        if (Verifier->CurrentPagedPoolAllocations > Verifier->PeakPagedPoolAllocations) {
            Verifier->PeakPagedPoolAllocations = Verifier->CurrentPagedPoolAllocations;
        }
    }
    else {
        Verifier->NonPagedBytes += ChargedBytes;
        if (Verifier->NonPagedBytes > Verifier->PeakNonPagedBytes) {
            Verifier->PeakNonPagedBytes = Verifier->NonPagedBytes;
        }

        Verifier->CurrentNonPagedPoolAllocations += 1;
        if (Verifier->CurrentNonPagedPoolAllocations > Verifier->PeakNonPagedPoolAllocations) {
            Verifier->PeakNonPagedPoolAllocations = Verifier->CurrentNonPagedPoolAllocations;
        }
    }

    ExReleaseSpinLock (&Verifier->VerifierPoolLock, OldIrql);

    //
    // Since the header for paged pool is paged, don't initialize it until the
    // spinlock above is released.
    //

    Header->ListIndex = InsertedIndex;

    //
    // Update systemwide counters.
    //

    if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {
        ExAcquireFastMutex (&VerifierPoolMutex);

        MmVerifierData.PagedBytes += ChargedBytes;
        if (MmVerifierData.PagedBytes > MmVerifierData.PeakPagedBytes) {
            MmVerifierData.PeakPagedBytes = MmVerifierData.PagedBytes;
        }

        MmVerifierData.CurrentPagedPoolAllocations += 1;
        if (MmVerifierData.CurrentPagedPoolAllocations > MmVerifierData.PeakPagedPoolAllocations) {
            MmVerifierData.PeakPagedPoolAllocations = MmVerifierData.CurrentPagedPoolAllocations;
        }

        ExReleaseFastMutex (&VerifierPoolMutex);
    }
    else {
        ExAcquireSpinLock (&VerifierPoolLock, &OldIrql);

        MmVerifierData.NonPagedBytes += ChargedBytes;
        if (MmVerifierData.NonPagedBytes > MmVerifierData.PeakNonPagedBytes) {
            MmVerifierData.PeakNonPagedBytes = MmVerifierData.NonPagedBytes;
        }

        MmVerifierData.CurrentNonPagedPoolAllocations += 1;
        if (MmVerifierData.CurrentNonPagedPoolAllocations > MmVerifierData.PeakNonPagedPoolAllocations) {
            MmVerifierData.PeakNonPagedPoolAllocations = MmVerifierData.CurrentNonPagedPoolAllocations;
        }

        ExReleaseSpinLock (&VerifierPoolLock, OldIrql);
    }

    return VirtualAddress;
}

PVOID
VeAllocatePoolWithTagPriority(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN EX_POOL_PRIORITY Priority,
    IN PVOID CallingAddress
    )

/*++

Routine Description:

    This routine is called both from ex\pool.c and directly within this module.

        - Performs sanity checks on the caller.
        - Can optionally provide allocation failures to the caller.
        - Attempts to provide the allocation from special pool.
        - Tracks pool to ensure callers free everything they allocate.

--*/

{
    PVOID VirtualAddress;
    EX_POOL_PRIORITY AllocationPriority;
    SIZE_T ChargedBytes;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;
    LOGICAL ReservedHash;
    ULONG HeaderSize;

    ExAllocatePoolSanityChecks (PoolType, NumberOfBytes);

    InterlockedIncrement ((PLONG)&MmVerifierData.AllocationsAttempted);

    if ((PoolType & MUST_SUCCEED_POOL_TYPE_MASK) == 0) {

        if (ViInjectResourceFailure () == TRUE) {

            //
            // Caller requested an exception - throw it here.
            //

            if ((PoolType & POOL_RAISE_IF_ALLOCATION_FAILURE) != 0) {
                ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
            }

            return NULL;
        }
    }

    ASSERT ((PoolType & POOL_VERIFIER_MASK) == 0);

    AllocationPriority = Priority;

    if (MmVerifierData.Level & DRIVER_VERIFIER_SPECIAL_POOLING) {

        //
        // Try for a special pool overrun allocation unless the caller has
        // explicitly specified otherwise.
        //

        if ((AllocationPriority & (LowPoolPrioritySpecialPoolOverrun | LowPoolPrioritySpecialPoolUnderrun)) == 0) {
            if (MmSpecialPoolCatchOverruns == TRUE) {
                AllocationPriority |= LowPoolPrioritySpecialPoolOverrun;
            }
            else {
                AllocationPriority |= LowPoolPrioritySpecialPoolUnderrun;
            }
        }
    }

    ReservedHash = FALSE;
    if (MmVerifierData.Level & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS) {

        if ((PoolType & SESSION_POOL_MASK) && (MiHydra == TRUE)) {

            //
            // Session pool is directly tracked by default already.
            //

            NOTHING;
        }
        else {
            HeaderSize = sizeof(MI_VERIFIER_POOL_HEADER);

            ChargedBytes = MI_ROUND_TO_SIZE (NumberOfBytes, sizeof(ULONG)) + HeaderSize;
            Verifier = ViLocateVerifierEntry (CallingAddress);

            if ((Verifier == NULL) ||
                ((Verifier->Flags & VI_VERIFYING_DIRECTLY) == 0)) {

                //
                // This can happen for many reasons including no framing (which
                // can cause RtlGetCallersAddress to return the wrong address),
                // etc.
                //

                MmVerifierData.UnTrackedPool += 1;
            }
            else if (ChargedBytes <= NumberOfBytes) {

                //
                // Don't let the verifier header transform a bad caller into a
                // good caller.  Fail via the fall through so an exception
                // can be thrown if asked for, etc.
                //

                MmVerifierData.UnTrackedPool += 1;
            }
            else if (((PoolType & MUST_SUCCEED_POOL_TYPE_MASK) == 0) ||
                     (ChargedBytes <= PAGE_SIZE)) {

                //
                // Any pool allocation that is allowed to fail or where the
                // total number of charged bytes fits in a page (nonpaged-
                // must-succeed requires this) is suitable for tracking.
                // Just ensure that the hash list has space for it.
                //

                if (ViReservePoolAllocation (Verifier) == TRUE) {
                    ReservedHash = TRUE;
                    NumberOfBytes = ChargedBytes;
                    PoolType |= POOL_VERIFIER_MASK;
                }
            }
            else {
                ASSERT ((PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool);
                MmVerifierData.UnTrackedPool += 1;
            }
        }
    }

    VirtualAddress = ExAllocatePoolWithTagPriority (PoolType,
                                                    NumberOfBytes,
                                                    Tag,
                                                    AllocationPriority);

    if (VirtualAddress == NULL) {
        MmVerifierData.AllocationsFailed += 1;

        if (ReservedHash == TRUE) {

            //
            // Release the hash table entry now as it's not needed.
            //

            ViCancelPoolAllocation (Verifier);
        }

        if ((PoolType & POOL_RAISE_IF_ALLOCATION_FAILURE) != 0) {
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }
        return NULL;
    }

    VirtualAddress = ViPostPoolAllocation (VirtualAddress,
                                           NumberOfBytes,
                                           PoolType,
                                           Tag,
                                           CallingAddress);

    return VirtualAddress;
}

VOID
VerifierFreeTrackedPool(
    IN PVOID VirtualAddress,
    IN SIZE_T ChargedBytes,
    IN LOGICAL CheckType,
    IN LOGICAL SpecialPool
    )

/*++

Routine Description:

    Called directly from the pool manager or the memory manager for verifier-
    tracked allocations.  The call to ExFreePool is already in progress.

Arguments:

    VirtualAddress - Supplies the virtual address being freed.

    ChargedBytes - Supplies the number of bytes charged to this allocation.

    CheckType - Supplies PagedPool or NonPagedPool.

    SpecialPool - Supplies TRUE if the allocation is from special pool.

Return Value:

    None.

Environment:

    Kernel mode.

    N.B.

    Callers freeing small pool allocations hold no locks or mutexes on entry.

    Callers freeing special pool hold no locks or mutexes on entry.

    Callers freeing pool of PAGE_SIZE or larger hold the PFN lock (for nonpaged
    allocations) or the PagedPool mutex (for paged allocations) on entry.

--*/

{
    KIRQL OldIrql;
    ULONG_PTR Index;
    PPOOL_HEADER PoolHeader;
    PMI_VERIFIER_POOL_HEADER Header;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;

    if (VerifierIsTrackingPool == FALSE) {

        //
        // The verifier is not enabled so the only way this routine is being
        // called is because the pool header is mangled or the caller specified
        // a bad address.  Either way it's a bugcheck.
        //

        KeBugCheckEx (BAD_POOL_CALLER,
                      0x99,
                      (ULONG_PTR)VirtualAddress,
                      0,
                      0);
    }

    if (SpecialPool == TRUE) {

        //
        // Special pool allocation.
        //

        if (((ULONG_PTR)VirtualAddress & (PAGE_SIZE - 1))) {
            PoolHeader = PAGE_ALIGN (VirtualAddress);
            Header = (PMI_VERIFIER_POOL_HEADER)(PoolHeader + 1);
        }
        else {
            PoolHeader = (PPOOL_HEADER)((PCHAR)PAGE_ALIGN (VirtualAddress) + PAGE_SIZE - POOL_OVERHEAD);
            Header = (PMI_VERIFIER_POOL_HEADER)(PoolHeader - 1);
        }
    }
    else if (PAGE_ALIGNED(VirtualAddress)) {

        //
        // Large page allocation.
        //

        Header = (PMI_VERIFIER_POOL_HEADER) ((PCHAR)VirtualAddress +
                     ChargedBytes -
                     sizeof(MI_VERIFIER_POOL_HEADER));
    }
    else {
        ChargedBytes -= POOL_OVERHEAD;
        Header = (PMI_VERIFIER_POOL_HEADER) ((PCHAR)VirtualAddress +
                     ChargedBytes -
                     sizeof(MI_VERIFIER_POOL_HEADER));
    }

    Verifier = Header->Verifier;

    //
    // Check the pointer now so we can give a more friendly bugcheck
    // rather than crashing below on a bad reference.
    //

    if ((((ULONG_PTR)Verifier & (sizeof(ULONG) - 1)) != 0) ||
        (!MmIsAddressValid(&Verifier->Signature)) ||
        (Verifier->Signature != MI_VERIFIER_ENTRY_SIGNATURE)) {

        //
        // The caller corrupted the saved verifier field.
        //

        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x53,
                      (ULONG_PTR)VirtualAddress,
                      (ULONG_PTR)Header,
                      (ULONG_PTR)Verifier);
    }

    Index = Header->ListIndex;

    ExAcquireSpinLock (&Verifier->VerifierPoolLock, &OldIrql);

    ViReleasePoolAllocation (Verifier,
                             VirtualAddress,
                             Index,
                             ChargedBytes);

    if (CheckType == PagedPool) {
        Verifier->PagedBytes -= ChargedBytes;
        Verifier->CurrentPagedPoolAllocations -= 1;

        ExReleaseSpinLock (&Verifier->VerifierPoolLock, OldIrql);

        ExAcquireFastMutex (&VerifierPoolMutex);
        MmVerifierData.PagedBytes -= ChargedBytes;
        MmVerifierData.CurrentPagedPoolAllocations -= 1;
        ExReleaseFastMutex (&VerifierPoolMutex);
    }
    else {
        Verifier->NonPagedBytes -= ChargedBytes;
        Verifier->CurrentNonPagedPoolAllocations -= 1;
        ExReleaseSpinLock (&Verifier->VerifierPoolLock, OldIrql);

        ExAcquireSpinLock (&VerifierPoolLock, &OldIrql);
        MmVerifierData.NonPagedBytes -= ChargedBytes;
        MmVerifierData.CurrentNonPagedPoolAllocations -= 1;
        ExReleaseSpinLock (&VerifierPoolLock, OldIrql);
    }
}

THUNKED_API
VOID
VerifierFreePool(
    IN PVOID P
    )
{
    if (KernelVerifier == TRUE) {
        ExFreePool (P);
        return;
    }

    VerifierFreePoolWithTag (P, 0);
}

THUNKED_API
VOID
VerifierFreePoolWithTag(
    IN PVOID P,
    IN ULONG TagToFree
    )
{
    if (KernelVerifier == TRUE) {
        ExFreePoolWithTag (P, TagToFree);
        return;
    }

    ExFreePoolSanityChecks (P);

    ExFreePoolWithTag (P, TagToFree);
}

THUNKED_API
LONG
VerifierSetEvent (
    IN PRKEVENT Event,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();
    if (CurrentIrql > DISPATCH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x80,
                      CurrentIrql,
                      (ULONG_PTR)Event,
                      (ULONG_PTR)0);
    }

    return KeSetEvent (Event, Increment, Wait);
}

THUNKED_API
BOOLEAN
VerifierExAcquireResourceExclusive(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql ();

    if ((CurrentIrql != APC_LEVEL) &&
        (!IS_SYSTEM_THREAD(PsGetCurrentThread())) &&
        (KeGetCurrentThread()->KernelApcDisable == 0)) {

            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x37,
                          CurrentIrql,
                          (ULONG_PTR)(KeGetCurrentThread()->KernelApcDisable),
                          (ULONG_PTR)Resource);
    }

    return ExAcquireResourceExclusiveLite (Resource, Wait);
}

THUNKED_API
VOID
FASTCALL
VerifierExReleaseResource(
    IN PERESOURCE Resource
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql ();

    if ((CurrentIrql != APC_LEVEL) &&
        (!IS_SYSTEM_THREAD(PsGetCurrentThread())) &&
        (KeGetCurrentThread()->KernelApcDisable == 0)) {

            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x38,
                          CurrentIrql,
                          (ULONG_PTR)(KeGetCurrentThread()->KernelApcDisable),
                          (ULONG_PTR)Resource);
    }

    ExReleaseResourceLite (Resource);
}

int VerifierIrqlData[0x10];

VOID
KfSanityCheckRaiseIrql (
    IN KIRQL NewIrql
    )
{
    KIRQL CurrentIrql;

    //
    // Check for the caller inadvertently lowering.
    //

    CurrentIrql = KeGetCurrentIrql ();

    if (CurrentIrql == NewIrql) {
        VerifierIrqlData[0] += 1;
        if (CurrentIrql == APC_LEVEL) {
            VerifierIrqlData[1] += 1;
        }
        else if (CurrentIrql == DISPATCH_LEVEL) {
            VerifierIrqlData[2] += 1;
        }
        else {
            VerifierIrqlData[3] += 1;
        }
    }
    else {
        VerifierIrqlData[4] += 1;
    }

    if (CurrentIrql > NewIrql) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x30,
                      CurrentIrql,
                      NewIrql,
                      0);
    }

    //
    // Check for the caller using an uninitialized variable.
    //

    if (NewIrql > HIGH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x30,
                      CurrentIrql,
                      NewIrql,
                      0);
    }
}

VOID
KfSanityCheckLowerIrql (
    IN KIRQL NewIrql
    )
{
    KIRQL CurrentIrql;

    //
    // Check for the caller inadvertently lowering.
    //

    CurrentIrql = KeGetCurrentIrql ();

    if (CurrentIrql == NewIrql) {
        VerifierIrqlData[8] += 1;
        if (CurrentIrql == APC_LEVEL) {
            VerifierIrqlData[9] += 1;
        }
        else if (CurrentIrql == DISPATCH_LEVEL) {
            VerifierIrqlData[10] += 1;
        }
        else {
            VerifierIrqlData[11] += 1;
        }
    }
    else {
        VerifierIrqlData[12] += 1;
    }

    if (CurrentIrql < NewIrql) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x31,
                      CurrentIrql,
                      NewIrql,
                      0);
    }

    //
    // Check for the caller using an uninitialized variable.
    //

    if (NewIrql > HIGH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x31,
                      CurrentIrql,
                      NewIrql,
                      0);
    }
}

VOID
ViTrimAllSystemPagableMemory (
    VOID
    )
{
    LARGE_INTEGER CurrentTime;
    LOGICAL PageOut;

    PageOut = TRUE;
    if (KernelVerifier == TRUE) {
        KeQueryTickCount(&CurrentTime);
        if ((CurrentTime.LowPart & KernelVerifierTickPage) != 0) {
            PageOut = FALSE;
        }
    }

    if ((PageOut == TRUE) && (MiNoPageOnRaiseIrql == 0)) {
        MmVerifierData.TrimRequests += 1;
        if (MmTrimAllSystemPagableMemory (FALSE) == TRUE) {
            MmVerifierData.Trims += 1;
        }
    }
}

typedef
VOID
(*PKE_ACQUIRE_SPINLOCK) (
    IN PKSPIN_LOCK SpinLock,
    OUT PKIRQL OldIrql
    );

THUNKED_API
VOID
VerifierKeAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock,
    OUT PKIRQL OldIrql
    )
{
    KIRQL CurrentIrql;
    PKE_ACQUIRE_SPINLOCK HalRoutine;

    CurrentIrql = KeGetCurrentIrql ();

    KfSanityCheckRaiseIrql (DISPATCH_LEVEL);

    MmVerifierData.AcquireSpinLocks += 1;

    if (MmVerifierData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) {
        if (CurrentIrql < DISPATCH_LEVEL) {
            ViTrimAllSystemPagableMemory ();
        }
    }

#if defined (_X86_)
    HalRoutine = (PKE_ACQUIRE_SPINLOCK) (ULONG_PTR) MiKernelVerifierOriginalCalls[VI_KE_ACQUIRE_SPINLOCK];

    if (HalRoutine) {
        (*HalRoutine)(SpinLock, OldIrql);
        return;
    }
#endif

    KeAcquireSpinLock (SpinLock, OldIrql);
}

typedef
VOID
(*PKE_RELEASE_SPINLOCK) (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    );

THUNKED_API
VOID
VerifierKeReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    )
{
    KIRQL CurrentIrql;
    PKE_RELEASE_SPINLOCK HalRoutine;

    CurrentIrql = KeGetCurrentIrql ();

    //
    // Caller better still be at DISPATCH_LEVEL when releasing the spinlock
    //

    if (CurrentIrql != DISPATCH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x32,
                      CurrentIrql,
                      (ULONG_PTR)SpinLock,
                      0);
    }

    KfSanityCheckLowerIrql (NewIrql);

#if defined (_X86_)
    HalRoutine = (PKE_RELEASE_SPINLOCK) (ULONG_PTR) MiKernelVerifierOriginalCalls[VI_KE_RELEASE_SPINLOCK];

    if (HalRoutine) {
        (*HalRoutine)(SpinLock, NewIrql);
        return;
    }
#endif

    KeReleaseSpinLock (SpinLock, NewIrql);
}

#if defined (_X86_)

typedef
KIRQL
(FASTCALL *PKF_ACQUIRE_SPINLOCK) (
    IN PKSPIN_LOCK SpinLock
    );

THUNKED_API
KIRQL
FASTCALL
VerifierKfAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock
    )
{
    KIRQL CurrentIrql;
    PKF_ACQUIRE_SPINLOCK HalRoutine;

    CurrentIrql = KeGetCurrentIrql ();

    KfSanityCheckRaiseIrql (DISPATCH_LEVEL);

    MmVerifierData.AcquireSpinLocks += 1;

    if (MmVerifierData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) {
        if (CurrentIrql < DISPATCH_LEVEL) {
            ViTrimAllSystemPagableMemory ();
        }
    }

#if defined (_X86_)
    HalRoutine = (PKF_ACQUIRE_SPINLOCK) (ULONG_PTR) MiKernelVerifierOriginalCalls[VI_KF_ACQUIRE_SPINLOCK];

    if (HalRoutine) {
        return (*HalRoutine)(SpinLock);
    }
#endif

    CurrentIrql = KfAcquireSpinLock (SpinLock);

    return CurrentIrql;
}

typedef
VOID
(FASTCALL *PKF_RELEASE_SPINLOCK) (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    );

THUNKED_API
VOID
FASTCALL
VerifierKfReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    )
{
    KIRQL CurrentIrql;
    PKF_RELEASE_SPINLOCK HalRoutine;

    CurrentIrql = KeGetCurrentIrql ();

    //
    // Caller better still be at DISPATCH_LEVEL when releasing the spinlock.
    //

    if (KernelVerifier == TRUE) {
        if (CurrentIrql < DISPATCH_LEVEL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x35,
                          CurrentIrql,
                          (ULONG_PTR)SpinLock,
                          NewIrql);
        }
    }
    else {
        if (CurrentIrql != DISPATCH_LEVEL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x32,
                          CurrentIrql,
                          (ULONG_PTR)SpinLock,
                          NewIrql);
        }
    }

    KfSanityCheckLowerIrql (NewIrql);

#if defined (_X86_)
    HalRoutine = (PKF_RELEASE_SPINLOCK) (ULONG_PTR) MiKernelVerifierOriginalCalls[VI_KF_RELEASE_SPINLOCK];

    if (HalRoutine) {
        (*HalRoutine)(SpinLock, NewIrql);
        return;
    }
#endif

    KfReleaseSpinLock (SpinLock, NewIrql);
}


#if !defined(NT_UP)

typedef
KIRQL
(FASTCALL *PKE_ACQUIRE_QUEUED_SPINLOCK) (
    IN KSPIN_LOCK_QUEUE_NUMBER Number
    );

THUNKED_API
KIRQL
FASTCALL
VerifierKeAcquireQueuedSpinLock (
    IN KSPIN_LOCK_QUEUE_NUMBER Number
    )
{
    KIRQL CurrentIrql;
    PKE_ACQUIRE_QUEUED_SPINLOCK HalRoutine;

    CurrentIrql = KeGetCurrentIrql ();

    KfSanityCheckRaiseIrql (DISPATCH_LEVEL);

    MmVerifierData.AcquireSpinLocks += 1;

    if (MmVerifierData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) {
        if (CurrentIrql < DISPATCH_LEVEL) {
            ViTrimAllSystemPagableMemory ();
        }
    }

#if defined (_X86_)
    HalRoutine = (PKE_ACQUIRE_QUEUED_SPINLOCK) MiKernelVerifierOriginalCalls[VI_KE_ACQUIRE_QUEUED_SPINLOCK];

    if (HalRoutine) {
        return (*HalRoutine)(Number);
    }
#endif

    CurrentIrql = KeAcquireQueuedSpinLock (Number);

    return CurrentIrql;
}

typedef
VOID
(FASTCALL *PKE_RELEASE_QUEUED_SPINLOCK) (
    IN KSPIN_LOCK_QUEUE_NUMBER Number,
    IN KIRQL OldIrql
    );

THUNKED_API
VOID
FASTCALL
VerifierKeReleaseQueuedSpinLock (
    IN KSPIN_LOCK_QUEUE_NUMBER Number,
    IN KIRQL OldIrql
    )
{
    KIRQL CurrentIrql;
    PKE_RELEASE_QUEUED_SPINLOCK HalRoutine;

    CurrentIrql = KeGetCurrentIrql ();

    if (KernelVerifier == TRUE) {
        if (CurrentIrql < DISPATCH_LEVEL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x36,
                          CurrentIrql,
                          (ULONG_PTR)Number,
                          (ULONG_PTR)OldIrql);
        }
    }

    KfSanityCheckLowerIrql (OldIrql);

#if defined (_X86_)
    HalRoutine = (PKE_RELEASE_QUEUED_SPINLOCK) MiKernelVerifierOriginalCalls[VI_KE_RELEASE_QUEUED_SPINLOCK];

    if (HalRoutine) {
        (*HalRoutine)(Number, OldIrql);
        return;
    }
#endif

    KeReleaseQueuedSpinLock (Number, OldIrql);
}
#endif

typedef
KIRQL
(FASTCALL *PKF_RAISE_IRQL) (
    IN KIRQL NewIrql
    );

THUNKED_API
KIRQL
FASTCALL
VerifierKfRaiseIrql (
    IN KIRQL NewIrql
    )
{
    PKF_RAISE_IRQL HalRoutine;
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql ();

    KfSanityCheckRaiseIrql (NewIrql);

    MmVerifierData.RaiseIrqls += 1;

    if (MmVerifierData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) {
        if ((CurrentIrql < DISPATCH_LEVEL) && (NewIrql >= DISPATCH_LEVEL)) {
            ViTrimAllSystemPagableMemory ();
        }
    }

#if defined (_X86_)
    HalRoutine = (PKF_RAISE_IRQL) (ULONG_PTR) MiKernelVerifierOriginalCalls[VI_KF_RAISE_IRQL];
    if (HalRoutine) {
        return (*HalRoutine)(NewIrql);
    }
#endif

    return KfRaiseIrql (NewIrql);
}

typedef
VOID
(FASTCALL *PKF_LOWER_IRQL) (
    IN KIRQL NewIrql
    );

THUNKED_API
VOID
FASTCALL
VerifierKfLowerIrql (
    IN KIRQL NewIrql
    )
{
    PKF_LOWER_IRQL HalRoutine;

    KfSanityCheckLowerIrql (NewIrql);

#if defined (_X86_)
    HalRoutine = (PKF_LOWER_IRQL) (ULONG_PTR) MiKernelVerifierOriginalCalls[VI_KF_LOWER_IRQL];
    if (HalRoutine) {
        (*HalRoutine)(NewIrql);
        return;
    }
#endif

    KfLowerIrql (NewIrql);
}

#endif

#if defined(_ALPHA_)
THUNKED_API
VOID
VerifierKeAcquireSpinLockAtDpcLevel (
    IN PKSPIN_LOCK SpinLock
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql ();

    //
    // Caller better be at DISPATCH_LEVEL.
    //

    if (CurrentIrql != DISPATCH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x40,
                      CurrentIrql,
                      (ULONG_PTR)SpinLock,
                      0);
    }

    MmVerifierData.AcquireSpinLocks += 1;

    KeAcquireSpinLockAtDpcLevel (SpinLock);
}

THUNKED_API
VOID
VerifierKeReleaseSpinLockFromDpcLevel (
    IN PKSPIN_LOCK SpinLock
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql ();

    //
    // Caller better be at DISPATCH_LEVEL.
    //

    if (CurrentIrql != DISPATCH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x41,
                      CurrentIrql,
                      (ULONG_PTR)SpinLock,
                      0);
    }

    KeReleaseSpinLockFromDpcLevel (SpinLock);
}

THUNKED_API
KIRQL
VerifierKeAcquireSpinLockRaiseToDpc (
    IN PKSPIN_LOCK SpinLock
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql ();

    //
    // Caller better be at or below DISPATCH_LEVEL.
    //

    if (CurrentIrql > DISPATCH_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x42,
                      CurrentIrql,
                      (ULONG_PTR)SpinLock,
                      0);
    }

    MmVerifierData.AcquireSpinLocks += 1;

    if (MmVerifierData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) {
        if (CurrentIrql < DISPATCH_LEVEL) {
            ViTrimAllSystemPagableMemory ();
        }
    }

    return KeAcquireSpinLockRaiseToDpc (SpinLock);
}
#endif

THUNKED_API
BOOLEAN
FASTCALL
VerifierExTryToAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql ();

    //
    // Caller better be at or below APC_LEVEL or have APCs blocked.
    //

    if (CurrentIrql > APC_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x33,
                      CurrentIrql,
                      (ULONG_PTR)FastMutex,
                      0);
    }

    return ExTryToAcquireFastMutex (FastMutex);
}

THUNKED_API
VOID
FASTCALL
VerifierExAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql ();

    //
    // Caller better be at or below APC_LEVEL or have APCs blocked.
    //

    if (CurrentIrql > APC_LEVEL) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x33,
                      CurrentIrql,
                      (ULONG_PTR)FastMutex,
                      0);
    }

    ExAcquireFastMutex (FastMutex);
}

THUNKED_API
VOID
FASTCALL
VerifierExAcquireFastMutexUnsafe (
    IN PFAST_MUTEX FastMutex
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql ();

    //
    // Caller better be at APC_LEVEL or have APCs blocked.
    //

    if ((CurrentIrql != APC_LEVEL) &&
        (!IS_SYSTEM_THREAD(PsGetCurrentThread())) &&
        (KeGetCurrentThread()->KernelApcDisable == 0)) {

        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x39,
                      CurrentIrql,
                      (ULONG_PTR)(KeGetCurrentThread()->KernelApcDisable),
                      (ULONG_PTR)FastMutex);
    }

    ExAcquireFastMutexUnsafe (FastMutex);
}

THUNKED_API
VOID
FASTCALL
VerifierExReleaseFastMutex (
    IN PFAST_MUTEX FastMutex
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql ();

    //
    // Caller better be at APC_LEVEL or have APCs blocked.
    //

    if ((CurrentIrql != APC_LEVEL) &&
        (!IS_SYSTEM_THREAD(PsGetCurrentThread())) &&
        (KeGetCurrentThread()->KernelApcDisable == 0)) {

        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x34,
                      CurrentIrql,
                      (ULONG_PTR)(KeGetCurrentThread()->KernelApcDisable),
                      (ULONG_PTR)FastMutex);
    }

    ExReleaseFastMutex (FastMutex);
}

THUNKED_API
VOID
FASTCALL
VerifierExReleaseFastMutexUnsafe (
    IN PFAST_MUTEX FastMutex
    )
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql ();

    //
    // Caller better be at APC_LEVEL or have APCs blocked.
    //

    if ((CurrentIrql != APC_LEVEL) &&
        (!IS_SYSTEM_THREAD(PsGetCurrentThread())) &&
        (KeGetCurrentThread()->KernelApcDisable == 0)) {

        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x3A,
                      CurrentIrql,
                      (ULONG_PTR)(KeGetCurrentThread()->KernelApcDisable),
                      (ULONG_PTR)FastMutex);
    }

    ExReleaseFastMutexUnsafe (FastMutex);
}

typedef
VOID
(*PKE_RAISE_IRQL) (
    IN KIRQL NewIrql,
    OUT PKIRQL OldIrql
    );

THUNKED_API
VOID
VerifierKeRaiseIrql (
    IN KIRQL NewIrql,
    OUT PKIRQL OldIrql
    )
{
    PKE_RAISE_IRQL HalRoutine;

    *OldIrql = KeGetCurrentIrql ();

    KfSanityCheckRaiseIrql (NewIrql);

    MmVerifierData.RaiseIrqls += 1;

    if (MmVerifierData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) {
        if ((*OldIrql < DISPATCH_LEVEL) && (NewIrql >= DISPATCH_LEVEL)) {
            ViTrimAllSystemPagableMemory ();
        }
    }

#if defined (_X86_)
    HalRoutine = (PKE_RAISE_IRQL) (ULONG_PTR) MiKernelVerifierOriginalCalls[VI_KE_RAISE_IRQL];
    if (HalRoutine) {
        (*HalRoutine)(NewIrql, OldIrql);
        return;
    }
#endif

    KeRaiseIrql (NewIrql, OldIrql);
}

typedef
VOID
(*PKE_LOWER_IRQL) (
    IN KIRQL NewIrql
    );

THUNKED_API
VOID
VerifierKeLowerIrql (
    IN KIRQL NewIrql
    )
{
    PKE_LOWER_IRQL HalRoutine;

    KfSanityCheckLowerIrql (NewIrql);

#if defined (_X86_)
    HalRoutine = (PKE_LOWER_IRQL) (ULONG_PTR) MiKernelVerifierOriginalCalls[VI_KE_LOWER_IRQL];
    if (HalRoutine) {
        (*HalRoutine)(NewIrql);
        return;
    }
#endif

    KeLowerIrql (NewIrql);
}

THUNKED_API
BOOLEAN
VerifierSynchronizeExecution (
    IN PKINTERRUPT Interrupt,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
    )
{
    KIRQL OldIrql;

    OldIrql = KeGetCurrentIrql ();

    KfSanityCheckRaiseIrql (Interrupt->SynchronizeIrql);

    MmVerifierData.SynchronizeExecutions += 1;

    if (MmVerifierData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) {
        if ((OldIrql < DISPATCH_LEVEL) && (Interrupt->SynchronizeIrql >= DISPATCH_LEVEL)) {
            ViTrimAllSystemPagableMemory ();
        }
    }

    return KeSynchronizeExecution (Interrupt,
                                   SynchronizeRoutine,
                                   SynchronizeContext);
}

THUNKED_API
VOID
VerifierKeInitializeTimerEx(
    IN PKTIMER Timer,
    IN TIMER_TYPE Type
    )
{
    //
    // Check the object being initialized isn't already an
    // active timer.  Make sure the timer table list is initialized.
    //

    if (KiTimerTableListHead[0].Flink != NULL) {
        KeCheckForTimer(Timer, sizeof(KTIMER));
    }

    KeInitializeTimerEx(Timer, Type);
}

THUNKED_API
VOID
VerifierKeInitializeTimer(
    IN PKTIMER Timer
    )
{
    VerifierKeInitializeTimerEx(Timer, NotificationTimer);
}

VOID
ViInitializeEntry (
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier,
    IN LOGICAL FirstLoad
    )

/*++

Routine Description:

    Initialize various verifier fields as the driver is being (re)loaded now.

Arguments:

    Verifier - Supplies the verifier entry to be initialized.

    FirstLoad - Supplies TRUE if this is the first load of this driver.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;

    //
    // Only the BaseName field is initialized on entry.
    //

    KeInitializeSpinLock (&Verifier->VerifierPoolLock);

    Verifier->CurrentPagedPoolAllocations = 0;
    Verifier->CurrentNonPagedPoolAllocations = 0;
    Verifier->PeakPagedPoolAllocations = 0;
    Verifier->PeakNonPagedPoolAllocations = 0;

    Verifier->PagedBytes = 0;
    Verifier->NonPagedBytes = 0;
    Verifier->PeakPagedBytes = 0;
    Verifier->PeakNonPagedBytes = 0;

    Verifier->PoolHash = NULL;
    Verifier->PoolHashSize = 0;
    Verifier->PoolHashFree = VI_POOL_FREELIST_END;
    Verifier->PoolHashReserved = 0;

    Verifier->Signature = MI_VERIFIER_ENTRY_SIGNATURE;

    if (FirstLoad == TRUE) {
        Verifier->Flags = 0;
        Verifier->Loads = 0;
        Verifier->Unloads = 0;
    }

    ExAcquireSpinLock (&VerifierListLock, &OldIrql);
    Verifier->StartAddress = NULL;
    Verifier->EndAddress = NULL;
    ExReleaseSpinLock (&VerifierListLock, OldIrql);
}

#define UNICODE_TAB               0x0009
#define UNICODE_LF                0x000A
#define UNICODE_CR                0x000D
#define UNICODE_SPACE             0x0020
#define UNICODE_CJK_SPACE         0x3000

#define UNICODE_WHITESPACE(_ch)     (((_ch) == UNICODE_TAB) || \
                                     ((_ch) == UNICODE_LF) || \
                                     ((_ch) == UNICODE_CR) || \
                                     ((_ch) == UNICODE_SPACE) || \
                                     ((_ch) == UNICODE_CJK_SPACE))

LOGICAL
MiInitializeDriverVerifierList (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    Parse the registry setting and set up the list of driver names that will
    be put through the validation process.

    Walk the loaded module list and thunk any drivers that need/deserve it.

Arguments:

    None.

Return Value:

    TRUE if successful, FALSE if not.

Environment:

    Kernel mode, Phase 0 Initialization.

    Nonpaged (but not paged) pool exists.

    The PsLoadedModuleList has not been set up yet although the boot drivers
    have been relocated to their final resting places.

--*/

{
    ULONG i;
    PLIST_ENTRY NextEntry;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PWCHAR Start;
    PWCHAR End;
    PWCHAR Walk;
    ULONG NameLength;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;
    LARGE_INTEGER CurrentTime;
    UNICODE_STRING KernelString;
    UNICODE_STRING HalString;
    PMI_VERIFIER_DRIVER_ENTRY KernelEntry;
    PMI_VERIFIER_DRIVER_ENTRY HalEntry;

    InitializeListHead (&MiSuspectDriverList);

    if (MmVerifyDriverLevel != (ULONG)-1) {
        if (MmVerifyDriverLevel & DRIVER_VERIFIER_IO_CHECKING) {
            if (MmVerifyDriverBufferLength == (ULONG)-1) {
                MmVerifyDriverBufferLength = 0;     // Mm will not page out verifier pages.
            }
        }
    }

    if (MmVerifyDriverBufferLength == (ULONG)-1) {

        if (MmDontVerifyRandomDrivers == TRUE) {
            return FALSE;
        }

        MmVerifyDriverBufferLength = 0;

        CurrentTime = KeQueryPerformanceCounter (NULL);
        CurrentTime.LowPart = (CurrentTime.LowPart % 26);

        MiVerifyRandomDrivers = (WCHAR)'A' + (WCHAR)CurrentTime.LowPart;

        if ((MiVerifyRandomDrivers == (WCHAR)'H') ||
            (MiVerifyRandomDrivers == (WCHAR)'J') ||
            (MiVerifyRandomDrivers == (WCHAR)'X') ||
            (MiVerifyRandomDrivers == (WCHAR)'Y') ||
            (MiVerifyRandomDrivers == (WCHAR)'Z')) {
                MiVerifyRandomDrivers = (WCHAR)'X';
        }
    }

    KeInitializeSpinLock (&ViBadMapperLock);
    KeInitializeSpinLock (&VerifierListLock);

    KeInitializeSpinLock (&VerifierPoolLock);
    ExInitializeFastMutex (&VerifierPoolMutex);

    InitializeListHead (&MiVerifierDriverAddedThunkListHead);

    //
    // If no default is specified, then special pool, pagable code/data
    // flushing and pool leak detection are enabled.
    //

    if (MmVerifyDriverLevel == (ULONG)-1) {
        MmVerifierData.Level = DRIVER_VERIFIER_SPECIAL_POOLING |
                               DRIVER_VERIFIER_FORCE_IRQL_CHECKING |
                               DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS;
    }
    else {
        MmVerifierData.Level = MmVerifyDriverLevel;
    }

    VerifierModifyableOptions = (DRIVER_VERIFIER_SPECIAL_POOLING |
                                 DRIVER_VERIFIER_FORCE_IRQL_CHECKING |
                                 DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES);

    IoVerifierInit (MmVerifierData.Level, IOVERIFIERINIT_PHASE0 |
                                          IOVERIFIERINIT_EVERYTHING_TRACKED |
                                          IOVERIFIERINIT_VERIFIER_DRIVER_LIST);

    KernelEntry = NULL;
    HalEntry = NULL;

    if (MiVerifyRandomDrivers == (WCHAR)0) {

        RtlInitUnicodeString (&KernelString, L"ntoskrnl.exe");
        RtlInitUnicodeString (&HalString, L"hal.dll");

        Start = MmVerifyDriverBuffer;
        End = MmVerifyDriverBuffer + (MmVerifyDriverBufferLength - sizeof(WCHAR)) / sizeof(WCHAR);

        while (Start < End) {
            if (UNICODE_WHITESPACE(*Start)) {
                Start += 1;
                continue;
            }

            if (*Start == (WCHAR)'*') {
                MiVerifyAllDrivers = TRUE;
                break;
            }

            for (Walk = Start; Walk < End; Walk += 1) {
                if (UNICODE_WHITESPACE(*Walk)) {
                    break;
                }
            }

            //
            // Got a string.  Save it.
            //

            NameLength = (ULONG)(Walk - Start + 1) * sizeof (WCHAR);

            Verifier = (PMI_VERIFIER_DRIVER_ENTRY)ExAllocatePoolWithTag (
                                        NonPagedPool,
                                        sizeof (MI_VERIFIER_DRIVER_ENTRY) +
                                                            NameLength,
                                        'dLmM');

            if (Verifier == NULL) {
                break;
            }

            Verifier->BaseName.Buffer = (PWSTR)((PCHAR)Verifier +
                                                sizeof (MI_VERIFIER_DRIVER_ENTRY));
            Verifier->BaseName.Length = (USHORT)NameLength - sizeof (UNICODE_NULL);
            Verifier->BaseName.MaximumLength = (USHORT)NameLength;

            RtlMoveMemory (Verifier->BaseName.Buffer,
                           Start,
                           NameLength - sizeof (UNICODE_NULL));

            ViInitializeEntry (Verifier, TRUE);

            Verifier->Flags |= VI_VERIFYING_DIRECTLY;

            ViInsertVerifierEntry (Verifier);

            if (RtlEqualUnicodeString (&KernelString,
                                       &Verifier->BaseName,
                                       TRUE)) {

                //
                // All driver pool allocation calls must be intercepted so
                // they are not mistaken for kernel pool allocations.
                //

                MiVerifyAllDrivers = TRUE;

                KernelVerifier = TRUE;
                KernelEntry = Verifier;

            }
            else if (RtlEqualUnicodeString (&HalString,
                                            &Verifier->BaseName,
                                            TRUE)) {
                HalEntry = Verifier;
            }

            Start = Walk + 1;
        }
    }

    if (MiTriageAddDrivers (LoaderBlock) == TRUE) {

        //
        // Disable random driver verification if triage has picked driver(s).
        //

        MiVerifyRandomDrivers = (WCHAR)0;
    }

    //
    // Process the boot-loaded drivers now.
    //

    i = 0;
    NextEntry = LoaderBlock->LoadOrderListHead.Flink;

    for ( ; NextEntry != &LoaderBlock->LoadOrderListHead; NextEntry = NextEntry->Flink) {

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           LDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        //
        // Process the kernel and HAL specially.
        //

        if (i == 0) {
            if (KernelEntry != NULL) {
                MiApplyDriverVerifier (DataTableEntry, KernelEntry);
            }
        }
        else if (i == 1) {
            if (HalEntry != NULL) {
                MiApplyDriverVerifier (DataTableEntry, HalEntry);
            }
        }
        else {
            MiApplyDriverVerifier (DataTableEntry, NULL);
        }
        i += 1;
    }

    return TRUE;
}

VOID
ViInsertVerifierEntry (
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier
    )

/*++

Routine Description:

    Nonpagable wrapper to insert a new verifier entry.

    Note that the system load mutant or the verifier load spinlock is sufficient
    for readers to access the list.  This is because the insertion path
    acquires both.

    Lock synchronization is needed because pool allocators walk the
    verifier list at DISPATCH_LEVEL.

Arguments:

    Verifier - Supplies a caller-initialized entry for the driver.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;

    ExAcquireSpinLock (&VerifierListLock, &OldIrql);
    InsertTailList (&MiSuspectDriverList, &Verifier->Links);
    ExReleaseSpinLock (&VerifierListLock, OldIrql);
}

PMI_VERIFIER_DRIVER_ENTRY
ViLocateVerifierEntry (
    IN PVOID SystemAddress
    )

/*++

Routine Description:

    Locate the Driver Verifier entry for the specified system address.

Arguments:

    SystemAddress - Supplies a code or data address within a driver.

Return Value:

    The Verifier entry corresponding to the driver or NULL.

Environment:

    The caller may be at DISPATCH_LEVEL and does not hold the MmSystemLoadLock.

--*/

{
    KIRQL OldIrql;
    PLIST_ENTRY NextEntry;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;

    ExAcquireSpinLock (&VerifierListLock, &OldIrql);

    NextEntry = MiSuspectDriverList.Flink;
    while (NextEntry != &MiSuspectDriverList) {

        Verifier = CONTAINING_RECORD(NextEntry,
                                     MI_VERIFIER_DRIVER_ENTRY,
                                     Links);

        if ((SystemAddress >= Verifier->StartAddress) &&
            (SystemAddress < Verifier->EndAddress)) {

            ExReleaseSpinLock (&VerifierListLock, OldIrql);
            return Verifier;
        }
        NextEntry = NextEntry->Flink;
    }

    ExReleaseSpinLock (&VerifierListLock, OldIrql);
    return NULL;
}

LOGICAL
MiApplyDriverVerifier (
    IN PLDR_DATA_TABLE_ENTRY DataTableEntry,
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier
    )

/*++

Routine Description:

    This function is called as each module is loaded.  If the module being
    loaded is in the suspect list, thunk it here.

Arguments:

    DataTableEntry - Supplies the data table entry for the module.

    Verifier - Non-NULL if verification must be applied.  FALSE indicates
                      that the driver name must match for verification to be
                      applied.

Return Value:

    TRUE if thunking was applied, FALSE if not.

Environment:

    Kernel mode, Phase 0 Initialization and normal runtime.
    Non paged pool exists in Phase0, but paged pool does not.
    Post-Phase0 serialization is provided by the MmSystemLoadLock.

--*/

{
    WCHAR FirstChar;
    LOGICAL Found;
    PLIST_ENTRY NextEntry;
    ULONG VerifierFlags;

    if (Verifier != NULL) {
        Found = TRUE;
    }
    else {
        Found = FALSE;
        NextEntry = MiSuspectDriverList.Flink;
        while (NextEntry != &MiSuspectDriverList) {

            Verifier = CONTAINING_RECORD(NextEntry,
                                         MI_VERIFIER_DRIVER_ENTRY,
                                         Links);

            if (RtlEqualUnicodeString (&Verifier->BaseName,
                                       &DataTableEntry->BaseDllName,
                                       TRUE)) {

                Found = TRUE;
                ViInitializeEntry (Verifier, FALSE);
                break;
            }
            NextEntry = NextEntry->Flink;
        }
    }

    if (Found == FALSE) {
        VerifierFlags = VI_VERIFYING_DIRECTLY;
        if (MiVerifyAllDrivers == TRUE) {
            if (KernelVerifier == TRUE) {
                VerifierFlags = VI_VERIFYING_INVERSELY;
            }
            Found = TRUE;
        }
        else if (MiVerifyRandomDrivers != (WCHAR)0) {

            //
            // Wildcard match drivers randomly.
            //

            FirstChar = RtlUpcaseUnicodeChar(DataTableEntry->BaseDllName.Buffer[0]);

            if (MiVerifyRandomDrivers == FirstChar) {
                Found = TRUE;
            }
            else if (MiVerifyRandomDrivers == (WCHAR)'X') {
                if ((FirstChar >= (WCHAR)'0') && (FirstChar <= (WCHAR)'9')) {
                    Found = TRUE;
                }
            }
        }

        if (Found == FALSE) {
            return FALSE;
        }

        Verifier = (PMI_VERIFIER_DRIVER_ENTRY)ExAllocatePoolWithTag (
                                    NonPagedPool,
                                    sizeof (MI_VERIFIER_DRIVER_ENTRY) +
                                        DataTableEntry->BaseDllName.MaximumLength,
                                    'dLmM');

        if (Verifier == NULL) {
            return FALSE;
        }

        Verifier->BaseName.Buffer = (PWSTR)((PCHAR)Verifier +
                                        sizeof (MI_VERIFIER_DRIVER_ENTRY));
        Verifier->BaseName.Length = DataTableEntry->BaseDllName.Length;
        Verifier->BaseName.MaximumLength = DataTableEntry->BaseDllName.MaximumLength;

        RtlMoveMemory (Verifier->BaseName.Buffer,
                       DataTableEntry->BaseDllName.Buffer,
                       DataTableEntry->BaseDllName.Length);

        ViInitializeEntry (Verifier, TRUE);

        Verifier->Flags = VerifierFlags;

        ViInsertVerifierEntry (Verifier);
    }

    Verifier->StartAddress = DataTableEntry->DllBase;
    Verifier->EndAddress = (PVOID)((ULONG_PTR)DataTableEntry->DllBase + DataTableEntry->SizeOfImage);

    if (MiEnableVerifier (DataTableEntry) == TRUE) {

        if (Verifier->Flags & VI_VERIFYING_DIRECTLY) {
            ViPrintString (&DataTableEntry->BaseDllName);
        }

        MmVerifierData.Loads += 1;

        Verifier->Loads += 1;

        DataTableEntry->Flags |= LDRP_IMAGE_VERIFYING;
        MiActiveVerifies += 1;

        if (MiActiveVerifies == 1) {

            //
            // Up the retry mechanism for potential injection failures.
            //

            MiIoRetryLevel = (ULONG)-1;
            MiFaultRetries = MiIoRetryLevel;
            MiUserIoRetryLevel = (ULONG)-1;
            MiUserFaultRetries = MiUserIoRetryLevel;

#ifndef NO_POOL_CHECKS

            //
            // If a loaded driver(s) is undergoing validation, the default
            // special pool randomizer is disabled as the precious virtual
            // address space and physical memory is being put to specific
            // use.
            //

            MiEnableRandomSpecialPool (FALSE);
#endif
            if (MmVerifierData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) {

                //
                // Page out all thread stacks as soon as possible to
                // catch drivers using local events that do usermode waits.
                //

                if (KernelVerifier == FALSE) {
                    MiVerifierStackProtectTime = KiStackProtectTime;
                    KiStackProtectTime = 0;
                }
            }
        }
    }

    return Found;
}

PUNICODE_STRING ViBadDriver;

VOID
MiVerifyingDriverUnloading (
    IN PLDR_DATA_TABLE_ENTRY DataTableEntry
    )

/*++

Routine Description:

    This function is called as a driver that was being verified is now being
    unloaded.

Arguments:

    DataTableEntry - Supplies the data table entry for the driver.

Return Value:

    TRUE if thunking was applied, FALSE if not.

Environment:

    Kernel mode, Phase 0 Initialization and normal runtime.
    Non paged pool exists in Phase0, but paged pool does not.
    Post-Phase0 serialization is provided by the MmSystemLoadLock.

--*/

{
    KIRQL OldIrql;
    LOGICAL Found;
    PLIST_ENTRY NextEntry;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;
    PVI_POOL_ENTRY OldHashTable;

    Found = FALSE;
    NextEntry = MiSuspectDriverList.Flink;
    while (NextEntry != &MiSuspectDriverList) {

        Verifier = CONTAINING_RECORD(NextEntry,
                                          MI_VERIFIER_DRIVER_ENTRY,
                                          Links);

        if (RtlEqualUnicodeString (&Verifier->BaseName,
                                   &DataTableEntry->BaseDllName,
                                   TRUE)) {

            Found = TRUE;
            break;
        }
        NextEntry = NextEntry->Flink;
    }

    ASSERT (Found == TRUE);

    if (MmVerifierData.Level & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS) {

        //
        // Better not be any pool left that wasn't freed.  Walk the pagable
        // allocations in an attempt to make them all resident for a
        // kernel debugger session.
        //

        if (Verifier->PagedBytes) {

#if DBG
            DbgPrint ("Driver %wZ leaked %d paged pool allocations (0x%x bytes)\n",
                &DataTableEntry->FullDllName,
                Verifier->CurrentPagedPoolAllocations,
                Verifier->PagedBytes);
#endif

            //
            // It would be nice to fault in the driver's paged pool allocations
            // now to make debugging easier, but this cannot be easily done
            // in a deadlock free manner.
            //
            // At least disable the paging of pool on IRQL raising in attempt
            // to keep some of these allocations resident for debugging.
            // No need to undo the increment as we're about to bugcheck anyway.
            //

            InterlockedIncrement ((PLONG)&MiNoPageOnRaiseIrql);
        }

#if DBG
        if (Verifier->NonPagedBytes) {
            DbgPrint ("Driver %wZ leaked %d nonpaged pool allocations (0x%x bytes)\n",
                &DataTableEntry->FullDllName,
                Verifier->CurrentNonPagedPoolAllocations,
                Verifier->NonPagedBytes);
        }
#endif

        if (Verifier->PagedBytes || Verifier->NonPagedBytes) {
#if 0
            DbgBreakPoint ();
            InterlockedDecrement (&MiNoPageOnRaiseIrql);
#else
            //
            // Snap this so the build/BVT lab can easily triage the culprit.
            //

            ViBadDriver = &Verifier->BaseName;

            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x60,
                          Verifier->PagedBytes,
                          Verifier->NonPagedBytes,
                          Verifier->CurrentPagedPoolAllocations +
                            Verifier->CurrentNonPagedPoolAllocations);
#endif
        }

        ExAcquireSpinLock (&Verifier->VerifierPoolLock, &OldIrql);

        if (Verifier->PoolHashReserved != 0) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x61,
                          Verifier->PagedBytes,
                          Verifier->NonPagedBytes,
                          Verifier->CurrentPagedPoolAllocations +
                            Verifier->CurrentNonPagedPoolAllocations);
        }

        OldHashTable = Verifier->PoolHash;
        if (OldHashTable != NULL) {
            Verifier->PoolHashSize = 0;
            Verifier->PoolHashFree = VI_POOL_FREELIST_END;
            Verifier->PoolHash = NULL;
        }
        else {
            ASSERT (Verifier->PoolHashSize == 0);
            ASSERT (Verifier->PoolHashFree == VI_POOL_FREELIST_END);
        }

        ExReleaseSpinLock (&Verifier->VerifierPoolLock, OldIrql);

        if (OldHashTable != NULL) {
            ExFreePool (OldHashTable);
        }

        //
        // Clear these fields so reuse of stale addresses don't trigger
        // erroneous bucket fills.
        //

        ExAcquireSpinLock (&VerifierListLock, &OldIrql);
        Verifier->StartAddress = NULL;
        Verifier->EndAddress = NULL;
        ExReleaseSpinLock (&VerifierListLock, OldIrql);
    }

    Verifier->Unloads += 1;
    MmVerifierData.Unloads += 1;
    MiActiveVerifies -= 1;

    if (MiActiveVerifies == 0) {

        if (MmVerifierData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) {

            //
            // Return to normal thread stack protection.
            //

            if (KernelVerifier == FALSE) {
                KiStackProtectTime = MiVerifierStackProtectTime;
            }
        }

#ifndef NO_POOL_CHECKS
        MiEnableRandomSpecialPool (TRUE);
#endif
    }
}

NTKERNELAPI
LOGICAL
MmIsDriverVerifying (
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This function informs the caller if the argument driver is being verified.

Arguments:

    DriverObject - Supplies the driver object.

Return Value:

    TRUE if this driver is being verified, FALSE if not.

Environment:

    Kernel mode, any IRQL, any needed synchronization must be provided by the
    caller.

--*/

{
    PLDR_DATA_TABLE_ENTRY DataTableEntry;

    DataTableEntry = (PLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection;

    if (DataTableEntry == NULL) {
        return FALSE;
    }

    if ((DataTableEntry->Flags & LDRP_IMAGE_VERIFYING) == 0) {
        return FALSE;
    }

    return TRUE;
}

#define MM_BOOT_IMAGE_SIZE (16 * 1024 * 1024)

NTSTATUS
MmAddVerifierThunks (
    IN PVOID ThunkBuffer,
    IN ULONG ThunkBufferSize
    )

/*++

Routine Description:

    This routine adds another set of thunks to the verifier list.

Arguments:

    ThunkBuffer - Supplies the buffer containing the thunk pairs.

    ThunkBufferSize - Supplies the number of bytes in the thunk buffer.

Return Value:

    Returns the status of the operation.

Environment:

    Kernel mode.  APC_LEVEL and below.

--*/

{
    ULONG i;
    ULONG NumberOfThunkPairs;
    PDRIVER_VERIFIER_THUNK_PAIRS ThunkPairs;
    PDRIVER_VERIFIER_THUNK_PAIRS ThunkTable;
    PDRIVER_SPECIFIED_VERIFIER_THUNKS ThunkTableBase;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PVOID DriverStartAddress;
    PVOID DriverEndAddress;

    PAGED_CODE();

    if (MiVerifierDriverAddedThunkListHead.Flink == NULL) {
        return STATUS_NOT_SUPPORTED;
    }

    ThunkPairs = (PDRIVER_VERIFIER_THUNK_PAIRS)ThunkBuffer;
    NumberOfThunkPairs = ThunkBufferSize / sizeof(DRIVER_VERIFIER_THUNK_PAIRS);

    if (NumberOfThunkPairs == 0) {
        return STATUS_INVALID_PARAMETER_1;
    }

    ThunkTableBase = (PDRIVER_SPECIFIED_VERIFIER_THUNKS) ExAllocatePoolWithTag (
                            PagedPool,
                            sizeof (DRIVER_SPECIFIED_VERIFIER_THUNKS) + NumberOfThunkPairs * sizeof (DRIVER_VERIFIER_THUNK_PAIRS),
                            'tVmM');

    if (ThunkTableBase == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ThunkTable = (PDRIVER_VERIFIER_THUNK_PAIRS)(ThunkTableBase + 1);

    RtlCopyMemory (ThunkTable,
                   ThunkPairs,
                   NumberOfThunkPairs * sizeof(DRIVER_VERIFIER_THUNK_PAIRS));

    KeEnterCriticalRegion();

    KeWaitForSingleObject (&MmSystemLoadLock,
                           WrVirtualMemory,
                           KernelMode,
                           FALSE,
                           (PLARGE_INTEGER)NULL);

    //
    // Find and validate the image that contains the routines to be thunked.
    //

    DataTableEntry = MiLookupDataTableEntry (ThunkTable->PristineRoutine,
                                             TRUE);

    if (DataTableEntry == NULL) {
        KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
        KeLeaveCriticalRegion();
        ExFreePool (ThunkTableBase);
        return STATUS_INVALID_PARAMETER_2;
    }

    DriverStartAddress = (PVOID)(DataTableEntry->DllBase);
    DriverEndAddress = (PVOID)((PCHAR)DataTableEntry->DllBase + DataTableEntry->SizeOfImage);

    //
    // Don't let drivers hook calls to kernel or HAL routines.
    //

    if (DriverStartAddress < (PVOID)(KSEG0_BASE + MM_BOOT_IMAGE_SIZE)) {
        KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
        KeLeaveCriticalRegion();
        ExFreePool (ThunkTableBase);
        return STATUS_INVALID_PARAMETER_2;
    }

    for (i = 0; i < NumberOfThunkPairs; i += 1) {

        //
        // Ensure all the routines being thunked are in the same driver.
        //

        if (((ULONG_PTR)ThunkTable->PristineRoutine < (ULONG_PTR)DriverStartAddress) ||
            ((ULONG_PTR)ThunkTable->PristineRoutine >= (ULONG_PTR)DriverEndAddress)) {

            KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
            KeLeaveCriticalRegion();
            ExFreePool (ThunkTableBase);
            return STATUS_INVALID_PARAMETER_2;
        }
        ThunkTable += 1;
    }

    //
    // Add the validated thunk table to the verifier's global list.
    //

    ThunkTableBase->DataTableEntry = DataTableEntry;
    ThunkTableBase->NumberOfThunks = NumberOfThunkPairs;
    MiActiveVerifierThunks += 1;

    InsertTailList (&MiVerifierDriverAddedThunkListHead,
                    &ThunkTableBase->ListEntry);

    KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
    KeLeaveCriticalRegion();

    return STATUS_SUCCESS;
}

VOID
MiVerifierCheckThunks (
    IN PLDR_DATA_TABLE_ENTRY DataTableEntry
    )

/*++

Routine Description:

    This routine adds another set of thunks to the verifier list.

Arguments:

    DataTableEntry - Supplies the data table entry for the driver.

Return Value:

    None.

Environment:

    Kernel mode.  APC_LEVEL and below.
    The system load lock must be held by the caller.

--*/

{
    PLIST_ENTRY NextEntry;
    PDRIVER_SPECIFIED_VERIFIER_THUNKS ThunkTableBase;

    PAGED_CODE ();

    //
    // N.B.  The DataTableEntry can move (see MiInitializeLoadedModuleList),
    //       but this only happens long before IoInitialize so this is safe.
    //

    NextEntry = MiVerifierDriverAddedThunkListHead.Flink;
    while (NextEntry != &MiVerifierDriverAddedThunkListHead) {

        ThunkTableBase = CONTAINING_RECORD(NextEntry,
                                           DRIVER_SPECIFIED_VERIFIER_THUNKS,
                                           ListEntry);

        if (ThunkTableBase->DataTableEntry == DataTableEntry) {
            RemoveEntryList (NextEntry);
            NextEntry = NextEntry->Flink;
            ExFreePool (ThunkTableBase);
            MiActiveVerifierThunks -= 1;

            //
            // Keep looking as the driver may have made multiple calls.
            //

            continue;
        }

        NextEntry = NextEntry->Flink;
    }
}

#define ROUND_UP(VALUE,ROUND) ((ULONG)(((ULONG)VALUE + \
                               ((ULONG)ROUND - 1L)) & (~((ULONG)ROUND - 1L))))

NTSTATUS
MmGetVerifierInformation (
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    )

/*++

Routine Description:

    This routine returns information about drivers undergoing verification.

Arguments:

    SystemInformation - Returns the driver verification information.

    SystemInformationLength - Supplies the length of the SystemInformation
                              buffer.

    Length - Returns the length of the driver verification file information
             placed in the buffer.

Return Value:

    Returns the status of the operation.

Environment:

    The SystemInformation buffer is in user space and our caller has wrapped
    a try-except around this entire routine.  Capture any exceptions here and
    release resources accordingly.

--*/

{
    PSYSTEM_VERIFIER_INFORMATION UserVerifyBuffer;
    ULONG NextEntryOffset;
    ULONG TotalSize;
    NTSTATUS Status;
    PLIST_ENTRY NextEntry;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;
    UNICODE_STRING UserBufferDriverName;

    PAGED_CODE();

    NextEntryOffset = 0;
    TotalSize = 0;

    *Length = 0;
    UserVerifyBuffer = (PSYSTEM_VERIFIER_INFORMATION)SystemInformation;

    //
    // Capture the number of verifying drivers and the relevant data while
    // synchronized.  Then return it to our caller.
    //

    Status = STATUS_SUCCESS;

    KeEnterCriticalRegion();

    KeWaitForSingleObject (&MmSystemLoadLock,
                           WrVirtualMemory,
                           KernelMode,
                           FALSE,
                           (PLARGE_INTEGER)NULL);

    try {

        NextEntry = MiSuspectDriverList.Flink;
        while (NextEntry != &MiSuspectDriverList) {

            Verifier = CONTAINING_RECORD(NextEntry,
                                              MI_VERIFIER_DRIVER_ENTRY,
                                              Links);

            if ((Verifier->Flags & VI_VERIFYING_DIRECTLY) == 0) {
                NextEntry = NextEntry->Flink;
                continue;
            }

            UserVerifyBuffer = (PSYSTEM_VERIFIER_INFORMATION)(
                                    (PUCHAR)UserVerifyBuffer + NextEntryOffset);
            NextEntryOffset = sizeof(SYSTEM_VERIFIER_INFORMATION);
            TotalSize += sizeof(SYSTEM_VERIFIER_INFORMATION);

            if (TotalSize > SystemInformationLength) {
                ExRaiseStatus (STATUS_INFO_LENGTH_MISMATCH);
            }

            //
            // This data is cumulative for all drivers.
            //

            UserVerifyBuffer->Level = MmVerifierData.Level;
            UserVerifyBuffer->RaiseIrqls = MmVerifierData.RaiseIrqls;
            UserVerifyBuffer->AcquireSpinLocks = MmVerifierData.AcquireSpinLocks;

            UserVerifyBuffer->UnTrackedPool = MmVerifierData.UnTrackedPool;
            UserVerifyBuffer->SynchronizeExecutions = MmVerifierData.SynchronizeExecutions;

            UserVerifyBuffer->AllocationsAttempted = MmVerifierData.AllocationsAttempted;
            UserVerifyBuffer->AllocationsSucceeded = MmVerifierData.AllocationsSucceeded;
            UserVerifyBuffer->AllocationsSucceededSpecialPool = MmVerifierData.AllocationsSucceededSpecialPool;
            UserVerifyBuffer->AllocationsWithNoTag = MmVerifierData.AllocationsWithNoTag;

            UserVerifyBuffer->TrimRequests = MmVerifierData.TrimRequests;
            UserVerifyBuffer->Trims = MmVerifierData.Trims;
            UserVerifyBuffer->AllocationsFailed = MmVerifierData.AllocationsFailed;
            UserVerifyBuffer->AllocationsFailedDeliberately = MmVerifierData.AllocationsFailedDeliberately;

            //
            // This data is kept on a per-driver basis.
            //

            UserVerifyBuffer->CurrentPagedPoolAllocations = Verifier->CurrentPagedPoolAllocations;
            UserVerifyBuffer->CurrentNonPagedPoolAllocations = Verifier->CurrentNonPagedPoolAllocations;
            UserVerifyBuffer->PeakPagedPoolAllocations = Verifier->PeakPagedPoolAllocations;
            UserVerifyBuffer->PeakNonPagedPoolAllocations = Verifier->PeakNonPagedPoolAllocations;

            UserVerifyBuffer->PagedPoolUsageInBytes = Verifier->PagedBytes;
            UserVerifyBuffer->NonPagedPoolUsageInBytes = Verifier->NonPagedBytes;
            UserVerifyBuffer->PeakPagedPoolUsageInBytes = Verifier->PeakPagedBytes;
            UserVerifyBuffer->PeakNonPagedPoolUsageInBytes = Verifier->PeakNonPagedBytes;

            UserVerifyBuffer->Loads = Verifier->Loads;
            UserVerifyBuffer->Unloads = Verifier->Unloads;

            //
            // The DriverName portion of the UserVerifyBuffer must be saved
            // locally to protect against a malicious thread changing the
            // contents.  This is because we will reference the contents
            // ourselves when the actual string is copied out carefully below.
            //

            UserBufferDriverName.Length = Verifier->BaseName.Length;
            UserBufferDriverName.MaximumLength = Verifier->BaseName.Length + sizeof (WCHAR);
            UserBufferDriverName.Buffer = (PWCHAR)(UserVerifyBuffer + 1);

            UserVerifyBuffer->DriverName = UserBufferDriverName;

            TotalSize += ROUND_UP (UserBufferDriverName.MaximumLength,
                                   sizeof(ULONG));
            NextEntryOffset += ROUND_UP (UserBufferDriverName.MaximumLength,
                                         sizeof(ULONG));

            if (TotalSize > SystemInformationLength) {
                ExRaiseStatus (STATUS_INFO_LENGTH_MISMATCH);
            }

            //
            // Carefully reference the UserVerifyBuffer here.
            //

            RtlMoveMemory(UserBufferDriverName.Buffer,
                          Verifier->BaseName.Buffer,
                          Verifier->BaseName.Length);

            UserBufferDriverName.Buffer[
                        Verifier->BaseName.Length/sizeof(WCHAR)] = UNICODE_NULL;
            UserVerifyBuffer->NextEntryOffset = NextEntryOffset;

            NextEntry = NextEntry->Flink;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);

    KeLeaveCriticalRegion();

    if (Status != STATUS_INFO_LENGTH_MISMATCH) {
        UserVerifyBuffer->NextEntryOffset = 0;
        *Length = TotalSize;
    }

    return Status;
}

NTSTATUS
MmSetVerifierInformation (
    IN OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength
    )

/*++

Routine Description:

    This routine sets any driver verifier flags that can be done without
    rebooting.

Arguments:

    SystemInformation - Gets and returns the driver verification flags.

    SystemInformationLength - Supplies the length of the SystemInformation
                              buffer.

Return Value:

    Returns the status of the operation.

Environment:

    The SystemInformation buffer is in user space and our caller has wrapped
    a try-except around this entire routine.  Capture any exceptions here and
    release resources accordingly.

--*/

{
    ULONG UserFlags;
    ULONG NewFlags;
    ULONG NewFlagsOn;
    ULONG NewFlagsOff;
    NTSTATUS Status;
    PULONG UserVerifyBuffer;

    PAGED_CODE();

    if (SystemInformationLength < sizeof (ULONG)) {
        ExRaiseStatus (STATUS_INFO_LENGTH_MISMATCH);
    }

    UserVerifyBuffer = (PULONG)SystemInformation;

    //
    // Synchronize all changes to the flags here.
    //

    Status = STATUS_SUCCESS;

    KeEnterCriticalRegion();

    KeWaitForSingleObject (&MmSystemLoadLock,
                           WrVirtualMemory,
                           KernelMode,
                           FALSE,
                           (PLARGE_INTEGER)NULL);

    try {

        UserFlags = *UserVerifyBuffer;

        //
        // Ensure nothing is being set or cleared that isn't supported.
        //
        //

        NewFlagsOn = UserFlags & VerifierModifyableOptions;

        NewFlags = MmVerifierData.Level | NewFlagsOn;

        //
        // Any bits set in NewFlagsOff must be zeroed in the NewFlags.
        //

        NewFlagsOff = ((~UserFlags) & VerifierModifyableOptions);

        NewFlags &= ~NewFlagsOff;

        if (NewFlags != MmVerifierData.Level) {
            VerifierOptionChanges += 1;
            MmVerifierData.Level = NewFlags;
            *UserVerifyBuffer = NewFlags;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);

    KeLeaveCriticalRegion();

    return Status;
}

typedef struct _VERIFIER_STRING_INFO {
   ULONG BuildNumber;
   ULONG DriverVerifierLevel;
   ULONG Flags;
   ULONG Check;
} VERIFIER_STRING_INFO, *PVERIFIER_STRING_INFO;

static WCHAR Printable[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
static ULONG PrintableChars = sizeof (Printable) / sizeof (Printable[0]) - 1;

VOID
ViPrintString (
    IN PUNICODE_STRING DriverName
    )

/*++

Routine Description:

    This routine does a really bad hash of build number, verifier level and
    flags by using the driver name as a stream of bytes to XOR into the flags,
    etc.

    This is a Neill Clift special.

Arguments:

    DriverName - Supplies the name of the driver.

Return Value:

    None.

--*/

{
    VERIFIER_STRING_INFO Bld;
    PUCHAR BufPtr;
    PWCHAR DriverPtr;
    ULONG BufLen;
    ULONG i;
    ULONG j;
    ULONG DriverChars;
    ULONG MaxChars;
    WCHAR OutBuf[sizeof (VERIFIER_STRING_INFO) * 2 + 1];
    UNICODE_STRING OutBufU;
    ULONG Rem;
    ULONG LastRem;
    LOGICAL Done;

    Bld.BuildNumber = NtBuildNumber;
    Bld.DriverVerifierLevel = MmVerifierData.Level;

    //
    // Unloads and other actions could be encoded in the Flags field here.
    //

    Bld.Flags = 0;

    //
    // Make the last ULONG a weird function of the others.
    //

    Bld.Check = ((Bld.Flags + 1) * Bld.BuildNumber * (Bld.DriverVerifierLevel + 1)) * 123456789;

    BufPtr = (PUCHAR) &Bld;
    BufLen = sizeof (Bld);

    DriverChars = DriverName->Length / sizeof (DriverName->Buffer[0]);
    DriverPtr = DriverName->Buffer;
    MaxChars = DriverChars;

    if (DriverChars < sizeof (VERIFIER_STRING_INFO)) {
        MaxChars = sizeof (VERIFIER_STRING_INFO);
    }

    //
    // Xor each character in the driver name into the buffer.
    //

    for (i = 0; i < MaxChars; i += 1) {
        BufPtr[i % BufLen] ^= (UCHAR) RtlUpcaseUnicodeChar(DriverPtr[i % DriverChars]);
    }

    //
    // Produce a base N decoding of the binary buffer using the printable
    // characters defines. Treat the binary as a byte array and do the
    // division for each, tracking the carry.
    //

    j = 0;
    do {
        Done = TRUE;

        for (i = 0, LastRem = 0; i < sizeof (VERIFIER_STRING_INFO); i += 1) {
            Rem = BufPtr[i] + 256 * LastRem;
            BufPtr[i] = (UCHAR) (Rem / PrintableChars);
            LastRem = Rem % PrintableChars;
            if (BufPtr[i]) {
                Done = FALSE;
            }
        }
        OutBuf[j++] = Printable[LastRem];

        if (j >= sizeof (OutBuf) / sizeof (OutBuf[0])) {

            //
            // The stack buffer isn't big enough.
            //

            return;
        }

    } while (Done == FALSE);

    OutBuf[j] = L'\0';

    OutBufU.Length = OutBufU.MaximumLength = (USHORT) j * sizeof (WCHAR);
    OutBufU.Buffer = OutBuf;

    DbgPrint ("*******************************************************************************\n");
    DbgPrint ("*                                                                             *\n");

    DbgPrint ("* This is the string to paste into your build mail\n");

    DbgPrint ("* Driver Verifier: Enabled for %Z on Build %ld %wZ\n",
              DriverName, NtBuildNumber & 0xFFFFFFF, &OutBufU);

    DbgPrint ("*                                                                             *\n");
    DbgPrint ("*******************************************************************************\n");

    return;
}

//
// BEWARE: Various kernel macros are undefined here so we can pull in the
// real routines.  This is needed because the real routines are exported for
// driver compatibility.  This module has been carefully laid out so these
// macros are not referenced from this point down and references go to the
// real routines.
//




#undef KeRaiseIrql
#undef KeLowerIrql
#undef KeAcquireSpinLock
#undef KeReleaseSpinLock
#undef ExAcquireResourceExclusive

VOID
KeRaiseIrql (
    IN KIRQL NewIrql,
    OUT PKIRQL OldIrql
    );

VOID
KeLowerIrql (
    IN KIRQL NewIrql
    );

VOID
KeAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock,
    OUT PKIRQL OldIrql
    );

VOID
KeReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    );

BOOLEAN
ExAcquireResourceExclusive (
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
    );

#if defined(_X86_)
#define X86_HAL_ROUTINE     1
#else
#define X86_HAL_ROUTINE     0
#endif

#define POOL_ROUTINE        2

#define VERIFIER_THUNK_IN_HAL(Flag) (Flag & X86_HAL_ROUTINE)
#define VERIFIER_POOL_ROUTINE(Flag) (Flag & POOL_ROUTINE)

VERIFIER_THUNKS MiVerifierThunks[] = {

    (PDRIVER_VERIFIER_THUNK_ROUTINE)ExAllocatePool,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierAllocatePool,
    POOL_ROUTINE,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)ExAllocatePoolWithQuota,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierAllocatePoolWithQuota,
    POOL_ROUTINE,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)ExAllocatePoolWithQuotaTag,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierAllocatePoolWithQuotaTag,
    POOL_ROUTINE,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)ExAllocatePoolWithTag,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierAllocatePoolWithTag,
    POOL_ROUTINE,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)ExAllocatePoolWithTagPriority,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierAllocatePoolWithTagPriority,
    POOL_ROUTINE,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)ExFreePool,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierFreePool,
    POOL_ROUTINE,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)ExFreePoolWithTag,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierFreePoolWithTag,
    POOL_ROUTINE,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeSetEvent,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierSetEvent,
    0,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)ExAcquireFastMutexUnsafe,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierExAcquireFastMutexUnsafe,
    0,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)ExReleaseFastMutexUnsafe,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierExReleaseFastMutexUnsafe,
    0,

#if 0
    //
    // This API is re-routed through the stale ddkresrc.c.
    //
    (PDRIVER_VERIFIER_THUNK_ROUTINE)ExAcquireResourceExclusive,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierExAcquireResourceExclusive,
    0,
#endif

    (PDRIVER_VERIFIER_THUNK_ROUTINE)ExAcquireResourceExclusiveLite,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierExAcquireResourceExclusive,
    0,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)ExReleaseResourceLite,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierExReleaseResource,
    0,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)MmProbeAndLockPages,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierProbeAndLockPages,
    0,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)MmProbeAndLockProcessPages,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierProbeAndLockProcessPages,
    0,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)MmMapIoSpace,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierMapIoSpace,
    0,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)MmMapLockedPages,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierMapLockedPages,
    0,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)MmMapLockedPagesSpecifyCache,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierMapLockedPagesSpecifyCache,
    0,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)MmUnlockPages,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierUnlockPages,
    0,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)MmUnmapLockedPages,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierUnmapLockedPages,
    0,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)MmUnmapIoSpace,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierUnmapIoSpace,
    0,

    //
    // These routines cannot be referenced as data on x86 because they
    // reside in the HAL.  So their addresses must be checked as a pointer.
    //

    (PDRIVER_VERIFIER_THUNK_ROUTINE)ExAcquireFastMutex,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierExAcquireFastMutex,
    X86_HAL_ROUTINE,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)ExTryToAcquireFastMutex,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierExTryToAcquireFastMutex,
    X86_HAL_ROUTINE,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)ExReleaseFastMutex,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierExReleaseFastMutex,
    X86_HAL_ROUTINE,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeRaiseIrql,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeRaiseIrql,
    X86_HAL_ROUTINE,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeLowerIrql,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeLowerIrql,
    X86_HAL_ROUTINE,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeAcquireSpinLock,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeAcquireSpinLock,
    X86_HAL_ROUTINE,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeReleaseSpinLock,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeReleaseSpinLock,
    X86_HAL_ROUTINE,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeSynchronizeExecution,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierSynchronizeExecution,
    0,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeInitializeTimerEx,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeInitializeTimerEx,
    0,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeInitializeTimer,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeInitializeTimer,
    0,

#if defined(_X86_)

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KfRaiseIrql,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKfRaiseIrql,
    X86_HAL_ROUTINE,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KfLowerIrql,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKfLowerIrql,
    X86_HAL_ROUTINE,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KfAcquireSpinLock,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKfAcquireSpinLock,
    X86_HAL_ROUTINE,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KfReleaseSpinLock,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKfReleaseSpinLock,
    X86_HAL_ROUTINE,

#endif

#if defined(_ALPHA_)

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeAcquireSpinLockAtDpcLevel,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeAcquireSpinLockAtDpcLevel,
    0,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeReleaseSpinLockFromDpcLevel,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeReleaseSpinLockFromDpcLevel,
    0,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeAcquireSpinLockRaiseToDpc,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeAcquireSpinLockRaiseToDpc,
    0,

#endif

    (PDRIVER_VERIFIER_THUNK_ROUTINE)IoFreeIrp,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)IovFreeIrp,
    0,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)IofCallDriver,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)IovCallDriver,
    0,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)IofCompleteRequest,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)IovCompleteRequest,
    0,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)IoBuildDeviceIoControlRequest,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)IovBuildDeviceIoControlRequest,
    0,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)IoBuildAsynchronousFsdRequest,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)IovBuildAsynchronousFsdRequest,
    0,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)IoInitializeTimer,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)IovInitializeTimer,
    0
};

LOGICAL
MiEnableVerifier (
    IN PLDR_DATA_TABLE_ENTRY DataTableEntry
    )

/*++

Routine Description:

    This function enables the verifier for the argument driver by thunking
    relevant system APIs in the argument driver import table.

Arguments:

    DataTableEntry - Supplies the data table entry for the driver.

Return Value:

    TRUE if thunking was applied, FALSE if not.

Environment:

    Kernel mode, Phase 0 Initialization and normal runtime.
    Non paged pool exists in Phase0, but paged pool does not.

--*/

{
    ULONG i;
    ULONG j;
    PULONG_PTR ImportThunk;
    ULONG ImportSize;
    PVERIFIER_THUNKS VerifierThunk;
    ULONG ThunkCount;
    LOGICAL Found;
    ULONG_PTR RealRoutine;
    PULONG_PTR PointerRealRoutine;
    PLIST_ENTRY NextEntry;
    PDRIVER_VERIFIER_THUNK_PAIRS ThunkTable;
    PDRIVER_SPECIFIED_VERIFIER_THUNKS ThunkTableBase;

    ImportThunk = (PULONG_PTR)RtlImageDirectoryEntryToData(
                                               DataTableEntry->DllBase,
                                               TRUE,
                                               IMAGE_DIRECTORY_ENTRY_IAT,
                                               &ImportSize);

    if (ImportThunk == NULL) {
        return FALSE;
    }

    ImportSize /= sizeof(PULONG_PTR);

    for (i = 0; i < ImportSize; i += 1, ImportThunk += 1) {

        Found = FALSE;

        VerifierThunk = MiVerifierThunks;

        for (ThunkCount = 0; ThunkCount < sizeof (MiVerifierThunks) / sizeof (VERIFIER_THUNKS); ThunkCount += 1) {

            if (KernelVerifier == TRUE) {
                if (VERIFIER_POOL_ROUTINE (VerifierThunk->Flag) == 0) {
                    VerifierThunk += 1;
                    continue;
                }
            }

            if (VERIFIER_THUNK_IN_HAL (VerifierThunk->Flag) == 0) {
                RealRoutine = (ULONG_PTR)VerifierThunk->PristineRoutine;
            }
            else {

                //
                // Only the x86 has/needs this oddity - take the kernel address,
                // knowing that it points at a 2 byte jmp opcode followed by
                // a 4-byte indirect pointer to a destination address.
                //

                PointerRealRoutine = (PULONG_PTR)*((PULONG_PTR)((PCHAR)VerifierThunk->PristineRoutine + 2));
                RealRoutine = *PointerRealRoutine;
            }

            if (*ImportThunk == RealRoutine) {
                *ImportThunk = (ULONG_PTR)(VerifierThunk->NewRoutine);
                Found = TRUE;
                break;
            }
            VerifierThunk += 1;
        }

        if (Found == FALSE) {

            NextEntry = MiVerifierDriverAddedThunkListHead.Flink;
            while (NextEntry != &MiVerifierDriverAddedThunkListHead) {

                ThunkTableBase = CONTAINING_RECORD(NextEntry,
                                                   DRIVER_SPECIFIED_VERIFIER_THUNKS,
                                                   ListEntry);

                ThunkTable = (PDRIVER_VERIFIER_THUNK_PAIRS)(ThunkTableBase + 1);

                for (j = 0; j < ThunkTableBase->NumberOfThunks; j += 1) {

                    if (*ImportThunk == (ULONG_PTR)ThunkTable->PristineRoutine) {
                        *ImportThunk = (ULONG_PTR)(ThunkTable->NewRoutine);
                        Found = TRUE;
                        break;
                    }
                    ThunkTable += 1;
                }

                if (Found == TRUE) {
                    break;
                }

                NextEntry = NextEntry->Flink;
            }
        }
    }
    return TRUE;
}

#if defined(_X86_)

DRIVER_VERIFIER_THUNK_PAIRS MiKernelVerifierThunks[] = {

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeRaiseIrql,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeRaiseIrql,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeLowerIrql,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeLowerIrql,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeAcquireSpinLock,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeAcquireSpinLock,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeReleaseSpinLock,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeReleaseSpinLock,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KfRaiseIrql,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKfRaiseIrql,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KfLowerIrql,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKfLowerIrql,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KfAcquireSpinLock,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKfAcquireSpinLock,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KfReleaseSpinLock,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKfReleaseSpinLock,

#if !defined(NT_UP)
    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeAcquireQueuedSpinLock,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeAcquireQueuedSpinLock,

    (PDRIVER_VERIFIER_THUNK_ROUTINE)KeReleaseQueuedSpinLock,
    (PDRIVER_VERIFIER_THUNK_ROUTINE)VerifierKeReleaseQueuedSpinLock,
#endif
};

VOID
MiEnableKernelVerifier (
    VOID
    )

/*++

Routine Description:

    This function enables the verifier for the kernel by thunking
    relevant HAL APIs in the kernel's import table.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, Phase 1 Initialization.

--*/

{
    ULONG i;
    PULONG_PTR ImportThunk;
    ULONG ImportSize;
    PDRIVER_VERIFIER_THUNK_PAIRS VerifierThunk;
    ULONG ThunkCount;
    ULONG_PTR RealRoutine;
    PULONG_PTR PointerRealRoutine;

    if (KernelVerifier == FALSE) {
        return;
    }

    ImportThunk = (PULONG_PTR)RtlImageDirectoryEntryToData(
                                               PsNtosImageBase,
                                               TRUE,
                                               IMAGE_DIRECTORY_ENTRY_IAT,
                                               &ImportSize);

    if (ImportThunk == NULL) {
        return;
    }

    ImportSize /= sizeof(PULONG_PTR);

    for (i = 0; i < ImportSize; i += 1, ImportThunk += 1) {

        VerifierThunk = MiKernelVerifierThunks;

        for (ThunkCount = 0; ThunkCount < sizeof (MiKernelVerifierThunks) / sizeof (DRIVER_VERIFIER_THUNK_PAIRS); ThunkCount += 1) {

            //
            // Only the x86 has/needs this oddity - take the kernel address,
            // knowing that it points at a 2 byte jmp opcode followed by
            // a 4-byte indirect pointer to a destination address.
            //

            PointerRealRoutine = (PULONG_PTR)*((PULONG_PTR)((PCHAR)VerifierThunk->PristineRoutine + 2));
            RealRoutine = *PointerRealRoutine;

            if (*ImportThunk == RealRoutine) {

                //
                // Order is important here.
                //

                if (MiKernelVerifierOriginalCalls[ThunkCount] == NULL) {
                    MiKernelVerifierOriginalCalls[ThunkCount] = (PVOID)RealRoutine;
                }

                *ImportThunk = (ULONG_PTR)(VerifierThunk->NewRoutine);

                break;
            }
            VerifierThunk += 1;
        }
    }
    return;
}
#endif

//
// BEWARE: Various kernel macros were undefined above so we can pull in the
// real routines.  This is needed because the real routines are exported for
// driver compatibility.  This module has been carefully laid out so these
// macros are not referenced from that point to here and references go to the
// real routines.
//
// BE EXTREMELY CAREFUL IF YOU DECIDE TO ADD ROUTINES BELOW THIS POINT !
//
