/*
 * PROJECT:     ReactOS Arbitration Library
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

CODE_SEG("PAGE")
NTSTATUS
NTAPI
#if (NTDDI_VERSION >= NTDDI_VISTA)
ArbiterLibBootAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_BOOT_ALLOCATION_PARAMETERS Parameters)
#else
ArbiterLibBootAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PLIST_ENTRY ArbitrationList)
#endif
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
#if (NTDDI_VERSION >= NTDDI_VISTA)
ArbiterLibAddReserved(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ADD_RESERVED_PARAMETERS Parameters)
#else
ArbiterLibAddReserved(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_opt_ PIO_RESOURCE_DESCRIPTOR Requirement,
    _In_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource)
#endif
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
#if (NTDDI_VERSION >= NTDDI_VISTA)
ArbiterLibQueryConflict(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_QUERY_CONFLICT_PARAMETERS Parameters)
#else
ArbiterLibQueryConflict(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _In_ PIO_RESOURCE_DESCRIPTOR ConflictingResource,
    _Out_ PULONG ConflictCount,
    _Out_ PARBITER_CONFLICT_INFO *Conflicts)
#endif
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

#if (NTDDI_VERSION >= NTDDI_VISTA)
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

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
#endif

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibStartArbiter(
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
ArbiterLibPreprocessEntry(
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
ArbiterLibAllocateEntry(
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
ArbiterLibSortArbitrationList(
    _Inout_ PLIST_ENTRY ArbitrationList)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
VOID
NTAPI
ArbiterLibConfirmAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    PAGED_CODE();

    UNIMPLEMENTED;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
ArbiterLibOverrideConflict(
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
ArbiterLibHandler(
    _In_ PVOID Context,
    _In_ ARBITER_ACTION Action,
    _Inout_ PARBITER_PARAMETERS Parameters)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
VOID
NTAPI
ArbiterLibDeleteInstance(
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

    ArbiterLibFreeOrderingList(&Arbiter->OrderingList);
    ArbiterLibFreeOrderingList(&Arbiter->ReservedList);

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
ArbiterLibInitializeInstance(
    _Inout_ PARBITER_INSTANCE Arbiter,
    _In_ PDEVICE_OBJECT BusDeviceObject,
    _In_ CM_RESOURCE_TYPE ResourceType,
    _In_ PCWSTR ArbiterName,
    _In_ PCWSTR OrderName,
    _In_ PARB_TRANSLATE_ORDERING TranslateOrderingFunction)
{
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("ArbiterLibInitializeInstance: '%S'\n", ArbiterName);

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
        Arbiter->TestAllocation = ArbiterLibTestAllocation;
    if (!Arbiter->RetestAllocation)
        Arbiter->RetestAllocation = ArbiterLibRetestAllocation;
    if (!Arbiter->CommitAllocation)
        Arbiter->CommitAllocation = ArbiterLibCommitAllocation;
    if (!Arbiter->RollbackAllocation)
        Arbiter->RollbackAllocation = ArbiterLibRollbackAllocation;
    if (!Arbiter->BootAllocation)
        Arbiter->BootAllocation = ArbiterLibBootAllocation;
    if (!Arbiter->AddReserved)
        Arbiter->AddReserved = ArbiterLibAddReserved;
    if (!Arbiter->QueryConflict)
        Arbiter->QueryConflict = ArbiterLibQueryConflict;
    if (!Arbiter->StartArbiter)
        Arbiter->StartArbiter = ArbiterLibStartArbiter;
    if (!Arbiter->PreprocessEntry)
        Arbiter->PreprocessEntry = ArbiterLibPreprocessEntry;
    if (!Arbiter->AllocateEntry)
        Arbiter->AllocateEntry = ArbiterLibAllocateEntry;
    if (!Arbiter->GetNextAllocationRange)
        Arbiter->GetNextAllocationRange = ArbiterLibGetNextAllocationRange;
    if (!Arbiter->FindSuitableRange)
        Arbiter->FindSuitableRange = ArbiterLibFindSuitableRange;
    if (!Arbiter->AddAllocation)
        Arbiter->AddAllocation = ArbiterLibAddAllocation;
    if (!Arbiter->BacktrackAllocation)
        Arbiter->BacktrackAllocation = ArbiterLibBacktrackAllocation;
    if (!Arbiter->OverrideConflict)
        Arbiter->OverrideConflict = ArbiterLibOverrideConflict;
#if (NTDDI_VERSION >= NTDDI_VISTA)
    if (!Arbiter->InitializeRangeList)
        Arbiter->InitializeRangeList = ArbiterLibInitializeRangeList;
#endif

    Status = ArbiterLibDefaultAssignmentOrdering(Arbiter, OrderName, OrderName, TranslateOrderingFunction);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ArbiterLibInitializeInstance: ArbiterLibDefaultAssignmentOrdering failed, Status %X\n", Status);
        goto Failure;
    }

    return STATUS_SUCCESS;

Failure:
    DPRINT1("ArbiterLibInitializeInstance: '%S' failed, Status %X\n", ArbiterName, Status);
    ArbiterLibDeleteInstance(Arbiter);
    return Status;
}
