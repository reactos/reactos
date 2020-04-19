/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:            lib/drivers/arbiter/arbiter.h
 * PURPOSE:         Hardware Resources Arbiter Library
 * PROGRAMMERS:     Copyright 2020 Vadim Galyant <vgal@rambler.ru>
 */

#ifndef _ARBITER_H
#define _ARBITER_H

typedef struct _ARBITER_ORDERING
{
    ULONGLONG Start;
    ULONGLONG End;
} ARBITER_ORDERING, *PARBITER_ORDERING;

typedef struct _ARBITER_ORDERING_LIST
{
    USHORT Count;
    USHORT Maximum;
    PARBITER_ORDERING Orderings;
} ARBITER_ORDERING_LIST, *PARBITER_ORDERING_LIST;

typedef struct _ARBITER_ALTERNATIVE
{
    ULONGLONG Minimum;
    ULONGLONG Maximum;
    ULONG Length;
    ULONG Alignment;
    LONG Priority;
    ULONG Flags;
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    ULONG Reserved[3];
} ARBITER_ALTERNATIVE, *PARBITER_ALTERNATIVE;

typedef struct _ARBITER_ALLOCATION_STATE
{
    ULONGLONG Start;
    ULONGLONG End;
    ULONGLONG CurrentMinimum;
    ULONGLONG CurrentMaximum;
    PARBITER_LIST_ENTRY Entry;
    PARBITER_ALTERNATIVE CurrentAlternative;
    ULONG AlternativeCount;
    PARBITER_ALTERNATIVE Alternatives;
    USHORT Flags;
    UCHAR RangeAttributes;
    UCHAR RangeAvailableAttributes;
    ULONG_PTR WorkSpace;
} ARBITER_ALLOCATION_STATE, *PARBITER_ALLOCATION_STATE;

typedef struct _ARBITER_INSTANCE *PARBITER_INSTANCE;

typedef NTSTATUS
(NTAPI * PARB_UNPACK_REQUIREMENT)(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _Out_ PULONGLONG OutMinimumAddress,
    _Out_ PULONGLONG OutMaximumAddress,
    _Out_ PULONG OutLength,
    _Out_ PULONG OutAlignment
);

typedef NTSTATUS
(NTAPI * PARB_PACK_RESOURCE)(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _In_ ULONGLONG Start,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor
);

typedef NTSTATUS
(NTAPI * PARB_UNPACK_RESOURCE)(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    _Out_ PULONGLONG Start,
    _Out_ PULONG OutLength
);

typedef LONG
(NTAPI * PARB_SCORE_REQUIREMENT)(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor
);

typedef NTSTATUS
(NTAPI * PARB_TEST_ALLOCATION)(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PLIST_ENTRY ArbitrationList
);

typedef NTSTATUS
(NTAPI * PARB_RETEST_ALLOCATION)(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PLIST_ENTRY ArbitrationList
);

typedef NTSTATUS
(NTAPI * PARB_COMMIT_ALLOCATION)(
    _In_ PARBITER_INSTANCE Arbiter
);

typedef NTSTATUS
(NTAPI * PARB_ROLLBACK_ALLOCATION)(
    _In_ PARBITER_INSTANCE Arbiter
);

typedef NTSTATUS
(NTAPI * PARB_BOOT_ALLOCATION)(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PLIST_ENTRY ArbitrationList
);

/*  Not correct yet, FIXME! */
typedef NTSTATUS
(NTAPI * PARB_QUERY_ARBITRATE)(
    _In_ PARBITER_INSTANCE Arbiter
);

/*  Not correct yet, FIXME! */
typedef NTSTATUS
(NTAPI * PARB_QUERY_CONFLICT)(
    _In_ PARBITER_INSTANCE Arbiter
);

/*  Not correct yet, FIXME! */
typedef NTSTATUS
(NTAPI * PARB_ADD_RESERVED)(
    _In_ PARBITER_INSTANCE Arbiter
);

/*  Not correct yet, FIXME! */
typedef NTSTATUS
(NTAPI * PARB_START_ARBITER)(
    _In_ PARBITER_INSTANCE Arbiter
);

typedef NTSTATUS
(NTAPI * PARB_PREPROCESS_ENTRY)(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState
);

typedef NTSTATUS
(NTAPI * PARB_ALLOCATE_ENTRY)(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState
);

typedef BOOLEAN
(NTAPI * PARB_GET_NEXT_ALLOCATION_RANGE)(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState
);

typedef BOOLEAN
(NTAPI * PARB_FIND_SUITABLE_RANGE)(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState
);

typedef VOID
(NTAPI * PARB_ADD_ALLOCATION)(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState
);

typedef VOID
(NTAPI * PARB_BACKTRACK_ALLOCATION)(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState
);

/*  Not correct yet, FIXME! */
typedef NTSTATUS
(NTAPI * PARB_OVERRIDE_CONFLICT)(
    _In_ PARBITER_INSTANCE Arbiter
);

typedef struct _ARBITER_INSTANCE
{
    ULONG Signature;
    PKEVENT MutexEvent;
    PCWSTR Name;
    CM_RESOURCE_TYPE ResourceType;
    PRTL_RANGE_LIST Allocation;
    PRTL_RANGE_LIST PossibleAllocation;
    ARBITER_ORDERING_LIST OrderingList;
    ARBITER_ORDERING_LIST ReservedList;
    LONG ReferenceCount;
    PARBITER_INTERFACE Interface;
    ULONG AllocationStackMaxSize;
    PARBITER_ALLOCATION_STATE AllocationStack;
    PARB_UNPACK_REQUIREMENT UnpackRequirement;
    PARB_PACK_RESOURCE PackResource;
    PARB_UNPACK_RESOURCE UnpackResource;
    PARB_SCORE_REQUIREMENT ScoreRequirement;
    PARB_TEST_ALLOCATION TestAllocation;
    PARB_RETEST_ALLOCATION RetestAllocation;
    PARB_COMMIT_ALLOCATION CommitAllocation;
    PARB_ROLLBACK_ALLOCATION RollbackAllocation;
    PARB_BOOT_ALLOCATION BootAllocation;
    PARB_QUERY_ARBITRATE QueryArbitrate; // Not used yet
    PARB_QUERY_CONFLICT QueryConflict; // Not used yet
    PARB_ADD_RESERVED AddReserved; // Not used yet
    PARB_START_ARBITER StartArbiter; // Not used yet
    PARB_PREPROCESS_ENTRY PreprocessEntry;
    PARB_ALLOCATE_ENTRY AllocateEntry;
    PARB_GET_NEXT_ALLOCATION_RANGE GetNextAllocationRange;
    PARB_FIND_SUITABLE_RANGE FindSuitableRange;
    PARB_ADD_ALLOCATION AddAllocation;
    PARB_BACKTRACK_ALLOCATION BacktrackAllocation;
    PARB_OVERRIDE_CONFLICT OverrideConflict; // Not used yet
    BOOLEAN TransactionInProgress;
    PVOID Extension;
    PDEVICE_OBJECT BusDeviceObject;
    PVOID ConflictCallbackContext;
    PVOID ConflictCallback;
} ARBITER_INSTANCE, *PARBITER_INSTANCE;

typedef NTSTATUS
(NTAPI * PARB_TRANSLATE_ORDERING)(
    _Out_ PIO_RESOURCE_DESCRIPTOR OutIoDescriptor,
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor
);

NTSTATUS
NTAPI
ArbInitializeArbiterInstance(
    _Inout_ PARBITER_INSTANCE Arbiter,
    _In_ PDEVICE_OBJECT BusDeviceObject,
    _In_ CM_RESOURCE_TYPE ResourceType,
    _In_ PCWSTR ArbiterName,
    _In_ PCWSTR OrderName,
    _In_ PARB_TRANSLATE_ORDERING TranslateOrderingFunction
);

#endif  /* _ARBITER_H */
