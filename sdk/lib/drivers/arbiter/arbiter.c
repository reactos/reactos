/*
 * PROJECT:     ReactOS Kernel&Driver SDK
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware Resources Arbiter Library
 * COPYRIGHT:   Copyright 2020-2022 Vadim Galyant <vgal@rambler.ru>
 */

/* INCLUDES *******************************************************************/

#include <ntifs.h>
#include <ndk/rtlfuncs.h>
#include "arbiter.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* DATA **********************************************************************/

/* FUNCTIONS ******************************************************************/

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbTestAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PLIST_ENTRY ArbitrationList)
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
    _In_ PLIST_ENTRY ArbitrationList)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

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

/* FIXME: the prototype is not correct yet. */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbAddReserved(
    _In_ PARBITER_INSTANCE Arbiter)
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

/* FIXME: the prototype is not correct yet. */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbOverrideConflict(
    _In_ PARBITER_INSTANCE Arbiter)
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
    _In_ PLIST_ENTRY ArbitrationList)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* FIXME: the prototype is not correct yet. */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbQueryConflict(
    _In_ PARBITER_INSTANCE Arbiter)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* FIXME: the prototype is not correct yet. */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbStartArbiter(
    _In_ PARBITER_INSTANCE Arbiter)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbAddOrdering(
    _Out_ PARBITER_ORDERING_LIST OrderList,
    _In_ UINT64 MinimumAddress,
    _In_ UINT64 MaximumAddress)
{
    PARBITER_ORDERING NewOrderings;
    UINT32 NewCountSize;

    PAGED_CODE();

    DPRINT("ArbAddOrdering: OrderList %p, MinimumAddress - %I64X, MaximumAddress - %I64X\n",
           OrderList, MinimumAddress, MaximumAddress);

    if (MaximumAddress < MinimumAddress)
    {
        DPRINT1("ArbAddOrdering: STATUS_INVALID_PARAMETER. [%p] %I64X : %I64X\n", OrderList, MinimumAddress, MaximumAddress);
        return STATUS_INVALID_PARAMETER;
    }

    if (OrderList->Count < OrderList->Maximum)
    {
        /* There is no need to add Orderings. */
        goto Exit;
    }

    /* Add Orderings. */
    NewCountSize = ((OrderList->Count + ARB_ORDERING_LIST_ADD_COUNT) * sizeof(ARBITER_ORDERING));

    NewOrderings = ExAllocatePoolWithTag(PagedPool, NewCountSize, TAG_ARB_ORDERING);
    if (!NewOrderings)
    {
        DPRINT1("ArbAddOrdering: STATUS_INSUFFICIENT_RESOURCES\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (OrderList->Orderings)
    {
        RtlCopyMemory(NewOrderings, OrderList->Orderings, (OrderList->Count * sizeof(ARBITER_ORDERING)));
        ExFreePoolWithTag(OrderList->Orderings, TAG_ARB_ORDERING);
    }

    OrderList->Orderings = NewOrderings;
    OrderList->Maximum += ARB_ORDERING_LIST_ADD_COUNT;

Exit:

    OrderList->Orderings[OrderList->Count].Start = MinimumAddress;
    OrderList->Orderings[OrderList->Count].End = MaximumAddress;

    OrderList->Count++;
    ASSERT(OrderList->Count <= OrderList->Maximum);

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbPruneOrdering(
    _Out_ PARBITER_ORDERING_LIST OrderingList,
    _In_ UINT64 MinimumAddress,
    _In_ UINT64 MaximumAddress)
{
    PARBITER_ORDERING Current;
    PARBITER_ORDERING Orderings;
    PARBITER_ORDERING NewOrderings;
    PARBITER_ORDERING TmpOrderings;
    UINT32 TmpOrderingsSize;
    UINT32 ix;
    USHORT Count;

    PAGED_CODE();

    DPRINT("ArbPruneOrdering: %X, %I64X, %I64X\n", OrderList->Count, MinimumAddress, MaximumAddress);

    ASSERT(OrderList);
    ASSERT(OrderList->Orderings);

    if (MaximumAddress < MinimumAddress)
    {
        DPRINT1("ArbPruneOrdering: STATUS_INVALID_PARAMETER. [%p] %I64X : %I64X\n", OrderList, MinimumAddress, MaximumAddress);
        return STATUS_INVALID_PARAMETER;
    }

    TmpOrderingsSize = (OrderList->Count * (2 * sizeof(ARBITER_ORDERING)) + sizeof(ARBITER_ORDERING));

    TmpOrderings = ExAllocatePoolWithTag(PagedPool, TmpOrderingsSize, TAG_ARB_ORDERING);
    if (!TmpOrderings)
    {
        DPRINT1("ArbPruneOrdering: STATUS_INSUFFICIENT_RESOURCES\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Current = TmpOrderings;
    Orderings = OrderList->Orderings;

    for (ix = 0; ix < OrderList->Count; ix++)
    {
        if (MaximumAddress < Orderings[0].Start ||
            MinimumAddress > Orderings[0].End)
        {
            Current->Start = Orderings[0].Start;
            Current->End = Orderings[0].End;
        }
        else if (MinimumAddress <= Orderings[0].Start)
        {
            if (MaximumAddress >= Orderings[0].End)
            {
                continue;
            }
            else
            {
                Current->Start = (MaximumAddress + 1);
                Current->End = Orderings[0].End;
            }
        }
        else
        {
            if (MaximumAddress >= Orderings[0].End)
            {
                Current->Start = Orderings[0].Start;
                Current->End = (MinimumAddress - 1);
            }
            else
            {
                Current->Start = (MaximumAddress + 1);
                Current->End = Orderings[0].End;

                Current++;

                Current->Start = Orderings[0].Start;
                Current->End = (MinimumAddress - 1);
            }
        }

        Current++;
    }

    Count = (Current - TmpOrderings);
    ASSERT(Count >= 0);
    if (!Count)
    {
        ExFreePoolWithTag(TmpOrderings, TAG_ARB_ORDERING);
        OrderList->Count = Count;
        return STATUS_SUCCESS;
    }

    if (Count > OrderList->Maximum)
    {
        NewOrderings = ExAllocatePoolWithTag(PagedPool, (Count * sizeof(ARBITER_ORDERING)), TAG_ARB_ORDERING);
        if (!NewOrderings)
        {
            if (TmpOrderings)
                ExFreePoolWithTag(TmpOrderings, TAG_ARB_ORDERING);

            DPRINT1("ArbPruneOrdering: STATUS_INSUFFICIENT_RESOURCES\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (OrderList->Orderings)
            ExFreePoolWithTag(OrderList->Orderings, TAG_ARB_ORDERING);

        OrderList->Orderings = NewOrderings;
        OrderList->Maximum = Count;
    }

    RtlCopyMemory(OrderList->Orderings, TmpOrderings, (Count * sizeof(ARBITER_ORDERING)));

    ExFreePoolWithTag(TmpOrderings, TAG_ARB_ORDERING);
    OrderList->Count = Count;

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbInitializeOrderingList(
    _Out_ PARBITER_ORDERING_LIST OrderList)
{
    UINT32 Size;

    PAGED_CODE();
    ASSERT(OrderList);

    OrderList->Count = 0;
    Size = (ARB_ORDERING_LIST_DEFAULT_COUNT * sizeof(ARBITER_ORDERING));

    OrderList->Orderings = ExAllocatePoolWithTag(PagedPool, Size, TAG_ARB_ORDERING);
    if (!OrderList->Orderings)
    {
        DPRINT1("ArbInitializeOrderingList: STATUS_INSUFFICIENT_RESOURCES\n");
        OrderList->Maximum = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    OrderList->Maximum = ARB_ORDERING_LIST_DEFAULT_COUNT;

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
VOID
NTAPI
ArbFreeOrderingList(
    _Out_ PARBITER_ORDERING_LIST OrderList)
{
    PAGED_CODE();

    if (OrderList->Orderings)
    {
        ASSERT(OrderList->Maximum);
        ExFreePoolWithTag(OrderList->Orderings, TAG_ARB_ORDERING);
    }

    OrderList->Count = 0;
    OrderList->Maximum = 0;
    OrderList->Orderings = NULL;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbBuildAssignmentOrdering(
    _Inout_ PARBITER_INSTANCE ArbInstance,
    _In_ PCWSTR OrderName,
    _In_ PCWSTR ReservedOrderName,
    _In_ PARB_TRANSLATE_ORDERING TranslateOrderingFunction)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_SUCCESS;
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

    Arbiter->Signature = ARBITER_SIGNATURE;
    Arbiter->BusDeviceObject = BusDeviceObject;

    Arbiter->MutexEvent = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG_ARBITER);
    if (!Arbiter->MutexEvent)
    {
        DPRINT1("ArbInitializeArbiterInstance: STATUS_INSUFFICIENT_RESOURCES\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(Arbiter->MutexEvent, SynchronizationEvent, TRUE);

    Arbiter->AllocationStack = ExAllocatePoolWithTag(PagedPool, PAGE_SIZE, TAG_ARB_ALLOCATION);
    if (!Arbiter->AllocationStack)
    {
        DPRINT1("ArbInitializeArbiterInstance: STATUS_INSUFFICIENT_RESOURCES\n");
        ExFreePoolWithTag(Arbiter->MutexEvent, TAG_ARBITER);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Arbiter->AllocationStackMaxSize = PAGE_SIZE;

    Arbiter->Allocation = ExAllocatePoolWithTag(PagedPool, sizeof(RTL_RANGE_LIST), TAG_ARB_RANGE);
    if (!Arbiter->Allocation)
    {
        DPRINT1("ArbInitializeArbiterInstance: STATUS_INSUFFICIENT_RESOURCES\n");
        ExFreePoolWithTag(Arbiter->AllocationStack, TAG_ARB_ALLOCATION);
        ExFreePoolWithTag(Arbiter->MutexEvent, TAG_ARBITER);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Arbiter->PossibleAllocation = ExAllocatePoolWithTag(PagedPool, sizeof(RTL_RANGE_LIST), TAG_ARB_RANGE);
    if (!Arbiter->PossibleAllocation)
    {
        DPRINT1("ArbInitializeArbiterInstance: STATUS_INSUFFICIENT_RESOURCES\n");
        ExFreePoolWithTag(Arbiter->Allocation, TAG_ARB_RANGE);
        ExFreePoolWithTag(Arbiter->AllocationStack, TAG_ARB_ALLOCATION);
        ExFreePoolWithTag(Arbiter->MutexEvent, TAG_ARBITER);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitializeRangeList(Arbiter->Allocation);
    RtlInitializeRangeList(Arbiter->PossibleAllocation);

    Arbiter->Name = ArbiterName;
    Arbiter->ResourceType = ResourceType;
    Arbiter->TransactionInProgress = FALSE;

    if (!Arbiter->TestAllocation)
        Arbiter->TestAllocation = ArbTestAllocation;

    if (!Arbiter->RetestAllocation)
        Arbiter->RetestAllocation = ArbRetestAllocation;

    if (!Arbiter->CommitAllocation)
        Arbiter->CommitAllocation = ArbCommitAllocation;

    if (!Arbiter->RollbackAllocation)
        Arbiter->RollbackAllocation = ArbRollbackAllocation;

    if (!Arbiter->AddReserved)
        Arbiter->AddReserved = ArbAddReserved;

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

    if (!Arbiter->BootAllocation)
        Arbiter->BootAllocation = ArbBootAllocation;

    if (!Arbiter->QueryConflict)
        Arbiter->QueryConflict = ArbQueryConflict;

    if (!Arbiter->StartArbiter)
        Arbiter->StartArbiter = ArbStartArbiter;

    Status = ArbBuildAssignmentOrdering(Arbiter, OrderName, OrderName, TranslateOrderingFunction);
    if (NT_SUCCESS(Status))
    {
        return STATUS_SUCCESS;
    }

    DPRINT1("ArbInitializeArbiterInstance: Status %X\n", Status);

    return Status;
}
