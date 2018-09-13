/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    priolist.hxx

Abstract:

    Contains Prioritized serialized list class definitions

Author:

    Richard L Firth (rfirth) 03-May-1997

Revision History:

    03-May-1997 rfirth
        Created

--*/

//
// classes
//

//
// CPriorityListEntry - prioritized list entry
//

class CPriorityListEntry {

public:

    LIST_ENTRY m_ListEntry;

private:

    LONG m_lPriority;

public:

    CPriorityListEntry(LONG lPriority) {
#if INET_DEBUG
        m_ListEntry.Flink = m_ListEntry.Blink = NULL;
#endif
        m_lPriority = lPriority;
    }

    ~CPriorityListEntry() {

        INET_ASSERT((m_ListEntry.Flink == NULL)
                    && (m_ListEntry.Blink == NULL));

    }

    PLIST_ENTRY List(VOID) {
        return &m_ListEntry;
    }

    PLIST_ENTRY Next(VOID) {
        return m_ListEntry.Flink;
    }

    PLIST_ENTRY Prev(VOID) {
        return m_ListEntry.Blink;
    }

    LONG GetPriority(VOID) const {
        return m_lPriority;
    }

    VOID SetPriority(LONG lPriority) {
        m_lPriority = lPriority;
    }

    VOID Remove(VOID) {
        RemoveEntryList(&m_ListEntry);
#if INET_DEBUG
        m_ListEntry.Flink = m_ListEntry.Blink = NULL;
#endif
    }
};

//
// CPriorityList - maintains serialized list of CPriorityListEntry's
//

class CPriorityList {

private:

    SERIALIZED_LIST m_List;

public:

    CPriorityList() {
        InitializeSerializedList(&m_List);
    }

    ~CPriorityList() {
        TerminateSerializedList(&m_List);
    }

    LPSERIALIZED_LIST List(VOID) {
        return &m_List;
    }

    PLIST_ENTRY Self(VOID) {
        return (PLIST_ENTRY)&m_List.List.Flink;
    }

    PLIST_ENTRY Head(VOID) {
        return m_List.List.Flink;
    }

    VOID Acquire(VOID) {
        LockSerializedList(&m_List);
    }

    VOID Release(VOID) {
        UnlockSerializedList(&m_List);
    }

    BOOL IsEmpty(VOID) {
        return IsSerializedListEmpty(&m_List);
    }

    LONG Count(VOID) {
        return ElementsOnSerializedList(&m_List);
    }

    VOID
    Insert(
        IN CPriorityListEntry * pEntry,
        IN LONG lPriority
        ) {
        pEntry->SetPriority(lPriority);
        Insert(pEntry);
    }

    VOID
    Insert(
        IN CPriorityListEntry * pEntry
        );

    VOID
    Remove(
        IN CPriorityListEntry * pEntry
        );

    CPriorityListEntry * RemoveHead(VOID) {
        return (CPriorityListEntry * )SlDequeueHead(&m_List);
    }
};
