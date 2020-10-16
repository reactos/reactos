/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDisposeList.hpp

Abstract:

    This class implements a Disposal list for deferring Dispose
    processing from dispatch to passive level.

    It works tightly with FxObject, and is a friend class.

Author:






Environment:

    Both kernel and user mode

Revision History:


--*/

#include "coreprivshared.hpp"

#include "fxdisposelist.hpp"

// Tracing support
extern "C" {
// #include "FxDisposeList.tmh"
}

FxDisposeList::FxDisposeList(
    PFX_DRIVER_GLOBALS FxDriverGlobals
    ) :
    FxNonPagedObject(FX_TYPE_DISPOSELIST, 0, FxDriverGlobals)
{
   m_List.Next = NULL;
   m_ListEnd = &m_List.Next;

   m_SystemWorkItem = NULL;
   m_WorkItemThread = NULL;
}

FxDisposeList::~FxDisposeList()
{
    ASSERT(m_List.Next == NULL);
}

NTSTATUS
FxDisposeList::_Create(
    PFX_DRIVER_GLOBALS FxDriverGlobals,
    PVOID              WdmObject,
    FxDisposeList**    pObject
    )
{
    FxDisposeList* list;
    NTSTATUS status;

    *pObject = NULL;

    list = new(FxDriverGlobals) FxDisposeList(FxDriverGlobals);

    if (list == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = list->Initialize(WdmObject);

    if (NT_SUCCESS(status)) {
        *pObject = list;
    }
    else {
        list->DeleteFromFailedCreate();
    }

    return status;
}

NTSTATUS
FxDisposeList::Initialize(
    PVOID          WdmObject
    )
{
    NTSTATUS status;
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();

    MarkDisposeOverride(ObjectDoNotLock);

    status = FxSystemWorkItem::_Create(FxDriverGlobals,
                                      WdmObject,
                                      &m_SystemWorkItem
                                      );
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "Could not allocate workitem: %!STATUS!", status);
        return status;
    }

    m_WdmObject = WdmObject;

    return STATUS_SUCCESS;
}

BOOLEAN
FxDisposeList::Dispose(
    )
{
    if (m_SystemWorkItem != NULL) {
        m_SystemWorkItem->DeleteObject();
        m_SystemWorkItem = NULL;
    }

    ASSERT(m_List.Next == NULL);

    FxNonPagedObject::Dispose(); // __super call

    return TRUE;
}

VOID
FxDisposeList::Add(
    FxObject* Object
    )

/*++

Routine Description:

    Add an object to the cleanup list.







    The caller is expected to manage any reference counts on
    the object while on the cleanup list

Arguments:

    object - Object to cleanup at passive level

Returns:

    None

--*/

{
    KIRQL irql;
    BOOLEAN previouslyEmpty;

    Lock(&irql);

    ASSERT(Object->m_DisposeSingleEntry.Next == NULL);

    previouslyEmpty = m_List.Next == NULL ? TRUE : FALSE;

    //
    // Add to the end of m_List
    //
    *m_ListEnd = &Object->m_DisposeSingleEntry;

    //
    // Update the end
    //
    m_ListEnd = &Object->m_DisposeSingleEntry.Next;

    if (previouslyEmpty) {
        m_SystemWorkItem->TryToEnqueue(_WorkItemThunk, this);
    }

    Unlock(irql);
}

VOID
FxDisposeList::DrainListLocked(
    PKIRQL PreviousIrql
    )
{
    FxObject* pObject;
    PSINGLE_LIST_ENTRY pEntry;

    //
    // Process items on the list until it is empty
    //
    while (m_List.Next != NULL) {
        pEntry = m_List.Next;

        //
        // Remove pEntry from the list
        //
        m_List.Next = pEntry->Next;

        //
        // Indicate pEntry is no longer in the list
        //
        pEntry->Next = NULL;

        //
        // Convert back to the object
        //
        pObject = FxObject::_FromDisposeEntry(pEntry);

        //
        // If the list is empty, we just popped off the entry and we need to
        // update m_ListEnd to a head of the list so it points to valid pool.
        //
        if (m_List.Next == NULL) {
            m_ListEnd = &m_List.Next;
        }

        Unlock(*PreviousIrql);

        //
        // Invoke the objects deferred dispose entry
        //
        pObject->DeferredDisposeWorkItem();

        //
        // pObject may be invalid at this point due to its dereferencing itself
        //
        Lock(PreviousIrql);
    }
}

VOID
FxDisposeList::_WorkItemThunk(
    __in PVOID Parameter
    )
{
    FxDisposeList* pThis;
    KIRQL irql;

    pThis = (FxDisposeList*) Parameter;

    pThis->Lock(&irql);

    pThis->DrainListLocked(&irql);

    pThis->Unlock(irql);
}

VOID
FxDisposeList::WaitForEmpty(
    )

/*++

Routine Description:

    Wait until the list goes empty with no items.

    Note, on wakeup, new items could have been added, only the assurance
    is that the list at least went empty for a moment.

    This allows a waiter to wait for all previous Add() items to
    finishing processing before return.

Arguments:

Returns:

--*/

{
    KIRQL irql;
    BOOLEAN wait;

    Lock(&irql);

    wait = TRUE;

    if (m_WorkItemThread == Mx::MxGetCurrentThread()) {







        ASSERT(FALSE);
        DrainListLocked(&irql);
        wait = FALSE;
    }

    Unlock(irql);

    if (wait) {
        m_SystemWorkItem->WaitForExit();
    }

    // Should only be true for an empty list
    ASSERT(m_List.Next == NULL);
}
