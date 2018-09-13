/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    serialst.cxx

Abstract:

    Functions to deal with a serialized list. These are replaced by macros in
    the retail version

    Contents:
        [InitializeSerializedList]
        [TerminateSerializedList]
        [LockSerializedList]
        [UnlockSerializedList]
        [InsertAtHeadOfSerializedList]
        [InsertAtTailOfSerializedList]
        [RemoveFromSerializedList]
        [IsSerializedListEmpty]
        [HeadOfSerializedList]
        [TailOfSerializedList]
        [CheckEntryOnSerializedList]
        [(CheckEntryOnList)]
        SlDequeueHead
        SlDequeueTail
        IsOnSerializedList

Author:

    Richard L Firth (rfirth) 16-Feb-1995

Environment:

    Win-32 user level

Revision History:

    16-Feb-1995 rfirth
        Created

--*/

#include <wininetp.h>

#if INET_DEBUG

//
// manifests
//

#define SERIALIZED_LIST_SIGNATURE   'tslS'

//
// private prototypes
//

PRIVATE
DEBUG_FUNCTION
BOOL
CheckEntryOnList(
    IN PLIST_ENTRY List,
    IN PLIST_ENTRY Entry,
    IN BOOL ExpectedResult
    );

//
// data
//

BOOL fCheckEntryOnList = FALSE;
BOOL ReportCheckEntryOnListErrors = FALSE;

//
// functions
//


DEBUG_FUNCTION
VOID
InitializeSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    initializes a serialized list

Arguments:

    SerializedList  - pointer to SERIALIZED_LIST

Return Value:

    None.

--*/

{
    INET_ASSERT(SerializedList != NULL);

    SerializedList->Signature = SERIALIZED_LIST_SIGNATURE;
    SerializedList->LockCount = 0;

    INITIALIZE_RESOURCE_INFO(&SerializedList->ResourceInfo);

    InitializeListHead(&SerializedList->List);
    SerializedList->ElementCount = 0;
    InitializeCriticalSection(&SerializedList->Lock);
}


DEBUG_FUNCTION
VOID
TerminateSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Undoes InitializeSerializeList

Arguments:

    SerializedList  - pointer to serialized list to terminate

Return Value:

    None.

--*/

{
    INET_ASSERT(SerializedList != NULL);
    INET_ASSERT(SerializedList->Signature == SERIALIZED_LIST_SIGNATURE);
    INET_ASSERT(SerializedList->ElementCount == 0);

    if (SerializedList->ElementCount != 0) {

        DEBUG_PRINT(SERIALST,
                    ERROR,
                    ("list @ %#x has %d elements, first is %#x\n",
                    SerializedList,
                    SerializedList->ElementCount,
                    SerializedList->List.Flink
                    ));

    } else {

        INET_ASSERT(IsListEmpty(&SerializedList->List));

    }
    DeleteCriticalSection(&SerializedList->Lock);
}


DEBUG_FUNCTION
VOID
LockSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Acquires a serialized list locks

Arguments:

    SerializedList  - SERIALIZED_LIST to lock

Return Value:

    None.

--*/

{
    INET_ASSERT(SerializedList->Signature == SERIALIZED_LIST_SIGNATURE);
    INET_ASSERT(SerializedList->LockCount >= 0);

    EnterCriticalSection(&SerializedList->Lock);
    if (SerializedList->LockCount != 0) {

        INET_ASSERT(SerializedList->ResourceInfo.Tid == GetCurrentThreadId());

    }
    ++SerializedList->LockCount;
    SerializedList->ResourceInfo.Tid = GetCurrentThreadId();
}


DEBUG_FUNCTION
VOID
UnlockSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Releases a serialized list lock

Arguments:

    SerializedList  - SERIALIZED_LIST to unlock

Return Value:

    None.

--*/

{
    INET_ASSERT(SerializedList->Signature == SERIALIZED_LIST_SIGNATURE);
    INET_ASSERT(SerializedList->ResourceInfo.Tid == GetCurrentThreadId());
    INET_ASSERT(SerializedList->LockCount > 0);

    --SerializedList->LockCount;
    LeaveCriticalSection(&SerializedList->Lock);
}


DEBUG_FUNCTION
VOID
InsertAtHeadOfSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry
    )

/*++

Routine Description:

    Adds an item to the head of a serialized list

Arguments:

    SerializedList  - SERIALIZED_LIST to update

    Entry           - thing to update it with

Return Value:

    None.

--*/

{
    INET_ASSERT(Entry != &SerializedList->List);

    LockSerializedList(SerializedList);
    if (fCheckEntryOnList) {
        CheckEntryOnList(&SerializedList->List, Entry, FALSE);
    }
    InsertHeadList(&SerializedList->List, Entry);
    ++SerializedList->ElementCount;

    INET_ASSERT(SerializedList->ElementCount > 0);

    UnlockSerializedList(SerializedList);
}


DEBUG_FUNCTION
VOID
InsertAtTailOfSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry
    )

/*++

Routine Description:

    Adds an item to the head of a serialized list

Arguments:

    SerializedList  - SERIALIZED_LIST to update

    Entry           - thing to update it with

Return Value:

    None.

--*/

{
    INET_ASSERT(Entry != &SerializedList->List);

    LockSerializedList(SerializedList);
    if (fCheckEntryOnList) {
        CheckEntryOnList(&SerializedList->List, Entry, FALSE);
    }
    InsertTailList(&SerializedList->List, Entry);
    ++SerializedList->ElementCount;

    INET_ASSERT(SerializedList->ElementCount > 0);

    UnlockSerializedList(SerializedList);
}


VOID
DEBUG_FUNCTION
RemoveFromSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry
    )

/*++

Routine Description:

    Removes the entry from a serialized list

Arguments:

    SerializedList  - SERIALIZED_LIST to remove entry from

    Entry           - pointer to entry to remove

Return Value:

    None.

--*/

{
    INET_ASSERT((Entry->Flink != NULL) && (Entry->Blink != NULL));

    LockSerializedList(SerializedList);
    if (fCheckEntryOnList) {
        CheckEntryOnList(&SerializedList->List, Entry, TRUE);
    }

    INET_ASSERT(SerializedList->ElementCount > 0);

    RemoveEntryList(Entry);
    --SerializedList->ElementCount;
    Entry->Flink = NULL;
    Entry->Blink = NULL;
    UnlockSerializedList(SerializedList);
}


DEBUG_FUNCTION
BOOL
IsSerializedListEmpty(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Checks if a serialized list contains any elements

Arguments:

    SerializedList  - pointer to list to check

Return Value:

    BOOL

--*/

{
    LockSerializedList(SerializedList);

    INET_ASSERT(SerializedList->Signature == SERIALIZED_LIST_SIGNATURE);

    BOOL empty;

    if (IsListEmpty(&SerializedList->List)) {

        INET_ASSERT(SerializedList->ElementCount == 0);

        empty = TRUE;
    } else {

        INET_ASSERT(SerializedList->ElementCount != 0);

        empty = FALSE;
    }

    UnlockSerializedList(SerializedList);

    return empty;
}


DEBUG_FUNCTION
PLIST_ENTRY
HeadOfSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Returns the element at the tail of the list, without taking the lock

Arguments:

    SerializedList  - pointer to SERIALIZED_LIST

Return Value:

    PLIST_ENTRY
        pointer to element at tail of list

--*/

{
    INET_ASSERT(SerializedList->Signature == SERIALIZED_LIST_SIGNATURE);

    return SerializedList->List.Flink;
}


DEBUG_FUNCTION
PLIST_ENTRY
TailOfSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Returns the element at the tail of the list, without taking the lock

Arguments:

    SerializedList  - pointer to SERIALIZED_LIST

Return Value:

    PLIST_ENTRY
        pointer to element at tail of list

--*/

{
    INET_ASSERT(SerializedList->Signature == SERIALIZED_LIST_SIGNATURE);

    return SerializedList->List.Blink;
}


DEBUG_FUNCTION
BOOL
CheckEntryOnSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry,
    IN BOOL ExpectedResult
    )

/*++

Routine Description:

    Checks an entry exists (or doesn't exist) on a list

Arguments:

    SerializedList  - pointer to serialized list

    Entry           - pointer to entry

    ExpectedResult  - TRUE if expected on list, else FALSE

Return Value:

    BOOL
        TRUE    - expected result

        FALSE   - unexpected result

--*/

{
    INET_ASSERT(SerializedList->Signature == SERIALIZED_LIST_SIGNATURE);

    LockSerializedList(SerializedList);

    BOOL result;

    __try {
        result = CheckEntryOnList(&SerializedList->List, Entry, ExpectedResult);
    } __except(EXCEPTION_EXECUTE_HANDLER) {

        DEBUG_PRINT(SERIALST,
                    FATAL,
                    ("List @ %#x (%d elements) is bad\n",
                    SerializedList,
                    SerializedList->ElementCount
                    ));

        result = FALSE;
    }
    ENDEXCEPT
    UnlockSerializedList(SerializedList);

    return result;
}


PRIVATE
DEBUG_FUNCTION
BOOL
CheckEntryOnList(
    IN PLIST_ENTRY List,
    IN PLIST_ENTRY Entry,
    IN BOOL ExpectedResult
    )
{
    BOOLEAN found = FALSE;
    PLIST_ENTRY p;

    if (!IsListEmpty(List)) {
        for (p = List->Flink; p != List; p = p->Flink) {
            if (p == Entry) {
                found = TRUE;
                break;
            }
        }
    }
    if (found != ExpectedResult) {
        if (ReportCheckEntryOnListErrors) {

            LPSTR description;

            description = found
                        ? "Entry %#x already on list %#x\n"
                        : "Entry %#x not found on list %#x\n"
                        ;

            DEBUG_PRINT(SERIALST,
                        ERROR,
                        (description,
                        Entry,
                        List
                        ));

            DEBUG_BREAK(SERIALST);

        }
        return FALSE;
    }
    return TRUE;
}

#endif // INET_DEBUG

//
// functions that are always functions
//


LPVOID
SlDequeueHead(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Dequeues the element at the head of the queue and returns its address or
    NULL if the queue is empty

Arguments:

    SerializedList  - pointer to SERIALIZED_LIST to dequeue from

Return Value:

    LPVOID

--*/

{
    LPVOID entry;

    if (!IsSerializedListEmpty(SerializedList)) {
        LockSerializedList(SerializedList);
        if (!IsSerializedListEmpty(SerializedList)) {
            entry = (LPVOID)HeadOfSerializedList(SerializedList);
            RemoveFromSerializedList(SerializedList, (PLIST_ENTRY)entry);
        } else {
            entry = NULL;
        }
        UnlockSerializedList(SerializedList);
    } else {
        entry = NULL;
    }
    return entry;
}


LPVOID
SlDequeueTail(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Dequeues the element at the tail of the queue and returns its address or
    NULL if the queue is empty

Arguments:

    SerializedList  - pointer to SERIALIZED_LIST to dequeue from

Return Value:

    LPVOID

--*/

{
    LPVOID entry;

    if (!IsSerializedListEmpty(SerializedList)) {
        LockSerializedList(SerializedList);
        if (!IsSerializedListEmpty(SerializedList)) {
            entry = (LPVOID)TailOfSerializedList(SerializedList);
            RemoveFromSerializedList(SerializedList, (PLIST_ENTRY)entry);
        } else {
            entry = NULL;
        }
        UnlockSerializedList(SerializedList);
    } else {
        entry = NULL;
    }
    return entry;
}


BOOL
IsOnSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry
    )

/*++

Routine Description:

    Checks if an entry is on a serialized list. Useful to call before
    RemoveFromSerializedList() if multiple threads can remove the element

Arguments:

    SerializedList  - pointer to SERIALIZED_LIST

    Entry           - pointer to element to check

Return Value:

    BOOL
        TRUE    - Entry is on SerializedList

        FALSE   -   "    " not on     "

--*/

{
    BOOL onList = FALSE;
    LPVOID entry;

    if (!IsSerializedListEmpty(SerializedList)) {
        LockSerializedList(SerializedList);
        if (!IsSerializedListEmpty(SerializedList)) {
            for (PLIST_ENTRY entry = HeadOfSerializedList(SerializedList);
                entry != (PLIST_ENTRY)SlSelf(SerializedList);
                entry = entry->Flink) {

                if (entry == Entry) {
                    onList = TRUE;
                    break;
                }
            }
        }
        UnlockSerializedList(SerializedList);
    }
    return onList;
}
