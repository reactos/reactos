/*
 * PROJECT:     ReactOS Arbitration Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Generic Arbiter Library
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#define TAG_ARBITER  'ibrA'

/*
 * ARBITER_ALTERNATIVE.Priority:
 * The arbiter allocation engine walks the alternatives in increasing priority.
 * An ordinary alternative's priority is its ordering-list index biased by one
 * (except for IO_RESOURCE_PREFERRED, so preferred ranges sort first).
 * Once the orderings are exhausted it gets one final whole-window pass
 * at (PREFERRED_)RESERVED before getting set to EXHAUSTED.
 *
 * Public as any driver can modify these of any range that's passed down.
 */
#define ARBITER_PRIORITY_NULL               0x00000000
#define ARBITER_PRIORITY_PREFERRED_RESERVED 0x7FFFFFFD
#define ARBITER_PRIORITY_RESERVED           0x7FFFFFFE
#define ARBITER_PRIORITY_EXHAUSTED          0x7FFFFFFF

/* ARBITER_ALTERNATIVE.Flags */
#define ARBITER_ALTERNATIVE_FLAG_FIXED      0x00000001  // one placement only
#define ARBITER_ALTERNATIVE_FLAG_SHARED     0x00000002  // CmResourceShareShared
#define ARBITER_ALTERNATIVE_FLAG_BADRANGE   0x00000004  // Maximum < Minimum

/*
 * Range attribute bits
 *
 * ARBITER_RANGE_SHARED_DRIVER:
 * Marks a range with a CmResourceShareDriverExclusive
 *
 * ARBITER_RANGE_BOOT_ALLOCATED:
 * Marks a firmware boot configuration
 */
#define ARBITER_RANGE_SHARED_DRIVER         0x02
#define ARBITER_RANGE_BOOT_ALLOCATED        0x04

/* ARBITER_ALLOCATION_STATE.Flags */
#define ARBITER_STATE_FLAG_NULL_CONFLICT_OK 0x0001  // a NULL-owner conflict is OK
#define ARBITER_STATE_FLAG_BOOT             0x0004  // reserving a firmware boot config
#define ARBITER_STATE_FLAG_WORKSPACE        0x0010  // WorkSpace holds a pool block to free

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
ArbiterLibInitializeInstance(
    _Inout_ PARBITER_INSTANCE Arbiter,
    _In_ PDEVICE_OBJECT BusDeviceObject,
    _In_ CM_RESOURCE_TYPE ResourceType,
    _In_ PCWSTR ArbiterName,
    _In_ PCWSTR OrderName,
    _In_ PARB_TRANSLATE_ORDERING TranslateOrderingFunction
);

CODE_SEG("PAGE")
VOID
NTAPI
ArbiterLibDeleteInstance(
    _In_ PARBITER_INSTANCE Arbiter
);

CODE_SEG("PAGE")
VOID
NTAPI
ArbiterLibFreeOrderingList(
    _Inout_ PARBITER_ORDERING_LIST OrderingList
);

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibAddOrdering(
    _Inout_ PARBITER_ORDERING_LIST OrderingList,
    _In_ UINT64 Start,
    _In_ UINT64 End
);

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibDefaultAssignmentOrdering(
    _Inout_ PARBITER_INSTANCE Arbiter,
    _In_ PCWSTR AllocationOrderName,
    _In_ PCWSTR ReservedResourcesName,
    _In_opt_ PARB_TRANSLATE_ORDERING TranslateOrderingFunction
);

/* The generic dispatch entry point (installed as ARBITER_INTERFACE.ArbiterHandler). */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibHandler(
    _In_ PVOID Context,
    _In_ ARBITER_ACTION Action,
    _Inout_ PARBITER_PARAMETERS Parameters
);

#if (NTDDI_VERSION >= NTDDI_VISTA)
CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibTestAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_TEST_ALLOCATION_PARAMETERS Parameters
);

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibRetestAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_RETEST_ALLOCATION_PARAMETERS Parameters
);

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibBootAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_BOOT_ALLOCATION_PARAMETERS Parameters
);

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibQueryConflict(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_QUERY_CONFLICT_PARAMETERS Parameters
);

#else // (NTDDI_VERSION < NTDDI_VISTA)

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibTestAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PLIST_ENTRY ArbitrationList
);

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibRetestAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PLIST_ENTRY ArbitrationList
);

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibBootAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PLIST_ENTRY ArbitrationList
);

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibQueryConflict(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _In_ PIO_RESOURCE_DESCRIPTOR ConflictingResource,
    _Out_ PULONG ConflictCount,
    _Out_ PARBITER_CONFLICT_INFO *Conflicts
);
#endif // (NTDDI_VERSION >= NTDDI_VISTA)

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibCommitAllocation(
    _In_ PARBITER_INSTANCE Arbiter
);

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibRollbackAllocation(
    _In_ PARBITER_INSTANCE Arbiter
);

CODE_SEG("PAGE")
BOOLEAN
NTAPI
ArbiterLibGetNextAllocationRange(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState
);

CODE_SEG("PAGE")
BOOLEAN
NTAPI
ArbiterLibFindSuitableRange(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState
);

CODE_SEG("PAGE")
VOID
NTAPI
ArbiterLibAddAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState
);

CODE_SEG("PAGE")
VOID
NTAPI
ArbiterLibBacktrackAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState
);

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibSortArbitrationList(
    _Inout_ PLIST_ENTRY ArbitrationList);
