#include "common/fxdisposelist.h"


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

    if (previouslyEmpty)
    {
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
    while (m_List.Next != NULL)
    {
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
        if (m_List.Next == NULL)
        {
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
