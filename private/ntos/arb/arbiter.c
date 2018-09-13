/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    arbiter.c

Abstract:

    This module contains support routines for the Pnp resource arbiters.

Author:

    Andrew Thornton (andrewth) 1-April-1997


Environment:

    Kernel mode

Revision History:

--*/

#include "arbp.h"

//
// Conditional compilation constants
//

#define ALLOW_BOOT_ALLOC_CONFLICTS      1
#define PLUG_FEST_HACKS                 0

//
// Pool Tags
//

#define ARBITER_ALLOCATION_STATE_TAG    'AbrA'
#define ARBITER_ORDERING_LIST_TAG       'LbrA'
#define ARBITER_MISC_TAG                'MbrA'
#define ARBITER_RANGE_LIST_TAG          'RbrA'
#define ARBITER_CONFLICT_INFO_TAG       'CbrA'

//
// Constants
//

#define PATH_ARBITERS            L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Arbiters"
#define KEY_ALLOCATIONORDER      L"AllocationOrder"
#define KEY_RESERVEDRESOURCES    L"ReservedResources"
#define ARBITER_ORDERING_GROW_SIZE  8


//
// Macros
//

//
// PVOID
// FULL_INFO_DATA(
//    IN PKEY_VALUE_FULL_INFORMATION k
// );
//
// This macro returns the pointer to the beginning of the data area of a
// KEY_VALUE_FULL_INFORMATION structure.
//

#define FULL_INFO_DATA(k) ((PCHAR)(k) + (k)->DataOffset)

//
// BOOLEAN
// DISJOINT(
//      IN ULONGLONG s1,
//      IN ULONGLONG e1,
//      IN ULONGLONG s2,
//      IN ULONGLONG e2
//      );
//
#define DISJOINT(s1,e1,s2,e2)                                           \
    ( ((s1) < (s2) && (e1) < (s2))                                      \
    ||((s2) < (s1) && (e2) < (s1)) )

//
// VOID
// ArbpWstrToUnicodeString(
//      IN PUNICODE_STRING u,
//      IN PWSTR p
//      );
//

#define ArbpWstrToUnicodeString(u, p)                                   \
    (u)->Length = ((u)->MaximumLength =                                 \
        (USHORT) (sizeof((p))) - sizeof(WCHAR));                        \
    (u)->Buffer = (p)

//
// ULONG
// INDEX_FROM_PRIORITY(
//     LONG Priority
// );
//

#define ORDERING_INDEX_FROM_PRIORITY(P)                                 \
    ( (ULONG) ( (P) > 0 ? (P) - 1 : ((P) * -1) - 1) )

//
// Prototypes
//

NTSTATUS
ArbpBuildAllocationStack(
    IN PARBITER_INSTANCE Arbiter,
    IN PLIST_ENTRY ArbitrationList,
    IN ULONG ArbitrationListCount
    );

NTSTATUS
ArbpGetRegistryValue(
    IN HANDLE KeyHandle,
    IN PWSTR  ValueName,
    OUT PKEY_VALUE_FULL_INFORMATION *Information
    );

NTSTATUS
ArbpBuildAlternative(
    IN PARBITER_INSTANCE Arbiter,
    IN PIO_RESOURCE_DESCRIPTOR Requirement,
    OUT PARBITER_ALTERNATIVE Alternative
    );

VOID
ArbpUpdatePriority(
    PARBITER_INSTANCE Arbiter,
    PARBITER_ALTERNATIVE Alternative
    );

BOOLEAN
ArbpQueryConflictCallback(
    IN PVOID Context,
    IN PRTL_RANGE Range
    );

//
// Make everything pageable
//

#ifdef ALLOC_PRAGMA

#if ARB_DBG
#pragma alloc_text(PAGE, ArbpDumpArbiterInstance)
#pragma alloc_text(PAGE, ArbpDumpArbiterRange)
#pragma alloc_text(PAGE, ArbpDumpArbitrationList)
#endif

#pragma alloc_text(PAGE, ArbInitializeArbiterInstance)
#pragma alloc_text(PAGE, ArbDeleteArbiterInstance)
#pragma alloc_text(PAGE, ArbArbiterHandler)
#pragma alloc_text(PAGE, ArbStartArbiter)
#pragma alloc_text(PAGE, ArbTestAllocation)
#pragma alloc_text(PAGE, ArbRetestAllocation)
#pragma alloc_text(PAGE, ArbCommitAllocation)
#pragma alloc_text(PAGE, ArbRollbackAllocation)
#pragma alloc_text(PAGE, ArbAddReserved)
#pragma alloc_text(PAGE, ArbPreprocessEntry)
#pragma alloc_text(PAGE, ArbAllocateEntry)
#pragma alloc_text(PAGE, ArbSortArbitrationList)
#pragma alloc_text(PAGE, ArbBacktrackAllocation)
#pragma alloc_text(PAGE, ArbGetNextAllocationRange)
#pragma alloc_text(PAGE, ArbFindSuitableRange)
#pragma alloc_text(PAGE, ArbAddAllocation)
#pragma alloc_text(PAGE, ArbQueryConflict)
#pragma alloc_text(PAGE, ArbInitializeOrderingList)
#pragma alloc_text(PAGE, ArbCopyOrderingList)
#pragma alloc_text(PAGE, ArbPruneOrdering)
#pragma alloc_text(PAGE, ArbAddOrdering)
#pragma alloc_text(PAGE, ArbFreeOrderingList)
#pragma alloc_text(PAGE, ArbBuildAssignmentOrdering)

#pragma alloc_text(PAGE, ArbpGetRegistryValue)
#pragma alloc_text(PAGE, ArbpBuildAllocationStack)
#pragma alloc_text(PAGE, ArbpBuildAlternative)
#pragma alloc_text(PAGE, ArbpQueryConflictCallback)
#endif // ALLOC_PRAGMA

//
// Implementation
//


NTSTATUS
ArbInitializeArbiterInstance(
    OUT PARBITER_INSTANCE Arbiter,
    IN PDEVICE_OBJECT BusDeviceObject,
    IN CM_RESOURCE_TYPE ResourceType,
    IN PWSTR Name,
    IN PWSTR OrderingName,
    IN PARBITER_TRANSLATE_ALLOCATION_ORDER TranslateOrdering OPTIONAL
    )

/*++

Routine Description:

    This routine initializes an arbiter instance and fills in any optional NULL
    dispatch table entries with system default functions.

Parameters:

    Arbiter - A caller allocated arbiter instance structure.
        The UnpackRequirement, PackResource, UnpackResource and ScoreRequirement
        entries should be initialized with the appropriate routines as should
        any other entries if the default system routines are not sufficient.

    BusDeviceObject - The device object that exposes this arbiter - normally an
        FDO.

    ResourceType - The resource type this arbiter arbitrates.

    Name - A string used to identify the arbiter, used in debug messages and
        for registry storage

    OrderingName - The name of the preferred assignment ordering list under
        HKLM\System\CurrentControlSet\Control\SystemResources\AssignmentOrdering


    TranslateOrdering - Function that, if present, will be called to translate
        each descriptor from the ordering list

Return Value:

    Status code that indicates whether or not the function was successful.

Notes:


--*/

{
    NTSTATUS status;

    PAGED_CODE();

    ASSERT(Arbiter->UnpackRequirement);
    ASSERT(Arbiter->PackResource);
    ASSERT(Arbiter->UnpackResource);

    ARB_PRINT(2,("Initializing %S Arbiter...\n", Name));

    //
    // Initialize all pool allocation pointers to NULL so we can cleanup
    //

    ASSERT(Arbiter->MutexEvent == NULL
           && Arbiter->Allocation == NULL
           && Arbiter->PossibleAllocation == NULL
           && Arbiter->AllocationStack == NULL
           );

    //
    // We are an arbiter
    //

    Arbiter->Signature = ARBITER_INSTANCE_SIGNATURE;

    //
    // Remember the bus that produced us
    //

    Arbiter->BusDeviceObject = BusDeviceObject;

    //
    // Initialize state lock (KEVENT must be non-paged)
    //

    Arbiter->MutexEvent = ExAllocatePoolWithTag(NonPagedPool,
                                                sizeof(KEVENT),
                                                ARBITER_MISC_TAG
                                                );

    if (!Arbiter->MutexEvent) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    KeInitializeEvent(Arbiter->MutexEvent, SynchronizationEvent, TRUE);

    //
    // Initialize the allocation stack to a reasonable size
    //

    Arbiter->AllocationStack = ExAllocatePoolWithTag(PagedPool,
                                                     INITIAL_ALLOCATION_STATE_SIZE,
                                                     ARBITER_ALLOCATION_STATE_TAG
                                                     );

    if (!Arbiter->AllocationStack) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    Arbiter->AllocationStackMaxSize = INITIAL_ALLOCATION_STATE_SIZE;


    //
    // Allocate buffers to hold the range lists
    //

    Arbiter->Allocation = ExAllocatePoolWithTag(PagedPool,
                                                sizeof(RTL_RANGE_LIST),
                                                ARBITER_RANGE_LIST_TAG
                                                );

    if (!Arbiter->Allocation) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    Arbiter->PossibleAllocation = ExAllocatePoolWithTag(PagedPool,
                                                        sizeof(RTL_RANGE_LIST),
                                                        ARBITER_RANGE_LIST_TAG
                                                        );

    if (!Arbiter->PossibleAllocation) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    //
    // Initialize the range lists
    //

    RtlInitializeRangeList(Arbiter->Allocation);
    RtlInitializeRangeList(Arbiter->PossibleAllocation);

    //
    // Initialize the data fields
    //
    Arbiter->TransactionInProgress = FALSE;
    Arbiter->Name = Name;
    Arbiter->ResourceType = ResourceType;

    //
    // If the caller has not supplied the optional functions set them to the
    // defaults (If this were C++ we'd just inherit this loit but seeing as its
    // not we'll do it the old fashioned way...)
    //

    if (!Arbiter->TestAllocation) {
        Arbiter->TestAllocation = ArbTestAllocation;
    }

    if (!Arbiter->RetestAllocation) {
        Arbiter->RetestAllocation = ArbRetestAllocation;
    }

    if (!Arbiter->CommitAllocation) {
        Arbiter->CommitAllocation = ArbCommitAllocation;
    }

    if (!Arbiter->RollbackAllocation) {
        Arbiter->RollbackAllocation = ArbRollbackAllocation;
    }

    if (!Arbiter->AddReserved) {
        Arbiter->AddReserved = ArbAddReserved;
    }

    if (!Arbiter->PreprocessEntry) {
        Arbiter->PreprocessEntry = ArbPreprocessEntry;
    }

    if (!Arbiter->AllocateEntry) {
        Arbiter->AllocateEntry = ArbAllocateEntry;
    }

    if (!Arbiter->GetNextAllocationRange) {
        Arbiter->GetNextAllocationRange = ArbGetNextAllocationRange;
    }

    if (!Arbiter->FindSuitableRange) {
        Arbiter->FindSuitableRange = ArbFindSuitableRange;
    }

    if (!Arbiter->AddAllocation) {
        Arbiter->AddAllocation = ArbAddAllocation;
    }

    if (!Arbiter->BacktrackAllocation) {
        Arbiter->BacktrackAllocation = ArbBacktrackAllocation;
    }

    if (!Arbiter->OverrideConflict) {
        Arbiter->OverrideConflict = ArbOverrideConflict;
    }

    if (!Arbiter->BootAllocation) {
        Arbiter->BootAllocation = ArbBootAllocation;
    }

    if (!Arbiter->QueryConflict) {
        Arbiter->QueryConflict = ArbQueryConflict;
    }

    if (!Arbiter->StartArbiter) {
        Arbiter->StartArbiter = ArbStartArbiter;
    }

    //
    // Build the prefered assignment ordering - we assume that the reserved
    // ranges have the same name as the assignment ordering
    //

    status = ArbBuildAssignmentOrdering(Arbiter,
                                        OrderingName,
                                        OrderingName,
                                        TranslateOrdering
                                        );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    return STATUS_SUCCESS;

cleanup:

    if (Arbiter->MutexEvent) {
        ExFreePool(Arbiter->MutexEvent);
    }

    if (Arbiter->Allocation) {
        ExFreePool(Arbiter->Allocation);
    }

    if (Arbiter->PossibleAllocation) {
        ExFreePool(Arbiter->PossibleAllocation);
    }

    if (Arbiter->AllocationStack) {
        ExFreePool(Arbiter->AllocationStack);
    }

    return status;

}

VOID
ArbReferenceArbiterInstance(
    IN PARBITER_INSTANCE Arbiter
    )
{
    InterlockedIncrement(&Arbiter->ReferenceCount);
}

VOID
ArbDereferenceArbiterInstance(
    IN PARBITER_INSTANCE Arbiter
    )
{
    InterlockedDecrement(&Arbiter->ReferenceCount);

    if (Arbiter->ReferenceCount == 0) {
        ArbDeleteArbiterInstance(Arbiter);
    }
}

VOID
ArbDeleteArbiterInstance(
    IN PARBITER_INSTANCE Arbiter
    )
{

    PAGED_CODE();

    if (Arbiter->MutexEvent) {
        ExFreePool(Arbiter->MutexEvent);
    }

    if (Arbiter->Allocation) {
        RtlFreeRangeList(Arbiter->Allocation);
        ExFreePool(Arbiter->Allocation);
    }

    if (Arbiter->PossibleAllocation) {
        RtlFreeRangeList(Arbiter->PossibleAllocation);
        ExFreePool(Arbiter->PossibleAllocation);
    }

    if (Arbiter->AllocationStack) {
        ExFreePool(Arbiter->AllocationStack);
    }

    ArbFreeOrderingList(&Arbiter->OrderingList);
    ArbFreeOrderingList(&Arbiter->ReservedList);

#if ARB_DBG

    RtlFillMemory(Arbiter, sizeof(ARBITER_INSTANCE), 'A');

#endif

}

NTSTATUS
ArbTestAllocation(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    )

/*++

Routine Description:

    This is the default implementation of the arbiter Test Allocation action.
    It takes a list of requests for resources for particular devices and attempts
    to satisfy them.

Parameters:

    Arbiter - The instance of the arbiter being called.

    ArbitrationList - A list of ARBITER_LIST_ENTRY entries which contain the
        requirements and associated devices.

Return Value:

    Status code that indicates whether or not the function was successful.
    These include:

    STATUS_SUCCESSFUL - Arbitration suceeded and an allocation has been made for
        all the entries in the arbitration list.

    STATUS_UNSUCCESSFUL - Arbitration failed to find an allocation for all
        entries.

    STATUS_ARBITRATION_UNHANDLED - If returning this error the arbiter is
        partial (and therefore must have set the ARBITER_PARTIAL flag in its
        interface.)  This status indicates that this arbiter doesn't handle the
        resources requested and the next arbiter towards the root of the device
        tree should be asked instead.

--*/

{

    NTSTATUS status;
    PARBITER_LIST_ENTRY current;
    PIO_RESOURCE_DESCRIPTOR alternative;
    ULONG count;
    PDEVICE_OBJECT previousOwner;
    PDEVICE_OBJECT currentOwner;
    LONG score;
    BOOLEAN performScoring;

    PAGED_CODE();
    ASSERT(Arbiter);

    //
    // Copy the current allocation
    //

    ARB_PRINT(3, ("Copy current allocation\n"));
    status = RtlCopyRangeList(Arbiter->PossibleAllocation, Arbiter->Allocation);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Free all the resources currently allocated to all the devices we
    // are arbitrating for
    //

    count = 0;
    previousOwner = NULL;

    FOR_ALL_IN_LIST(ARBITER_LIST_ENTRY, ArbitrationList, current) {

        count++;

        currentOwner = current->PhysicalDeviceObject;

        if (previousOwner != currentOwner) {

            previousOwner = currentOwner;

            ARB_PRINT(3,
                        ("Delete 0x%08x's resources\n",
                        currentOwner
                        ));

            status = RtlDeleteOwnersRanges(Arbiter->PossibleAllocation,
                                           (PVOID)currentOwner
                                           );

            if (!NT_SUCCESS(status)) {
                goto cleanup;
            }
        }

        //
        // Score the entries in the arbitration list if a scoring function was
        // provided and this is not a legacy request (which is guaranteed to be
        // made of all fixed requests so sorting is pointless)
        //

        performScoring = Arbiter->ScoreRequirement != NULL;
        // BUGBUG - fix for rebalance being broken and not passing in flags...
        // && !LEGACY_REQUEST(current);
        current->WorkSpace = 0;

        if (performScoring) {

            FOR_ALL_IN_ARRAY(current->Alternatives,
                             current->AlternativeCount,
                             alternative) {

                ARB_PRINT(3,
                            ("Scoring entry %p\n",
                            currentOwner
                            ));



                score = Arbiter->ScoreRequirement(alternative);

                //
                // Ensure the score is valid
                //

                if (score < 0) {
                    status = STATUS_DEVICE_CONFIGURATION_ERROR;
                    goto cleanup;
                }

                current->WorkSpace += score;
            }
        }
    }

    status = ArbSortArbitrationList(ArbitrationList);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Build the arbitration stack
    //

    status = ArbpBuildAllocationStack(Arbiter,
                                     ArbitrationList,
                                     count
                                     );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Attempt allocation
    //

    status = Arbiter->AllocateEntry(Arbiter, Arbiter->AllocationStack);

    if (NT_SUCCESS(status)) {

        //
        // Success.
        //

        return status;
    }

cleanup:

    //
    // We didn't succeed so empty the possible allocation list...
    //

    RtlFreeRangeList(Arbiter->PossibleAllocation);

    return status;
}


NTSTATUS
ArbpBuildAlternative(
    IN PARBITER_INSTANCE Arbiter,
    IN PIO_RESOURCE_DESCRIPTOR Requirement,
    OUT PARBITER_ALTERNATIVE Alternative
    )

/*++

Routine Description:

    This routine initializes a arbiter alternative from a given resource
    requirement descriptor

Parameters:

    Arbiter - The arbiter instance data where the allocation stack should be
        placed.

    Requirement - The requirement descriptor describing this requirement

    Alternative - The alternative to be initialized

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{

    NTSTATUS status;

    PAGED_CODE();
    ASSERT(Alternative && Requirement);

    Alternative->Descriptor = Requirement;

    //
    // Unpack the requirement into the alternatives table
    //

    status = Arbiter->UnpackRequirement(Requirement,
                                        &Alternative->Minimum,
                                        &Alternative->Maximum,
                                        &Alternative->Length,
                                        &Alternative->Alignment
                                        );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Align the minimum if necessary
    //

    if (Alternative->Minimum % Alternative->Alignment != 0) {
        ALIGN_ADDRESS_UP(Alternative->Minimum,
                         Alternative->Alignment
                         );
    }

    Alternative->Flags = 0;

    //
    // Check if this alternative is shared
    //

    if(Requirement->ShareDisposition == CmResourceShareShared) {
        Alternative->Flags |= ARBITER_ALTERNATIVE_FLAG_SHARED;
    }

    //
    // Check if this alternative is fixed
    //

    if (Alternative->Maximum - Alternative->Minimum + 1 == Alternative->Length) {
        Alternative->Flags |= ARBITER_ALTERNATIVE_FLAG_FIXED;
    }

    //
    // Check for validity
    //

    if (Alternative->Maximum < Alternative->Minimum) {
        Alternative->Flags |= ARBITER_ALTERNATIVE_FLAG_INVALID;
    }

    return STATUS_SUCCESS;

cleanup:

    return status;
}


NTSTATUS
ArbpBuildAllocationStack(
    IN PARBITER_INSTANCE Arbiter,
    IN PLIST_ENTRY ArbitrationList,
    IN ULONG ArbitrationListCount
    )

/*++

Routine Description:

    This routine initializes the allocation stack for the requests in
    ArbitrationList.  It overwrites any previous allocation stack and allocates
    additional memory if more is required.  Arbiter->AllocationStack contains
    the initialized stack on success.

Parameters:

    Arbiter - The arbiter instance data where the allocation stack should be
        placed.

    ArbitrationList - A list of ARBITER_LIST_ENTRY entries which contain the
        requirements and associated devices.

    ArbitrationListCount - The number of entries in the ArbitrationList

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    PARBITER_LIST_ENTRY currentEntry;
    PARBITER_ALLOCATION_STATE currentState;
    ULONG stackSize = 0, allocationCount = ArbitrationListCount + 1;
    PARBITER_ALTERNATIVE currentAlternative;
    PIO_RESOURCE_DESCRIPTOR currentDescriptor;

    PAGED_CODE();

    //
    // Calculate the size the stack needs to be and the
    //

    FOR_ALL_IN_LIST(ARBITER_LIST_ENTRY, ArbitrationList, currentEntry) {

        if (currentEntry->AlternativeCount > 0) {
            stackSize += currentEntry->AlternativeCount
                            * sizeof(ARBITER_ALTERNATIVE);
        } else {
            allocationCount--;
        }
    }

    stackSize += allocationCount * sizeof(ARBITER_ALLOCATION_STATE);

    //
    // Make sure the allocation stack is large enough
    //

    if (Arbiter->AllocationStackMaxSize < stackSize) {

        PARBITER_ALLOCATION_STATE temp;

        //
        // Enlarge the allocation stack
        //

        temp = ExAllocatePoolWithTag(PagedPool,
                                     stackSize,
                                     ARBITER_ALLOCATION_STATE_TAG
                                     );
        if (!temp) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ExFreePool(Arbiter->AllocationStack);
        Arbiter->AllocationStack = temp;
    }

    RtlZeroMemory(Arbiter->AllocationStack, stackSize);

    //
    // Fill in the locations
    //

    currentState = Arbiter->AllocationStack;
    currentAlternative = (PARBITER_ALTERNATIVE) (Arbiter->AllocationStack
        + ArbitrationListCount + 1);

    FOR_ALL_IN_LIST(ARBITER_LIST_ENTRY, ArbitrationList, currentEntry) {

        //
        // Do we need to allocate anything for this entry?
        //

        if (currentEntry->AlternativeCount > 0) {

            //
            // Initialize the stack location
            //

            currentState->Entry = currentEntry;
            currentState->AlternativeCount = currentEntry->AlternativeCount;
            currentState->Alternatives = currentAlternative;

            //
            // Initialize the start and end values to an invalid range so
            // that we don't skip the range 0-0 every time...
            //

            currentState->Start = 1;
            ASSERT(currentState->End == 0);  // From RtlZeroMemory

            //
            // Initialize the alternatives table
            //

            FOR_ALL_IN_ARRAY(currentEntry->Alternatives,
                             currentEntry->AlternativeCount,
                             currentDescriptor) {


                status = ArbpBuildAlternative(Arbiter,
                                            currentDescriptor,
                                            currentAlternative
                                            );

                if (!NT_SUCCESS(status)) {
                    goto cleanup;
                }

                //
                // Initialize the priority
                //

                currentAlternative->Priority = ARBITER_PRIORITY_NULL;

                //
                // Advance to the next alternative
                //

                currentAlternative++;

            }
        }
        currentState++;
    }

    //
    // Terminate the stack with NULL entry
    //

    currentState->Entry = NULL;

    return STATUS_SUCCESS;

cleanup:

    //
    // We don't need to free the buffer as it is attached to the arbiter and
    // will be used next time
    //

    return status;
}

NTSTATUS
ArbSortArbitrationList(
    IN OUT PLIST_ENTRY ArbitrationList
    )

/*++

Routine Description:

    This routine sorts the arbitration list in order of each entry's
    WorkSpace value.

Parameters:

    ArbitrationList - The list to be sorted.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    //
    // BUGBUG - maybe should use a get next type thing or a better sort!
    //
    BOOLEAN sorted = FALSE;
    PARBITER_LIST_ENTRY current, next;

    PAGED_CODE();

    ARB_PRINT(3, ("IoSortArbiterList(%p)\n", ArbitrationList));

    while (!sorted) {

        sorted = TRUE;

        for (current=(PARBITER_LIST_ENTRY) ArbitrationList->Flink,
               next=(PARBITER_LIST_ENTRY) current->ListEntry.Flink;

            (PLIST_ENTRY) current != ArbitrationList
               && (PLIST_ENTRY) next != ArbitrationList;

            current = (PARBITER_LIST_ENTRY) current->ListEntry.Flink,
                next = (PARBITER_LIST_ENTRY)current->ListEntry.Flink) {


            if (current->WorkSpace > next->WorkSpace) {

                PLIST_ENTRY before = current->ListEntry.Blink;
                PLIST_ENTRY after = next->ListEntry.Flink;

                //
                // Swap the locations of current and next
                //

                before->Flink = (PLIST_ENTRY) next;
                after->Blink = (PLIST_ENTRY) current;
                current->ListEntry.Flink = after;
                current->ListEntry.Blink = (PLIST_ENTRY) next;
                next->ListEntry.Flink = (PLIST_ENTRY) current;
                next->ListEntry.Blink = before;

                sorted = FALSE;
            }
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
ArbCommitAllocation(
    PARBITER_INSTANCE Arbiter
    )

/*++

Routine Description:

    This provides the default implementation of the CommitAllocation action.
    It frees the old allocation and replaces it with the new allocation.

Parameters:

    Arbiter - The arbiter instance data for the arbiter being called.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PRTL_RANGE_LIST temp;

    PAGED_CODE();

    //
    // Free up the current allocation
    //

    RtlFreeRangeList(Arbiter->Allocation);

    //
    // Swap the allocated and duplicate lists
    //

    temp = Arbiter->Allocation;
    Arbiter->Allocation = Arbiter->PossibleAllocation;
    Arbiter->PossibleAllocation = temp;

    return STATUS_SUCCESS;
}

NTSTATUS
ArbRollbackAllocation(
    IN PARBITER_INSTANCE Arbiter
    )

/*++

Routine Description:

    This provides the default implementation of the RollbackAllocation action.
    It frees the possible allocation the last TestAllocation provided.

Parameters:

    Arbiter - The arbiter instance data for the arbiter being called.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{

    PAGED_CODE();

    //
    // Free up the possible allocation
    //

    RtlFreeRangeList(Arbiter->PossibleAllocation);

    return STATUS_SUCCESS;
}

NTSTATUS
ArbRetestAllocation(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    )

/*++

Routine Description:

    This provides the default implementation of the RetestAllocation action.
    It walks the arbitration list and updates the possible allocation to reflect
    the allocation entries of the list.  For these entries to be valid
    TestAllocation must have been performed on this arbitration list.

Parameters:

    Arbiter - The arbiter instance data for the arbiter being called.

    ArbitrationList - A list of ARBITER_LIST_ENTRY entries which contain the
        requirements and associated devices.  TestAllocation for this arbiter
        should have been called on this list.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    PARBITER_LIST_ENTRY current;
    ARBITER_ALLOCATION_STATE state;
    ARBITER_ALTERNATIVE alternative;
    ULONG length;

    PAGED_CODE();

    //
    // Initialize the state
    //

    RtlZeroMemory(&state, sizeof(ARBITER_ALLOCATION_STATE));
    RtlZeroMemory(&alternative, sizeof(ARBITER_ALTERNATIVE));
    state.AlternativeCount = 1;
    state.Alternatives = &alternative;
    state.CurrentAlternative = &alternative;
    state.Flags = ARBITER_STATE_FLAG_RETEST;

    //
    // Copy the current allocation and reserved
    //

    ARB_PRINT(2, ("Retest: Copy current allocation\n"));
    status = RtlCopyRangeList(Arbiter->PossibleAllocation, Arbiter->Allocation);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Free all the resources currently allocated to all the devices we
    // are arbitrating for
    //

    FOR_ALL_IN_LIST(ARBITER_LIST_ENTRY, ArbitrationList, current) {

        ARB_PRINT(3,
                    ("Retest: Delete 0x%08x's resources\n",
                    current->PhysicalDeviceObject
                    ));

        status = RtlDeleteOwnersRanges(Arbiter->PossibleAllocation,
                                       (PVOID) current->PhysicalDeviceObject
                                       );

        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }
    }

    //
    // Build an allocation state for the allocation and call AddAllocation to
    // update the range lists accordingly
    //

    FOR_ALL_IN_LIST(ARBITER_LIST_ENTRY, ArbitrationList, current) {

        ASSERT(current->Assignment && current->SelectedAlternative);

        state.WorkSpace = 0;
        state.Entry = current;

        //
        // Initialize the alternative
        //

        status = ArbpBuildAlternative(Arbiter,
                                    current->SelectedAlternative,
                                    &alternative
                                    );

        ASSERT(NT_SUCCESS(status));

        //
        // Update it with our allocation
        //

        status = Arbiter->UnpackResource(current->Assignment,
                                         &state.Start,
                                         &length
                                         );

        ASSERT(NT_SUCCESS(status));

        state.End = state.Start + length - 1;

        //
        // Do any preprocessing that is required
        //

        status = Arbiter->PreprocessEntry(Arbiter,&state);

        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }

        //
        // If we had a requirement for length 0 then don't attemp to add the
        // range - it will fail!
        //

        if (length != 0) {

            Arbiter->AddAllocation(Arbiter, &state);

        }
    }

    return status;

cleanup:

    RtlFreeRangeList(Arbiter->PossibleAllocation);
    return status;
}

NTSTATUS
ArbBootAllocation(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    )
/*++

Routine Description:

    This provides the default implementation of the BootAllocation action.
    It walks the arbitration list and updates the allocation to reflect the fact
    that the allocation entries in the list are in use.

Parameters:

    Arbiter - The arbiter instance data for the arbiter being called.

    ArbitrationList - A list of ARBITER_LIST_ENTRY entries which contain the
        requirements and associated devices.  Each device should have one and
        only one requirement reflecting the resources it is currently consuming.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{

    NTSTATUS status;
    PARBITER_LIST_ENTRY current;
    PRTL_RANGE_LIST temp;
    ARBITER_ALLOCATION_STATE state;
    ARBITER_ALTERNATIVE alternative;

    //
    // Initialize the state
    //

    RtlZeroMemory(&state, sizeof(ARBITER_ALLOCATION_STATE));
    RtlZeroMemory(&alternative, sizeof(ARBITER_ALTERNATIVE));
    state.AlternativeCount = 1;
    state.Alternatives = &alternative;
    state.CurrentAlternative = &alternative;
    state.Flags = ARBITER_STATE_FLAG_BOOT;
    state.RangeAttributes = ARBITER_RANGE_BOOT_ALLOCATED;

    //
    // Work on the possible allocation list
    //

    status = RtlCopyRangeList(Arbiter->PossibleAllocation, Arbiter->Allocation);

    FOR_ALL_IN_LIST(ARBITER_LIST_ENTRY, ArbitrationList, current) {

        ASSERT(current->AlternativeCount == 1);
        ASSERT(current->PhysicalDeviceObject);

        //
        // Build an alternative and state structure for this allocation and
        // add it to the range list
        //

        state.Entry = current;

        //
        // Initialize the alternative
        //

        status = ArbpBuildAlternative(Arbiter,
                                    &current->Alternatives[0],
                                    &alternative
                                    );

        ASSERT(NT_SUCCESS(status));
        ASSERT(alternative.Flags &
               (ARBITER_ALTERNATIVE_FLAG_FIXED | ARBITER_ALTERNATIVE_FLAG_INVALID)
               );

        state.Start = alternative.Minimum;
        state.End = alternative.Maximum;

        //
        // Blow away the old workspace and masks
        //

        state.WorkSpace = 0;
        state.RangeAvailableAttributes = 0;

        //
        // Validate the requirement
        //

        if (alternative.Length == 0
        || alternative.Alignment == 0
        || state.End < state.Start
        || state.Start % alternative.Alignment != 0
        || LENGTH_OF(state.Start, state.End) != alternative.Length) {

            ARB_PRINT(1,
                        ("Skipping invalid boot allocation 0x%I64x-0x%I64x L 0x%x A 0x%x for 0x%08x\n",
                         state.Start,
                         state.End,
                         alternative.Length,
                         alternative.Alignment,
                         current->PhysicalDeviceObject
                         ));

            continue;
        }

#if PLUG_FEST_HACKS

        if (alternative.Flags & ARBITER_ALTERNATIVE_FLAG_SHARED) {

            ARB_PRINT(1,
                         ("Skipping shared boot allocation 0x%I64x-0x%I64x L 0x%x A 0x%x for 0x%08x\n",
                          state.Start,
                          state.End,
                          alternative.Length,
                          alternative.Alignment,
                          current->PhysicalDeviceObject
                          ));

            continue;
        }
#endif


        //
        // Do any preprocessing that is required
        //

        status = Arbiter->PreprocessEntry(Arbiter,&state);

        if (!NT_SUCCESS(status)) {
            goto cleanup;;
        }

        Arbiter->AddAllocation(Arbiter, &state);

    }

    //
    // Everything went OK so make this our allocated range
    //

    RtlFreeRangeList(Arbiter->Allocation);
    temp = Arbiter->Allocation;
    Arbiter->Allocation = Arbiter->PossibleAllocation;
    Arbiter->PossibleAllocation = temp;

    return STATUS_SUCCESS;

cleanup:

    RtlFreeRangeList(Arbiter->PossibleAllocation);
    return status;

}


NTSTATUS
ArbArbiterHandler(
    IN PVOID Context,
    IN ARBITER_ACTION Action,
    IN OUT PARBITER_PARAMETERS Params
    )

/*++

Routine Description:

    This provides the default entry point to an arbiter.

Parameters:

    Context - The context provided in the interface where this function was
        called from.  This is converted to an ARBITER_INSTANCE using the
        ARBITER_CONTEXT_TO_INSTANCE macro which should be defined.

    Action - The action the arbiter should perform.

    Params - The parameters for the action.

Return Value:

    Status code that indicates whether or not the function was successful.

Note:

    The routines which implement each action are determined from the dispatch
    table in the arbiter instance.

--*/

{

    NTSTATUS status;
    PARBITER_INSTANCE arbiter = Context;

    PAGED_CODE();
    ASSERT(Context);
    ASSERT(Action >= 0 && Action <= ArbiterActionBootAllocation);
    ASSERT(arbiter->Signature == ARBITER_INSTANCE_SIGNATURE);

    //
    // Acquire the state lock
    //

    ArbAcquireArbiterLock(arbiter);

    //
    // Announce ourselves
    //

    ARB_PRINT(2,
                ("%s %S\n",
                ArbpActionStrings[Action],
                arbiter->Name
                ));

    //
    // Check the transaction flag
    //

    if (Action == ArbiterActionTestAllocation
    ||  Action == ArbiterActionRetestAllocation
    ||  Action == ArbiterActionBootAllocation) {

        ASSERT(!arbiter->TransactionInProgress);

    } else if (Action == ArbiterActionCommitAllocation
           ||  Action == ArbiterActionRollbackAllocation) {

        ASSERT(arbiter->TransactionInProgress);
    }

#if ARB_DBG

replay:

#endif

    //
    // Do the appropriate thing
    //

    switch (Action) {

    case ArbiterActionTestAllocation:

        //
        // Suballocation can not occur for a root arbiter
        // BUGBUG - this should be generalised
        //
        ASSERT(Params->Parameters.TestAllocation.AllocateFromCount == 0);
        ASSERT(Params->Parameters.TestAllocation.AllocateFrom == NULL);

        status = arbiter->TestAllocation(
                     arbiter,
                     Params->Parameters.TestAllocation.ArbitrationList
                     );
        break;

    case ArbiterActionRetestAllocation:

        //
        // Suballocation can not occur for a root arbiter
        // BUGBUG - this should be generalised
        //
        ASSERT(Params->Parameters.TestAllocation.AllocateFromCount == 0);
        ASSERT(Params->Parameters.TestAllocation.AllocateFrom == NULL);

        status = arbiter->RetestAllocation(
                     arbiter,
                     Params->Parameters.TestAllocation.ArbitrationList
                     );
        break;

    case ArbiterActionCommitAllocation:

        status = arbiter->CommitAllocation(arbiter);

        break;

    case ArbiterActionRollbackAllocation:

        status = arbiter->RollbackAllocation(arbiter);

        break;

    case ArbiterActionBootAllocation:

        status = arbiter->BootAllocation(
                    arbiter,
                    Params->Parameters.BootAllocation.ArbitrationList
                    );
        break;

    case ArbiterActionQueryConflict:

        status = arbiter->QueryConflict(
                    arbiter,
                    Params->Parameters.QueryConflict.PhysicalDeviceObject,
                    Params->Parameters.QueryConflict.ConflictingResource,
                    Params->Parameters.QueryConflict.ConflictCount,
                    Params->Parameters.QueryConflict.Conflicts
                    );
        break;

    case ArbiterActionQueryArbitrate:
    case ArbiterActionQueryAllocatedResources:
    case ArbiterActionWriteReservedResources:
    case ArbiterActionAddReserved:

        status = STATUS_NOT_IMPLEMENTED;
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

#if ARB_DBG

    //
    // Check if we failed and want to stop or replay on errors
    //

    if (!NT_SUCCESS(status)) {

        ARB_PRINT(1,
                 ("*** %s for %S FAILED status = %08x\n",
                  ArbpActionStrings[Action],
                  arbiter->Name,
                  status
                 ));

        if (ArbStopOnError) {
            DbgBreakPoint();
        }

        if (ArbReplayOnError) {
            goto replay;
        }
    }

#endif // ARB_DBG

    if (NT_SUCCESS(status)) {

        if (Action == ArbiterActionTestAllocation
        ||  Action == ArbiterActionRetestAllocation) {

            arbiter->TransactionInProgress = TRUE;

        } else if (Action == ArbiterActionCommitAllocation
               ||  Action == ArbiterActionRollbackAllocation) {

            arbiter->TransactionInProgress = FALSE;
        }
    }

    ArbReleaseArbiterLock(arbiter);

    return status;

}

NTSTATUS
ArbBuildAssignmentOrdering(
    IN OUT PARBITER_INSTANCE Arbiter,
    IN PWSTR AllocationOrderName,
    IN PWSTR ReservedResourcesName,
    IN PARBITER_TRANSLATE_ALLOCATION_ORDER Translate OPTIONAL
    )

/*++

Routine Description:

    This is called as part of arbiter initialization and extracts the allocation
    ordering and reserved information from the registry and combines them into
    an ordering list.  The reserved ranges are put in Arbiter->ReservedList
    and the initial ordering in Arbiter->OrderingList.

Parameters:

    Arbiter - The instance data of the arbiter to be initialized.

    AllocationOrderName - The name of the key under HKLM\System\
        CurrentControlSet\Control\Arbiters\AllocationOrder the ordering
        information should be taken from.

    ReservedResourcesName - The name of the key under HKLM\System\
        CurrentControlSet\Control\Arbiters\ReservedResources the reserved ranges
        information should be taken from.

    Translate - A function to be called for each range that will perform system
        dependant translations required for this system.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    HANDLE arbitersHandle = NULL, tempHandle = NULL;
    UNICODE_STRING unicodeString;
    PKEY_VALUE_FULL_INFORMATION info = NULL;
    ULONG dummy;
    PIO_RESOURCE_LIST resourceList;
    PIO_RESOURCE_DESCRIPTOR current;
    ULONGLONG start, end;
    OBJECT_ATTRIBUTES attributes;
    IO_RESOURCE_DESCRIPTOR translated;

    PAGED_CODE();

    ArbAcquireArbiterLock(Arbiter);

    //
    // If we are reinitializing the orderings free the old ones
    //

    ArbFreeOrderingList(&Arbiter->OrderingList);
    ArbFreeOrderingList(&Arbiter->ReservedList);

    //
    // Initialize the orderings
    //

    status = ArbInitializeOrderingList(&Arbiter->OrderingList);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    status = ArbInitializeOrderingList(&Arbiter->ReservedList);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Open HKLM\System\CurrentControlSet\Control\Arbiters
    //

    ArbpWstrToUnicodeString(&unicodeString, PATH_ARBITERS);
    InitializeObjectAttributes(&attributes,
                               &unicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               (PSECURITY_DESCRIPTOR) NULL
                               );


    status = ZwOpenKey(&arbitersHandle,
                       KEY_READ,
                       &attributes
                       );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Open AllocationOrder
    //

    ArbpWstrToUnicodeString(&unicodeString, KEY_ALLOCATIONORDER);
    InitializeObjectAttributes(&attributes,
                               &unicodeString,
                               OBJ_CASE_INSENSITIVE,
                               arbitersHandle,
                               (PSECURITY_DESCRIPTOR) NULL
                               );


    status = ZwOpenKey(&tempHandle,
                       KEY_READ,
                       &attributes
                       );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Extract the value the user asked for
    //

    status = ArbpGetRegistryValue(tempHandle,
                                  AllocationOrderName,
                                  &info
                                  );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Check if the value we retrieved was a string and if so then it was a
    // short cut to a value of that name - open it.
    //

    if (info->Type == REG_SZ) {

        PKEY_VALUE_FULL_INFORMATION tempInfo;

        // BUGBUG - check this is a valid string...

        status = ArbpGetRegistryValue(tempHandle,
                                     (PWSTR) FULL_INFO_DATA(info),
                                     &tempInfo
                                     );

        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }

        ExFreePool(info);
        info = tempInfo;

    }

    ZwClose(tempHandle);

    //
    // We only support one level of short cuts so this should be a
    // REG_RESOURCE_REQUIREMENTS_LIST
    //

    if (info->Type != REG_RESOURCE_REQUIREMENTS_LIST) {
        status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Extract the resource list
    //

    ASSERT(((PIO_RESOURCE_REQUIREMENTS_LIST) FULL_INFO_DATA(info))
             ->AlternativeLists == 1);

    resourceList = (PIO_RESOURCE_LIST) &((PIO_RESOURCE_REQUIREMENTS_LIST)
                       FULL_INFO_DATA(info))->List[0];

    //
    // Convert the resource list into an ordering list
    //

    FOR_ALL_IN_ARRAY(resourceList->Descriptors,
                     resourceList->Count,
                     current) {

        //
        // Perform any translation that is necessary on the resources
        //

        if (ARGUMENT_PRESENT(Translate)) {

            status = (Translate)(&translated, current);

            if (!NT_SUCCESS(status)) {
                goto cleanup;
            }
        } else {
            translated = *current;
        }

        if (translated.Type == Arbiter->ResourceType) {

            status = Arbiter->UnpackRequirement(&translated,
                                                &start,
                                                &end,
                                                &dummy,  //length
                                                &dummy   //alignment
                                               );

            if (!NT_SUCCESS(status)) {
                goto cleanup;
            }

            status = ArbAddOrdering(&Arbiter->OrderingList,
                                    start,
                                    end
                                    );

            if (!NT_SUCCESS(status)) {
                    goto cleanup;
            }
        }
    }

    //
    // We're finished with info...
    //

    ExFreePool(info);
    info = NULL;

    //
    // Open ReservedResources
    //

    ArbpWstrToUnicodeString(&unicodeString, KEY_RESERVEDRESOURCES);
    InitializeObjectAttributes(&attributes,
                               &unicodeString,
                               OBJ_CASE_INSENSITIVE,
                               arbitersHandle,
                               (PSECURITY_DESCRIPTOR) NULL
                               );


    status = ZwCreateKey(&tempHandle,
                         KEY_READ,
                         &attributes,
                         0,
                         (PUNICODE_STRING) NULL,
                         REG_OPTION_NON_VOLATILE,
                         NULL
                         );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Extract the arbiter's reserved resources
    //

    status = ArbpGetRegistryValue(tempHandle,
                                  ReservedResourcesName,
                                  &info
                                  );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Check if the value we retrieved was a string and if so then it was a
    // short cut to a value of that name - open it.
    //

    if (info->Type == REG_SZ) {

        PKEY_VALUE_FULL_INFORMATION tempInfo;

        // BUGBUG - check this is a valid string...

        status = ArbpGetRegistryValue(tempHandle,
                                     (PWSTR) FULL_INFO_DATA(info),
                                     &tempInfo
                                     );

        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }

        ExFreePool(info);
        info = tempInfo;

    }

    ZwClose(tempHandle);

    if (NT_SUCCESS(status)) {

        ASSERT(((PIO_RESOURCE_REQUIREMENTS_LIST) FULL_INFO_DATA(info))
             ->AlternativeLists == 1);

        resourceList = (PIO_RESOURCE_LIST) &((PIO_RESOURCE_REQUIREMENTS_LIST)
                       FULL_INFO_DATA(info))->List[0];

        //
        // Apply the reserved ranges to the ordering
        //

        FOR_ALL_IN_ARRAY(resourceList->Descriptors,
                         resourceList->Count,
                         current) {

            //
            // Perform any translation that is necessary on the resources
            //

            if (ARGUMENT_PRESENT(Translate)) {

                status = (Translate)(&translated, current);

                if (!NT_SUCCESS(status)) {
                    goto cleanup;
                }
            } else {
                translated = *current;
            }

            if (translated.Type == Arbiter->ResourceType) {

                status = Arbiter->UnpackRequirement(&translated,
                                                    &start,
                                                    &end,
                                                    &dummy,  //length
                                                    &dummy   //alignment
                                                   );

                if (!NT_SUCCESS(status)) {
                    goto cleanup;
                }

                //
                // Add the reserved range to the reserved ordering
                //

                status = ArbAddOrdering(&Arbiter->ReservedList, start, end);

                if (!NT_SUCCESS(status)) {
                    goto cleanup;
                }

                //
                // Prune the reserved range from the current ordering
                //

                status = ArbPruneOrdering(&Arbiter->OrderingList, start, end);

                if (!NT_SUCCESS(status)) {
                    goto cleanup;
                }

            }
        }

        ExFreePool(info);
    }

    //
    // All done!
    //

    ZwClose(arbitersHandle);

#if ARB_DBG

    {
        PARBITER_ORDERING current;

        FOR_ALL_IN_ARRAY(Arbiter->OrderingList.Orderings,
                         Arbiter->OrderingList.Count,
                         current) {
            ARB_PRINT(2,
                        ("Ordering: 0x%I64x-0x%I64x\n",
                         current->Start,
                         current->End
                        ));
        }

        ARB_PRINT(2, ("\n"));

        FOR_ALL_IN_ARRAY(Arbiter->ReservedList.Orderings,
                     Arbiter->ReservedList.Count,
                     current) {
            ARB_PRINT(2,
                        ("Reserved: 0x%I64x-0x%I64x\n",
                         current->Start,
                         current->End
                        ));
        }

    }

#endif

    ArbReleaseArbiterLock(Arbiter);

    return STATUS_SUCCESS;

cleanup:

    if (arbitersHandle) {
        ZwClose(arbitersHandle);
    }

    if (tempHandle) {
        ZwClose(tempHandle);
    }

    if (info) {
        ExFreePool(info);
    }

    if (Arbiter->OrderingList.Orderings) {
        ExFreePool(Arbiter->OrderingList.Orderings);
        Arbiter->OrderingList.Count = 0;
        Arbiter->OrderingList.Maximum = 0;
    }

    if (Arbiter->ReservedList.Orderings) {
        ExFreePool(Arbiter->ReservedList.Orderings);
        Arbiter->ReservedList.Count = 0;
        Arbiter->ReservedList.Maximum = 0;
    }

    ArbReleaseArbiterLock(Arbiter);

    return status;
}

BOOLEAN
ArbFindSuitableRange(
    PARBITER_INSTANCE Arbiter,
    PARBITER_ALLOCATION_STATE State
    )

/*++

Routine Description:

    This routine is called from AllocateEntry once we have decided where we want
    to allocate from.  It tries to find a free range that matches the
    requirements in State while restricting its possible solutions to the range
    State->CurrentMinimum to State->CurrentMaximum.  On success State->Start and
    State->End represent this range.

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

Return Value:

    TRUE if we found a range, FALSE otherwise.

--*/

{

    NTSTATUS status;
    ULONG findRangeFlags = 0;

    PAGED_CODE();

    ASSERT(State->CurrentAlternative);

    //
    // Catch the case where we backtrack and advance past the maximum
    //

    if (State->CurrentMinimum > State->CurrentMaximum) {
        return FALSE;
    }

    //
    // If we are asking for zero ports then trivially succeed with the minimum
    // value and remember that backtracking this is a recipe for infinite loops
    //

    if (State->CurrentAlternative->Length == 0) {
        State->End = State->Start = State->CurrentMinimum;
        return TRUE;
    }

    //
    // For legacy requests from IoAssignResources (directly or by way of
    // HalAssignSlotResources) or IoReportResourceUsage we consider preallocated
    // resources to be available for backward compatibility reasons.
    //
    // If we are allocating a devices boot config then we consider all other
    // boot configs to be available.  BUGBUG(andrewth) - this behavior is bad!
    //

    if (State->Entry->RequestSource == ArbiterRequestLegacyReported
        || State->Entry->RequestSource == ArbiterRequestLegacyAssigned) {

        State->RangeAvailableAttributes |= ARBITER_RANGE_BOOT_ALLOCATED;
    }

    //
    // Check if null conflicts are OK...
    //

    if (State->Flags & ARBITER_STATE_FLAG_NULL_CONFLICT_OK) {
        findRangeFlags |= RTL_RANGE_LIST_NULL_CONFLICT_OK;
    }

    //
    // ...or we are shareable...
    //

    if (State->CurrentAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_SHARED) {
        findRangeFlags |= RTL_RANGE_LIST_SHARED_OK;
    }

    //
    // Select the first free alternative from the current alternative
    //

    status = RtlFindRange(
                 Arbiter->PossibleAllocation,
                 State->CurrentMinimum,
                 State->CurrentMaximum,
                 State->CurrentAlternative->Length,
                 State->CurrentAlternative->Alignment,
                 findRangeFlags,
                 State->RangeAvailableAttributes,
                 Arbiter->ConflictCallbackContext,
                 Arbiter->ConflictCallback,
                 &State->Start
                 );


    if (NT_SUCCESS(status)) {

        //
        // We found a suitable range
        //
        State->End = State->Start + State->CurrentAlternative->Length - 1;

        return TRUE;

    } else {

        //
        // We couldn't find any range so check if we will allow this conflict
        // - if so don'd fail!
        //

        return Arbiter->OverrideConflict(Arbiter, State);
    }
}

VOID
ArbAddAllocation(
     IN PARBITER_INSTANCE Arbiter,
     IN PARBITER_ALLOCATION_STATE State
     )

/*++

Routine Description:

    This routine is called from AllocateEntry once we have found a possible
    solution (State->Start - State->End).  It adds the ranges that will not be
    available if we commit to this solution to Arbiter->PossibleAllocation.

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

Return Value:

    None.

--*/

{

    NTSTATUS status;

    PAGED_CODE();

    status = RtlAddRange(
                 Arbiter->PossibleAllocation,
                 State->Start,
                 State->End,
                 State->RangeAttributes,
                 RTL_RANGE_LIST_ADD_IF_CONFLICT +
                    (State->CurrentAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_SHARED
                        ? RTL_RANGE_LIST_ADD_SHARED : 0),
                 NULL,
                 State->Entry->PhysicalDeviceObject
                 );

    ASSERT(NT_SUCCESS(status));

}


VOID
ArbBacktrackAllocation(
     IN PARBITER_INSTANCE Arbiter,
     IN PARBITER_ALLOCATION_STATE State
     )

/*++

Routine Description:

    This routine is called from AllocateEntry if the possible solution
    (State->Start - State->End) does not allow us to allocate resources to
    the rest of the devices being considered.  It deletes the ranges that were
    added to Arbiter->PossibleAllocation by AddAllocation.

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

Return Value:

    None.

--*/


{
    NTSTATUS status;

    PAGED_CODE();

    //
    // We couldn't allocate for the rest of the ranges then
    // backtrack
    //

    status = RtlDeleteRange(
                 Arbiter->PossibleAllocation,
                 State->Start,
                 State->End,
                 State->Entry->PhysicalDeviceObject
                 );

    ASSERT(NT_SUCCESS(status));

    ARB_PRINT(2,
                ("\t\tBacktracking on 0x%I64x-0x%I64x for %p\n",
                State->Start,
                State->End,
                State->Entry->PhysicalDeviceObject
                ));

}


NTSTATUS
ArbPreprocessEntry(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    )
/*++

Routine Description:

    This routine is called from AllocateEntry to allow preprocessing of
    entries

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

Return Value:

    None.

--*/
{

    PAGED_CODE();

    return STATUS_SUCCESS;
}

NTSTATUS
ArbAllocateEntry(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    )
/*++

Routine Description:

    This is the core arbitration routine and is called from TestAllocation
    to allocate resources for all of the entries in the allocation stack.
    It calls off to various helper routines (described above) to perform this
    task.

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

Return Value:

    None.

--*/



{

    NTSTATUS status;
    PARBITER_ALLOCATION_STATE currentState = State;
    BOOLEAN backtracking = FALSE;

    PAGED_CODE();

    //
    // Have we reached the end of the list?  If so then we have a working
    // allocation.
    //

tryAllocation:

    while(currentState >= State && currentState->Entry != NULL) {

        //
        // Do any preprocessing that is required
        //

        status = Arbiter->PreprocessEntry(Arbiter,currentState);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // If we need to backtrack do so!
        //

        if (backtracking) {

            ULONGLONG possibleCurrentMinimum;

            backtracking = FALSE;

            //
            // Clear the CurrentAlternative of the *next* alternative - this will
            // cause the priorities to be recalculated next time through so we
            // will attempt to explore the search space again
            //
            // The currentState+1 is guaranteed to be safe because the only way
            // we can get here is from where we currentState-- below.
            //

            (currentState + 1)->CurrentAlternative = NULL;

            //
            // We can't backtrack length 0 requests because there is nothing to
            // backtrack so we would get stuck in an inifinite loop...
            //

            if (currentState->CurrentAlternative->Length == 0) {
                goto failAllocation;
            }

            //
            // Backtrack
            //

            Arbiter->BacktrackAllocation(Arbiter, currentState);

            //
            // Reduce allocation window to not include the range we backtracked
            // and check that that doesn't underflow the minimum or wrap
            //

            possibleCurrentMinimum = currentState->Start - 1;

            if (possibleCurrentMinimum > currentState->CurrentMinimum // wrapped
            ||  possibleCurrentMinimum < currentState->CurrentAlternative->Minimum) {

                //
                // We have run out space in this alternative move on to the next
                //

                goto continueWithNextAllocationRange;

            } else {

                currentState->CurrentMaximum = possibleCurrentMinimum;

                //
                // Get back into arbitrating at the right point
                //

                goto continueWithNextSuitableRange;
            }
        }

        //
        // Try to allocate for this entry
        //

continueWithNextAllocationRange:

        while (Arbiter->GetNextAllocationRange(Arbiter, currentState)) {

            ARB_INDENT(2, (ULONG)(currentState - State));

            ARB_PRINT(2,
                        ("Testing 0x%I64x-0x%I64x %s\n",
                        currentState->CurrentMinimum,
                        currentState->CurrentMaximum,
                        currentState->CurrentAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_SHARED ?
                            "shared" : "non-shared"
                        ));

continueWithNextSuitableRange:

            while (Arbiter->FindSuitableRange(Arbiter, currentState)) {

                //
                // We found a possible solution
                //

                ARB_INDENT(2, (ULONG)(currentState - State));

                if (currentState->CurrentAlternative->Length != 0) {

                    ARB_PRINT(2,
                        ("Possible solution for %p = 0x%I64x-0x%I64x, %s\n",
                        currentState->Entry->PhysicalDeviceObject,
                        currentState->Start,
                        currentState->End,
                        currentState->CurrentAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_SHARED ?
                            "shared" : "non-shared"
                        ));

                    //
                    // Update the arbiter with the possible allocation
                    //

                    Arbiter->AddAllocation(Arbiter, currentState);

                } else {

                    ARB_PRINT(2,
                        ("Zero length solution solution for %p = 0x%I64x-0x%I64x, %s\n",
                        currentState->Entry->PhysicalDeviceObject,
                        currentState->Start,
                        currentState->End,
                        currentState->CurrentAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_SHARED ?
                            "shared" : "non-shared"
                        ));

                    //
                    // Set the result in the arbiter appropriatley so that we
                    // don't try and translate this zero requirement - it won't!
                    //

                    currentState->Entry->Result = ArbiterResultNullRequest;
                }

                //
                // Move on to the next entry
                //

                currentState++;
                goto tryAllocation;
            }
        }

failAllocation:

        //
        // We couldn't allocate for this device
        //

        if (currentState == State) {

            //
            // We are at the top of the allocation stack to we can't backtrack -
            // *** GAME OVER ***
            //

            return STATUS_UNSUCCESSFUL;

        } else {

            //
            // Backtrack and try again
            //

            ARB_INDENT(2, (ULONG)(currentState - State));

            ARB_PRINT(2,
                ("Allocation failed for %p - backtracking\n",
                currentState->Entry->PhysicalDeviceObject
                ));

            backtracking = TRUE;

            //
            // Pop the last state off the stack and try a different path
            //

            currentState--;
            goto tryAllocation;
        }
    }

    //
    // We have successfully allocated for all ranges so fill in the allocation
    //

    currentState = State;

    while (currentState->Entry != NULL) {

        status = Arbiter->PackResource(
                    currentState->CurrentAlternative->Descriptor,
                    currentState->Start,
                    currentState->Entry->Assignment
                    );

        ASSERT(NT_SUCCESS(status));

        //
        // Remember the alternative we chose from so we can retrieve it during retest
        //

        currentState->Entry->SelectedAlternative
            = currentState->CurrentAlternative->Descriptor;

        ARB_PRINT(2,
                    ("Assigned - 0x%I64x-0x%I64x\n",
                    currentState->Start,
                    currentState->End
                    ));

        currentState++;
    }

    return STATUS_SUCCESS;

}

BOOLEAN
ArbGetNextAllocationRange(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PARBITER_ALLOCATION_STATE State
    )

/*++

Routine Description:

    This routine attempts to find the next range where allocation should be
    tried.  It updates State->CurrentMinimum, State->CurrentMaximum and
    State->CurrentAlternative to indicate this range.

Arguments:

    Arbiter - The instance data of the arbiter

    State - The state of the current arbitration

Return Value:

    TRUE if a range to attemp allocation in is found, FALSE otherwise

--*/

{

    PARBITER_ALTERNATIVE current, lowestAlternative;
    ULONGLONG min, max;
    PARBITER_ORDERING ordering;


    for (;;) {

        if (State->CurrentAlternative) {

            //
            // Update the priority of the alternative we selected last time
            //

            ArbpUpdatePriority(Arbiter, State->CurrentAlternative);

        } else {

            //
            // This is the first time we are looking at this alternative or a
            // backtrack - either way we need to update all the priorities
            //

            FOR_ALL_IN_ARRAY(State->Alternatives,
                             State->AlternativeCount,
                             current) {

                current->Priority = ARBITER_PRIORITY_NULL;
                ArbpUpdatePriority(Arbiter, current);

            }
        }

        //
        // Find the lowest priority of the alternatives
        //

        lowestAlternative = State->Alternatives;

        FOR_ALL_IN_ARRAY(State->Alternatives + 1,
                         State->AlternativeCount - 1,
                         current) {

            if (current->Priority < lowestAlternative->Priority) {
                lowestAlternative = current;
            }
        }

        ARB_INDENT(2, (ULONG)(State - Arbiter->AllocationStack));

        //
        // Check if we have run out of allocation ranges
        //

        if (lowestAlternative->Priority == ARBITER_PRIORITY_EXHAUSTED) {

            if (lowestAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_FIXED) {

                ARB_PRINT(2,("Fixed alternative exhausted\n"));

            } else {

                ARB_PRINT(2,("Alternative exhausted\n"));
            }

            return FALSE;

        } else {

            ARB_PRINT(2,(
                "LowestAlternative: [%i] 0x%I64x-0x%I64x L=0x%08x A=0x%08x\n",
                lowestAlternative->Priority,
                lowestAlternative->Minimum,
                lowestAlternative->Maximum,
                lowestAlternative->Length,
                lowestAlternative->Alignment
                ));

        }

        //
        // Check if we are now allowing reserved ranges
        //

        if (lowestAlternative->Priority == ARBITER_PRIORITY_RESERVED
        ||  lowestAlternative->Priority == ARBITER_PRIORITY_PREFERRED_RESERVED) {

            //
            // Set min and max to be the Minimum and Maximum that the descriptor
            // specified ignoring any reservations or orderings - this is our
            // last chance
            //

            min = lowestAlternative->Minimum;
            max = lowestAlternative->Maximum;

            ARB_INDENT(2, (ULONG)(State - Arbiter->AllocationStack));

            ARB_PRINT(2,("Allowing reserved ranges\n"));

        } else {

            ASSERT(ORDERING_INDEX_FROM_PRIORITY(lowestAlternative->Priority) >= 0 &&
                    ORDERING_INDEX_FROM_PRIORITY(lowestAlternative->Priority) <
                      Arbiter->OrderingList.Count);

            //
            // Locate the ordering we match
            //

            ordering = &Arbiter->OrderingList.Orderings
                [ORDERING_INDEX_FROM_PRIORITY(lowestAlternative->Priority)];

            //
            // Make sure they overlap and are big enough - this is just paranoia
            //

            ASSERT(INTERSECT(lowestAlternative->Minimum,
                             lowestAlternative->Maximum,
                             ordering->Start,
                             ordering->End)
                && INTERSECT_SIZE(lowestAlternative->Minimum,
                                  lowestAlternative->Maximum,
                                  ordering->Start,
                                  ordering->End) >= lowestAlternative->Length);

            //
            // Calculate the allocation range
            //

            min = __max(lowestAlternative->Minimum, ordering->Start);

            max = __min(lowestAlternative->Maximum, ordering->End);

        }

        //
        // If this is a length 0 requirement then succeed now and avoid much
        // trauma later
        //

        if (lowestAlternative->Length == 0) {

            min = lowestAlternative->Minimum;
            max = lowestAlternative->Maximum;

        } else {

            //
            // Trim range to match alignment.
            //

            min += lowestAlternative->Alignment - 1;
            min -= min % lowestAlternative->Alignment;

            if ((lowestAlternative->Length - 1) > (max - min)) {

                ARB_INDENT(3, (ULONG)(State - Arbiter->AllocationStack));
                ARB_PRINT(3, ("Range cannot be aligned ... Skipping\n"));

                //
                // Set CurrentAlternative so we will update the priority of this
                // alternative
                //

                State->CurrentAlternative = lowestAlternative;
                continue;
            }

            max -= lowestAlternative->Length - 1;
            max -= max % lowestAlternative->Alignment;
            max += lowestAlternative->Length - 1;

        }

        //
        // Check if we handed back the same range last time, for the same
        // alternative, if so try to find another range
        //

        if (min == State->CurrentMinimum
        && max == State->CurrentMaximum
        && State->CurrentAlternative == lowestAlternative) {

            ARB_INDENT(2, (ULONG)(State - Arbiter->AllocationStack));

            ARB_PRINT(2,
                  ("Skipping identical allocation range\n"
            ));

            continue;
        }

        State->CurrentMinimum = min;
        State->CurrentMaximum = max;
        State->CurrentAlternative = lowestAlternative;

        ARB_INDENT(2, (ULONG)(State - Arbiter->AllocationStack));
        ARB_PRINT(1, ("AllocationRange: 0x%I64x-0x%I64x\n", min, max));

        return TRUE;

    }
}

NTSTATUS
ArbpGetRegistryValue(
    IN HANDLE KeyHandle,
    IN PWSTR  ValueName,
    OUT PKEY_VALUE_FULL_INFORMATION *Information
    )

/*++

Routine Description:

    This routine is invoked to retrieve the data for a registry key's value.
    This is done by querying the value of the key with a zero-length buffer
    to determine the size of the value, and then allocating a buffer and
    actually querying the value into the buffer.

    It is the responsibility of the caller to free the buffer.

Arguments:

    KeyHandle - Supplies the key handle whose value is to be queried

    ValueName - Supplies the null-terminated Unicode name of the value.

    Information - Returns a pointer to the allocated data buffer.

Return Value:

    The function value is the final status of the query operation.

Note:

    The same as IopGetRegistryValue - it allows us to share the arbiter
    code with pci.sys

--*/

{
    UNICODE_STRING unicodeString;
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION infoBuffer;
    ULONG keyValueLength;

    PAGED_CODE();

    RtlInitUnicodeString( &unicodeString, ValueName );

    //
    // Figure out how big the data value is so that a buffer of the
    // appropriate size can be allocated.
    //

    status = ZwQueryValueKey( KeyHandle,
                              &unicodeString,
                              KeyValueFullInformationAlign64,
                              (PVOID) NULL,
                              0,
                              &keyValueLength );
    if (status != STATUS_BUFFER_OVERFLOW &&
        status != STATUS_BUFFER_TOO_SMALL) {
        return status;
    }

    //
    // Allocate a buffer large enough to contain the entire key data value.
    //

    infoBuffer = ExAllocatePoolWithTag( PagedPool,
                                        keyValueLength,
                                        ARBITER_MISC_TAG
                                        );

    if (!infoBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Query the data for the key value.
    //

    status = ZwQueryValueKey( KeyHandle,
                              &unicodeString,
                              KeyValueFullInformationAlign64,
                              infoBuffer,
                              keyValueLength,
                              &keyValueLength );
    if (!NT_SUCCESS( status )) {
        ExFreePool( infoBuffer );
        return status;
    }

    //
    // Everything worked, so simply return the address of the allocated
    // buffer to the caller, who is now responsible for freeing it.
    //

    *Information = infoBuffer;
    return STATUS_SUCCESS;
}


#define ARBITER_ORDERING_LIST_INITIAL_SIZE      16

NTSTATUS
ArbInitializeOrderingList(
    IN OUT PARBITER_ORDERING_LIST List
    )

/*++

Routine Description:

    This routine inititialize an arbiter ordering list.

Arguments:

    List - The list to be initialized

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PAGED_CODE();

    ASSERT(List);

    List->Orderings = ExAllocatePoolWithTag(PagedPool,
                                            ARBITER_ORDERING_LIST_INITIAL_SIZE *
                                                sizeof(ARBITER_ORDERING),
                                            ARBITER_ORDERING_LIST_TAG
                                            );

    if (!List->Orderings) {
        List->Maximum = 0;
        List->Count = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    List->Count = 0;
    List->Maximum = ARBITER_ORDERING_LIST_INITIAL_SIZE;

    return STATUS_SUCCESS;
}

NTSTATUS
ArbCopyOrderingList(
    OUT PARBITER_ORDERING_LIST Destination,
    IN PARBITER_ORDERING_LIST Source
    )

/*++

Routine Description:

    This routine copies an arbiter ordering list.

Arguments:

    Destination - An uninitialized arbiter ordering list where the data
        should be copied from

    Source - Arbiter ordering list to be copied
Return Value:

    Status code that indicates whether or not the function was successful.

--*/


{

    PAGED_CODE()

    ASSERT(Source->Count <= Source->Maximum);
    ASSERT(Source->Maximum > 0);

    Destination->Orderings =
        ExAllocatePoolWithTag(PagedPool,
                              Source->Maximum * sizeof(ARBITER_ORDERING),
                              ARBITER_ORDERING_LIST_TAG
                              );

    if (Destination->Orderings == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Destination->Count = Source->Count;
    Destination->Maximum = Source->Maximum;

    if (Source->Count > 0) {

        RtlCopyMemory(Destination->Orderings,
                      Source->Orderings,
                      Source->Count * sizeof(ARBITER_ORDERING)
                      );
    }

    return STATUS_SUCCESS;
}


NTSTATUS
ArbAddOrdering(
    OUT PARBITER_ORDERING_LIST List,
    IN ULONGLONG Start,
    IN ULONGLONG End
    )

/*++

Routine Description:

    This routine adds the range Start-End to the end of the ordering list.  No
    checking for overlaps or pruning is done (see ArbpPruneOrdering)

Arguments:

    OrderingList - The list where the range should be added.

    Start - The start of the range to be added.

    End - The end of the range to be added.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{

    //
    // Validate parameters
    //

    if (End < Start) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Check if the buffer is full
    //

    if (List->Count == List->Maximum) {

        PARBITER_ORDERING temp;

        //
        // Out of space - grow the buffer
        //

        temp = ExAllocatePoolWithTag(PagedPool,
                              (List->Count + ARBITER_ORDERING_GROW_SIZE) *
                                  sizeof(ARBITER_ORDERING),
                              ARBITER_ORDERING_LIST_TAG
                              );

        if (!temp) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // If we had any orderings copy them
        //

        if (List->Orderings) {

            RtlCopyMemory(temp,
                          List->Orderings,
                          List->Count * sizeof(ARBITER_ORDERING)
                          );

            ExFreePool(List->Orderings);
        }

        List->Maximum += ARBITER_ORDERING_GROW_SIZE;
        List->Orderings = temp;

    }

    //
    // Add the entry to the list
    //

    List->Orderings[List->Count].Start = Start;
    List->Orderings[List->Count].End = End;
    List->Count++;

    ASSERT(List->Count <= List->Maximum);

    return STATUS_SUCCESS;
}

NTSTATUS
ArbPruneOrdering(
    IN OUT PARBITER_ORDERING_LIST OrderingList,
    IN ULONGLONG Start,
    IN ULONGLONG End
    )

/*++

Routine Description:

    This routine removes the range Start-End from all entries in the ordering
    list, splitting ranges into two or deleting them as necessary.

Arguments:

    OrderingList - The list to be pruned.

    Start - The start of the range to be deleted.

    End - The end of the range to be deleted.

Return Value:

    Status code that indicates whether or not the function was successful.

Note:

    In the comments below *** represents the range Start - End and --- the range
    current->Start - current->End.

--*/

{

    NTSTATUS status;
    PARBITER_ORDERING current, currentInsert, newOrdering = NULL, temp = NULL;
    USHORT count;

    ASSERT(OrderingList);
    ASSERT(OrderingList->Orderings);

    //
    // Validate parameters
    //

    if (End < Start) {
        status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Allocate a buffer big enough for all eventualities
    //

    newOrdering = ExAllocatePoolWithTag(PagedPool,
                                        (OrderingList->Count * 2 + 1) *
                                            sizeof(ARBITER_ORDERING),
                                        ARBITER_ORDERING_LIST_TAG
                                        );

    if (!newOrdering) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    currentInsert = newOrdering;

    //
    // Do we have a current ordering?
    //

    if (OrderingList->Count > 0) {

        //
        // Iterate through the current ordering and prune accordingly
        //

        FOR_ALL_IN_ARRAY(OrderingList->Orderings, OrderingList->Count, current) {

            if (End < current->Start || Start > current->End) {

                //
                // ****      or      ****
                //      ----    ----
                //
                // We don't overlap so copy the range unchanged
                //

                *currentInsert++ = *current;

            } else if (Start > current->Start) {

                if (End < current->End) {

                    //
                    //   ****
                    // --------
                    //
                    // Split the range into two
                    //

                    currentInsert->Start = End + 1;
                    currentInsert->End = current->End;
                    currentInsert++;

                    currentInsert->Start = current->Start;
                    currentInsert->End = Start - 1;
                    currentInsert++;


                } else {

                    //
                    //       **** or     ****
                    // --------      --------
                    //
                    // Prune the end of the range
                    //

                    ASSERT(End >= current->End);

                    currentInsert->Start = current->Start;
                    currentInsert->End = Start - 1;
                    currentInsert++;
                }
            } else {

                ASSERT(Start <= current->Start);

                if (End < current->End) {

                    //
                    // ****       or ****
                    //   --------    --------
                    //
                    // Prune the start of the range
                    //

                    currentInsert->Start = End + 1;
                    currentInsert->End = current->End;
                    currentInsert++;

                } else {

                    ASSERT(End >= current->End);

                    //
                    // ******** or ********
                    //   ----      --------
                    //
                    // Don't copy the range (ie. Delete it)
                    //

                }
            }
        }
    }


    ASSERT(currentInsert - newOrdering >= 0);

    count = (USHORT)(currentInsert - newOrdering);

    //
    // Check if we have any orderings left
    //

    if (count > 0) {

        if (count > OrderingList->Maximum) {

            //
            // There isn't enough space so allocate a new buffer
            //

            temp =
                ExAllocatePoolWithTag(PagedPool,
                                      count * sizeof(ARBITER_ORDERING),
                                      ARBITER_ORDERING_LIST_TAG
                                      );

            if (!temp) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto cleanup;
            }

            if (OrderingList->Orderings) {
                ExFreePool(OrderingList->Orderings);
            }

            OrderingList->Orderings = temp;
            OrderingList->Maximum = count;

        }


        //
        // Copy the new ordering
        //

        RtlCopyMemory(OrderingList->Orderings,
                      newOrdering,
                      count * sizeof(ARBITER_ORDERING)
                      );
    }

    //
    // Free our temporary buffer
    //

    ExFreePool(newOrdering);

    OrderingList->Count = count;

    return STATUS_SUCCESS;

cleanup:

    if (newOrdering) {
        ExFreePool(newOrdering);
    }

    if (temp) {
        ExFreePool(temp);
    }

    return status;

}
VOID
ArbFreeOrderingList(
    IN PARBITER_ORDERING_LIST List
    )
/*++

Routine Description:

    Frees storage associated with an ordering list.
    Reverses ArbInitializeOrderingList.

Arguments:

    List - The list to be fred

Return Value:

    None
--*/

{
    if (List->Orderings) {
        ASSERT(List->Maximum);
        ExFreePool(List->Orderings);
    }

    List->Count = 0;
    List->Maximum = 0;
    List->Orderings = NULL;
}



BOOLEAN
ArbOverrideConflict(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    )

/*++

Routine Description:

    This is the default implementation of override conflict which

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

Return Value:

    TRUE if the conflict is allowable, false otherwise

--*/

{

    PRTL_RANGE current;
    RTL_RANGE_LIST_ITERATOR iterator;
    BOOLEAN ok = FALSE;

    PAGED_CODE();

    if (!(State->CurrentAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_FIXED)) {
        return FALSE;
    }

    FOR_ALL_RANGES(Arbiter->PossibleAllocation, &iterator, current) {

        //
        // Only test the overlapping ones
        //

        if (INTERSECT(current->Start, current->End, State->CurrentMinimum, State->CurrentMaximum)) {


            //
            // Check if we should ignore the range because of its attributes
            //

            if (current->Attributes & State->RangeAvailableAttributes) {

                //
                // We DON'T set ok to true because we are just ignoring the range,
                // as RtlFindRange would have and thus it can't be the cause of
                // RtlFindRange failing, so ignoring it can't fix the conflict.
                //

                continue;
            }

            //
            // Check if we are conflicting with ourselves AND the conflicting range
            // is a fixed requirement
            //

            if (current->Owner == State->Entry->PhysicalDeviceObject
            && State->CurrentAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_FIXED) {

                State->Start=State->CurrentMinimum;
                State->End=State->CurrentMaximum;

                ok = TRUE;
                continue;
            }

            //
            // The conflict is still valid
            //

            return FALSE;
        }
    }
    return ok;
}

VOID
ArbpUpdatePriority(
    PARBITER_INSTANCE Arbiter,
    PARBITER_ALTERNATIVE Alternative
    )

/*++

Routine Description:

    This routine updates the priority of an arbiter alternative.

Arguments:

    Arbiter - The arbiter we are operating on

    Alternative - The alternative currently being considered

Return Value:

    Status code that indicates whether or not the function was successful.

Note:

    The priorities are a LONG values organised as:

    <------Preferred priorities-----> <-----Ordinary Priorities----->

    MINLONG--------------------------0-----------------------------MAXLONG
                                     ^                               ^ ^ ^
                                     |                               | | |
                                    NULL            PREFERRED_RESERVED | |
                                                                RESERVED |
                                                                     EXHAUSTED

    An ordinary priority is calculated the (index + 1) of the next ordering it
    intersects with (and has enough space for an allocation).

    A preferred priority is the ordinary priority * - 1

    In this way by examining each of the alternatives in priority order (lowest
    first) we achieve the desired allocation order of:

    (1) Preferred alternative with non-reserved resources
    (2) Alternatives with non-reserved resources
    (3) Preferred reserved resources
    (4) Reserved Resources

    MAXLONG the worst priority indicates that there are no more allocation ranges
    left.

--*/

{

    PARBITER_ORDERING ordering;
    BOOLEAN preferred;
    LONG priority;

    PAGED_CODE();

    priority = Alternative->Priority;

    //
    // If we have already tried the reserved resources then we are out of luck!
    //

    if (priority == ARBITER_PRIORITY_RESERVED
    ||  priority == ARBITER_PRIORITY_PREFERRED_RESERVED) {

        Alternative->Priority = ARBITER_PRIORITY_EXHAUSTED;
        return;
    }

    //
    // Check if this is a preferred value - we treat them specially
    //

    preferred = Alternative->Descriptor->Option & IO_RESOURCE_PREFERRED;

    //
    // If priority is NULL then we haven't started calculating one so we
    // should start the search from the initial ordering
    //

    if (priority == ARBITER_PRIORITY_NULL) {

        ordering = Arbiter->OrderingList.Orderings;

    } else {

        //
        // If we are a fixed resource then there is no point
        // in trying to find another range - it will be the
        // same and thus still conflict.  Mark this alternative as
        // exhausted
        //

        if (Alternative->Flags & ARBITER_ALTERNATIVE_FLAG_FIXED) {

            Alternative->Priority = ARBITER_PRIORITY_EXHAUSTED;

            return;
        }

        ASSERT(ORDERING_INDEX_FROM_PRIORITY(Alternative->Priority) >= 0 &&
                ORDERING_INDEX_FROM_PRIORITY(Alternative->Priority) <
                 Arbiter->OrderingList.Count);

        ordering = &Arbiter->OrderingList.Orderings
            [ORDERING_INDEX_FROM_PRIORITY(Alternative->Priority) + 1];

    }

    //
    // Now find the first member of the assignent ordering for this arbiter
    // where we have an overlap big enough
    //

    FOR_REST_IN_ARRAY(Arbiter->OrderingList.Orderings,
                      Arbiter->OrderingList.Count,
                      ordering) {

        //
        // Is the ordering applicable?
        //

        if (INTERSECT(Alternative->Minimum, Alternative->Maximum,
                      ordering->Start, ordering->End)
        && INTERSECT_SIZE(Alternative->Minimum, Alternative->Maximum,
                          ordering->Start,ordering->End) >= Alternative->Length) {

            //
            // This is out guy, calculate his priority
            //

            Alternative->Priority = (LONG)(ordering - Arbiter->OrderingList.Orderings + 1);

            //
            // Preferred priorities are -ve
            //

            if (preferred) {
                Alternative->Priority *= -1;
            }

            return;
        }
    }

    //
    // We have runout of non-reserved resources so try the reserved ones
    //

    if (preferred) {
        Alternative->Priority = ARBITER_PRIORITY_PREFERRED_RESERVED;
    } else {
        Alternative->Priority = ARBITER_PRIORITY_RESERVED;
    }

}

NTSTATUS
ArbAddReserved(
    IN PARBITER_INSTANCE Arbiter,
    IN PIO_RESOURCE_DESCRIPTOR Requirement      OPTIONAL,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource OPTIONAL
    )
{
    PAGED_CODE();

    return STATUS_NOT_SUPPORTED;
}

BOOLEAN
ArbpQueryConflictCallback(
    IN PVOID Context,
    IN PRTL_RANGE Range
    )

/*++

Routine Description:

    This call back is called from FindSuitableRange (via RtlFindRange) when we
    encounter an conflicting range.

Arguments:

    Context - Actually a PRTL_RANGE * where we store the range we conflicted
        with.

    Range - The range we conflict with.

Return Value:

    FALSE

--*/

{
    PRTL_RANGE *conflictingRange = (PRTL_RANGE*)Context;

    PAGED_CODE();

    ARB_PRINT(2,("Possible conflict: (%p) 0x%I64x-0x%I64x Owner: %p",
                   Range,
                   Range->Start,
                   Range->End,
                   Range->Owner
                ));

    //
    // Remember the conflicting range
    //

    *conflictingRange = Range;

    //
    // We want to allow the rest of FindSuitableRange to determine if this really
    // is a conflict.
    //

    return FALSE;
}


NTSTATUS
ArbQueryConflict(
    IN PARBITER_INSTANCE Arbiter,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PIO_RESOURCE_DESCRIPTOR ConflictingResource,
    OUT PULONG ConflictCount,
    OUT PARBITER_CONFLICT_INFO *Conflicts
    )

/*++

Routine Description:

    This routine examines the arbiter state and returns a list of devices that
    conflict with ConflictingResource

Arguments:

    Arbiter - The arbiter to examine conflicts in

    ConflictingResource - The resource we want to know the conflicts with

    ConflictCount - On success contains the number of conflicts detected

    ConflictList - On success contains a pointer to an array of conflicting
        devices

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    NTSTATUS status;
    RTL_RANGE_LIST backupAllocation;
    BOOLEAN backedUp = FALSE;
    ARBITER_LIST_ENTRY entry;
    ARBITER_ALLOCATION_STATE state;
    ARBITER_ALTERNATIVE alternative;
    ULONG count = 0, size = 10;
    PRTL_RANGE conflictingRange;
    PARBITER_CONFLICT_INFO conflictInfo = NULL;
    PVOID savedContext;
    PRTL_CONFLICT_RANGE_CALLBACK savedCallback;
    ULONG sz;

    PAGED_CODE();

    ASSERT(PhysicalDeviceObject);
    ASSERT(ConflictingResource);
    ASSERT(ConflictCount);
    ASSERT(Conflicts);
    //
    // Set up our conflict callback
    //
    savedCallback = Arbiter->ConflictCallback;
    savedContext = Arbiter->ConflictCallbackContext;
    Arbiter->ConflictCallback = ArbpQueryConflictCallback;
    Arbiter->ConflictCallbackContext = &conflictingRange;

    //
    // If there is a transaction in progress then we need to backup the
    // the possible allocation so we can restore it when we are done.
    //

    if (Arbiter->TransactionInProgress) {

        RtlInitializeRangeList(&backupAllocation);

        status = RtlCopyRangeList(&backupAllocation, Arbiter->PossibleAllocation);

        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }

        RtlFreeRangeList(Arbiter->PossibleAllocation);

        backedUp = TRUE;
    }

    //
    // Fake up the allocation state
    //


    status = RtlCopyRangeList(Arbiter->PossibleAllocation, Arbiter->Allocation);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    status = ArbpBuildAlternative(Arbiter, ConflictingResource, &alternative);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    RtlZeroMemory(&state, sizeof(ARBITER_ALLOCATION_STATE));

    state.Start = alternative.Minimum;
    state.End = alternative.Maximum;
    state.CurrentMinimum = state.Start;
    state.CurrentMaximum = state.End;
    state.CurrentAlternative = &alternative;
    state.AlternativeCount = 1;
    state.Alternatives = &alternative;
    state.Flags = ARBITER_STATE_FLAG_CONFLICT;
    state.Entry = &entry;

    //
    // BUGBUG(andrewth) - need to fill in more of this false entry
    //
    RtlZeroMemory(&entry, sizeof(ARBITER_LIST_ENTRY));
    entry.RequestSource = ArbiterRequestPnpEnumerated;
    entry.PhysicalDeviceObject = PhysicalDeviceObject;
    //
    // BUGBUG(jamiehun) - we didn't allow for passing interface type
    // now we have to live with the decision
    // this really only comes into being for PCI Translator AFAIK
    // upshot of not doing this is we get false conflicts when PCI Boot alloc
    // maps over top of ISA alias
    // IoGetDeviceProperty generally does the right thing for the times we have to use this info
    //
    if (!NT_SUCCESS(IoGetDeviceProperty(PhysicalDeviceObject,DevicePropertyLegacyBusType,sizeof(entry.InterfaceType),&entry.InterfaceType,&sz))) {
        entry.InterfaceType = Isa; // not what I want to do! However this has the right effect - good enough for conflict detection
    }
    if (!NT_SUCCESS(IoGetDeviceProperty(PhysicalDeviceObject,DevicePropertyBusNumber,sizeof(entry.InterfaceType),&entry.BusNumber,&sz))) {
        entry.BusNumber = 0; // not what I want to do! However this has the right effect - good enough for conflict detection
    }

    //
    // Initialize the return buffers
    //

    conflictInfo = ExAllocatePoolWithTag(PagedPool,
                                         size * sizeof(ARBITER_CONFLICT_INFO),
                                         ARBITER_CONFLICT_INFO_TAG
                                         );

    if (!conflictInfo) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    //
    // Perform any necessary preprocessing
    //

    status = Arbiter->PreprocessEntry(Arbiter, &state);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Remove self from list of possible allocations
    // status may be set, but can be ignored
    // we take ourself out of test completely, so that a user can
    // pick new values in context of rest of the world
    // if we decide to use RtlDeleteRange instead
    // make sure we do it for every alias formed in PreprocessEntry
    //

    status = RtlDeleteOwnersRanges(Arbiter->PossibleAllocation,
                            state.Entry->PhysicalDeviceObject
                            );

    //
    // Keep trying to find a suitable range and each time we fail remember why.
    //
    conflictingRange = NULL;
    state.CurrentMinimum = state.Start;
    state.CurrentMaximum = state.End;

    while (!Arbiter->FindSuitableRange(Arbiter, &state)) {

        if (count == size) {

            //
            // We need to resize the return buffer
            //

            PARBITER_CONFLICT_INFO temp = conflictInfo;

            size += 5;

            conflictInfo =
                ExAllocatePoolWithTag(PagedPool,
                                      size * sizeof(ARBITER_CONFLICT_INFO),
                                      ARBITER_CONFLICT_INFO_TAG
                                      );

            if (!conflictInfo) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                conflictInfo = temp;
                goto cleanup;
            }

            RtlCopyMemory(conflictInfo,
                          temp,
                          count * sizeof(ARBITER_CONFLICT_INFO)
                          );

            ExFreePool(temp);

        }

        if (conflictingRange != NULL) {
            conflictInfo[count].OwningObject = conflictingRange->Owner;
            conflictInfo[count].Start = conflictingRange->Start;
            conflictInfo[count].End = conflictingRange->End;
            // BUGBUG - maybe need some more info...
            count++;

            //
            // Delete the range we conflicted with so we don't loop forever
            //
#if 0
            status = RtlDeleteRange(Arbiter->PossibleAllocation,
                                    conflictingRange->Start,
                                    conflictingRange->End,
                                    conflictingRange->Owner
                                    );
#endif
            status = RtlDeleteOwnersRanges(Arbiter->PossibleAllocation,
                                    conflictingRange->Owner
                                    );

            if (!NT_SUCCESS(status)) {
                goto cleanup;
            }

        } else {
            //
            // someone isn't playing by the rules (such as ACPI!)
            //
            ARB_PRINT(0,("Conflict detected - but someone hasn't set conflicting info\n"));

            conflictInfo[count].OwningObject = NULL;
            conflictInfo[count].Start = (ULONGLONG)0;
            conflictInfo[count].End = (ULONGLONG)(-1);
            // BUGBUG - maybe need some more info...
            count++;

            //
            // we daren't continue at risk of looping forever
            //
            break;
        }

        //
        // reset for next round
        //
        conflictingRange = NULL;
        state.CurrentMinimum = state.Start;
        state.CurrentMaximum = state.End;
    }

    RtlFreeRangeList(Arbiter->PossibleAllocation);

    if (Arbiter->TransactionInProgress) {

        status = RtlCopyRangeList(Arbiter->PossibleAllocation, &backupAllocation);

        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }

        RtlFreeRangeList(&backupAllocation);
    }

    Arbiter->ConflictCallback = savedCallback;
    Arbiter->ConflictCallbackContext = savedContext;

    *Conflicts = conflictInfo;
    *ConflictCount = count;

    return STATUS_SUCCESS;

cleanup:

    if (conflictInfo) {
        ExFreePool(conflictInfo);
    }

    RtlFreeRangeList(Arbiter->PossibleAllocation);

    if (Arbiter->TransactionInProgress && backedUp) {
        status = RtlCopyRangeList(Arbiter->PossibleAllocation, &backupAllocation);
        RtlFreeRangeList(&backupAllocation);
    }

    Arbiter->ConflictCallback = savedCallback;
    Arbiter->ConflictCallbackContext = savedContext;

    *Conflicts = NULL;

    return status;
}


NTSTATUS
ArbStartArbiter(
    IN PARBITER_INSTANCE Arbiter,
    IN PCM_RESOURCE_LIST StartResources
    )

/*++

Routine Description:

    This function is called by the driver that implements the arbiter once
    it has been started and knowns what resources it can allocate to its
    children.

    BUGBUG - It will eventually initialize the range lists correctly but for
    now it is just an overloadable place holder as that work is done elsewhere.

Parameters:

    Arbiter - The instance of the arbiter being called.


Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PAGED_CODE();

    return STATUS_SUCCESS;
}




VOID
ArbpIndent(
    IN ULONG Count
    )
{
    UCHAR spaces[80];

    ASSERT(Count <= 80);

    RtlFillMemory(spaces, Count, '*');

    spaces[Count] = 0;

    DbgPrint("%s", spaces);

}
