/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxResourceAPI.cpp

Abstract:

    This module implements the resource class.

Author:




Environment:

    Both kernel and user mode

Revision History:

--*/
#include "FxSupportPch.hpp"

extern "C" {
#include "FxResourceAPI.tmh"
}

//
// Extern "C" the entire file
//
extern "C" {

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfIoResourceRequirementsListSetSlotNumber)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    ULONG SlotNumber
    )
/*++

Routine Description:
    Sets the slot number for a given resource requirements list

Arguments:
    RequirementsList - list to be modified

    SlotNumber - slot value to assign

Return Value:
    None

  --*/
{
    FxIoResReqList* pIoResReqList;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         RequirementsList,
                         FX_TYPE_IO_RES_REQ_LIST,
                         (PVOID*) &pIoResReqList);

    if (pIoResReqList->m_SlotNumber != SlotNumber) {
        pIoResReqList->MarkChanged();
    }
    pIoResReqList->m_SlotNumber = SlotNumber;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfIoResourceRequirementsListSetInterfaceType)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    __drv_strictTypeMatch(__drv_typeCond)
    INTERFACE_TYPE InterfaceType
    )
/*++

Routine Description:
    Sets the InterfaceType for a given resource requirements list

Arguments:
    RequirementsList - list to be modified

    InterfaceType - interface type to assign

Return Value:
    None

  --*/
{
    FxIoResReqList* pIoResReqList;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         RequirementsList,
                         FX_TYPE_IO_RES_REQ_LIST,
                         (PVOID*) &pIoResReqList);

    if (pIoResReqList->m_InterfaceType != InterfaceType) {
        pIoResReqList->MarkChanged();
    }

    pIoResReqList->m_InterfaceType = InterfaceType;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FxIoResourceRequirementsListInsertIoResList(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    WDFIORESLIST IoResList,
    ULONG Index
    )
/*++

Routine Description:
    Inserts a resource list into a requirements list at a particular index.

Arguments:
    RequirementsList - list to be modified

    IoResList - resource list to add

    Index - zero based index to insert at

Return Value:
    NTSTATUS

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoResReqList* pIoResReqList;
    FxIoResList* pIoResList;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   RequirementsList,
                                   FX_TYPE_IO_RES_REQ_LIST,
                                   (PVOID*) &pIoResReqList,
                                   &pFxDriverGlobals);

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         IoResList,
                         FX_TYPE_IO_RES_LIST,
                         (PVOID*) &pIoResList);

    if (pIoResList->m_OwningList != pIoResReqList) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    status = pIoResReqList->AddAt(Index, pIoResList);

    if (NT_SUCCESS(status)) {
        //
        // Mirror the access flags as well.
        //
        pIoResList->m_AccessFlags = pIoResReqList->m_AccessFlags;
        pIoResList->m_OwningList = pIoResReqList;
    }

    return status;
}


_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoResourceRequirementsListInsertIoResList)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    WDFIORESLIST IoResList,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Inserts a resource list into a requirements list at a particular index.

Arguments:
    RequirementsList - list to be modified

    IoResList - resource list to add

    Index - zero based index to insert at

Return Value:
    NTSTATUS

  --*/
{
    return FxIoResourceRequirementsListInsertIoResList(DriverGlobals,
                                                       RequirementsList,
                                                       IoResList,
                                                       Index);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoResourceRequirementsListAppendIoResList)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    WDFIORESLIST IoResList
    )
/*++

Routine Description:
   Appends a resource list to a resource requirements list

Arguments:
    RequirementsList - list to be modified

    IoResList - resource list to append

Return Value:
    NTSTATUS

  --*/

{
    return FxIoResourceRequirementsListInsertIoResList(DriverGlobals,
                                                       RequirementsList,
                                                       IoResList,
                                                       WDF_INSERT_AT_END);
}


__drv_maxIRQL(DISPATCH_LEVEL)
ULONG
WDFEXPORT(WdfIoResourceRequirementsListGetCount)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList
    )
/*++

Routine Description:
    Returns the number of resource lists in the requirements list


Arguments:
    RequirementsList - requirements list whose count will be returned

Return Value:
    number of elements in the list

  --*/

{
    FxIoResReqList* pList;
    ULONG count;
    KIRQL irql;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         RequirementsList,
                         FX_TYPE_IO_RES_REQ_LIST,
                         (PVOID*) &pList);

    pList->Lock(&irql);
    count = pList->Count();
    pList->Unlock(irql);

    return count;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFIORESLIST
WDFEXPORT(WdfIoResourceRequirementsListGetIoResList)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Retrieves a resource list from the requirements list at a given index.

Arguments:
    RequirementsList - list to retrieve the resource list from

    Index - zero based index from which to retrieve the list

Return Value:
    resource list handle or NULL

  --*/
{
    FxIoResReqList* pIoResReqList;
    FxObject* pObject;
    KIRQL irql;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         RequirementsList,
                         FX_TYPE_IO_RES_REQ_LIST,
                         (PVOID*) &pIoResReqList);

    pIoResReqList->Lock(&irql);
    pObject = pIoResReqList->GetItem(Index);
    pIoResReqList->Unlock(irql);

    if (pObject == NULL) {
        return NULL;
    }
    else {
        return (WDFIORESLIST) pObject->GetObjectHandle();
    }
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfIoResourceRequirementsListRemove)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Removes a resource list from the requirements list at a given index

Arguments:
    RequirementsList - list of resource requirements which will be modified

    Index - zero based index which indictes location in the list to find the
            resource list

Return Value:
    None

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoResReqList* pList;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   RequirementsList,
                                   FX_TYPE_IO_RES_REQ_LIST,
                                   (PVOID*) &pList,
                                   &pFxDriverGlobals);

    if (pList->RemoveAndDelete(Index) == FALSE) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFIORESLIST %p, could not remove list at index %d (not found), "
            "list item count is %d", RequirementsList, Index, pList->Count());

        FxVerifierDbgBreakPoint(pFxDriverGlobals);
    }
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfIoResourceRequirementsListRemoveByIoResList)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    WDFIORESLIST IoResList
    )
/*++

Routine Description:
    Removes a resource list from the requirements list based on the resource list's
    handle

Arguments:
    RequirementsList - resource requirements list being modified

    IoResList - resource list to be removed

Return Value:
    None

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxCollectionEntry* cur, *end;
    FxIoResReqList* pList;
    FxIoResList* pResList;
    KIRQL irql;
    BOOLEAN listFound;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   RequirementsList,
                                   FX_TYPE_IO_RES_REQ_LIST,
                                   (PVOID*) &pList,
                                   &pFxDriverGlobals);

    if (pList->IsRemoveAllowed() == FALSE) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "WDFIORESREQLIST %p: Removes not allowed",
                            RequirementsList);

        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         IoResList,
                         FX_TYPE_IO_RES_LIST,
                         (PVOID*) &pResList);

    pList->Lock(&irql);

    cur = pList->Start();
    end = pList->End();
    listFound = FALSE;

    while (cur != end) {
        if (cur->m_Object == pResList) {
            pList->MarkChanged();

            pList->RemoveEntry(cur);
            listFound = TRUE;
            break;
        }

        cur = cur->Next();
    }

    pList->Unlock(irql);

    if (listFound) {
        pResList->DeleteObject();
        pResList = NULL;
    }
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoResourceListCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFIORESLIST* ResourceList
    )
/*++

Routine Description:
   Creates a resource list.

Arguments:
    RequirementsList - the resource requirements list that the resource list will
                       be associated with

    Attributes - generic object attributes for the new resource list

    ResourceList - pointer which will receive the new object handle

Return Value:
    NTSTATUS

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoResReqList* pIoResReqList;
    FxIoResList* pIoResList;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   RequirementsList,
                                   FX_TYPE_IO_RES_REQ_LIST,
                                   (PVOID*) &pIoResReqList,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, ResourceList);
    *ResourceList = NULL;

    status = FxValidateObjectAttributes(pFxDriverGlobals,
                                        Attributes,
                                        FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    pIoResList = new (pFxDriverGlobals, Attributes) FxIoResList(
        pFxDriverGlobals, pIoResReqList);

    if (pIoResList == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = pIoResList->Commit(Attributes,
                                (WDFOBJECT*) ResourceList,
                                pIoResReqList);

    if (!NT_SUCCESS(status)) {
        pIoResList->DeleteFromFailedCreate();
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FxIoResourceListInsertDescriptor(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList,
    __in
    PIO_RESOURCE_DESCRIPTOR Descriptor,
    ULONG Index
    )
/*++

Routine Description:
    Inserts a descriptor into a resource list at a particular index.

Arguments:
    ResourceList - list to be modified

    Descriptor - descriptor to insert

    Index - zero based index to insert at

Return Value:
    NTSTATUS

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoResList* pList;
    FxResourceIo* pObject;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   ResourceList,
                                   FX_TYPE_IO_RES_LIST,
                                   (PVOID*) &pList,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Descriptor);

    if (pList->m_OwningList->IsAddAllowed() == FALSE) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Removes not allowed on WDFIORESLIST %p",
                            ResourceList);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return STATUS_ACCESS_DENIED;
    }

    pObject = new(pFxDriverGlobals)
        FxResourceIo(pFxDriverGlobals, Descriptor);

    if (pObject == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = pObject->AssignParentObject(pList);
    if (!NT_SUCCESS(status)) {
        pObject->DeleteObject();
        return status;
    }

    status = pList->AddAt(Index, pObject);

    //
    // Mark both this list and its owning list as changed so when it comes
    // time to evaluate the entire requirements list for changes, we do not
    // have to iterate over all the resource lists.
    //
    if (NT_SUCCESS(status)) {
        pList->m_OwningList->MarkChanged();
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoResourceListInsertDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList,
    __in
    PIO_RESOURCE_DESCRIPTOR Descriptor,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Inserts a descriptor into a resource list at a particular index.

Arguments:
    ResourceList - list to be modified

    Descriptor - descriptor to insert

    Index - zero based index to insert at

Return Value:
    NTSTATUS

  --*/
{
    return FxIoResourceListInsertDescriptor(DriverGlobals,
                                            ResourceList,
                                            Descriptor,
                                            Index);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoResourceListAppendDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList,
    __in
    PIO_RESOURCE_DESCRIPTOR Descriptor
    )
/*++

Routine Description:
   Appends a descriptor to a resource list

Arguments:
    ResourceList - list to be modified

    Descriptor - item to be appended

Return Value:
    NTSTATUS

  --*/
{
    return FxIoResourceListInsertDescriptor(DriverGlobals,
                                            ResourceList,
                                            Descriptor,
                                            WDF_INSERT_AT_END);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfIoResourceListUpdateDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList,
    __in
    PIO_RESOURCE_DESCRIPTOR Descriptor,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Updates resource requirement in place in the list.

Arguments:
    ResourceList - list to be modified

    Descriptor - Pointer to descriptor whic contains the updated value

    Index - zero based location in the list to update

Return Value:
    None

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoResList* pList;
    FxResourceIo* pObject;
    KIRQL irql;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   ResourceList,
                                   FX_TYPE_IO_RES_LIST,
                                   (PVOID*) &pList,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Descriptor);

    pList->Lock(&irql);
    pObject = (FxResourceIo*) pList->GetItem(Index);
    pList->Unlock(irql);

    if (pObject != NULL) {
        //
        // We don't check for add or remove access because we don't know what
        // the update is actually doing (ie widening a range, shortening it, etc).
        // For this operation we have to trust the driver that it is doing the
        // right thing at the right time.
        //
        RtlCopyMemory(&pObject->m_Descriptor,
                      Descriptor,
                      sizeof(pObject->m_Descriptor));

        //
        // Mark both this list and its owning list as changed so when it comes
        // time to evaluate the entire requirements list for changes, we do not
        // have to iterate over all the resource lists.
        //
        pList->MarkChanged();
        pList->m_OwningList->MarkChanged();
    }
    else {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFIORESREQLIST %p, cannot update item at index %d, item not found,"
            " list item count is %d", ResourceList, Index, pList->Count());

        FxVerifierDbgBreakPoint(pFxDriverGlobals);
    }
}

__drv_maxIRQL(DISPATCH_LEVEL)
ULONG
WDFEXPORT(WdfIoResourceListGetCount)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList
    )
/*++

Routine Description:
    Returns the number of descriptors in the resource list

Arguments:
    ResourceList - resource list whose count will be returned

Return Value:
    number of elements in the list

  --*/
{
    FxIoResList* pList;
    ULONG count;
    KIRQL irql;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         ResourceList,
                         FX_TYPE_IO_RES_LIST,
                         (PVOID*) &pList);

    pList->Lock(&irql);
    count = pList->Count();
    pList->Unlock(irql);

    return count;
}


__drv_maxIRQL(DISPATCH_LEVEL)
PIO_RESOURCE_DESCRIPTOR
WDFEXPORT(WdfIoResourceListGetDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Retrieves an io resource desciptor for a given index in the resource list

Arguments:
    ResourceList - list being looked up

    Index - zero based index into the list to find the value of

Return Value:
    pointer to an io resource descriptor upon success, NULL upon error

  --*/

{
    FxIoResList* pList;
    FxResourceIo* pObject;
    KIRQL irql;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         ResourceList,
                         FX_TYPE_IO_RES_LIST,
                         (PVOID*) &pList);

    pList->Lock(&irql);
    pObject = (FxResourceIo*) pList->GetItem(Index);
    pList->Unlock(irql);

    if (pObject == NULL) {
        return NULL;
    }
    else {
        //
        // Copy the current descriptor to the clone and return it
        //
        RtlCopyMemory(&pObject->m_DescriptorClone,
                      &pObject->m_Descriptor,
                      sizeof(pObject->m_Descriptor));

        return &pObject->m_DescriptorClone;
    }
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfIoResourceListRemove)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Removes a descriptor in an io resource list

Arguments:
    ResourceList - resource list to modify

    Index - zero based index into the list in which to remove the descriptor

Return Value:
    None

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoResList* pList;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   ResourceList,
                                   FX_TYPE_IO_RES_LIST,
                                   (PVOID*) &pList,
                                   &pFxDriverGlobals);

    if (pList->RemoveAndDelete(Index)) {
        //
        // Mark this list's owning list as changed so when it comes
        // time to evaluate the entire requirements list for changes, we do not
        // have to iterate over all the resource lists.
        //
        // RemoveAndDelete marked pList as changed already
        //
        pList->m_OwningList->MarkChanged();
    }
    else {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFIORESLIST %p, could not remove item at index %d (not found), "
            "list item count is %d", ResourceList, Index, pList->Count());

        FxVerifierDbgBreakPoint(pFxDriverGlobals);
    }
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfIoResourceListRemoveByDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList,
    __in
    PIO_RESOURCE_DESCRIPTOR Descriptor
    )
/*++

Routine Description:
    Removes a descriptor by value in a given io resource list.  Equality is
    determined by RtlCompareMemory.

Arguments:
    ResourceList - the io resource list to modify

    Descriptor - pointer to a descriptor to remove.

Return Value:
    None

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxCollectionEntry* cur, *end;
    FxIoResList* pList;
    FxResourceIo* pObject;
    KIRQL irql;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   ResourceList,
                                   FX_TYPE_IO_RES_LIST,
                                   (PVOID*) &pList,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Descriptor);

    if (pList->IsRemoveAllowed() == FALSE) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Removes not allowed on WDFIORESLIST %p",
                            ResourceList);

        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    pList->Lock(&irql);

    cur = pList->Start();
    end = pList->End();
    pObject = NULL;

    while (cur != end) {
        pObject = (FxResourceIo*) cur->m_Object;

        if (RtlCompareMemory(&pObject->m_Descriptor,
                             Descriptor,
                             sizeof(*Descriptor)) == sizeof(*Descriptor)) {
            //
            // Mark both this list and its owning list as changed so when it
            // comes time to evaluate the entire requirements list for
            // changes, we do not have to iterate over all the resource lists.
            //
            pList->MarkChanged();
            pList->m_OwningList->MarkChanged();

            pList->RemoveEntry(cur);
            break;
        }

        //
        // Set to NULL so that we do not delete it if this is the last item in
        // the list.
        //
        pObject = NULL;

        cur = cur->Next();
    }

    pList->Unlock(irql);

    if (pObject != NULL) {
        pObject->DeleteObject();
    }
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FxCmResourceListInsertDescriptor(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCMRESLIST List,
    __in
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Inserts a descriptor into a cm resource list at a particular index.

Arguments:
    ResourceList - list to be modified

    Descriptor - descriptor to insert

    Index - zero based index to insert at

Return Value:
    NTSTATUS

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxCmResList* pList;
    FxResourceCm* pObject;
    NTSTATUS status;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Descriptor);

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         List,
                         FX_TYPE_CM_RES_LIST,
                         (PVOID*) &pList);

    pObject = new(pFxDriverGlobals) FxResourceCm(pFxDriverGlobals, Descriptor);

    if (pObject == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = pObject->AssignParentObject(pList);
    if (!NT_SUCCESS(status)) {
        pObject->DeleteObject();
        return status;
    }

    return pList->AddAt(Index, pObject);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfCmResourceListInsertDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCMRESLIST List,
    __in
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Inserts a descriptor into a cm resource list at a particular index.

Arguments:
    ResourceList - list to be modified

    Descriptor - descriptor to insert

    Index - zero based index to insert at

Return Value:
    NTSTATUS

  --*/
{
    DDI_ENTRY();

    return FxCmResourceListInsertDescriptor(DriverGlobals,
                                            List,
                                            Descriptor,
                                            Index);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfCmResourceListAppendDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCMRESLIST List,
    __in
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    )
/*++

Routine Description:
   Appends a descriptor to a cm resource list

Arguments:
    ResourceList - list to be modified

    Descriptor - item to be appended

Return Value:
    NTSTATUS

  --*/
{
    DDI_ENTRY();

    return FxCmResourceListInsertDescriptor(DriverGlobals,
                                            List,
                                            Descriptor,
                                            WDF_INSERT_AT_END);
}

__drv_maxIRQL(DISPATCH_LEVEL)
ULONG
WDFEXPORT(WdfCmResourceListGetCount)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCMRESLIST List
    )
/*++

Routine Description:
    Returns the number of cm descriptors in the resource list

Arguments:
    ResourceList - resource list whose count will be returned

Return Value:
    number of elements in the list

  --*/
{
    FxCmResList* pList;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         List,
                         FX_TYPE_CM_RES_LIST,
                         (PVOID*) &pList);

    return pList->GetCount();
}


__drv_maxIRQL(DISPATCH_LEVEL)
PCM_PARTIAL_RESOURCE_DESCRIPTOR
WDFEXPORT(WdfCmResourceListGetDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCMRESLIST List,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Retrieves a cm resource desciptor for a given index in the resource list

Arguments:
    ResourceList - list being looked up

    Index - zero based index into the list to find the value of

Return Value:
    pointer to a cm resource descriptor upon success, NULL upon error

  --*/
{
    DDI_ENTRY();

    FxCmResList* pList;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         List,
                         FX_TYPE_CM_RES_LIST,
                         (PVOID*) &pList);

    return pList->GetDescriptor(Index);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfCmResourceListRemove)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCMRESLIST List,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Removes a descriptor in an cm resource list

Arguments:
    ResourceList - resource list to modify

    Index - zero based index into the list in which to remove the descriptor

Return Value:
    None

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxCmResList* pList;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   List,
                                   FX_TYPE_CM_RES_LIST,
                                   (PVOID*) &pList,
                                   &pFxDriverGlobals);

    if (pList->RemoveAndDelete(Index) == FALSE) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFCMRESLIST %p, could not remove list at index %d (not found), "
            "list item count is %d", List, Index, pList->Count());

        FxVerifierDbgBreakPoint(pFxDriverGlobals);
    }
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfCmResourceListRemoveByDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCMRESLIST List,
    __in
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    )
/*++

Routine Description:
    Removes a descriptor by value in a given cm resource list.  Equality is
    determined by RtlCompareMemory.

Arguments:
    ResourceList - the io resource list to modify

    Descriptor - pointer to a descriptor to remove.

Return Value:
    None

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxCollectionEntry* cur;
    FxCollectionEntry* end;
    FxCmResList* pList;
    FxResourceCm* pObject;
    KIRQL irql;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   List,
                                   FX_TYPE_CM_RES_LIST,
                                   (PVOID*) &pList,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Descriptor);

    if (pList->IsRemoveAllowed() == FALSE) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Removes not allowed on WDFCMRESLIST %p", List);

        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    pList->Lock(&irql);

    cur = pList->Start();
    end = pList->End();
    pObject = NULL;

    while (cur != end) {
        pObject = (FxResourceCm*) cur->m_Object;

        if (RtlCompareMemory(&pObject->m_Descriptor,
                             Descriptor,
                             sizeof(*Descriptor)) == sizeof(*Descriptor)) {
            pList->MarkChanged();

            pList->RemoveEntry(cur);
            break;
        }

        //
        // Set to NULL so that we do not delete it if this is the last item in
        // the list.
        //
        pObject = NULL;

        cur = cur->Next();
    }

    pList->Unlock(irql);

    if (pObject != NULL) {
        pObject->DeleteObject();
        pObject = NULL;
    }
}

} // extern "C"
