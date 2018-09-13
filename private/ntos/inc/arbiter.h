/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    arbiter.h

Abstract:

    This module contains support routines for the Pnp resource arbiters.

Author:

    Andrew Thornton (andrewth) 1-April-1997


Environment:

    Kernel mode

--*/

#ifndef _ARBITER_
#define _ARBITER_

#if !defined(MAXULONGLONG)
#define MAXULONGLONG ((ULONGLONG)-1)
#endif


#if ARB_DBG

//
// Debug print level:
//    -1 = no messages
//     0 = vital messages only
//     1 = call trace
//     2 = verbose messages
//

extern LONG ArbDebugLevel;

#define ARB_PRINT(Level, Message) \
    if (Level <= ArbDebugLevel) DbgPrint Message

#define ARB_INDENT(Level, Count) \
    if (Level < ArbDebugLevel) ArbpIndent(Count)

#else

#define ARB_PRINT(Level, Message)
#define ARB_INDENT(Level, Count)

#endif // ARB_DBG


//
// The ARBITER_ORDRING_LIST abstract data type
//

typedef struct _ARBITER_ORDERING {
    ULONGLONG Start;
    ULONGLONG End;
} ARBITER_ORDERING, *PARBITER_ORDERING;


typedef struct _ARBITER_ORDERING_LIST {

    //
    // The number of valid entries in the array
    //
    USHORT Count;

    //
    // The maximum number of entries that can fit in the Ordering buffer
    //
    USHORT Maximum;

    //
    // Array of orderings
    //
    PARBITER_ORDERING Orderings;

} ARBITER_ORDERING_LIST, *PARBITER_ORDERING_LIST;


NTSTATUS
ArbInitializeOrderingList(
    IN OUT PARBITER_ORDERING_LIST List
    );

VOID
ArbFreeOrderingList(
    IN OUT PARBITER_ORDERING_LIST List
    );

NTSTATUS
ArbCopyOrderingList(
    OUT PARBITER_ORDERING_LIST Destination,
    IN PARBITER_ORDERING_LIST Source
    );

NTSTATUS
ArbAddOrdering(
    OUT PARBITER_ORDERING_LIST List,
    IN ULONGLONG Start,
    IN ULONGLONG End
    );

NTSTATUS
ArbPruneOrdering(
    IN OUT PARBITER_ORDERING_LIST OrderingList,
    IN ULONGLONG Start,
    IN ULONGLONG End
    );

//
// ULONGLONG
// ALIGN_ADDRESS_DOWN(
//    ULONGLONG address,
//    ULONG alignment
//    );
//
// This aligns address to the previously correctly aligned value
//
#define ALIGN_ADDRESS_DOWN(address, alignment) \
    ((address) & ~((ULONGLONG)alignment - 1))

//
// ULONGLONG
// ALIGN_ADDRESS_UP(
//    ULONGLONG address,
//    ULONG alignment
//    );
//
// This aligns address to the next correctly aligned value
//
#define ALIGN_ADDRESS_UP(address, alignment) \
    (ALIGN_ADDRESS_DOWN( (address + alignment - 1), alignment))


#define LENGTH_OF(_start, _end) \
    ((_end) - (_start) + 1)

//
// This indicates that the alternative can coexist with shared resources and
// should be added to the range lists shared
//
#define ARBITER_ALTERNATIVE_FLAG_SHARED         0x00000001

//
// This indicates that the request if for a specific range with no alternatives.
// ie (End - Start + 1 == Length) eg port 60-60 L1 A1
//
#define ARBITER_ALTERNATIVE_FLAG_FIXED          0x00000002

//
// This indicates that request is invalid
//
#define ARBITER_ALTERNATIVE_FLAG_INVALID        0x00000004

typedef struct _ARBITER_ALTERNATIVE {

    //
    // The minimum acceptable start value from the requirement descriptor
    //
    ULONGLONG Minimum;

    //
    // The maximum acceptable end value from the requirement descriptor
    //
    ULONGLONG Maximum;

    //
    // The length from the requirement descriptor
    //
    ULONG Length;

    //
    // The alignment from the requirement descriptor
    //
    ULONG Alignment;

    //
    // Priority index - BUGBUG - explain better
    //

    LONG Priority;

    //
    // Flags - ARBITER_ALTERNATIVE_FLAG_SHARED - indicates the current
    //             requirement was for a shared resource.
    //         ARBITER_ALTERNATIVE_FLAG_FIXED - indicates the current
    //             requirement is for a specific resource (eg ports 220-230 and
    //             nothing else)
    //
    ULONG Flags;

    //
    // Descriptor - the descriptor describing this alternative
    //
    PIO_RESOURCE_DESCRIPTOR Descriptor;

    //
    // Packing...
    //
    ULONG Reserved[3];

} ARBITER_ALTERNATIVE, *PARBITER_ALTERNATIVE;

//
// The least significant 16 bits are reserved for the base arbitration code
// the most significant are arbiter specific
//

#define ARBITER_STATE_FLAG_RETEST           0x0001
#define ARBITER_STATE_FLAG_BOOT             0x0002
#define ARBITER_STATE_FLAG_CONFLICT         0x0004
#define ARBITER_STATE_FLAG_NULL_CONFLICT_OK 0x0008

typedef struct _ARBITER_ALLOCATION_STATE {

    //
    // The current value being considered as a possible start value
    //
    ULONGLONG Start;

    //
    // The current value being considered as a possible end value
    //
    ULONGLONG End;

    //
    // The values currently being considered as the Minimum and Maximum (this is
    // different because the prefered orderings can restrict the ranges where
    // we can allocate)
    //
    ULONGLONG CurrentMinimum;
    ULONGLONG CurrentMaximum;

    //
    // The entry in the arbitration list containing this request.
    //
    PARBITER_LIST_ENTRY Entry;

    //
    // The alternative currently being considered
    //
    PARBITER_ALTERNATIVE CurrentAlternative;

    //
    // The number of alternatives in the Alternatives array
    //
    ULONG AlternativeCount;

    //
    // The arbiters representation of the alternatives being considered
    //
    PARBITER_ALTERNATIVE Alternatives;

    //
    // Flags - ARBITER_STATE_FLAG_RETEST - indicates that we are in a retest
    //              operation not a test.
    //         ARBITER_STATE_FLAG_BOOT - indicates we are in a boot allocation
    //              operation not a test.
    //
    USHORT Flags;

    //
    // RangeAttributes - these are logically ORed in to the attributes for all
    // ranges added to the range list.
    //
    UCHAR RangeAttributes;

    //
    // Ranges that are to be considered available
    //
    UCHAR RangeAvailableAttributes;

    //
    // Space for the arbiter to use as it wishes
    //
    ULONG_PTR WorkSpace;

} ARBITER_ALLOCATION_STATE, *PARBITER_ALLOCATION_STATE;

typedef struct _ARBITER_INSTANCE ARBITER_INSTANCE, *PARBITER_INSTANCE;

typedef
NTSTATUS
(*PARBITER_UNPACK_REQUIREMENT) (
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Minimum,
    OUT PULONGLONG Maximum,
    OUT PULONG Length,
    OUT PULONG Alignment
    );

typedef
NTSTATUS
(*PARBITER_PACK_RESOURCE) (
    IN PIO_RESOURCE_DESCRIPTOR Requirement,
    IN ULONGLONG Start,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    );

typedef
NTSTATUS
(*PARBITER_UNPACK_RESOURCE) (
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Start,
    OUT PULONG Length
    );

typedef
LONG
(*PARBITER_SCORE_REQUIREMENT) (
    IN PIO_RESOURCE_DESCRIPTOR Descriptor
    );

typedef
NTSTATUS
(*PARBITER_PREPROCESS_ENTRY)(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE Entry
    );

typedef
NTSTATUS
(*PARBITER_ALLOCATE_ENTRY)(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE Entry
    );

typedef
NTSTATUS
(*PARBITER_TEST_ALLOCATION)(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    );

typedef
NTSTATUS
(*PARBITER_COMMIT_ALLOCATION)(
    IN PARBITER_INSTANCE Arbiter
    );

typedef
NTSTATUS
(*PARBITER_ROLLBACK_ALLOCATION)(
    IN PARBITER_INSTANCE Arbiter
    );

typedef
NTSTATUS
(*PARBITER_RETEST_ALLOCATION)(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    );

typedef
NTSTATUS
(*PARBITER_BOOT_ALLOCATION)(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    );

typedef
NTSTATUS
(*PARBITER_ADD_RESERVED)(
    IN PARBITER_INSTANCE Arbiter,
    IN PIO_RESOURCE_DESCRIPTOR Requirement      OPTIONAL,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource OPTIONAL
    );

typedef
BOOLEAN
(*PARBITER_GET_NEXT_ALLOCATION_RANGE)(
    PARBITER_INSTANCE Arbiter,
    PARBITER_ALLOCATION_STATE State
    );

typedef
BOOLEAN
(*PARBITER_FIND_SUITABLE_RANGE)(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    );

typedef
VOID
(*PARBITER_ADD_ALLOCATION)(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    );

typedef
VOID
(*PARBITER_BACKTRACK_ALLOCATION)(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    );

typedef
BOOLEAN
(*PARBITER_OVERRIDE_CONFLICT)(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    );

typedef
NTSTATUS
(*PARBITER_QUERY_ARBITRATE)(
    IN PARBITER_INSTANCE Arbiter,
    IN PLIST_ENTRY ArbitrationList
    );

typedef
NTSTATUS
(*PARBITER_QUERY_CONFLICT)(
    IN PARBITER_INSTANCE Arbiter,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PIO_RESOURCE_DESCRIPTOR ConflictingResource,
    OUT PULONG ConflictCount,
    OUT PARBITER_CONFLICT_INFO *Conflicts
    );

typedef
NTSTATUS
(*PARBITER_START_ARBITER)(
    IN PARBITER_INSTANCE Arbiter,
    IN PCM_RESOURCE_LIST StartResources
    );

//
// Attributes for the ranges
//

#define ARBITER_RANGE_BOOT_ALLOCATED    0x01

#define ARBITER_RANGE_ALIAS             0x10
#define ARBITER_RANGE_POSITIVE_DECODE   0x20

#define INITIAL_ALLOCATION_STATE_SIZE   PAGE_SIZE

#define ARBITER_INSTANCE_SIGNATURE      'sbrA'


typedef struct _ARBITER_INSTANCE {
    //
    // Signature - must be ARBITER_INSTANCE_SIGNATURE
    //
    ULONG Signature;

    //
    // Synchronisation lock
    //
    PKEVENT MutexEvent;

    //
    // The name of this arbiter - used for debugging and registry storage
    //
    PWSTR Name;

    //
    // The resource type this arbiter arbitrates.
    //
    CM_RESOURCE_TYPE ResourceType;

    //
    // Pointer to a pool allocated range list which contains the current
    // allocation
    //
    PRTL_RANGE_LIST Allocation;

    //
    // Pointer to a pool allocated range list which contains the allocation
    // under considetation.  This is set by test allocation.
    //
    PRTL_RANGE_LIST PossibleAllocation;

    //
    // The order in which these resources should be allocated.  Taken from the
    // HKLM\System\CurrentControlSet\Control\SystemResources\AssignmentOrdering
    // key and modified based on the reserved resources.
    //
    ARBITER_ORDERING_LIST OrderingList;

    //
    // The resources that should be reserved (not allocated until absolutley
    // necessary)
    //
    ARBITER_ORDERING_LIST ReservedList;

    //
    // The reference count of the number of entities that are using the
    // ARBITER_INTERFACE associated with this instance.
    //
    LONG ReferenceCount;

    //
    // The ARBITER_INTERFACE associated with this instance.
    //
    PARBITER_INTERFACE Interface;

    //
    // The size in bytes of the currently allocated AllocationStack
    //
    ULONG AllocationStackMaxSize;

    //
    // A pointer to an array of ARBITER_ALLOCATION_STATE entries encapsulating
    // the state of the current arbitration
    //
    PARBITER_ALLOCATION_STATE AllocationStack;


    //
    // Required helper function dispatches - these functions must always be
    // provided
    //

    PARBITER_UNPACK_REQUIREMENT UnpackRequirement;
    PARBITER_PACK_RESOURCE PackResource;
    PARBITER_UNPACK_RESOURCE UnpackResource;
    PARBITER_SCORE_REQUIREMENT ScoreRequirement;


    //
    // Main arbiter action dispatches
    //
    PARBITER_TEST_ALLOCATION TestAllocation;                    OPTIONAL
    PARBITER_RETEST_ALLOCATION RetestAllocation;                OPTIONAL
    PARBITER_COMMIT_ALLOCATION CommitAllocation;                OPTIONAL
    PARBITER_ROLLBACK_ALLOCATION RollbackAllocation;            OPTIONAL
    PARBITER_BOOT_ALLOCATION BootAllocation;                    OPTIONAL
    PARBITER_QUERY_ARBITRATE QueryArbitrate;                    OPTIONAL
    PARBITER_QUERY_CONFLICT QueryConflict;                      OPTIONAL
    PARBITER_ADD_RESERVED AddReserved;                          OPTIONAL
    PARBITER_START_ARBITER StartArbiter;                        OPTIONAL
    //
    // Optional helper functions
    //
    PARBITER_PREPROCESS_ENTRY PreprocessEntry;                  OPTIONAL
    PARBITER_ALLOCATE_ENTRY AllocateEntry;                      OPTIONAL
    PARBITER_GET_NEXT_ALLOCATION_RANGE GetNextAllocationRange;  OPTIONAL
    PARBITER_FIND_SUITABLE_RANGE FindSuitableRange;             OPTIONAL
    PARBITER_ADD_ALLOCATION AddAllocation;                      OPTIONAL
    PARBITER_BACKTRACK_ALLOCATION BacktrackAllocation;          OPTIONAL
    PARBITER_OVERRIDE_CONFLICT OverrideConflict;                OPTIONAL

    //
    // Debugging support
    //
    BOOLEAN TransactionInProgress;

    //
    // Arbiter specific extension - can be used to store extra arbiter specific
    // information
    //
    PVOID Extension;

    //
    // The bus device we arbitrate for
    //
    PDEVICE_OBJECT BusDeviceObject;

    //
    // Callback and context for RtlFindRange/RtlIsRangeAvailable to allow
    // complex conflicts
    //
    PVOID ConflictCallbackContext;
    PRTL_CONFLICT_RANGE_CALLBACK ConflictCallback;

} ARBITER_INSTANCE, *PARBITER_INSTANCE;


//
// Lock primitives that leave us at PASSIVE_LEVEL after acquiring the lock.
// (A FAST_MUTEX or CriticalRegion leave us at APC level and some people (ACPI)
// need to be at passive level in their arbiter)
//

#define ArbAcquireArbiterLock(_Arbiter) \
    KeWaitForSingleObject( (_Arbiter)->MutexEvent, Executive, KernelMode, FALSE, NULL )

#define ArbReleaseArbiterLock(_Arbiter) \
    KeSetEvent( (_Arbiter)->MutexEvent, 0, FALSE )

//
// Iteration macros
//

//
// Control macro (used like a for loop) which iterates over all entries in
// a standard doubly linked list.  Head is the list head and the entries are of
// type Type.  A member called ListEntry is assumed to be the LIST_ENTRY
// structure linking the entries together.  Current contains a pointer to each
// entry in turn.
//
#define FOR_ALL_IN_LIST(Type, Head, Current)                            \
    for((Current) = CONTAINING_RECORD((Head)->Flink, Type, ListEntry);  \
       (Head) != &(Current)->ListEntry;                                 \
       (Current) = CONTAINING_RECORD((Current)->ListEntry.Flink,        \
                                     Type,                              \
                                     ListEntry)                         \
       )
//
// Similar to the above only iteration is over an array of length _Size.
//
#define FOR_ALL_IN_ARRAY(_Array, _Size, _Current)                       \
    for ( (_Current) = (_Array);                                        \
          (_Current) < (_Array) + (_Size);                              \
          (_Current)++ )

//
// As above only iteration begins with the entry _Current
//
#define FOR_REST_IN_ARRAY(_Array, _Size, _Current)                      \
    for ( ;                                                             \
          (_Current) < (_Array) + (_Size);                              \
          (_Current)++ )

//
// BOOLEAN
// INTERSECT(
//      ULONGLONG s1,
//      ULONGLONG e1,
//      ULONGLONG s2,
//      ULONGLONG e2
//  );
//
// Determines if the ranges s1-e1 and s2-e2 intersect
//
#define INTERSECT(s1,e1,s2,e2)                                          \
    !( ((s1) < (s2) && (e1) < (s2))                                     \
    ||((s2) < (s1) && (e2) < (s1)) )


//
// ULONGLONG
// INTERSECT_SIZE(
//      ULONGLONG s1,
//      ULONGLONG e1,
//      ULONGLONG s2,
//      ULONGLONG e2
//  );
//
// Returns the size of the intersection of s1-e1 and s2-e2, undefined if they
// don't intersect
//
#define INTERSECT_SIZE(s1,e1,s2,e2)                                     \
    ( __min((e1),(e2)) - __max((s1),(s2)) + 1)


#define LEGACY_REQUEST(_Entry)                                                \
    ((_Entry)->RequestSource == ArbiterRequestLegacyReported ||               \
        (_Entry)->RequestSource == ArbiterRequestLegacyAssigned)

#define PNP_REQUEST(_Entry)                                                   \
    ((_Entry)->RequestSource == ArbiterRequestPnpDetected ||                  \
        (_Entry)->RequestSource == ArbiterRequestPnpEnumerated)

//
// Priorities used in ArbGetNextAllocationRange
//

#define ARBITER_PRIORITY_NULL                 0
#define ARBITER_PRIORITY_PREFERRED_RESERVED   (MAXLONG-2)
#define ARBITER_PRIORITY_RESERVED             (MAXLONG-1)
#define ARBITER_PRIORITY_EXHAUSTED            (MAXLONG)


typedef
NTSTATUS
(*PARBITER_TRANSLATE_ALLOCATION_ORDER)(
    OUT PIO_RESOURCE_DESCRIPTOR TranslatedDescriptor,
    IN PIO_RESOURCE_DESCRIPTOR RawDescriptor
    );

//
// Common arbiter routines
//

NTSTATUS
ArbInitializeArbiterInstance(
    OUT PARBITER_INSTANCE Arbiter,
    IN PDEVICE_OBJECT BusDevice,
    IN CM_RESOURCE_TYPE ResourceType,
    IN PWSTR Name,
    IN PWSTR OrderingName,
    IN PARBITER_TRANSLATE_ALLOCATION_ORDER TranslateOrdering
    );

VOID
ArbDeleteArbiterInstance(
    IN PARBITER_INSTANCE Arbiter
    );

NTSTATUS
ArbArbiterHandler(
    IN PVOID Context,
    IN ARBITER_ACTION Action,
    IN OUT PARBITER_PARAMETERS Params
    );

NTSTATUS
ArbTestAllocation(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    );

NTSTATUS
ArbRetestAllocation(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    );

NTSTATUS
ArbCommitAllocation(
    PARBITER_INSTANCE Arbiter
    );

NTSTATUS
ArbRollbackAllocation(
    PARBITER_INSTANCE Arbiter
    );

NTSTATUS
ArbAddReserved(
    IN PARBITER_INSTANCE Arbiter,
    IN PIO_RESOURCE_DESCRIPTOR Requirement      OPTIONAL,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource OPTIONAL
    );

NTSTATUS
ArbPreprocessEntry(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    );

NTSTATUS
ArbAllocateEntry(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    );

NTSTATUS
ArbSortArbitrationList(
    IN OUT PLIST_ENTRY ArbitrationList
    );

VOID
ArbConfirmAllocation(
     IN PARBITER_INSTANCE Arbiter,
     IN PARBITER_ALLOCATION_STATE State
     );

BOOLEAN
ArbOverrideConflict(
     IN PARBITER_INSTANCE Arbiter,
     IN PARBITER_ALLOCATION_STATE State
     );


NTSTATUS
ArbQueryConflict(
     IN PARBITER_INSTANCE Arbiter,
     IN PDEVICE_OBJECT PhysicalDeviceObject,
     IN PIO_RESOURCE_DESCRIPTOR ConflictingResource,
     OUT PULONG ConflictCount,
     OUT PARBITER_CONFLICT_INFO *Conflicts
     );

VOID
ArbBacktrackAllocation(
     IN PARBITER_INSTANCE Arbiter,
     IN PARBITER_ALLOCATION_STATE State
     );

BOOLEAN
ArbGetNextAllocationRange(
    PARBITER_INSTANCE Arbiter,
    PARBITER_ALLOCATION_STATE State
    );

BOOLEAN
ArbFindSuitableRange(
    PARBITER_INSTANCE Arbiter,
    PARBITER_ALLOCATION_STATE State
    );

VOID
ArbAddAllocation(
     IN PARBITER_INSTANCE Arbiter,
     IN PARBITER_ALLOCATION_STATE State
     );

NTSTATUS
ArbBootAllocation(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    );

NTSTATUS
ArbStartArbiter(
    IN PARBITER_INSTANCE Arbiter,
    IN PCM_RESOURCE_LIST StartResources
    );

NTSTATUS
ArbBuildAssignmentOrdering(
    IN OUT PARBITER_INSTANCE Arbiter,
    IN PWSTR AllocationOrderName,
    IN PWSTR ReservedResourcesName,
    IN PARBITER_TRANSLATE_ALLOCATION_ORDER Translate OPTIONAL
    );


#if ARB_DBG

VOID
ArbpIndent(
    ULONG Count
    );

#endif // DBG


#endif


