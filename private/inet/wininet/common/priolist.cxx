/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    priolist.cxx

Abstract:

    Contains prioritized, serialized list class implementation

    Contents:
        CPriorityList::Insert
        CPriorityList::Remove

Author:

    Richard L Firth (rfirth) 03-May-1997

Notes:

    Properly, the CPriorityList class should extend a CSerializedList class, but
    we don't currently have one, just a serialized list type (common\serialst.cxx).

    WARNING: Code in this module makes assumptions about the contents of a
    SERIALIZED_LIST

Revision History:

    03-May-1997 rfirth
        Created

--*/

#include <wininetp.h>

//
// class methods
//


VOID
CPriorityList::Insert(
    IN CPriorityListEntry * pEntry
    )

/*++

Routine Description:

    Insert prioritized list entry into prioritized, serialized list

Arguments:

    pEntry  - pointer to prioritized list entry to add

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_UTIL,
                 None,
                 "CPriorityList::Insert",
                 "{%#x} %#x",
                 this,
                 pEntry
                 ));

    Acquire();

    INET_ASSERT(!IsOnSerializedList(&m_List, pEntry->List()));
    INET_ASSERT(pEntry->Next() == NULL);
    INET_ASSERT(pEntry->Prev() == NULL);

    CPriorityListEntry * pCur;

    for (pCur = (CPriorityListEntry *)m_List.List.Flink;
         pCur != (CPriorityListEntry *)&m_List.List.Flink;
         pCur = (CPriorityListEntry *)pCur->Next()) {

        if (pCur->GetPriority() < pEntry->GetPriority()) {
            break;
        }
    }
    InsertHeadList(pCur->Prev(), pEntry->List());
    ++m_List.ElementCount;
    Release();

    DEBUG_LEAVE(0);
}


VOID
CPriorityList::Remove(
    IN CPriorityListEntry * pEntry
    )

/*++

Routine Description:

    Remove entry from prioritized serialized list

Arguments:

    pEntry  - address of entry to remove

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_UTIL,
                 None,
                 "CPriorityList::Remove",
                 "{%#x} %#x",
                 this,
                 pEntry
                 ));

    Acquire();

    INET_ASSERT(IsOnSerializedList(&m_List, pEntry->List()));

    pEntry->Remove();
    --m_List.ElementCount;
    Release();

    DEBUG_LEAVE(0);
}
