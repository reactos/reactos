/*
 * PROJECT:     ReactOS Arbitration Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Action dispatch and arbiter startup
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntifs.h>
#include <ndk/rtlfuncs.h>
#include "arbiter.h"

#define NDEBUG
#include <debug.h>

/* DISPATCH *******************************************************************/

/**
 * @brief
 * Acquires the arbiter's per-instance lock: one arbitration at a
 * time.
 *
 * @param[in] Arbiter
 * The arbiter instance to lock.
 *
 * @remarks
 * NT brackets the hold with KeEnterCriticalRegion so the owning
 * thread cannot be suspended holding the lock. This library links
 * into ntoskrnl itself, where ReactOS provides that routine only
 * as an internal macro (internal/ke_x.h) with no linkable symbol,
 * so the bracket cannot be expressed here portably.
 */
CODE_SEG("PAGE")
static
VOID
ArbpAcquireLock(
    _In_ PARBITER_INSTANCE Arbiter)
{
    PAGED_CODE();
    KeWaitForSingleObject(Arbiter->MutexEvent, Executive, KernelMode, FALSE, NULL);
}

/**
 * @brief
 * Releases the arbiter's per-instance lock.
 *
 * @param[in] Arbiter
 * The arbiter instance to unlock.
 */
CODE_SEG("PAGE")
static
VOID
ArbpReleaseLock(
    _In_ PARBITER_INSTANCE Arbiter)
{
    PAGED_CODE();
    KeSetEvent(Arbiter->MutexEvent, IO_NO_INCREMENT, FALSE);
}

/**
 * @brief
 * The QueryArbitrate action default: reports that this arbiter is
 * willing to arbitrate the given list.
 *
 * @param[in] Arbiter
 * The arbiter instance being queried.
 *
 * @param[in,out] Parameters
 * The action parameters carrying the arbitration list (pre-Vista
 * builds receive the list directly).
 *
 * @return
 * Returns STATUS_SUCCESS.
 */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
#if (NTDDI_VERSION >= NTDDI_VISTA)
ArbiterLibQueryArbitrate(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_QUERY_ARBITRATE_PARAMETERS Parameters)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Arbiter);
    UNREFERENCED_PARAMETER(Parameters);
    return STATUS_SUCCESS;
}
#else
ArbiterLibQueryArbitrate(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PLIST_ENTRY ArbitrationList)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Arbiter);
    UNREFERENCED_PARAMETER(ArbitrationList);
    return STATUS_SUCCESS;
}
#endif

/**
 * @brief
 * The AddReserved action default: nothing to reserve.
 *
 * @param[in] Arbiter
 * The arbiter instance the reservation was aimed at.
 *
 * @param[in,out] Parameters
 * The action parameters (pre-Vista builds receive the requirement
 * and resource descriptors directly).
 *
 * @return
 * Returns STATUS_SUCCESS.
 */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
#if (NTDDI_VERSION >= NTDDI_VISTA)
ArbiterLibAddReserved(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ADD_RESERVED_PARAMETERS Parameters)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Arbiter);
    UNREFERENCED_PARAMETER(Parameters);
    return STATUS_SUCCESS;
}
#else
ArbiterLibAddReserved(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_opt_ PIO_RESOURCE_DESCRIPTOR Requirement,
    _In_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Arbiter);
    UNREFERENCED_PARAMETER(Requirement);
    UNREFERENCED_PARAMETER(Resource);
    return STATUS_SUCCESS;
}
#endif

/**
 * @brief
 * The single ARBITER_ACTION dispatcher every exported
 * ARBITER_INTERFACE points its ArbiterHandler at; Context is the
 * ARBITER_INSTANCE.
 *
 * @param[in] Context
 * The ARBITER_INSTANCE the interface was created for.
 *
 * @param[in] Action
 * The ARBITER_ACTION to perform.
 *
 * @param[in,out] Parameters
 * The parameters union matching the action.
 *
 * @return
 * Returns the dispatched action's status,
 * STATUS_NOT_IMPLEMENTED for a known but unrouted action, or
 * STATUS_INVALID_PARAMETER for an unrecognised one.
 *
 * @remarks
 * The call runs under the instance lock, so a Test and its later
 * Commit or Rollback apply to the same transaction. On Vista+ the
 * transaction event tracks that window: Test and Retest arm it,
 * Commit and Rollback resolve it and wake waiters.
 */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibHandler(
    _In_ PVOID Context,
    _In_ ARBITER_ACTION Action,
    _Inout_ PARBITER_PARAMETERS Parameters)
{
    PARBITER_INSTANCE Arbiter = (PARBITER_INSTANCE)Context;
    NTSTATUS Status;

    PAGED_CODE();

    ArbpAcquireLock(Arbiter);

    switch (Action)
    {
        case ArbiterActionTestAllocation:
#if (NTDDI_VERSION >= NTDDI_VISTA)
            Status = Arbiter->TestAllocation(Arbiter, &Parameters->Parameters.TestAllocation);
#else
            Status = Arbiter->TestAllocation(
                Arbiter,
                Parameters->Parameters.TestAllocation.ArbitrationList);
#endif
            break;

        case ArbiterActionRetestAllocation:
#if (NTDDI_VERSION >= NTDDI_VISTA)
            Status = Arbiter->RetestAllocation(Arbiter, &Parameters->Parameters.RetestAllocation);
#else
            Status = Arbiter->RetestAllocation(
                Arbiter,
                Parameters->Parameters.RetestAllocation.ArbitrationList);
#endif
            break;

        case ArbiterActionCommitAllocation:
            Status = Arbiter->CommitAllocation(Arbiter);
            break;

        case ArbiterActionRollbackAllocation:
            Status = Arbiter->RollbackAllocation(Arbiter);
            break;

        case ArbiterActionBootAllocation:
#if (NTDDI_VERSION >= NTDDI_VISTA)
            Status = Arbiter->BootAllocation(Arbiter, &Parameters->Parameters.BootAllocation);
#else
            Status = Arbiter->BootAllocation(
                Arbiter,
                Parameters->Parameters.BootAllocation.ArbitrationList);
#endif
            break;

        case ArbiterActionQueryConflict:
#if (NTDDI_VERSION >= NTDDI_VISTA)
            Status = Arbiter->QueryConflict(Arbiter, &Parameters->Parameters.QueryConflict);
#else
            Status = Arbiter->QueryConflict(
                Arbiter,
                Parameters->Parameters.QueryConflict.PhysicalDeviceObject,
                Parameters->Parameters.QueryConflict.ConflictingResource,
                Parameters->Parameters.QueryConflict.ConflictCount,
                Parameters->Parameters.QueryConflict.Conflicts);
#endif
            break;

        case ArbiterActionQueryArbitrate:
#if (NTDDI_VERSION >= NTDDI_VISTA)
            Status = Arbiter->QueryArbitrate(Arbiter, &Parameters->Parameters.QueryArbitrate);
#else
            Status = Arbiter->QueryArbitrate(
                Arbiter,
                Parameters->Parameters.QueryArbitrate.ArbitrationList);
#endif
            break;

        /*
         * Actions the interface defines but this arbiter does not carry
         * out.  AddReserved is one: the instance slot exists and is
         * defaulted, but nothing routes to it.
         */
        case ArbiterActionQueryAllocatedResources:
        case ArbiterActionWriteReservedResources:
        case ArbiterActionAddReserved:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

#if (NTDDI_VERSION >= NTDDI_VISTA)
    /* Track the tested-but-not-yet-committed window for transaction waiters. */
    if (NT_SUCCESS(Status))
    {
        if (Action == ArbiterActionTestAllocation ||
            Action == ArbiterActionRetestAllocation)
        {
            Arbiter->TransactionInProgress = TRUE;
            KeClearEvent(Arbiter->TransactionEvent);
        }
        else if (Action == ArbiterActionCommitAllocation ||
                 Action == ArbiterActionRollbackAllocation)
        {
            Arbiter->TransactionInProgress = FALSE;
            KeSetEvent(Arbiter->TransactionEvent, IO_NO_INCREMENT, FALSE);
        }
    }
#endif

    ArbpReleaseLock(Arbiter);
    return Status;
}

/* STARTUP ********************************************************************/

/**
 * @brief
 * Seeds a range list with the resources the bus actually decodes:
 * every descriptor of the arbiter's resource type becomes one
 * unowned range.
 *
 * @param[in] Arbiter
 * The arbiter instance whose UnpackResource decodes the
 * descriptors and whose resource type filters them.
 *
 * @param[in] ResourceCount
 * The number of descriptors in Resources.
 *
 * @param[in] Resources
 * The CM partial resource descriptors describing the decoded
 * windows.
 *
 * @param[in,out] RangeList
 * The range list to rebuild from the descriptors.
 *
 * @return
 * Returns STATUS_SUCCESS.
 */
CODE_SEG("PAGE")
static
NTSTATUS
ArbpSeedRangeList(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ ULONG ResourceCount,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Resources,
    _Inout_ PRTL_RANGE_LIST RangeList)
{
    ULONG Index;

    PAGED_CODE();

    RtlFreeRangeList(RangeList);
    RtlInitializeRangeList(RangeList);

    if (Arbiter->UnpackResource == NULL)
        return STATUS_SUCCESS;

    for (Index = 0; Index < ResourceCount; ++Index)
    {
        UINT64 Start, Length;

        if (Resources[Index].Type != (UCHAR)Arbiter->ResourceType)
            continue;

        Arbiter->UnpackResource(&Resources[Index], &Start, &Length);
        if (Length != 0)
        {
            RtlAddRange(RangeList, Start, Start + Length - 1, 0,
                        RTL_RANGE_LIST_ADD_IF_CONFLICT, NULL, NULL);
        }
    }

    return STATUS_SUCCESS;
}

#if (NTDDI_VERSION >= NTDDI_VISTA)
/**
 * @brief
 * The InitializeRangeList callback default.
 *
 * @param[in] Arbiter
 * The arbiter instance the range list belongs to.
 *
 * @param[in] ResourceCount
 * The number of descriptors in Resources.
 *
 * @param[in] Resources
 * The CM partial resource descriptors describing the decoded
 * windows.
 *
 * @param[in,out] RangeList
 * The range list to rebuild from the descriptors.
 *
 * @return
 * Returns STATUS_SUCCESS.
 */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibInitializeRangeList(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ ULONG ResourceCount,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Resources,
    _Inout_ PRTL_RANGE_LIST RangeList)
{
    PAGED_CODE();
    return ArbpSeedRangeList(Arbiter, ResourceCount, Resources, RangeList);
}
#endif

/**
 * @brief
 * The StartArbiter action default: seeds the committed allocation
 * with the windows the bus decodes, so arbitration starts from
 * the hardware's real extent.
 *
 * @param[in] Arbiter
 * The arbiter instance being started.
 *
 * @param[in] StartResources
 * The bus's decoded resources; NULL or empty means nothing to
 * seed.
 *
 * @return
 * Returns the range-list initialization status.
 */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibStartArbiter(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PCM_RESOURCE_LIST StartResources)
{
    PCM_FULL_RESOURCE_DESCRIPTOR Full;

    PAGED_CODE();

    if (StartResources == NULL || StartResources->Count == 0)
        return STATUS_SUCCESS;

    Full = &StartResources->List[0];

#if (NTDDI_VERSION >= NTDDI_VISTA)
    /* Vista+ routes through the slot, so an arbiter may override the seeding. */
    return Arbiter->InitializeRangeList(Arbiter,
                                        Full->PartialResourceList.Count,
                                        Full->PartialResourceList.PartialDescriptors,
                                        Arbiter->Allocation);
#else
    return ArbpSeedRangeList(Arbiter,
                             Full->PartialResourceList.Count,
                             Full->PartialResourceList.PartialDescriptors,
                             Arbiter->Allocation);
#endif
}

/* EXTERNAL RESERVATIONS ******************************************************/

/**
 * @brief
 * Commits a single [Start, End] range into the arbiter as an
 * ordinary allocation owned by Owner.
 *
 * @param[in] Arbiter
 * The arbiter instance to reserve the range in.
 *
 * @param[in] Start
 * The first address (or unit) of the range.
 *
 * @param[in] End
 * The last address (or unit) of the range, inclusive.
 *
 * @param[in] Owner
 * The owning device object, or NULL for a system reservation.
 *
 * @param[in] Shared
 * TRUE if the range may be shared with other shareable owners.
 *
 * @remarks
 * Used to record resources assigned outside the PnP transaction
 * (e.g. a legacy device's IoAssignResources), so later arbitration
 * sees them. Unlike a boot reservation this carries no
 * BOOT_ALLOCATED attribute, so it is not treated as reclaimable.
 */
CODE_SEG("PAGE")
VOID
NTAPI
ArbiterLibReserveRange(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ ULONGLONG Start,
    _In_ ULONGLONG End,
    _In_opt_ PVOID Owner,
    _In_ BOOLEAN Shared)
{
    PAGED_CODE();

    ArbpAcquireLock(Arbiter);
    RtlAddRange(Arbiter->Allocation, Start, End, 0,
                RTL_RANGE_LIST_ADD_IF_CONFLICT | (Shared ? RTL_RANGE_LIST_ADD_SHARED : 0),
                NULL, Owner);
    ArbpReleaseLock(Arbiter);
}

/**
 * @brief
 * Releases every committed range owned by Owner, returning the
 * resources it held to the free pool.
 *
 * @param[in] Arbiter
 * The arbiter instance to release the ranges from.
 *
 * @param[in] Owner
 * The owning device object (a removed device's PDO).
 *
 * @remarks
 * Without this a torn-down device's ranges stay reserved forever
 * and later devices see phantom conflicts.
 */
CODE_SEG("PAGE")
VOID
NTAPI
ArbiterLibReleaseResources(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PVOID Owner)
{
    PAGED_CODE();

    ArbpAcquireLock(Arbiter);
    RtlDeleteOwnersRanges(Arbiter->Allocation, Owner);
    ArbpReleaseLock(Arbiter);
}
