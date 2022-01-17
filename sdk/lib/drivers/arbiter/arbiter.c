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

    DPRINT("ArbAddOrdering: [%p] %I64X-%I64X\n", OrderList, MinimumAddress, MaximumAddress);

    if (MaximumAddress < MinimumAddress)
    {
        DPRINT1("ArbAddOrdering: STATUS_INVALID_PARAMETER. [%p] %I64X-%I64X\n", OrderList, MinimumAddress, MaximumAddress);
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
    _Out_ PARBITER_ORDERING_LIST OrderList,
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

    DPRINT("ArbPruneOrdering: %X, %I64X-%I64X\n", OrderList->Count, MinimumAddress, MaximumAddress);

    ASSERT(OrderList);
    ASSERT(OrderList->Orderings);

    if (MaximumAddress < MinimumAddress)
    {
        DPRINT1("ArbPruneOrdering: STATUS_INVALID_PARAMETER. [%p] %I64X-%I64X\n", OrderList, MinimumAddress, MaximumAddress);
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

    for (ix = 0; ix < OrderList->Count; ix++, Orderings++)
    {
        if (MaximumAddress < Orderings->Start ||
            MinimumAddress > Orderings->End)
        {
            Current->Start = Orderings->Start;
            Current->End = Orderings->End;
        }
        else if (MinimumAddress <= Orderings->Start)
        {
            if (MaximumAddress >= Orderings->End)
                continue;

            Current->Start = (MaximumAddress + 1);
            Current->End = Orderings->End;
        }
        else if (MaximumAddress >= Orderings->End)
        {
            Current->Start = Orderings->Start;
            Current->End = (MinimumAddress - 1);
        }
        else
        {
            Current->Start = (MaximumAddress + 1);
            Current->End = Orderings->End;

            Current++;

            Current->Start = Orderings->Start;
            Current->End = (MinimumAddress - 1);
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
static
NTSTATUS
ArbpGetRegistryValue(
    _In_ HANDLE KeyHandle,
    _In_ PCWSTR NameString,
    _Out_ PKEY_VALUE_FULL_INFORMATION * OutValueInfo)
{
    PKEY_VALUE_FULL_INFORMATION ValueInfo;
    UNICODE_STRING ValueName;
    ULONG ResultLength;
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("ArbpGetRegistryValue: NameString '%S'\n", NameString);
    RtlInitUnicodeString(&ValueName, NameString);

    Status = ZwQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyFullInformation | KeyNodeInformation,
                             NULL,
                             0,
                             &ResultLength);

    if (Status != STATUS_BUFFER_OVERFLOW && Status != STATUS_BUFFER_TOO_SMALL)
    {
        DPRINT1("ArbpGetRegistryValue: Status %X\n", Status);
        return Status;
    }

    ValueInfo = ExAllocatePoolWithTag(PagedPool, ResultLength, TAG_ARBITER);
    if (!ValueInfo)
    {
        DPRINT1("ArbpGetRegistryValue: STATUS_INSUFFICIENT_RESOURCES\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ZwQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyFullInformation|KeyNodeInformation,
                             ValueInfo,
                             ResultLength,
                             &ResultLength);
    if (NT_SUCCESS(Status))
    {
        *OutValueInfo = ValueInfo;
        return STATUS_SUCCESS;
    }

    DPRINT1("ArbpGetRegistryValue: Status %X\n", Status);

    ExFreePoolWithTag(ValueInfo, TAG_ARBITER);
    return Status;
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
    UNICODE_STRING ReservedResourcesName = RTL_CONSTANT_STRING(L"ReservedResources");
    UNICODE_STRING AllocationOrderName = RTL_CONSTANT_STRING(L"AllocationOrder");
    UNICODE_STRING ArbitersKeyName = RTL_CONSTANT_STRING(IO_REG_KEY_ARBITERS);
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_VALUE_FULL_INFORMATION ReservedValueInfo = NULL;
    PKEY_VALUE_FULL_INFORMATION ValueInfo = NULL;
    PIO_RESOURCE_REQUIREMENTS_LIST IoResources;
    PIO_RESOURCE_DESCRIPTOR IoDescriptor;
    IO_RESOURCE_DESCRIPTOR TranslatedIoDesc;
    HANDLE ArbitersKeyHandle = NULL;
    HANDLE OrderingKeyHandle = NULL;
    PCWSTR CurrentOrderName;
    PCWSTR ValueName;
    UINT32 ix;
    NTSTATUS Status;
  #if DBG
    PARBITER_ORDERING Orderings;
  #endif

    PAGED_CODE();

    DPRINT("[%p] OrderName '%S', ReservedOrderName '%S'\n", ArbInstance, OrderName, ReservedOrderName);

    KeWaitForSingleObject(ArbInstance->MutexEvent,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    ArbFreeOrderingList(&ArbInstance->OrderingList);
    ArbFreeOrderingList(&ArbInstance->ReservedList);

    Status = ArbInitializeOrderingList(&ArbInstance->OrderingList);
    if (!NT_SUCCESS(Status))
        goto ErrorExit;

    Status = ArbInitializeOrderingList(&ArbInstance->ReservedList);
    if (!NT_SUCCESS(Status))
        goto ErrorExit;

    InitializeObjectAttributes(&ObjectAttributes,
                               &ArbitersKeyName,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&ArbitersKeyHandle, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
        goto ErrorExit;

    /* First loop for AllocationOrder, next for ReservedResources */
    for (ix = 0; ix <= 1; ix++)
    {
        ValueInfo = NULL;

        if (ix == 0)
        {
            CurrentOrderName = OrderName;

            InitializeObjectAttributes(&ObjectAttributes,
                                       &AllocationOrderName,
                                       OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                       ArbitersKeyHandle,
                                       NULL);

            Status = ZwOpenKey(&OrderingKeyHandle, KEY_READ, &ObjectAttributes);
        }
        else
        {
            CurrentOrderName = ReservedOrderName;

            InitializeObjectAttributes(&ObjectAttributes,
                                       &ReservedResourcesName,
                                       OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                       ArbitersKeyHandle,
                                       NULL);

            Status = ZwCreateKey(&OrderingKeyHandle,
                                 KEY_READ,
                                 &ObjectAttributes,
                                 0,
                                 NULL,
                                 REG_OPTION_NON_VOLATILE,
                                 NULL);
        }

        if (!NT_SUCCESS(Status))
            goto ErrorExit;

        Status = ArbpGetRegistryValue(OrderingKeyHandle, CurrentOrderName, &ValueInfo);
        if (!NT_SUCCESS(Status) || !ValueInfo)
            goto ErrorExit;

        if (ix == 1 && ValueInfo->Type == REG_SZ)
        {
            /* ReservedResources case */

            ValueName = (PCWSTR)((ULONG_PTR)ValueInfo + ValueInfo->DataOffset);
            DPRINT("ArbBuildAssignmentOrdering: ValueName '%S'\n", ValueName);

            if (ValueName[(ValueInfo->DataLength / sizeof(WCHAR)) - 1])
                goto ErrorExit;

            Status = ArbpGetRegistryValue(OrderingKeyHandle, ValueName, &ReservedValueInfo);
            if (!NT_SUCCESS(Status))
                goto ErrorExit;

            ExFreePoolWithTag(ValueInfo, TAG_ARBITER);
            ValueInfo = ReservedValueInfo;
        }

        ZwClose(OrderingKeyHandle);

        if (ValueInfo->Type != REG_RESOURCE_REQUIREMENTS_LIST)
        {
            DPRINT1("ArbBuildAssignmentOrdering: STATUS_INVALID_PARAMETER\n");
            Status = STATUS_INVALID_PARAMETER;
            goto ErrorExit;
        }

        IoResources = (PIO_RESOURCE_REQUIREMENTS_LIST)((ULONG_PTR)ValueInfo + ValueInfo->DataOffset);
        ASSERT(IoResources->AlternativeLists == 1);

        for (IoDescriptor = &IoResources->List[0].Descriptors[0];
             IoDescriptor < (&IoResources->List[0].Descriptors[0] + IoResources->List[0].Count);
             IoDescriptor++)
        {
            UINT64 MinimumAddress;
            UINT64 MaximumAddress;
            UINT32 Dummy1;
            UINT32 Dummy2;

            if (TranslateOrderingFunction)
            {
                Status = TranslateOrderingFunction(&TranslatedIoDesc, IoDescriptor);
                if (!NT_SUCCESS(Status))
                    goto ErrorExit;
            }
            else
            {
                RtlCopyMemory(&TranslatedIoDesc, IoDescriptor, sizeof(TranslatedIoDesc));
            }

            if (TranslatedIoDesc.Type != ArbInstance->ResourceType)
                continue;

            Status = ArbInstance->UnpackRequirement(&TranslatedIoDesc,
                                                    &MinimumAddress,
                                                    &MaximumAddress,
                                                    &Dummy1,
                                                    &Dummy2);
            if (!NT_SUCCESS(Status))
                goto ErrorExit;

            if (ix == 0)
            {
                /* AllocationOrder */
                Status = ArbAddOrdering(&ArbInstance->OrderingList, MinimumAddress, MaximumAddress);
                if (!NT_SUCCESS(Status))
                    goto ErrorExit;
            }
            else
            {
                /* ReservedResources */
                Status = ArbAddOrdering(&ArbInstance->ReservedList, MinimumAddress, MaximumAddress);
                if (!NT_SUCCESS(Status))
                    goto ErrorExit;

                /* Prune AllocationOrder */
                Status = ArbPruneOrdering(&ArbInstance->OrderingList, MinimumAddress, MaximumAddress);
                if (!NT_SUCCESS(Status))
                    goto ErrorExit;
            }
        }
    }

    ZwClose(ArbitersKeyHandle);

#if DBG

    Orderings = ArbInstance->OrderingList.Orderings;

    for (Orderings = &ArbInstance->OrderingList.Orderings[0];
         Orderings < &ArbInstance->OrderingList.Orderings[ArbInstance->OrderingList.Count];
         Orderings++)
    {
        DPRINT("ArbBuildAssignmentOrdering: OrderingList(%I64X-%I64X)\n",
               Orderings->Start, Orderings->End);
    }

    Orderings = ArbInstance->ReservedList.Orderings;

    for (Orderings = &ArbInstance->ReservedList.Orderings[0];
         Orderings < &ArbInstance->ReservedList.Orderings[ArbInstance->ReservedList.Count];
         Orderings++)
    {
        DPRINT("ArbBuildAssignmentOrdering: ReservedList(%I64X-%I64X)\n",
               Orderings->Start, Orderings->End);
    }

#endif

    KeSetEvent(ArbInstance->MutexEvent, IO_NO_INCREMENT, FALSE);
    return STATUS_SUCCESS;

ErrorExit:

    DPRINT1("ArbBuildAssignmentOrdering: ErrorExit. Status %X\n", Status);

    if (ArbitersKeyHandle)
        ZwClose(ArbitersKeyHandle);

    if (OrderingKeyHandle)
        ZwClose(OrderingKeyHandle);

    if (ValueInfo)
        ExFreePoolWithTag(ValueInfo, TAG_ARBITER);

    if (ReservedValueInfo)
        ExFreePoolWithTag(ReservedValueInfo, TAG_ARBITER);

    if (ArbInstance->OrderingList.Orderings)
    {
        ExFreePoolWithTag(ArbInstance->OrderingList.Orderings, TAG_ARB_ORDERING);
        ArbInstance->OrderingList.Count = 0;
        ArbInstance->OrderingList.Maximum = 0;
    }

    if (ArbInstance->ReservedList.Orderings)
    {
        ExFreePoolWithTag(ArbInstance->ReservedList.Orderings, TAG_ARB_ORDERING);
        ArbInstance->ReservedList.Count = 0;
        ArbInstance->ReservedList.Maximum = 0;
    }

    KeSetEvent(ArbInstance->MutexEvent, IO_NO_INCREMENT, FALSE);

    return Status;
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
        return STATUS_SUCCESS;

    DPRINT1("ArbInitializeArbiterInstance: Status %X\n", Status);

    return Status;
}
