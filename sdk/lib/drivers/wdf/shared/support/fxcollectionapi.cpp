/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxCollectionApi.cpp

Abstract:

    This module implements the "C" interface to the collection object.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#include "FxSupportPch.hpp"

extern "C" {
#include "FxCollectionApi.tmh"
}

//
// Extern the entire file
//
extern "C" {
_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfCollectionCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES CollectionAttributes,
    __out
    WDFCOLLECTION *Collection
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxCollection *pCollection;
    WDFCOLLECTION hCol;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    //
    // Get the parent's globals if it is present
    //
    if (NT_SUCCESS(FxValidateObjectAttributesForParentHandle(
                        pFxDriverGlobals, CollectionAttributes))) {
        FxObject* pParent;

        FxObjectHandleGetPtrAndGlobals(pFxDriverGlobals,
                                       CollectionAttributes->ParentObject,
                                       FX_TYPE_OBJECT,
                                       (PVOID*)&pParent,
                                       &pFxDriverGlobals);
    }

    FxPointerNotNull(pFxDriverGlobals, Collection);

    *Collection = NULL;

    status = FxValidateObjectAttributes(pFxDriverGlobals, CollectionAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pCollection = new (pFxDriverGlobals, CollectionAttributes)
        FxCollection(pFxDriverGlobals);

    if (pCollection != NULL) {
        status = pCollection->Commit(CollectionAttributes, (WDFOBJECT*)&hCol);

        if (NT_SUCCESS(status)) {
            *Collection = hCol;
        }
        else {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                                "Could not create collection object: %!STATUS!",
                                status);

            pCollection->DeleteFromFailedCreate();
        }
    }
    else {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                            "Could not create collection object: "
                            "STATUS_INSUFFICIENT_RESOURCES" );
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
ULONG
WDFEXPORT(WdfCollectionGetCount)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection
    )
{
    DDI_ENTRY();

    FxCollection *pCollection;
    KIRQL irql;
    ULONG count;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Collection,
                         FX_TYPE_COLLECTION,
                         (PVOID *)&pCollection);

    pCollection->Lock(&irql);
    count = pCollection->Count();
    pCollection->Unlock(irql);

    return count;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfCollectionAdd)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection,
    __in
    WDFOBJECT Object
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxCollection *pCollection;
    FxObject *pObject;
    NTSTATUS status;
    KIRQL irql;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Collection,
                                   FX_TYPE_COLLECTION,
                                   (PVOID*) &pCollection,
                                   &pFxDriverGlobals);

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Object,
                         FX_TYPE_OBJECT,
                         (PVOID*) &pObject);

    pCollection->Lock(&irql);
    status = pCollection->Add(pObject) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    pCollection->Unlock(irql);

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfCollectionRemoveItem)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection,
    __in
    ULONG Index
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxCollection* pCollection;
    FxCollectionEntry* pEntry;
    FxObject* pObject;
    NTSTATUS status;
    KIRQL irql;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Collection,
                                   FX_TYPE_COLLECTION,
                                   (PVOID*) &pCollection,
                                   &pFxDriverGlobals);

    pCollection->Lock(&irql);

    pEntry = pCollection->FindEntry(Index);

    if (pEntry != NULL) {
        pObject = pEntry->m_Object;
        pCollection->CleanupEntry(pEntry);
        status = STATUS_SUCCESS;
    }
    else {
        pObject = NULL;
        status = STATUS_NOT_FOUND;
    }

    pCollection->Unlock(irql);

    if (pObject != NULL) {
        pCollection->CleanupEntryObject(pObject);
    }

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "Index %d is not valid in WDFCOLLECTION %p (count is %d), %!STATUS!",
            Index, Collection, pCollection->Count(), status);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
    }
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfCollectionRemove)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection,
    __in
    WDFOBJECT Item
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxCollection *pCollection;
    FxCollectionEntry *pEntry;
    FxObject* pObject;
    NTSTATUS status;
    KIRQL irql;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Collection,
                                   FX_TYPE_COLLECTION,
                                   (PVOID*) &pCollection,
                                   &pFxDriverGlobals);

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Item,
                         FX_TYPE_OBJECT,
                         (PVOID*) &pObject);

    pCollection->Lock(&irql);

    pEntry = pCollection->FindEntryByObject(pObject);

    if (pEntry != NULL) {
        pCollection->CleanupEntry(pEntry);
        status = STATUS_SUCCESS;
    }
    else {
        pObject = NULL;
        status = STATUS_NOT_FOUND;
    }

    pCollection->Unlock(irql);

    if (pObject != NULL) {
        pCollection->CleanupEntryObject(pObject);
    }

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                            "WDFOBJECT %p not in WDFCOLLECTION %p, %!STATUS!",
                            Item, Collection, status);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
    }
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFOBJECT
WDFEXPORT(WdfCollectionGetItem)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection,
    __in
    ULONG Index
    )
{
    DDI_ENTRY();

    FxCollection *pCollection;
    FxObject *pObject;
    WDFOBJECT hObject;
    KIRQL irql;

    hObject = NULL;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Collection,
                         FX_TYPE_COLLECTION,
                         (PVOID*) &pCollection);

    pCollection->Lock(&irql);
    pObject = pCollection->GetItem(Index);
    pCollection->Unlock(irql);

    if (pObject == NULL) {
        return NULL;
    }

    return pObject->GetObjectHandle();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFOBJECT
WDFEXPORT(WdfCollectionGetFirstItem)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection
    )
{
    DDI_ENTRY();

    FxCollection *pCollection;
    FxObject* pObject;
    KIRQL irql;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Collection,
                         FX_TYPE_COLLECTION,
                         (PVOID*) &pCollection);

    pCollection->Lock(&irql);
    pObject = pCollection->GetFirstItem();
    pCollection->Unlock(irql);

    if (pObject != NULL) {
        return pObject->GetObjectHandle();
    }
    else {
        return NULL;
    }
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFOBJECT
WDFEXPORT(WdfCollectionGetLastItem)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection
    )
{
    DDI_ENTRY();

    FxCollection *pCollection;
    FxObject* pObject;
    KIRQL irql;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Collection,
                         FX_TYPE_COLLECTION,
                         (PVOID*) &pCollection);

    pCollection->Lock(&irql);
    pObject = pCollection->GetLastItem();
    pCollection->Unlock(irql);

    if (pObject != NULL) {
        return pObject->GetObjectHandle();
    }
    else {
        return NULL;
    }
}

} // extern "C" of entire file
