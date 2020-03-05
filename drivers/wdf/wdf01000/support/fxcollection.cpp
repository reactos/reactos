#include "common/fxcollection.h"


FxCollectionInternal::FxCollectionInternal(
    VOID
    )
{
    m_Count = 0;
    InitializeListHead(&m_ListHead);
}

FxCollectionInternal::~FxCollectionInternal(
    VOID
    )
{
    Clear();
}

FxCollection::FxCollection(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    ) :
    FxNonPagedObject(FX_TYPE_COLLECTION, sizeof(FxCollection), FxDriverGlobals)
{
}

FxCollection::FxCollection(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in WDFTYPE Type,
    __in USHORT Size
    ) : FxNonPagedObject(Type, Size, FxDriverGlobals)
{
}

FxCollection::~FxCollection(
    VOID
    )
{
    Clear();
}

VOID
FxCollectionInternal::Clear(
    VOID
    )
{
    while (!IsListEmpty(&m_ListHead))
    {
        Remove(0);
    }
}

NTSTATUS
FxCollectionInternal::Remove(
    __in ULONG Index
    )
{
    FxCollectionEntry *pNode;

    pNode = FindEntry(Index);

    if (pNode != NULL)
    {
        return RemoveEntry(pNode);
    }
    else
    {
        return STATUS_NOT_FOUND;
    }
}

_Must_inspect_result_
FxCollectionEntry*
FxCollectionInternal::FindEntry(
    __in ULONG Index
    )
{
    PLIST_ENTRY ple;
    ULONG i;

    if (Index >= m_Count)
    {
        return NULL;
    }

    for (i = 0, ple = m_ListHead.Flink;
         ple != &m_ListHead;
         ple = ple->Flink, i++)
    {
        if (i != Index) {
            continue;
        }

        return CONTAINING_RECORD(ple, FxCollectionEntry, m_ListEntry);
    }

    return NULL;
}

VOID
FxCollectionInternal::CleanupEntry(
    __in FxCollectionEntry* Entry
    )
{
    RemoveEntryList(&Entry->m_ListEntry);
    delete Entry;

    m_Count--;
}

NTSTATUS
FxCollectionInternal::RemoveEntry(
    __in FxCollectionEntry* Entry
    )
{
    CleanupEntryObject(Entry->m_Object);
    CleanupEntry(Entry);

    return STATUS_SUCCESS;
}

BOOLEAN
FxCollectionInternal::Add(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in FxObject *Item
    )
{
    FxCollectionEntry *pNode;

    pNode = AllocateEntry(FxDriverGlobals);

    if (pNode != NULL)
    {
        InsertTailList(&m_ListHead, &pNode->m_ListEntry);

        AddEntry(pNode, Item);
    }

    return pNode != NULL;
}

ULONG
FxCollectionInternal::Count(
    VOID
    )
{
    return m_Count;
}
