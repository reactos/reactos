/*
 * PROJECT:     ReactOS Arbitrartion Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Generic Arbiter Library
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntifs.h>
#include <ndk/rtlfuncs.h>
#include "arbiter.h"

#define NDEBUG
#include <debug.h>

#define ARBITER_SIG  'sbrA'
#define TAG_ARBITER  'ibrA'

/* 
 * TODO: ArbTestAllocation-ArbQueryConflict have some signature rewrites
 * that need to happen when we retarget to vista.
 */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbTestAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PLIST_ENTRY ArbitrationList)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbRetestAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PLIST_ENTRY ArbitrationList)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbBootAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PLIST_ENTRY ArbitrationList)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbAddReserved(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_opt_ PIO_RESOURCE_DESCRIPTOR Requirement,
    _In_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbQueryConflict(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _In_ PIO_RESOURCE_DESCRIPTOR ConflictingResource,
    _Out_ PULONG ConflictCount,
    _Out_ PARBITER_CONFLICT_INFO *Conflicts)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

#if (NTDDI_VERSION >= NTDDI_VISTA)
CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbInitializeRangeList(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ ULONG ResourceCount,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Resources,
    _Inout_ PRTL_RANGE_LIST RangeList)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
#endif

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbCommitAllocation(
    _In_ PARBITER_INSTANCE Arbiter)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbRollbackAllocation(
    _In_ PARBITER_INSTANCE Arbiter)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbStartArbiter(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PCM_RESOURCE_LIST StartResources)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbPreprocessEntry(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbAllocateEntry(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbSortArbitrationList(
    _Inout_ PLIST_ENTRY ArbitrationList)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
ArbGetNextAllocationRange(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
ArbFindSuitableRange(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return FALSE;
}

CODE_SEG("PAGE")
VOID
NTAPI
ArbAddAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    PAGED_CODE();

    UNIMPLEMENTED;
}

CODE_SEG("PAGE")
VOID
NTAPI
ArbBacktrackAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    PAGED_CODE();

    UNIMPLEMENTED;
}

CODE_SEG("PAGE")
VOID
NTAPI
ArbConfirmAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    PAGED_CODE();

    UNIMPLEMENTED;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
ArbOverrideConflict(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return FALSE;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbArbiterHandler(
    _In_ PVOID Context,
    _In_ ARBITER_ACTION Action,
    _Inout_ PARBITER_PARAMETERS Parameters)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbInitializeOrderingList(
    _Out_ PARBITER_ORDERING_LIST OrderingList)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
VOID
NTAPI
ArbFreeOrderingList(
    _Inout_ PARBITER_ORDERING_LIST OrderingList)
{
    PAGED_CODE();

    UNIMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbCopyOrderingList(
    _Out_ PARBITER_ORDERING_LIST Destination,
    _In_ PARBITER_ORDERING_LIST Source)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbAddOrdering(
    _Inout_ PARBITER_ORDERING_LIST OrderingList,
    _In_ UINT64 Start,
    _In_ UINT64 End)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbPruneOrdering(
    _Inout_ PARBITER_ORDERING_LIST OrderingList,
    _In_ UINT64 Start,
    _In_ UINT64 End)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbBuildAssignmentOrdering(
    _Inout_ PARBITER_INSTANCE Arbiter,
    _In_ PCWSTR AllocationOrderName,
    _In_ PCWSTR ReservedResourcesName,
    _In_opt_ PARB_TRANSLATE_ORDERING TranslateOrderingFunction)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
VOID
NTAPI
ArbDeleteArbiterInstance(
    _In_ PARBITER_INSTANCE Arbiter)
{
    PAGED_CODE();

    if (Arbiter->PossibleAllocation)
    {
        RtlFreeRangeList(Arbiter->PossibleAllocation);
        ExFreePoolWithTag(Arbiter->PossibleAllocation, TAG_ARBITER);
        Arbiter->PossibleAllocation = NULL;
    }

    if (Arbiter->Allocation)
    {
        RtlFreeRangeList(Arbiter->Allocation);
        ExFreePoolWithTag(Arbiter->Allocation, TAG_ARBITER);
        Arbiter->Allocation = NULL;
    }

    if (Arbiter->AllocationStack)
    {
        ExFreePoolWithTag(Arbiter->AllocationStack, TAG_ARBITER);
        Arbiter->AllocationStack = NULL;
        Arbiter->AllocationStackMaxSize = 0;
    }

#if (NTDDI_VERSION >= NTDDI_VISTA)
    if (Arbiter->TransactionEvent)
    {
        ExFreePoolWithTag(Arbiter->TransactionEvent, TAG_ARBITER);
        Arbiter->TransactionEvent = NULL;
    }
#endif

    if (Arbiter->MutexEvent)
    {
        ExFreePoolWithTag(Arbiter->MutexEvent, TAG_ARBITER);
        Arbiter->MutexEvent = NULL;
    }
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbInitializeArbiterInstance(
    _Inout_ PARBITER_INSTANCE Arbiter,
    _In_ PDEVICE_OBJECT BusDeviceObject,
    _In_ CM_RESOURCE_TYPE ResourceType,
    _In_ PCWSTR ArbiterName,
    _In_ PCWSTR OrderName,
    _In_ PARB_TRANSLATE_ORDERING TranslateOrderingFunction)
{
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("ArbInitializeArbiterInstance: '%S'\n", ArbiterName);

    ASSERT(Arbiter->UnpackRequirement != NULL);
    ASSERT(Arbiter->PackResource != NULL);
    ASSERT(Arbiter->UnpackResource != NULL);
    ASSERT(Arbiter->MutexEvent == NULL);
    ASSERT(Arbiter->Allocation == NULL);
    ASSERT(Arbiter->PossibleAllocation == NULL);
    ASSERT(Arbiter->AllocationStack == NULL);

    Arbiter->Signature = ARBITER_SIG;
    Arbiter->BusDeviceObject = BusDeviceObject;
    Arbiter->Name = ArbiterName;
    Arbiter->ResourceType = ResourceType;
    Arbiter->TransactionInProgress = FALSE;
#if (NTDDI_VERSION >= NTDDI_VISTA)
    Arbiter->OrderingName = OrderName;
#endif

    /* The per-instance lock: a signaled synchronization event used as a mutex. */
    Arbiter->MutexEvent = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG_ARBITER);
    if (!Arbiter->MutexEvent)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Failure;
    }
    KeInitializeEvent(Arbiter->MutexEvent, SynchronizationEvent, TRUE);

#if (NTDDI_VERSION >= NTDDI_VISTA)
    /* Vista+: a notification event exposing whether a Test is outstanding. */
    Arbiter->TransactionEvent = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG_ARBITER);
    if (!Arbiter->TransactionEvent)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Failure;
    }
    KeInitializeEvent(Arbiter->TransactionEvent, NotificationEvent, TRUE);
#endif

    Arbiter->AllocationStack = ExAllocatePoolWithTag(PagedPool, PAGE_SIZE, TAG_ARBITER);
    if (!Arbiter->AllocationStack)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Failure;
    }
    Arbiter->AllocationStackMaxSize = PAGE_SIZE;
    Arbiter->Allocation = ExAllocatePoolWithTag(PagedPool, sizeof(RTL_RANGE_LIST), TAG_ARBITER);
    if (!Arbiter->Allocation)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Failure;
    }
    RtlInitializeRangeList(Arbiter->Allocation);

    Arbiter->PossibleAllocation = ExAllocatePoolWithTag(PagedPool, sizeof(RTL_RANGE_LIST), TAG_ARBITER);
    if (!Arbiter->PossibleAllocation)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Failure;
    }
    RtlInitializeRangeList(Arbiter->PossibleAllocation);

    if (!Arbiter->TestAllocation)
        Arbiter->TestAllocation = ArbTestAllocation;
    if (!Arbiter->RetestAllocation)
        Arbiter->RetestAllocation = ArbRetestAllocation;
    if (!Arbiter->CommitAllocation)
        Arbiter->CommitAllocation = ArbCommitAllocation;
    if (!Arbiter->RollbackAllocation)
        Arbiter->RollbackAllocation = ArbRollbackAllocation;
    if (!Arbiter->BootAllocation)
        Arbiter->BootAllocation = ArbBootAllocation;
    if (!Arbiter->AddReserved)
        Arbiter->AddReserved = ArbAddReserved;
    if (!Arbiter->QueryConflict)
        Arbiter->QueryConflict = ArbQueryConflict;
    if (!Arbiter->StartArbiter)
        Arbiter->StartArbiter = ArbStartArbiter;
    if (!Arbiter->PreprocessEntry)
        Arbiter->PreprocessEntry = ArbPreprocessEntry;
    if (!Arbiter->AllocateEntry)
        Arbiter->AllocateEntry = ArbAllocateEntry;
    if (!Arbiter->GetNextAllocationRange)
        Arbiter->GetNextAllocationRange = ArbGetNextAllocationRange;
    if (!Arbiter->FindSuitableRange)
        Arbiter->FindSuitableRange = ArbFindSuitableRange;
    if (!Arbiter->AddAllocation)
        Arbiter->AddAllocation = ArbAddAllocation;
    if (!Arbiter->BacktrackAllocation)
        Arbiter->BacktrackAllocation = ArbBacktrackAllocation;
    if (!Arbiter->OverrideConflict)
        Arbiter->OverrideConflict = ArbOverrideConflict;
#if (NTDDI_VERSION >= NTDDI_VISTA)
    if (!Arbiter->InitializeRangeList)
        Arbiter->InitializeRangeList = ArbInitializeRangeList;
#endif

    Status = ArbBuildAssignmentOrdering(Arbiter, OrderName, OrderName, TranslateOrderingFunction);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ArbInitializeArbiterInstance: ArbBuildAssignmentOrdering failed, Status %X\n", Status);
        goto Failure;
    }

    return STATUS_SUCCESS;

Failure:
    DPRINT1("ArbInitializeArbiterInstance: '%S' failed, Status %X\n", ArbiterName, Status);
    ArbDeleteArbiterInstance(Arbiter);
    return Status;
}
