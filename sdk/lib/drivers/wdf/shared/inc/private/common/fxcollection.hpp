/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxCollection.hpp

Abstract:

    This module implements a simple collection class to operate on
    objects derived from FxObject.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXCOLLECTION_HPP_
#define _FXCOLLECTION_HPP_

class FxCollectionEntry : public FxStump {

    friend FxCollection;
    friend FxCollectionInternal;

protected:
    FxCollectionEntry(
        VOID
        )
    {
    }

public:
    FxObject *m_Object;

    LIST_ENTRY m_ListEntry;

public:
    FxCollectionEntry*
    Next(
        VOID
        )
    {
        return CONTAINING_RECORD(m_ListEntry.Flink, FxCollectionEntry, m_ListEntry);
    }
};

struct FxCollectionInternal {
protected:
    ULONG m_Count;

    LIST_ENTRY m_ListHead;

public:
    FxCollectionInternal(
        VOID
        );

    ~FxCollectionInternal(
        VOID
        );

    _Must_inspect_result_
    FxCollectionEntry*
    FindEntry(
        __in ULONG Index
        );

    _Must_inspect_result_
    FxCollectionEntry*
    FindEntryByObject(
        __in FxObject* Object
        );

    ULONG
    Count(
        VOID
        );

    BOOLEAN
    Add(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxObject *Item
        );

    _Must_inspect_result_
    FxObject *
    GetItem(
        __in ULONG Index
        );

    _Must_inspect_result_
    FxObject*
    GetFirstItem(
        VOID
        );

    _Must_inspect_result_
    FxObject*
    GetLastItem(
        VOID
        );

    NTSTATUS
    Remove(
        __in ULONG Index
        );

    VOID
    CleanupEntry(
        __in FxCollectionEntry* Entry
        );

    VOID
    CleanupEntryObject(
        __in FxObject* Object
        )
    {
        Object->RELEASE(this);
    }

    NTSTATUS
    RemoveEntry(
        __in FxCollectionEntry* Entry
        );

    _Must_inspect_result_
    NTSTATUS
    RemoveItem(
        __in FxObject* Item
        );

    _Must_inspect_result_
    FxCollectionEntry*
    Start(
        VOID
        )
    {
        return CONTAINING_RECORD(m_ListHead.Flink, FxCollectionEntry, m_ListEntry);
    }

    _Must_inspect_result_
    FxCollectionEntry*
    End(
        VOID
        )
    {
        return CONTAINING_RECORD(&m_ListHead, FxCollectionEntry, m_ListEntry);
    }

    VOID
    Clear(
        VOID
        );

protected:
    _Must_inspect_result_
    FxCollectionEntry*
    AllocateEntry(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        )
    {
        return new(FxDriverGlobals) FxCollectionEntry();
    }

    VOID
    AddEntry(
        __in FxCollectionEntry *Node,
        __in FxObject* Item
        )
    {
        Node->m_Object = Item;

        //
        // Refcount the item we are adding to the list.
        //
        Item->ADDREF(this);

        //
        // Increment the number of items in the collection.
        //
        m_Count++;
    }
};

class FxCollection : public FxNonPagedObject, public FxCollectionInternal {

public:
    FxCollection(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    ~FxCollection(
        VOID
        );

    BOOLEAN
    Add(
        __in FxObject *Item
        )
    {
        return FxCollectionInternal::Add(GetDriverGlobals(), Item);
    }

    VOID
    StealCollection(
        __in FxCollection* Collection
        );

protected:
    FxCollection(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in WDFTYPE Type,
        __in USHORT Size
        );

};

#endif // _FXCOLLECTION_HPP_
