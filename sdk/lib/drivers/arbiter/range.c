/*
 * PROJECT:     ReactOS Arbitration Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Allocation range search core
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntifs.h>
#include <ndk/rtlfuncs.h>
#include "arbiter.h"

#define NDEBUG
#include <debug.h>

/* RANGE WALKER ***************************************************************/

/**
 * @brief
 * Writes an alternative's priority to the next ordering-list
 * range it can be satisfied from.
 *
 * @param[in] Arbiter
 * The arbiter instance whose ordering list is walked.
 *
 * @param[in,out] Alternative
 * The alternative whose priority is written. Ordinary priorities
 * are ordering-list indices biased by one, negated for
 * IO_RESOURCE_PREFERRED alternatives so they sort first. Once the
 * orderings are exhausted the alternative is given one final
 * full-range pass at (PREFERRED_)RESERVED priority, after which
 * it goes EXHAUSTED.
 */
CODE_SEG("PAGE")
static
VOID
ArbpWritePriority(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALTERNATIVE Alternative)
{
    PARBITER_ORDERING Ordering;
    PARBITER_ORDERING End;
    INT32 Priority = Alternative->Priority;
    BOOLEAN Preferred;
    ULONG Index;

    PAGED_CODE();

    if (Priority == ARBITER_PRIORITY_RESERVED ||
        Priority == ARBITER_PRIORITY_PREFERRED_RESERVED)
    {
        Alternative->Priority = ARBITER_PRIORITY_EXHAUSTED;
        return;
    }

    Preferred = (Alternative->Descriptor->Option & IO_RESOURCE_PREFERRED) != 0;

    if (Priority == ARBITER_PRIORITY_NULL)
    {
        Ordering = Arbiter->OrderingList.Orderings;
    }
    else
    {
        /* A fixed alternative fits in exactly one place; it gets a single shot. */
        if (Alternative->Flags & ARBITER_ALTERNATIVE_FLAG_FIXED)
        {
            Alternative->Priority = ARBITER_PRIORITY_EXHAUSTED;
            return;
        }

        Index = (Priority < 0) ? (ULONG)(-(Priority + 1)) : (ULONG)(Priority - 1);
        if (Index >= Arbiter->OrderingList.Count)
        {
            Alternative->Priority = Preferred ? ARBITER_PRIORITY_PREFERRED_RESERVED
                                              : ARBITER_PRIORITY_RESERVED;
            return;
        }
        Ordering = &Arbiter->OrderingList.Orderings[Index + 1];
    }

    End = &Arbiter->OrderingList.Orderings[Arbiter->OrderingList.Count];
    for (; Ordering < End; ++Ordering)
    {
        UINT64 Start, RangeEnd;

        if (Ordering->Start > Alternative->Maximum ||
            Alternative->Minimum > Ordering->End)
        {
            continue;  /* No intersection with this alternative's window */
        }

        Start = max(Alternative->Minimum, Ordering->Start);
        RangeEnd = min(Alternative->Maximum, Ordering->End);

        if ((RangeEnd - Start + 1) >= Alternative->Length)
        {
            INT32 NewPriority = (INT32)(Ordering - Arbiter->OrderingList.Orderings) + 1;
            Alternative->Priority = Preferred ? -NewPriority : NewPriority;
            return;
        }
    }

    Alternative->Priority = Preferred ? ARBITER_PRIORITY_PREFERRED_RESERVED
                                      : ARBITER_PRIORITY_RESERVED;
}

/**
 * @brief
 * Determines whether a device is enumerated by the root enumerator.
 *
 * @param[in] DeviceObject
 * The physical device object to examine. May be NULL, in which
 * case the device is not considered root-enumerated.
 *
 * @return
 * Returns TRUE if the device's enumerator name is "ROOT",
 * FALSE otherwise or if the property cannot be read.
 */
CODE_SEG("PAGE")
static
BOOLEAN
ArbpIsRootEnumerated(
    _In_ PDEVICE_OBJECT DeviceObject)
{
    WCHAR Buffer[16];
    UNICODE_STRING Name;
    const UNICODE_STRING Root = RTL_CONSTANT_STRING(L"ROOT");
    ULONG Length = 0;

    PAGED_CODE();

    if (DeviceObject == NULL)
        return FALSE;

    if (!NT_SUCCESS(IoGetDeviceProperty(DeviceObject, DevicePropertyEnumeratorName,
                                        sizeof(Buffer), Buffer, &Length)))
    {
        return FALSE;
    }

    RtlInitUnicodeString(&Name, Buffer);
    return RtlEqualUnicodeString(&Root, &Name, TRUE);
}

/**
 * @brief
 * Determines whether a common driver is loaded on both device
 * stacks, above the physical device objects.
 *
 * @param[in] DeviceA
 * The first physical device object whose attached stack is walked.
 *
 * @param[in] DeviceB
 * The second physical device object whose attached stack is walked.
 *
 * @return
 * Returns TRUE if any driver attached above DeviceA also appears
 * above DeviceB, FALSE otherwise.
 */
CODE_SEG("PAGE")
static
BOOLEAN
ArbpSharesDriverStack(
    _In_ PDEVICE_OBJECT DeviceA,
    _In_ PDEVICE_OBJECT DeviceB)
{
    PDEVICE_OBJECT A, B;

    PAGED_CODE();

    for (A = DeviceA->AttachedDevice; A != NULL; A = A->AttachedDevice)
    {
        for (B = DeviceB->AttachedDevice; B != NULL; B = B->AttachedDevice)
        {
            if (A->DriverObject == B->DriverObject)
                return TRUE;
        }
    }

    return FALSE;
}

/**
 * @brief
 * Attempts last-chance sharing for a CmResourceShareDriverExclusive
 * requirement whose window RtlFindRange found occupied.
 *
 * @param[in] Arbiter
 * The arbiter instance whose tentative allocation list is walked
 * for an overlapping, owned, not-already-available range that the
 * request is allowed to share.
 *
 * @param[in,out] ArbState
 * The allocation state of the requirement. On success, Start and
 * End receive the requested window and the range attributes are
 * tagged ARBITER_RANGE_SHARED_DRIVER for a driver-exclusive
 * requirement.
 *
 * @return
 * Returns TRUE if the conflicting range may be shared with the
 * requester, FALSE if the conflict is real.
 *
 * @remarks
 * "DriverExclusive" excludes only OTHER drivers: the SAME driver
 * may share the resource across its devices, and two
 * root-enumerated ("ROOT") devices may share it. This is how a
 * device claims a resource the HAL/firmware reports for the same
 * hardware Example: the ports the kernel debugger reserves, which
 * the HAL marks DriverExclusive.
 */
CODE_SEG("PAGE")
static
BOOLEAN
ArbpShareDriverExclusive(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    PARBITER_LIST_ENTRY Entry = ArbState->Entry;
    PDEVICE_OBJECT Requester;
    RTL_RANGE_LIST_ITERATOR Iterator;
    PRTL_RANGE Range;
    BOOLEAN RequesterIsRoot;

    PAGED_CODE();

    if (Entry == NULL || Entry->PhysicalDeviceObject == NULL ||
        ArbState->CurrentAlternative == NULL)
    {
        return FALSE;
    }

    Requester = Entry->PhysicalDeviceObject;
    RequesterIsRoot = ArbpIsRootEnumerated(Requester);

    if (!NT_SUCCESS(RtlGetFirstRange(Arbiter->PossibleAllocation, &Iterator, &Range)))
        return FALSE;

    while (Range != NULL)
    {
        /*
         * Candidate: overlaps the requested window, is not already made available
         * by attribute, and either the request or the range is driver-exclusive.
         */
        if (Range->Start <= ArbState->CurrentMaximum &&
            Range->End >= ArbState->CurrentMinimum &&
            !(Range->Attributes & ArbState->RangeAvailableAttributes) &&
            (ArbState->CurrentAlternative->Descriptor->ShareDisposition == CmResourceShareDriverExclusive ||
             (Range->Attributes & ARBITER_RANGE_SHARED_DRIVER)) &&
            Range->Owner != NULL)
        {
            PDEVICE_OBJECT Owner = (PDEVICE_OBJECT)Range->Owner;
            BOOLEAN Share = FALSE;

            /* Two root-enumerated devices may share; else only a shared driver. */
            if (RequesterIsRoot && ArbpIsRootEnumerated(Owner))
                Share = TRUE;
            else if (ArbpSharesDriverStack(Requester, Owner))
                Share = TRUE;

            if (Share)
            {
                ArbState->Start = ArbState->CurrentMinimum;
                ArbState->End = ArbState->CurrentMaximum;
                if (ArbState->CurrentAlternative->Descriptor->ShareDisposition ==
                    CmResourceShareDriverExclusive)
                {
                    ArbState->RangeAttributes |= ARBITER_RANGE_SHARED_DRIVER;
                }
                return TRUE;
            }
        }

        if (!NT_SUCCESS(RtlGetNextRange(&Iterator, &Range, TRUE)))
            break;
    }

    return FALSE;
}

/**
 * @brief
 * Hands a device back its own already routed IRQ instead of
 * searching for a fresh one, on legacy-PIC / no-ACPI interrupt
 * routing setups.
 *
 * @param[in] Arbiter
 * The arbiter instance. The routine is a no-op for every resource
 * type other than CmResourceTypeInterrupt.
 *
 * @param[in,out] ArbState
 * The allocation state of the requirement. On success, Start and
 * End receive the vector this device already owns in the committed
 * allocation list.
 *
 * @return
 * Returns TRUE if an owned vector inside the requested window was
 * found and reused, FALSE otherwise.
 *
 * @remarks
 * pci.sys emits line-based interrupt requirement of
 * (MinimumVector 0, MaximumVector 0xFFFFFFFF) which expects an upstream
 * ACPI _PRT arbiter to clamp it to the routed GSIV.
 * With no ACPI the root IRQ arbiter is the only one in the tree,
 * and RtlFindRange searches top-down so a loose window resolves to 0xFFFFFFFF.
 * But the BIOS already handled each device's IRQ which pci.sys reports
 * as the device's boot config; the boot reservation recorded it as
 * a [Vector, Vector] range owned by this PDO in the committed
 * list, and every later commit re-records the assigned vector the
 * same way. Reusing that vector keeps the device on the interrupt
 * the firmware wired it to.
 */
CODE_SEG("PAGE")
static
BOOLEAN
ArbpReuseOwnedInterrupt(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    PARBITER_LIST_ENTRY Entry = ArbState->Entry;
    PARBITER_ALTERNATIVE Alternative = ArbState->CurrentAlternative;
    RTL_RANGE_LIST_ITERATOR Iterator;
    PRTL_RANGE Range;

    PAGED_CODE();

    if (Arbiter->ResourceType != CmResourceTypeInterrupt)
        return FALSE;

    if (Entry == NULL || Entry->PhysicalDeviceObject == NULL || Alternative == NULL)
        return FALSE;

    if (!NT_SUCCESS(RtlGetFirstRange(Arbiter->Allocation, &Iterator, &Range)))
        return FALSE;

    while (Range != NULL)
    {
        if ((PDEVICE_OBJECT)Range->Owner == Entry->PhysicalDeviceObject &&
            Range->Start >= ArbState->CurrentMinimum &&
            Range->Start <= ArbState->CurrentMaximum &&
            Range->End <= ArbState->CurrentMaximum &&
            (Range->End - Range->Start + 1) >= Alternative->Length)
        {
            ArbState->Start = Range->Start;
            ArbState->End = Range->Start + Alternative->Length - 1;
            return TRUE;
        }

        if (!NT_SUCCESS(RtlGetNextRange(&Iterator, &Range, TRUE)))
            break;
    }

    return FALSE;
}

/**
 * @brief
 * Moves the working window to the next candidate range, walking
 * the entry's alternatives in priority order across the arbiter's
 * ordering list.
 *
 * @param[in] Arbiter
 * The arbiter instance whose ordering list supplies the candidate
 * windows.
 *
 * @param[in,out] ArbState
 * The allocation state of the entry being placed. On success,
 * CurrentMinimum, CurrentMaximum and CurrentAlternative describe
 * the next window to search; the window is pre-trimmed so an
 * aligned allocation of the required length fits inside it.
 *
 * @return
 * Returns TRUE if a new candidate window was produced, FALSE once
 * every alternative is exhausted.
 */
CODE_SEG("PAGE")
BOOLEAN
NTAPI
ArbiterLibGetNextAllocationRange(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    PARBITER_ALTERNATIVE Alternative;
    PARBITER_ALTERNATIVE Lowest;
    UINT64 Minimum, Maximum;

    PAGED_CODE();

    if (ArbState->AlternativeCount == 0)
        return FALSE;

    for (;;)
    {
        /* Advance the alternative we last worked on, or seed all on first entry. */
        if (ArbState->CurrentAlternative != NULL)
        {
            ArbpWritePriority(Arbiter, ArbState->CurrentAlternative);
        }
        else
        {
            for (Alternative = ArbState->Alternatives;
                 Alternative < &ArbState->Alternatives[ArbState->AlternativeCount];
                 ++Alternative)
            {
                Alternative->Priority = ARBITER_PRIORITY_NULL;
                ArbpWritePriority(Arbiter, Alternative);
            }
        }

        /* Pick the best (lowest-priority) alternative. */
        Lowest = ArbState->Alternatives;
        for (Alternative = ArbState->Alternatives + 1;
             Alternative < &ArbState->Alternatives[ArbState->AlternativeCount];
             ++Alternative)
        {
            if (Alternative->Priority < Lowest->Priority)
                Lowest = Alternative;
        }

        if (Lowest->Priority == ARBITER_PRIORITY_EXHAUSTED)
            return FALSE;

        if (Lowest->Priority == ARBITER_PRIORITY_RESERVED ||
            Lowest->Priority == ARBITER_PRIORITY_PREFERRED_RESERVED)
        {
            /* Final pass: the whole requirement window. */
            Minimum = Lowest->Minimum;
            Maximum = Lowest->Maximum;
        }
        else
        {
            PARBITER_ORDERING Ordering;
            ULONG Index = (Lowest->Priority < 0) ? (ULONG)(-(Lowest->Priority + 1))
                                                 : (ULONG)(Lowest->Priority - 1);
            if (Index >= Arbiter->OrderingList.Count)
            {
                Lowest->Priority = ARBITER_PRIORITY_EXHAUSTED;
                continue;
            }
            Ordering = &Arbiter->OrderingList.Orderings[Index];
            Minimum = max(Lowest->Minimum, Ordering->Start);
            Maximum = min(Lowest->Maximum, Ordering->End);
        }

        /*
         * Trim the window so an aligned allocation of the required length is
         * possible; skip the window entirely if it cannot hold one.
         */
        if (Lowest->Length != 0)
        {
            UINT64 Alignment = Lowest->Alignment ? Lowest->Alignment : 1;
            UINT64 LengthMinusOne = Lowest->Length - 1;
            UINT64 AlignedMax;

            Minimum += Alignment - 1;
            Minimum -= Minimum % Alignment;

            if (Minimum > Maximum || LengthMinusOne > Maximum - Minimum)
            {
                ArbState->CurrentAlternative = Lowest;  /* consume this priority */
                continue;
            }

            AlignedMax = Maximum - LengthMinusOne;
            AlignedMax -= AlignedMax % Alignment;
            if (AlignedMax < Minimum)
            {
                ArbState->CurrentAlternative = Lowest;  /* no aligned start fits */
                continue;
            }
            Maximum = AlignedMax + LengthMinusOne;
        }
        else
        {
            Minimum = Lowest->Minimum;
            Maximum = Lowest->Maximum;
        }

        if (Minimum != ArbState->CurrentMinimum ||
            Maximum != ArbState->CurrentMaximum ||
            ArbState->CurrentAlternative != Lowest)
        {
            ArbState->CurrentMinimum = Minimum;
            ArbState->CurrentMaximum = Maximum;
            ArbState->CurrentAlternative = Lowest;
            return TRUE;
        }

        ArbState->CurrentAlternative = Lowest;
    }
}

/**
 * @brief
 * Finds a free range of the current candidate window in the
 * arbiter's tentative allocation list.
 *
 * @param[in] Arbiter
 * The arbiter instance whose PossibleAllocation list is searched.
 *
 * @param[in,out] ArbState
 * The allocation state of the entry being placed. On success,
 * Start and End receive the chosen window.
 *
 * @return
 * Returns TRUE if a placement was found,FALSE if the window cannot
 * satisfy the requirement.
 *
 * @remarks
 * Legacy requests treat boot-allocated ranges as available. When
 * RtlFindRange reports a conflict, a driver-exclusive requirement
 * may still share the range (ArbpShareDriverExclusive), and
 * failing that the arbiter's OverrideConflict callback gets a
 * last-chance override. this is how a device is re-assigned its own
 * boot configuration.
 */
CODE_SEG("PAGE")
BOOLEAN
NTAPI
ArbiterLibFindSuitableRange(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    PARBITER_ALTERNATIVE Alternative = ArbState->CurrentAlternative;
    ULONG Flags = 0;
    NTSTATUS Status;

    PAGED_CODE();

    if (Alternative == NULL)
        return FALSE;

    if (ArbState->CurrentMinimum > ArbState->CurrentMaximum)
        return FALSE;

    if (Alternative->Length == 0)
    {
        ArbState->Start = ArbState->CurrentMinimum;
        ArbState->End = ArbState->CurrentMinimum;
        return TRUE;
    }

    /*
     * Interrupt retention: give the device back its firmware-routed vector rather
     * than letting the top-down search pick an untranslatable one
     */
    if (ArbpReuseOwnedInterrupt(Arbiter, ArbState))
        return TRUE;

    /* Legacy requests consider preallocated (boot) ranges available. */
    if (ArbState->Entry != NULL &&
        (ArbState->Entry->RequestSource == ArbiterRequestLegacyReported ||
         ArbState->Entry->RequestSource == ArbiterRequestLegacyAssigned))
    {
        ArbState->RangeAvailableAttributes |= ARBITER_RANGE_BOOT_ALLOCATED;
    }

    if (ArbState->Flags & ARBITER_STATE_FLAG_NULL_CONFLICT_OK)
        Flags |= RTL_RANGE_LIST_NULL_CONFLICT_OK;
    if (Alternative->Flags & ARBITER_ALTERNATIVE_FLAG_SHARED)
        Flags |= RTL_RANGE_LIST_SHARED_OK;

    Status = RtlFindRange(Arbiter->PossibleAllocation,
                          ArbState->CurrentMinimum,
                          ArbState->CurrentMaximum,
                          Alternative->Length,
                          max(Alternative->Alignment, 1),
                          Flags,
                          ArbState->RangeAvailableAttributes,
                          Arbiter->ConflictCallbackContext,
                          Arbiter->ConflictCallback,
                          &ArbState->Start);
    if (!NT_SUCCESS(Status))
    {
        /*
         * The window is occupied.  A CmResourceShareDriverExclusive requirement
         * can still succeed by sharing the conflicting range with the same driver
         * or another root-enumerated device
         *
         * This matters a lot because HAL reverses quite a bit and marks it this.
         * This mechanism is how Windows "internally allows this".
         */
        if (ArbpShareDriverExclusive(Arbiter, ArbState))
            return TRUE;
        if (Arbiter->OverrideConflict != NULL &&
            Arbiter->OverrideConflict(Arbiter, ArbState))
        {
            return TRUE;
        }
        return FALSE;
    }

    ArbState->End = ArbState->Start + Alternative->Length - 1;
    return TRUE;
}

/**
 * @brief
 * Records the chosen placement in the arbiter's tentative
 * allocation list, owned by the requesting device.
 *
 * @param[in] Arbiter
 * The arbiter instance whose PossibleAllocation list receives
 * the range.
 *
 * @param[in,out] ArbState
 * The allocation state whose Start, End and RangeAttributes
 * describe the placement. The range is owned by the entry's
 * physical device object.
 *
 * @remarks
 * ADD_IF_CONFLICT is required because override solutions
 * intentionally overlap existing ranges.
 */
CODE_SEG("PAGE")
VOID
NTAPI
ArbiterLibAddAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    ULONG Flags = RTL_RANGE_LIST_ADD_IF_CONFLICT;

    PAGED_CODE();

    if (ArbState->CurrentAlternative != NULL &&
        (ArbState->CurrentAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_SHARED))
    {
        Flags |= RTL_RANGE_LIST_ADD_SHARED;
    }

    RtlAddRange(Arbiter->PossibleAllocation,
                ArbState->Start,
                ArbState->End,
                ArbState->RangeAttributes,
                Flags,
                NULL,
                ArbState->Entry ? ArbState->Entry->PhysicalDeviceObject : NULL);
}

/**
 * @brief
 * Undoes the last AddAllocation performed for this entry.
 *
 * @param[in] Arbiter
 * The arbiter instance whose PossibleAllocation list the tentative
 * range is deleted from.
 *
 * @param[in,out] ArbState
 * The allocation state whose Start and End describe the placement
 * being removed sanity checked by the entry's physical device object.
 */
CODE_SEG("PAGE")
VOID
NTAPI
ArbiterLibBacktrackAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    PAGED_CODE();

    RtlDeleteRange(Arbiter->PossibleAllocation,
                   ArbState->Start,
                   ArbState->End,
                   ArbState->Entry ? ArbState->Entry->PhysicalDeviceObject : NULL);
}
