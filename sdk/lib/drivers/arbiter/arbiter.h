/*
 * PROJECT:     ReactOS Arbitrartion Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Generic Arbiter Library
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

typedef struct _ARBITER_ALTERNATIVE
{
    UINT64 Minimum;
    UINT64 Maximum;
#if (NTDDI_VERSION >= NTDDI_VISTA)
    UINT64 Length;
    UINT64 Alignment;
#else
    UINT32 Length;
    UINT32 Alignment;
#endif
    INT32 Priority;
    UINT32 Flags;
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    UINT32 Reserved[3];
} ARBITER_ALTERNATIVE, *PARBITER_ALTERNATIVE;

typedef struct _ARBITER_ALLOCATION_STATE
{
    UINT64 Start;
    UINT64 End;
    UINT64 CurrentMinimum;
    UINT64 CurrentMaximum;
    PARBITER_LIST_ENTRY Entry;
    PARBITER_ALTERNATIVE CurrentAlternative;
    UINT32 AlternativeCount;
    PARBITER_ALTERNATIVE Alternatives;
    UINT16 Flags;
    UCHAR RangeAttributes;
    UCHAR RangeAvailableAttributes;
    ULONG_PTR WorkSpace;
} ARBITER_ALLOCATION_STATE, *PARBITER_ALLOCATION_STATE;

typedef struct _ARBITER_ORDERING
{
    UINT64 Start;
    UINT64 End;
} ARBITER_ORDERING, *PARBITER_ORDERING;

typedef struct _ARBITER_ORDERING_LIST
{
    UINT16 Count;
    UINT16 Maximum;
    PARBITER_ORDERING Orderings;
} ARBITER_ORDERING_LIST, *PARBITER_ORDERING_LIST;

typedef struct _ARBITER_INSTANCE *PARBITER_INSTANCE;

typedef NTSTATUS
(NTAPI * PARB_UNPACK_REQUIREMENT)(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _Out_ PUINT64 OutMinimumAddress,
    _Out_ PUINT64 OutMaximumAddress,
    _Out_ PUINT64 OutLength,
    _Out_ PUINT64 OutAlignment
);

typedef NTSTATUS
(NTAPI * PARB_PACK_RESOURCE)(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _In_ UINT64 Start,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor
);

typedef NTSTATUS
(NTAPI * PARB_UNPACK_RESOURCE)(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    _Out_ PUINT64 Start,
    _Out_ PUINT64 OutLength
);

typedef INT32
(NTAPI * PARB_SCORE_REQUIREMENT)(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor
);

#if (NTDDI_VERSION >= NTDDI_VISTA)
typedef NTSTATUS
(NTAPI * PARB_TEST_ALLOCATION)(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_TEST_ALLOCATION_PARAMETERS Parameters
);

typedef NTSTATUS
(NTAPI * PARB_RETEST_ALLOCATION)(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_RETEST_ALLOCATION_PARAMETERS Parameters
);

typedef NTSTATUS
(NTAPI * PARB_BOOT_ALLOCATION)(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_BOOT_ALLOCATION_PARAMETERS Parameters
);

typedef NTSTATUS
(NTAPI * PARB_QUERY_ARBITRATE)(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_QUERY_ARBITRATE_PARAMETERS Parameters
);

typedef NTSTATUS
(NTAPI * PARB_QUERY_CONFLICT)(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_QUERY_CONFLICT_PARAMETERS Parameters
);

typedef NTSTATUS
(NTAPI * PARB_ADD_RESERVED)(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ADD_RESERVED_PARAMETERS Parameters
);

#else
typedef NTSTATUS
(NTAPI * PARB_TEST_ALLOCATION)(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PLIST_ENTRY ArbitrationList
);

typedef NTSTATUS
(NTAPI * PARB_RETEST_ALLOCATION)(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PLIST_ENTRY ArbitrationList
);

typedef NTSTATUS
(NTAPI * PARB_BOOT_ALLOCATION)(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PLIST_ENTRY ArbitrationList
);

typedef NTSTATUS
(NTAPI * PARB_QUERY_ARBITRATE)(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PLIST_ENTRY ArbitrationList
);

typedef NTSTATUS
(NTAPI * PARB_QUERY_CONFLICT)(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _In_ PIO_RESOURCE_DESCRIPTOR ConflictingResource,
    _Out_ PULONG ConflictCount,
    _Out_ PARBITER_CONFLICT_INFO *Conflicts
);

typedef NTSTATUS
(NTAPI * PARB_ADD_RESERVED)(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_opt_ PIO_RESOURCE_DESCRIPTOR Requirement,
    _In_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource
);
#endif // (NTDDI_VERSION >= NTDDI_VISTA)

typedef NTSTATUS
(NTAPI * PARB_COMMIT_ALLOCATION)(
    _In_ PARBITER_INSTANCE Arbiter
);

typedef NTSTATUS
(NTAPI * PARB_ROLLBACK_ALLOCATION)(
    _In_ PARBITER_INSTANCE Arbiter
);

typedef NTSTATUS
(NTAPI * PARB_START_ARBITER)(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PCM_RESOURCE_LIST StartResources
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

typedef BOOLEAN
(NTAPI * PARB_OVERRIDE_CONFLICT)(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState
);

typedef NTSTATUS
(NTAPI * PARB_TRANSLATE_ORDERING)(
    _Out_ PIO_RESOURCE_DESCRIPTOR OutIoDescriptor,
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor
);

#if (NTDDI_VERSION >= NTDDI_VISTA)
typedef NTSTATUS
(NTAPI * PARB_INITIALIZE_RANGE_LIST)(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ ULONG DescriptorCount,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptors,
    _Inout_ PRTL_RANGE_LIST RangeList
);
#endif

typedef struct _ARBITER_INSTANCE
{
    UINT32 Signature;
    PKEVENT MutexEvent;
    PCWSTR Name;
#if (NTDDI_VERSION >= NTDDI_VISTA)
    PCWSTR OrderingName;    // Vista+: selects registry AllocationOrder\<OrderingName>
#endif
    CM_RESOURCE_TYPE ResourceType;
    PRTL_RANGE_LIST Allocation;
    PRTL_RANGE_LIST PossibleAllocation;
    ARBITER_ORDERING_LIST OrderingList;
    ARBITER_ORDERING_LIST ReservedList;
    ULONG ReferenceCount;
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
    PARB_QUERY_ARBITRATE QueryArbitrate;
    PARB_QUERY_CONFLICT QueryConflict;
    PARB_ADD_RESERVED AddReserved;
    PARB_START_ARBITER StartArbiter;
    PARB_PREPROCESS_ENTRY PreprocessEntry;
    PARB_ALLOCATE_ENTRY AllocateEntry;
    PARB_GET_NEXT_ALLOCATION_RANGE GetNextAllocationRange;
    PARB_FIND_SUITABLE_RANGE FindSuitableRange;
    PARB_ADD_ALLOCATION AddAllocation;
    PARB_BACKTRACK_ALLOCATION BacktrackAllocation;
    PARB_OVERRIDE_CONFLICT OverrideConflict;
#if (NTDDI_VERSION >= NTDDI_VISTA)
    PARB_INITIALIZE_RANGE_LIST InitializeRangeList;
#endif
    BOOLEAN TransactionInProgress;
#if (NTDDI_VERSION >= NTDDI_VISTA)
    PKEVENT TransactionEvent;
#endif
    PVOID Extension;
    PDEVICE_OBJECT BusDeviceObject;
    PVOID ConflictCallbackContext;
    PRTL_CONFLICT_RANGE_CALLBACK ConflictCallback;
#if (NTDDI_VERSION >= NTDDI_VISTA) && (NTDDI_VERSION < NTDDI_WINBLUE)
    WCHAR PdoDescriptionString[336];
    CHAR PdoSymbolicNameString[672];
    WCHAR PdoAddressString[1];
#endif
} ARBITER_INSTANCE, *PARBITER_INSTANCE;

CODE_SEG("PAGE")
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
