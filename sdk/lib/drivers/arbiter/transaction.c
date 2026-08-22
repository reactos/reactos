/*
 * PROJECT:     ReactOS Arbitrartion Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test/retest/commit/rollback transaction support
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntifs.h>
#include <ndk/rtlfuncs.h>
#include "arbiter.h"

#define NDEBUG
#include <debug.h>

/* TRANSACTION SUPPORT ********************************************************/

/**
 * @brief
 * Expands one IO_RESOURCE_DESCRIPTOR into an ARBITER_ALTERNATIVE
 * through the arbiter's UnpackRequirement callback and identifies
 * it.
 *
 * @param[in] Arbiter
 * The arbiter instance whose UnpackRequirement decodes the
 * descriptor.
 *
 * @param[in] Descriptor
 * The requirement descriptor to expand.
 *
 * @param[out] Alternative
 * Receives the decoded window, length, alignment and the derived
 * FIXED / SHARED / INVALID flags. The minimum is rounded up to the
 * requested alignment first, so a misaligned window base does not
 * later affect the range search.
 *
 * @return
 * Returns STATUS_SUCCESS, or the UnpackRequirement failure status.
 */
CODE_SEG("PAGE")
static
NTSTATUS
ArbpBuildAlternative(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PIO_RESOURCE_DESCRIPTOR Descriptor,
    _Out_ PARBITER_ALTERNATIVE Alternative)
{
    UINT64 Length, Alignment;
    NTSTATUS Status;

    PAGED_CODE();

    /*
     * UnpackRequirement always writes 64-bit Length/Alignment; ARBITER_ALTERNATIVE
     * narrows them to 32 bits pre-Vista, so decode through 64-bit temporaries and
     * assign, just to keep both versions in source.
     */
    Alternative->Descriptor = Descriptor;
    Status = Arbiter->UnpackRequirement(Descriptor,
                                        &Alternative->Minimum,
                                        &Alternative->Maximum,
                                        &Length,
                                        &Alignment);
    if (!NT_SUCCESS(Status))
        return Status;

#if (NTDDI_VERSION >= NTDDI_VISTA)
    Alternative->Length = Length;
    Alternative->Alignment = Alignment;
#else
    Alternative->Length = (UINT32)Length;
    Alternative->Alignment = (UINT32)Alignment;
#endif

    if (Alignment != 0 && (Alternative->Minimum % Alignment) != 0)
        Alternative->Minimum += Alignment - (Alternative->Minimum % Alignment);

    Alternative->Flags = 0;
    Alternative->Priority = ARBITER_PRIORITY_NULL;

    if (Descriptor->ShareDisposition == CmResourceShareShared)
        Alternative->Flags |= ARBITER_ALTERNATIVE_FLAG_SHARED;

    if (Alternative->Maximum < Alternative->Minimum)
        Alternative->Flags |= ARBITER_ALTERNATIVE_FLAG_BADRANGE;
    else if ((Alternative->Maximum - Alternative->Minimum + 1) == Alternative->Length)
        Alternative->Flags |= ARBITER_ALTERNATIVE_FLAG_FIXED;

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Builds the allocation stack for an arbitration list into
 * Arbiter->AllocationStack: one ARBITER_ALLOCATION_STATE per
 * contributing entry, in list order, terminated by a state whose
 * Entry is NULL, followed by the flattened ARBITER_ALTERNATIVE
 * array the states point into.
 *
 * @param[in] Arbiter
 * The arbiter instance whose AllocationStack buffer is (re)used,
 * growing it when the list needs more room.
 *
 * @param[in] ArbitrationList
 * The (sorted) list of ARBITER_LIST_ENTRY nodes to flatten.
 * Entries without alternatives contribute no state.
 *
 * @param[in] EntryCount
 * The number of entries on the list.
 *
 * @return
 * Returns STATUS_SUCCESS, STATUS_INSUFFICIENT_RESOURCES if the
 * stack cannot grow, or an ArbpBuildAlternative failure status.
 */
CODE_SEG("PAGE")
static
NTSTATUS
ArbpBuildAllocationStack(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PLIST_ENTRY ArbitrationList,
    _In_ ULONG EntryCount)
{
    PARBITER_ALLOCATION_STATE State;
    PARBITER_ALTERNATIVE Alternative;
    PLIST_ENTRY ListEntry;
    ULONG StateCount = EntryCount + 1;  /* + NULL terminator */
    ULONG AlternativeCount = 0;
    ULONG Size;

    PAGED_CODE();

    for (ListEntry = ArbitrationList->Flink;
         ListEntry != ArbitrationList;
         ListEntry = ListEntry->Flink)
    {
        PARBITER_LIST_ENTRY Entry = CONTAINING_RECORD(ListEntry, ARBITER_LIST_ENTRY, ListEntry);

        if (Entry->AlternativeCount == 0)
            StateCount--;  /* an empty entry contributes no state */
        else
            AlternativeCount += Entry->AlternativeCount;
    }

    Size = StateCount * sizeof(ARBITER_ALLOCATION_STATE) +
           AlternativeCount * sizeof(ARBITER_ALTERNATIVE);

    if (Arbiter->AllocationStackMaxSize < Size)
    {
        PARBITER_ALLOCATION_STATE NewStack;

        NewStack = ExAllocatePoolWithTag(PagedPool, Size, TAG_ARBITER);
        if (NewStack == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        if (Arbiter->AllocationStack != NULL)
            ExFreePoolWithTag(Arbiter->AllocationStack, TAG_ARBITER);

        Arbiter->AllocationStack = NewStack;
        Arbiter->AllocationStackMaxSize = Size;
    }
    RtlZeroMemory(Arbiter->AllocationStack, Size);

    State = Arbiter->AllocationStack;
    Alternative = (PARBITER_ALTERNATIVE)&Arbiter->AllocationStack[StateCount];

    for (ListEntry = ArbitrationList->Flink;
         ListEntry != ArbitrationList;
         ListEntry = ListEntry->Flink)
    {
        PARBITER_LIST_ENTRY Entry = CONTAINING_RECORD(ListEntry, ARBITER_LIST_ENTRY, ListEntry);
        ULONG Index;

        if (Entry->AlternativeCount == 0)
            continue;

        State->Entry = Entry;
        State->AlternativeCount = Entry->AlternativeCount;
        State->Alternatives = Alternative;
        State->Start = 1;  /* Start(1) > End(0): nothing chosen yet */

        for (Index = 0; Index < Entry->AlternativeCount; ++Index)
        {
            NTSTATUS Status = ArbpBuildAlternative(Arbiter, &Entry->Alternatives[Index], Alternative);
            if (!NT_SUCCESS(Status))
                return Status;
            Alternative++;
        }
        State++;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Test-allocates every entry on an arbitration list into the
 * arbiter's tentative allocation. The shared implementation behind
 * ArbiterLibAttemptAllocation.
 *
 * @param[in] Arbiter
 * The arbiter instance whose PossibleAllocation receives the
 * tentative solution.
 *
 * @param[in] ArbitrationList
 * The list of ARBITER_LIST_ENTRY nodes to place. Everything a
 * listed device already owns is removed from the working list
 * first, so a device does not conflict with itself (its boot
 * configuration in particular).
 *
 * @return
 * Returns STATUS_SUCCESS with the solution recorded in
 * PossibleAllocation, STATUS_DEVICE_CONFIGURATION_ERROR for a
 * malformed requirement, or the failure status of the stack build
 * or the solver. On failure the tentative allocation is discarded.
 */
CODE_SEG("PAGE")
static
NTSTATUS
ArbpTestAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PLIST_ENTRY ArbitrationList)
{
    PLIST_ENTRY ListEntry;
    PVOID PreviousOwner = NULL;
    ULONG EntryCount = 0;
    NTSTATUS Status;

    PAGED_CODE();

    /* Start the tentative allocation as a copy of the committed one. */
    RtlFreeRangeList(Arbiter->PossibleAllocation);
    RtlInitializeRangeList(Arbiter->PossibleAllocation);
    Status = RtlCopyRangeList(Arbiter->PossibleAllocation, Arbiter->Allocation);
    if (!NT_SUCCESS(Status))
        goto Failure;

    for (ListEntry = ArbitrationList->Flink;
         ListEntry != ArbitrationList;
         ListEntry = ListEntry->Flink)
    {
        PARBITER_LIST_ENTRY Entry = CONTAINING_RECORD(ListEntry, ARBITER_LIST_ENTRY, ListEntry);
        ULONG Index;

        EntryCount++;

        /*
         * Everything a device on the arbitration list already owns is up for
         * reassignment - remove it from the working list so the device does not
         * conflict with itself (its boot config in particular).
         */
        if (Entry->PhysicalDeviceObject != PreviousOwner)
        {
            PreviousOwner = Entry->PhysicalDeviceObject;
            RtlDeleteOwnersRanges(Arbiter->PossibleAllocation, Entry->PhysicalDeviceObject);
        }

        /* Score each entry: the sum of its alternatives' constrainedness. */
        Entry->WorkSpace = 0;
        if (Arbiter->ScoreRequirement != NULL)
        {
            for (Index = 0; Index < Entry->AlternativeCount; ++Index)
            {
                INT32 Score = Arbiter->ScoreRequirement(&Entry->Alternatives[Index]);
                if (Score < 0)
                {
                    Status = STATUS_DEVICE_CONFIGURATION_ERROR;
                    goto Failure;
                }
                Entry->WorkSpace += Score;
            }
        }
    }

    ArbiterLibSortArbitrationList(ArbitrationList);

    Status = ArbpBuildAllocationStack(Arbiter, ArbitrationList, EntryCount);
    if (!NT_SUCCESS(Status))
        goto Failure;

    Status = Arbiter->AllocateEntry(Arbiter, Arbiter->AllocationStack);
    if (!NT_SUCCESS(Status))
        goto Failure;

    return STATUS_SUCCESS;

Failure:
    RtlFreeRangeList(Arbiter->PossibleAllocation);
    RtlInitializeRangeList(Arbiter->PossibleAllocation);
    return Status;
}

/**
 * @brief
 * Re-establishes a previously tested solution in the arbiter's
 * tentative allocation without searching again: every entry's
 * SelectedAlternative is re-materialized at the position recorded
 * in its Assignment.
 *
 * @param[in] Arbiter
 * The arbiter instance whose PossibleAllocation is rebuilt from
 * the committed allocation.
 *
 * @param[in] ArbitrationList
 * The list of entries from an earlier successful test: each must
 * carry the SelectedAlternative and Assignment that test produced,
 * except entries whose Result is ArbiterResultNullRequest, which
 * are skipped.
 *
 * @return
 * Returns STATUS_SUCCESS with the solution re-recorded,
 * STATUS_INVALID_PARAMETER for an entry that was never tested, or
 * the failure status of the copy, decode or preprocess step. On
 * failure the tentative allocation is discarded.
 *
 * @remarks
 * after every arbiter's Test has succeeded, each is re-driven
 * through Retest so the exact chosen placements are laid down for the Commit that follows.
 */
CODE_SEG("PAGE")
static
NTSTATUS
ArbpRetestAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PLIST_ENTRY ArbitrationList)
{
    ARBITER_ALLOCATION_STATE State;
    ARBITER_ALTERNATIVE Alternative;
    PLIST_ENTRY ListEntry;
    NTSTATUS Status;

    PAGED_CODE();

    RtlZeroMemory(&State, sizeof(State));
    RtlZeroMemory(&Alternative, sizeof(Alternative));
    State.Alternatives = &Alternative;
    State.CurrentAlternative = &Alternative;
    State.AlternativeCount = 1;

    /*
     * Rebuild the tentative allocation from the committed one
     * minus everything the listed devices already own.
     */
    RtlFreeRangeList(Arbiter->PossibleAllocation);
    RtlInitializeRangeList(Arbiter->PossibleAllocation);
    Status = RtlCopyRangeList(Arbiter->PossibleAllocation, Arbiter->Allocation);
    if (!NT_SUCCESS(Status))
        goto Failure;

    for (ListEntry = ArbitrationList->Flink;
         ListEntry != ArbitrationList;
         ListEntry = ListEntry->Flink)
    {
        PARBITER_LIST_ENTRY Entry = CONTAINING_RECORD(ListEntry, ARBITER_LIST_ENTRY, ListEntry);

        Status = RtlDeleteOwnersRanges(Arbiter->PossibleAllocation,
                                       Entry->PhysicalDeviceObject);
        if (!NT_SUCCESS(Status))
            goto Failure;
    }

    for (ListEntry = ArbitrationList->Flink;
         ListEntry != ArbitrationList;
         ListEntry = ListEntry->Flink)
    {
        PARBITER_LIST_ENTRY Entry = CONTAINING_RECORD(ListEntry, ARBITER_LIST_ENTRY, ListEntry);
        UINT64 Length;

        if (Entry->Result == ArbiterResultNullRequest)
            continue;

        /* A retest without a preceding successful test is a caller bug. */
        if (Entry->SelectedAlternative == NULL || Entry->Assignment == NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Failure;
        }

        Status = ArbpBuildAlternative(Arbiter, Entry->SelectedAlternative, &Alternative);
        if (!NT_SUCCESS(Status))
            goto Failure;

        State.Entry = Entry;
        State.WorkSpace = 0;

        Status = Arbiter->UnpackResource(Entry->Assignment, &State.Start, &Length);
        if (!NT_SUCCESS(Status))
            goto Failure;
        State.End = State.Start + Length - 1;

        Status = Arbiter->PreprocessEntry(Arbiter, &State);
        if (!NT_SUCCESS(Status))
            goto Failure;

        if (Length != 0)
            Arbiter->AddAllocation(Arbiter, &State);

        if (State.Flags & ARBITER_STATE_FLAG_WORKSPACE)
        {
            ExFreePoolWithTag((PVOID)State.WorkSpace, TAG_ARBITER);
            State.Flags &= ~ARBITER_STATE_FLAG_WORKSPACE;
        }
    }

    return STATUS_SUCCESS;

Failure:
    RtlFreeRangeList(Arbiter->PossibleAllocation);
    RtlInitializeRangeList(Arbiter->PossibleAllocation);
    return Status;
}

/**
 * @brief
 * The TestAllocation action: tentatively places every entry of the
 * arbitration list, leaving the solution in PossibleAllocation for
 * a later commit or rollback.
 *
 * @param[in] Arbiter
 * The arbiter instance performing the test.
 *
 * @param[in,out] Parameters
 * The action parameters carrying the arbitration list (pre-Vista
 * builds receive the list directly).
 *
 * @return
 * Returns the ArbpTestAllocation status.
 */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
#if (NTDDI_VERSION >= NTDDI_VISTA)
ArbiterLibTestAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_TEST_ALLOCATION_PARAMETERS Parameters)
{
    PAGED_CODE();
    return ArbpTestAllocation(Arbiter, Parameters->ArbitrationList);
}
#else
ArbiterLibTestAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PLIST_ENTRY ArbitrationList)
{
    PAGED_CODE();
    return ArbpTestAllocation(Arbiter, ArbitrationList);
}
#endif

/**
 * @brief
 * The RetestAllocation action: deterministically re-establishes
 * the placements a previous test chose, without searching.
 *
 * @param[in] Arbiter
 * The arbiter instance performing the retest.
 *
 * @param[in,out] Parameters
 * The action parameters carrying the arbitration list (pre-Vista
 * builds receive the list directly).
 *
 * @return
 * Returns the ArbpRetestAllocation status.
 */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
#if (NTDDI_VERSION >= NTDDI_VISTA)
ArbiterLibRetestAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_RETEST_ALLOCATION_PARAMETERS Parameters)
{
    PAGED_CODE();
    return ArbpRetestAllocation(Arbiter, Parameters->ArbitrationList);
}
#else
ArbiterLibRetestAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PLIST_ENTRY ArbitrationList)
{
    PAGED_CODE();
    return ArbpRetestAllocation(Arbiter, ArbitrationList);
}
#endif

/**
 * @brief
 * The CommitAllocation action: the tentative PossibleAllocation
 * becomes the committed Allocation, and the old committed list is
 * recycled as the next scratch list.
 *
 * @param[in] Arbiter
 * The arbiter instance whose transaction is committed.
 *
 * @return
 * Returns STATUS_SUCCESS.
 */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibCommitAllocation(
    _In_ PARBITER_INSTANCE Arbiter)
{
    PRTL_RANGE_LIST Old = Arbiter->Allocation;

    PAGED_CODE();

    RtlFreeRangeList(Old);
    RtlInitializeRangeList(Old);
    Arbiter->Allocation = Arbiter->PossibleAllocation;
    Arbiter->PossibleAllocation = Old;
    return STATUS_SUCCESS;
}

/**
 * @brief
 * The RollbackAllocation action: discards the tentative
 * allocation; the committed one is untouched.
 *
 * @param[in] Arbiter
 * The arbiter instance whose transaction is rolled back.
 *
 * @return
 * Returns STATUS_SUCCESS.
 */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibRollbackAllocation(
    _In_ PARBITER_INSTANCE Arbiter)
{
    PAGED_CODE();

    RtlFreeRangeList(Arbiter->PossibleAllocation);
    RtlInitializeRangeList(Arbiter->PossibleAllocation);
    return STATUS_SUCCESS;
}
