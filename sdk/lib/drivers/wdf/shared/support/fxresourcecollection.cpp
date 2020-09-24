/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxResourceCollection.cpp

Abstract:

    This module implements a base object for derived collection classes and
    the derived collection classes.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#include "FxSupportPch.hpp"

extern "C" {
#if defined(EVENT_TRACING)
#include "FxResourceCollection.tmh"
#endif
}

BOOLEAN
FxResourceCollection::RemoveAndDelete(
    __in ULONG Index
    )
/*++

Routine Description:
    Removes an entry from the collection and then deletes it if found.  The
    caller must have removal permissions to perform this action.

Arguments:
    Index - zero based index into the collection at which to perform the removal

Return Value:
    TRUE if the item was found and deleted, FALSE otherwise

  --*/
{
    FxObject* pObject;
    FxCollectionEntry* pEntry;
    KIRQL irql;

    if (IsRemoveAllowed() == FALSE) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Removes not allowed on handle %p, remove at index %d"
                            "failed", GetObjectHandle(), Index);

        FxVerifierDbgBreakPoint(GetDriverGlobals());
        return FALSE;
    }

    pObject = NULL;

    Lock(&irql);

    pEntry = FindEntry(Index);
    if (pEntry != NULL) {

        //
        // Mark the list as changed so when we go to create a WDM resource list we
        // know if a new list is needed.
        //
        MarkChanged();

        pObject = pEntry->m_Object;

        //
        // Remove the entry
        //
        RemoveEntry(pEntry);
    }
    Unlock(irql);

    if (pObject != NULL) {
        //
        // Delete the object since we created it
        //
        pObject->DeleteObject();
        pObject = NULL;

        return TRUE;
    }
    else {
        return FALSE;
    }
}

_Must_inspect_result_
NTSTATUS
FxResourceCollection::AddAt(
    __in ULONG Index,
    __in FxObject* Object
    )
/*++

Routine Description:
    Adds an object into the collection at the specified index.

Arguments:
    Index - zero baesd index in which to insert into the list.   WDF_INSERT_AT_END
            is a special value which indicates that the insertion is an append.

    Object - object to add

Return Value:
    NTSTATUS

  --*/
{
    FxCollectionEntry *pNew;
    PLIST_ENTRY ple;
    NTSTATUS status;
    KIRQL irql;

    if (IsAddAllowed() == FALSE) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Adds not allowed on handle %p, add at index %d"
                            "failed", GetObjectHandle(), Index);

        FxVerifierDbgBreakPoint(GetDriverGlobals());

        return STATUS_ACCESS_DENIED;
    }

    Lock(&irql);

    ple = NULL;
    status = STATUS_SUCCESS;

    pNew = AllocateEntry(GetDriverGlobals());

    if (pNew != NULL) {
        //
        // Inserting at the current count (i.e. one past the end) is the same
        // as append.
        //
        if (Index == WDF_INSERT_AT_END || Index == Count()) {
            ple = &m_ListHead;
        }
        else {
            FxCollectionEntry* cur, *end;
            ULONG i;

            for (cur = Start(), end = End(), i = 0;
                 cur != end;
                 cur = cur->Next(), i++) {
                if (i == Index) {
                    ple = &cur->m_ListEntry;
                    break;
                }
            }

            if (ple == NULL) {
                delete pNew;
                status = STATUS_ARRAY_BOUNDS_EXCEEDED;
            }
        }
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(status)) {
        PLIST_ENTRY blink;

        // ple now points to the list entry which we will insert our node
        // *before*

        blink = ple->Blink;

        // Link the previous with the new entry
        blink->Flink = &pNew->m_ListEntry;
        pNew->m_ListEntry.Blink = blink;

        // Link the current with the new entry
        pNew->m_ListEntry.Flink = ple;
        ple->Blink = &pNew->m_ListEntry;

        AddEntry(pNew, Object);

        //
        // Mark the list as changed so when we go to create a WDM resource list
        // we know if a new list is needed.
        //
        MarkChanged();
    }

    Unlock(irql);

    if (!NT_SUCCESS(status)) {
        Object->DeleteFromFailedCreate();
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxIoResList::BuildFromWdmList(
    __deref_in PIO_RESOURCE_LIST* WdmResourceList
    )
/*++

Routine Description:
    Builds up the collection with FxResourceIo objects based on the passed in
    WDM io resource list

Arguments:
    WdmResourceList - list which specifies the io resource objects to create

Return Value:
    NTSTATUS

  --*/
{
    PIO_RESOURCE_DESCRIPTOR pWdmDescriptor;
    ULONG i, count;
    NTSTATUS status;

    pWdmDescriptor = &(*WdmResourceList)->Descriptors[0];
    count = (*WdmResourceList)->Count;
    status = STATUS_SUCCESS;

    for (i = 0; i < count; i++) {
        //
        // Now create a new resource object for each resource
        // in our list.
        //
        FxResourceIo *pResource;

        pResource = new(GetDriverGlobals())
            FxResourceIo(GetDriverGlobals(), pWdmDescriptor);

        if (pResource == NULL) {
            //
            // We failed, clean up, and exit.  Since we are only
            // keeping references on the master collection, if
            // we free this, everything else will go away too.
            //
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (NT_SUCCESS(status)) {
            status = pResource->AssignParentObject(this);

            //
            // See notes in previous AssignParentObject as to why
            // we are asserting.
            //
            ASSERT(NT_SUCCESS(status));
            UNREFERENCED_PARAMETER(status);

            status = Add(pResource) ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
        }

        if (!NT_SUCCESS(status)) {
            break;
        }

        pWdmDescriptor++;
    }

    if (NT_SUCCESS(status)) {
        status = m_OwningList->Add(this) ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(status)) {
        *WdmResourceList = (PIO_RESOURCE_LIST) pWdmDescriptor;
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxCmResList::BuildFromWdmList(
    __in PCM_RESOURCE_LIST WdmResourceList,
    __in UCHAR AccessFlags
    )
/*++

Routine Description:
    Builds up the collection with FxResourceCm objects based on the passed in
    WDM io resource list.

Arguments:
    WdmResourceList - list which specifies the io resource objects to create

    AccessFlags - permissions to be associated with the list

Return Value:
    NTSTATUS

  --*/
{
    NTSTATUS status;

    //
    // Predispose to success
    //
    status = STATUS_SUCCESS;

    Clear();

    m_AccessFlags = AccessFlags;

    if (WdmResourceList != NULL) {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptor;
        ULONG count, i;

        //
        // We only expect to see one full resource descriptor.
        //
        ASSERT(WdmResourceList->Count == 1);

        count = WdmResourceList->List[0].PartialResourceList.Count;
        pDescriptor = WdmResourceList->List[0].PartialResourceList.PartialDescriptors;

        for(i = 0; i < count; i++, pDescriptor++) {
            FxResourceCm *pResource;

            pResource = new(GetDriverGlobals())
                FxResourceCm(GetDriverGlobals(), pDescriptor);

            if (pResource == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }

            if (NT_SUCCESS(status)) {
                status = pResource->AssignParentObject(this);

                //
                // Since we control our own lifetime here, the assign should
                // always work.
                //
                ASSERT(NT_SUCCESS(status));

                status = Add(pResource) ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
            }

            if (!NT_SUCCESS(status)) {
                Clear();
                break;
            }
        }
    }

    return status;
}

_Must_inspect_result_
PCM_RESOURCE_LIST
FxCmResList::CreateWdmList(
    __in __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE PoolType
    )
/*++

Routine Description:
    Allocates and initializes a WDM CM resource list based off of the current
    contents of this collection.

Arguments:
    PoolType - the pool type from which to allocate the resource list

Return Value:
    a new resource list upon success, NULL upon failure

  --*/
{
    PCM_RESOURCE_LIST pWdmResourceList;
    ULONG size;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pWdmResourceList = NULL;
    pFxDriverGlobals = GetDriverGlobals();

    if (Count()) {
        //
        // NOTE: This function assumes all resources are on the same bus
        // and therefore there is only one FULL_RESOURCE_DESCRIPTOR.
        //
        size = sizeof(CM_RESOURCE_LIST) +
               (sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * (Count() - 1));

        pWdmResourceList = (PCM_RESOURCE_LIST)
            MxMemory::MxAllocatePoolWithTag(PoolType, size, pFxDriverGlobals->Tag);

        if (pWdmResourceList != NULL) {
            PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptor;
            FxCollectionEntry *cur, *end;

            RtlZeroMemory(pWdmResourceList, size);

            pWdmResourceList->Count = 1;  // We only return one full descriptor

            pWdmResourceList->List[0].PartialResourceList.Version  = 1;
            pWdmResourceList->List[0].PartialResourceList.Revision = 1;
            pWdmResourceList->List[0].PartialResourceList.Count = Count();

            pDescriptor =
                pWdmResourceList->List[0].PartialResourceList.PartialDescriptors;

            end = End();
            for (cur = Start(); cur != end; cur = cur->Next()) {
                FxResourceCm *pResource;

                pResource = (FxResourceCm*) cur->m_Object;

                RtlCopyMemory(pDescriptor,
                              &pResource->m_Descriptor,
                              sizeof(pResource->m_Descriptor));
                pDescriptor++;
            }
        }
    }

    return pWdmResourceList;
}

ULONG
FxCmResList::GetCount(
    VOID
    )
{
    ULONG count;
    KIRQL irql;

    Lock(&irql);
    count = Count();
    Unlock(irql);

    return count;
}

PCM_PARTIAL_RESOURCE_DESCRIPTOR
FxCmResList::GetDescriptor(
    __in ULONG Index
    )
{
    FxResourceCm* pObject;
    KIRQL irql;

    Lock(&irql);
    pObject = (FxResourceCm*) GetItem(Index);
    Unlock(irql);

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

_Must_inspect_result_
FxIoResReqList*
FxIoResReqList::_CreateFromWdmList(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PIO_RESOURCE_REQUIREMENTS_LIST WdmRequirementsList,
    __in UCHAR AccessFlags
    )
/*++

Routine Description:
    Allocates and populates an FxIoResReqList based on the WDM resource
    requirements list.

Arguments:
    WdmRequirementsList - a list of IO_RESOURCE_LISTs which will indicate how
                          to  fill in the returned collection object

    AccessFlags - permissions to associate with the newly created object

Return Value:
    a new object upon success, NULL upon failure

  --*/

{
    FxIoResReqList* pIoResReqList;
    ULONG i;

    pIoResReqList = new(FxDriverGlobals, WDF_NO_OBJECT_ATTRIBUTES)
        FxIoResReqList(FxDriverGlobals, AccessFlags);

    if (pIoResReqList != NULL) {
        PIO_RESOURCE_LIST pWdmResourceList;
        NTSTATUS status;

        if (WdmRequirementsList == NULL) {
            return pIoResReqList;
        }

        status = STATUS_SUCCESS;
        pWdmResourceList = &WdmRequirementsList->List[0];

        pIoResReqList->m_InterfaceType = WdmRequirementsList->InterfaceType;
        pIoResReqList->m_SlotNumber = WdmRequirementsList->SlotNumber;

        for (i = 0; i < WdmRequirementsList->AlternativeLists; i++) {
            FxIoResList *pResList;

            pResList = new(FxDriverGlobals, WDF_NO_OBJECT_ATTRIBUTES)
                FxIoResList(FxDriverGlobals, pIoResReqList);

            if (pResList != NULL) {
                status = pResList->AssignParentObject(pIoResReqList);

                //
                // Since we control our own lifetime, assigning the parent should
                // never fail.
                //
                ASSERT(NT_SUCCESS(status));

                status = pResList->BuildFromWdmList(&pWdmResourceList);
            }
            else {
                //
                // We failed to allocate a child collection.  Clean up
                // and break out of the loop.
                //
                status = STATUS_INSUFFICIENT_RESOURCES;
            }

            if (!NT_SUCCESS(status)) {
                break;
            }
        }

        if (!NT_SUCCESS(status)) {
            //
            // Cleanup and return a NULL object
            //
            pIoResReqList->DeleteObject();
            pIoResReqList = NULL;
        }
    }

    return pIoResReqList;
}

_Must_inspect_result_
PIO_RESOURCE_REQUIREMENTS_LIST
FxIoResReqList::CreateWdmList(
    VOID
    )
/*++

Routine Description:
    Creates a WDM io resource requirements list based off of the current
    contents of the collection

Arguments:
    None

Return Value:
    new WDM io resource requirements list allocated out of paged pool upon success,
    NULL upon failure or an empty list

  --*/
{
    PIO_RESOURCE_REQUIREMENTS_LIST pRequirementsList;
    FxCollectionEntry *cur, *end;
    NTSTATUS status;
    ULONG totalDescriptors;
    ULONG size;
    ULONG count;
    ULONG tmp;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    totalDescriptors = 0;
    pRequirementsList = NULL;

    count = Count();
    pFxDriverGlobals = GetDriverGlobals();

    if (count > 0) {
        //
        // The collection object should contain a set of child collections
        // with each of the various requirement lists.  Use the number of
        // these collections to determine the size of our requirements
        // list.
        //
        end = End();
        for (cur = Start(); cur != end; cur = cur->Next()) {
            status = RtlULongAdd(totalDescriptors,
                                 ((FxIoResList *) cur->m_Object)->Count(),
                                 &totalDescriptors);

            if (!NT_SUCCESS(status)) {
                goto Overflow;
            }
        }

        //
        // We now have enough information to determine how much memory we
        // need to allocate for our requirements list.
        //
        // size = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) +
        //        (sizeof(IO_RESOURCE_LIST) * (count - 1)) +
        //        (sizeof(IO_RESOURCE_DESCRIPTOR) * totalDescriptors) -
        //         (sizeof(IO_RESOURCE_DESCRIPTOR) * count);
        //
        // sizeof(IO_RESOURCE_DESCRIPTOR) * count is subtracted off because
        // each IO_RESOURCE_LIST has an embedded IO_RESOURCE_DESCRIPTOR in it
        // and we don't want to overallocated.
        //

        //
        // To handle overflow each mathematical operation is split out into an
        // overflow safe call.
        //
        size = sizeof(IO_RESOURCE_REQUIREMENTS_LIST);

        // sizeof(IO_RESOURCE_LIST) * (count - 1)
        status = RtlULongMult(sizeof(IO_RESOURCE_LIST), count - 1, &tmp);
        if (!NT_SUCCESS(status)) {
            goto Overflow;
        }

        status = RtlULongAdd(size, tmp, &size);
        if (!NT_SUCCESS(status)) {
            goto Overflow;
        }

        // (sizeof(IO_RESOURCE_DESCRIPTOR) * totalDescriptors)
        status = RtlULongMult(sizeof(IO_RESOURCE_DESCRIPTOR),
                              totalDescriptors,
                              &tmp);
        if (!NT_SUCCESS(status)) {
            goto Overflow;
        }

        status = RtlULongAdd(size, tmp, &size);
        if (!NT_SUCCESS(status)) {
            goto Overflow;
        }

        //  - sizeof(IO_RESOURCE_DESCRIPTOR) * Count() (note the subtraction!)
        status = RtlULongMult(sizeof(IO_RESOURCE_DESCRIPTOR), count, &tmp);
        if (!NT_SUCCESS(status)) {
            goto Overflow;
        }

        // Sub, not Add!
        status = RtlULongSub(size, tmp, &size);
        if (!NT_SUCCESS(status)) {
            goto Overflow;
        }

        pRequirementsList = (PIO_RESOURCE_REQUIREMENTS_LIST)
            MxMemory::MxAllocatePoolWithTag(PagedPool, size, pFxDriverGlobals->Tag);

        if (pRequirementsList != NULL) {
            PIO_RESOURCE_LIST pList;
            FxResourceIo *pResource;

            pList = pRequirementsList->List;

            //
            // Start by zero initializing our structure
            //
            RtlZeroMemory(pRequirementsList, size);

            //
            // InterfaceType and BusNumber are unused for WDM, but InterfaceType
            // is used by the arbiters.
            //
            pRequirementsList->InterfaceType = m_InterfaceType;

            pRequirementsList->SlotNumber = m_SlotNumber;

            //
            // Now populate the requirements list with the resources from
            // our collections.
            //
            pRequirementsList->ListSize = size;
            pRequirementsList->AlternativeLists = Count();

            end = End();
            for (cur = Start(); cur != end; cur = cur->Next()) {
                FxIoResList* pIoResList;
                PIO_RESOURCE_DESCRIPTOR pDescriptor;
                FxCollectionEntry *pIoResCur, *pIoResEnd;

                pIoResList = (FxIoResList*) cur->m_Object;

                pList->Version  = 1;
                pList->Revision = 1;
                pList->Count = pIoResList->Count();

                pDescriptor = pList->Descriptors;

                pIoResEnd = pIoResList->End();
                for (pIoResCur = pIoResList->Start();
                     pIoResCur != pIoResEnd;
                     pIoResCur = pIoResCur->Next()) {

                    pResource = (FxResourceIo *) pIoResCur->m_Object;
                    RtlCopyMemory(pDescriptor,
                                  &pResource->m_Descriptor,
                                  sizeof(pResource->m_Descriptor));
                    pDescriptor++;
                }

                pList = (PIO_RESOURCE_LIST) pDescriptor;
            }
        }
    }

    return pRequirementsList;

Overflow:
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                        "Integer overflow occured when computing size of "
                        "IO_RESOURCE_REQUIREMENTS_LIST");

    return NULL;
}
