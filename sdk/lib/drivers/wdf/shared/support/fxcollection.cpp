/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxCollection.cpp

Abstract:

    This module implements a simple collection class to operate on
    objects derived from FxObject.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#include "fxsupportpch.hpp"

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

VOID
FxCollectionInternal::Clear(
    VOID
    )
{
    while (!IsListEmpty(&m_ListHead)) {
        Remove(0);
    }
}

ULONG
FxCollectionInternal::Count(
    VOID
    )
{
    return m_Count;
}

BOOLEAN
FxCollectionInternal::Add(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in FxObject *Item
    )
{
    FxCollectionEntry *pNode;

    pNode = AllocateEntry(FxDriverGlobals);

    if (pNode != NULL) {
        InsertTailList(&m_ListHead, &pNode->m_ListEntry);

        AddEntry(pNode, Item);
    }

    return pNode != NULL;
}

_Must_inspect_result_
FxCollectionEntry*
FxCollectionInternal::FindEntry(
    __in ULONG Index
    )
{
    PLIST_ENTRY ple;
    ULONG i;

    if (Index >= m_Count) {
        return NULL;
    }

    for (i = 0, ple = m_ListHead.Flink;
         ple != &m_ListHead;
         ple = ple->Flink, i++) {
        if (i != Index) {
            continue;
        }

        return CONTAINING_RECORD(ple, FxCollectionEntry, m_ListEntry);
    }

    return NULL;
}

_Must_inspect_result_
FxCollectionEntry*
FxCollectionInternal::FindEntryByObject(
    __in FxObject* Object
    )
{
    PLIST_ENTRY ple;

    for (ple = m_ListHead.Flink; ple != &m_ListHead; ple = ple->Flink) {
        FxCollectionEntry* pNode;

        pNode = CONTAINING_RECORD(ple, FxCollectionEntry, m_ListEntry);
        if (pNode->m_Object == Object) {
            return pNode;
        }
    }

    return NULL;
}

NTSTATUS
FxCollectionInternal::Remove(
    __in ULONG Index
    )
{
    FxCollectionEntry *pNode;

    pNode = FindEntry(Index);

    if (pNode != NULL) {
        return RemoveEntry(pNode);
    }
    else {
        return STATUS_NOT_FOUND;
    }
}

_Must_inspect_result_
NTSTATUS
FxCollectionInternal::RemoveItem(
    __in FxObject* Item
    )
{
    FxCollectionEntry* pNode;

    pNode = FindEntryByObject(Item);

    if (pNode != NULL) {
        return RemoveEntry(pNode);
    }

    return STATUS_NOT_FOUND;
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

_Must_inspect_result_
FxObject*
FxCollectionInternal::GetItem(
    __in ULONG Index
    )

{
    FxCollectionEntry* pNode;

    pNode = FindEntry(Index);
    if (pNode != NULL) {
        return pNode->m_Object;
    }
    else {
        return NULL;
    }
}

_Must_inspect_result_
FxObject*
FxCollectionInternal::GetFirstItem(
    VOID
    )
{
    if (IsListEmpty(&m_ListHead)) {
        return NULL;
    }
    else {
        return CONTAINING_RECORD(m_ListHead.Flink,
                                 FxCollectionEntry,
                                 m_ListEntry)->m_Object;
    }
}

_Must_inspect_result_
FxObject*
FxCollectionInternal::GetLastItem(
    VOID
    )
{
    if (IsListEmpty(&m_ListHead)) {
        return NULL;
    }
    else {
        return CONTAINING_RECORD(m_ListHead.Blink,
                                 FxCollectionEntry,
                                 m_ListEntry)->m_Object;
    }
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
FxCollection::StealCollection(
    __in FxCollection* Collection
    )
{
    PLIST_ENTRY ple;

    m_Count = Collection->m_Count;
    Collection->m_Count = 0;

    while (!IsListEmpty(&Collection->m_ListHead)) {
        FxCollectionEntry* pEntry;

        ple = RemoveHeadList(&Collection->m_ListHead);
        pEntry = CONTAINING_RECORD(ple, FxCollectionEntry, m_ListEntry);

        //
        // When we are tracking reference tags, the tag associated with the
        // reference matters.  When we added the object to Collection, we used
        // that pointer as the tag.  We must remove that tag and readd the
        // reference using the this value as a tag.
        //
        // Obviously, order is important here.  Add the reference first so that
        // we know the relese will make the object go away.
        //
        pEntry->m_Object->ADDREF(this);
        pEntry->m_Object->RELEASE(Collection);

        InsertTailList(&m_ListHead, ple);
    }
}

