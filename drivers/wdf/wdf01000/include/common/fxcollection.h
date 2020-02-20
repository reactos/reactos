#ifndef _FXCOLLECTION_H_
#define _FXCOLLECTION_H_

#include "common/fxstump.h"
#include "common/fxnonpagedobject.h"


class FxCollectionEntry : public FxStump {

    friend class FxCollection;
    friend class FxCollectionInternal;

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

    BOOLEAN
    Add(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxObject *Item
        );

    NTSTATUS
    Remove(
        __in ULONG Index
        );

    VOID
    Clear(
        VOID
        );

    _Must_inspect_result_
    FxCollectionEntry*
    FindEntry(
        __in ULONG Index
        );

    NTSTATUS
    RemoveEntry(
        __in FxCollectionEntry* Entry
        );

    VOID
    CleanupEntryObject(
        __in FxObject* Object
        )
    {
        Object->RELEASE(this);
    }

    VOID
    CleanupEntry(
        __in FxCollectionEntry* Entry
        );

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

protected:
    FxCollection(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in WDFTYPE Type,
        __in USHORT Size
        );

};

#endif //_FXCOLLECTION_H_