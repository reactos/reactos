/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    tpsclass.h

Abstract:

    Basic classes for Win32 thread pool services functions

    Contents:
        CCriticalSection_NoCtor
        CCriticalSection
        CDoubleLinkedListEntry
        CDoubleLinkedList
        CTimedListEntry
        CTimedList
        CPrioritizedListEntry
        CPrioritizedList

Author:

    Richard L Firth (rfirth) 11-Feb-1998

Notes:

    Some of these classes have no constructor so that we avoid requiring global
    object initialization (via main() e.g.) Therefore, these objects must be
    explicitly initialized through the Init() member

Revision History:

    11-Feb-1998 rfirth
        Created

--*/

// These linked-list helper macros and types are taken from
// ntdef.h and ntrtl.h.  We don't want to include those because
// we have no other reason, and including the nt headers as a
// win32 component causes compilation conflicts.

//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
//

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

//
//  PLIST_ENTRY
//  RemoveHeadList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

//
//  PLIST_ENTRY
//  RemoveTailList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveTailList(ListHead) \
    (ListHead)->Blink;\
    {RemoveEntryList((ListHead)->Blink)}

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

//
//  VOID
//  InsertHeadList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }

//
// data
//

extern const char g_cszShlwapi[];
extern DWORD g_ActiveRequests;
extern DWORD g_dwTerminationThreadId;
extern BOOL g_bTpsTerminating;
extern BOOL g_bDeferredWorkerTermination;
extern BOOL g_bDeferredWaiterTermination;
extern BOOL g_bDeferredTimerTermination;

EXTERN_C DWORD g_TpsTls;
EXTERN_C BOOL g_bDllTerminating;

//
// macros
//

#if !defined(ARRAY_ELEMENTS)
#define ARRAY_ELEMENTS(array)   (sizeof(array)/sizeof(array[0]))
#endif

#if !defined(LAST_ELEMENT)
#define LAST_ELEMENT(array)     (ARRAY_ELEMENTS(array) - 1)
#endif

#if !defined(FT2LL)
#define FT2LL(x)                (*(LONGLONG *)&(x))
#endif

//
// classes
//

//
// CCriticalSection_NoCtor - critical section class without constructor or
// destructor for use in global variables
//

class CCriticalSection_NoCtor {

private:

    CRITICAL_SECTION m_critsec;

public:

    VOID Init(VOID) {
        InitializeCriticalSection(&m_critsec);
    }

    VOID Terminate(VOID) {
        DeleteCriticalSection(&m_critsec);
    }

    VOID Acquire(VOID) {
        EnterCriticalSection(&m_critsec);
    }

    VOID Release(VOID) {
        LeaveCriticalSection(&m_critsec);
    }
};

//
// CCriticalSection
//

class CCriticalSection : public CCriticalSection_NoCtor {

public:

    CCriticalSection() {
        Init();
    }

    ~CCriticalSection() {
        Terminate();
    }
};

//
// CDoubleLinkedListEntry/CDoubleLinkedList
//

#define CDoubleLinkedList CDoubleLinkedListEntry

class CDoubleLinkedListEntry {

private:

    LIST_ENTRY m_List;

public:

    VOID Init(VOID) {
        InitializeListHead(&m_List);
    }

    CDoubleLinkedListEntry * Head(VOID) {
        return (CDoubleLinkedListEntry *)&m_List;
    }

    CDoubleLinkedListEntry * Next(VOID) {
        return (CDoubleLinkedListEntry *)m_List.Flink;
    }

    CDoubleLinkedListEntry * Prev(VOID) {
        return (CDoubleLinkedListEntry *)m_List.Blink;
    }

    BOOL IsHead(CDoubleLinkedListEntry * pEntry) {
        return pEntry == Head();
    }

    VOID InsertHead(CDoubleLinkedList * pList) {
        InsertHeadList(&pList->m_List, &m_List);
    }

    VOID InsertTail(CDoubleLinkedList * pList) {
        InsertTailList(&pList->m_List, &m_List);
    }

    VOID Remove(VOID) {
        RemoveEntryList(&m_List);
    }

    CDoubleLinkedListEntry * RemoveHead(VOID) {

        //
        // BUGBUG - (compiler?) for some reason, the line:
        //
        //  return (CDoubleLinkedListEntry *)RemoveHeadList(&List);
        //
        // returns the Flink pointer, but doesn't remove it from List
        //

        PLIST_ENTRY pEntry = RemoveHeadList(&m_List);

        return (CDoubleLinkedListEntry *)pEntry;
    }

    CDoubleLinkedListEntry * RemoveTail(VOID) {

        //
        // BUGBUG - see RemoveHead()
        //

        PLIST_ENTRY pEntry = RemoveTailList(&m_List);

        return (CDoubleLinkedListEntry *)pEntry;
    }

    BOOL IsEmpty(VOID) {
        return IsListEmpty(&m_List);
    }

    CDoubleLinkedListEntry * FindEntry(CDoubleLinkedListEntry * pEntry) {
        for (CDoubleLinkedListEntry * p = Next(); p != Head(); p = p->Next()) {
            if (p == pEntry) {
                return pEntry;
            }
        }
        return NULL;
    }
};

//
// CTimedListEntry/CTimedList
//

#define CTimedList CTimedListEntry

class CTimedListEntry : public CDoubleLinkedListEntry {

private:

    DWORD m_dwTimeStamp;
    DWORD m_dwWaitTime;

public:

    CTimedListEntry() {
    }

    CTimedListEntry(DWORD dwWaitTime) {
        m_dwTimeStamp = GetTickCount();
        m_dwWaitTime = dwWaitTime;
    }

    VOID Init(VOID) {
        CDoubleLinkedListEntry::Init();
        m_dwTimeStamp = 0;
        m_dwWaitTime = 0;
    }

    DWORD GetTimeStamp(VOID) const {
        return m_dwTimeStamp;
    }

    VOID SetTimeStamp(DWORD dwTimeStamp = GetTickCount()) {
        m_dwTimeStamp = dwTimeStamp;
    }

    DWORD GetWaitTime(VOID) const {
        return m_dwWaitTime;
    }

    VOID SetWaitTime(DWORD dwWaitTime) {
        m_dwWaitTime = dwWaitTime;
    }

    VOID SetExpirationTime(DWORD dwWaitTime) {
        SetTimeStamp();
        SetWaitTime(dwWaitTime);
    }

    BOOL IsTimedOut(DWORD dwTimeNow = GetTickCount()) const {
        return (m_dwWaitTime != INFINITE)
            ? (dwTimeNow >= (m_dwTimeStamp + m_dwWaitTime))
            : FALSE;
    }

    BOOL IsInfiniteTimeout(VOID) const {
        return (m_dwWaitTime == INFINITE);
    }

    DWORD ExpiryTime(VOID) const {
        return m_dwTimeStamp + m_dwWaitTime;
    }

    DWORD TimeToWait(DWORD dwTimeNow = GetTickCount()) {

        DWORD expiryTime = ExpiryTime();

        return IsInfiniteTimeout()
            ? INFINITE
            : ((dwTimeNow >= expiryTime)
                ? 0
                : expiryTime - dwTimeNow);
    }

    //BOOL InsertFront(CDoubleLinkedList * pList) {
    //
    //    DWORD dwExpiryTime = ExpiryTime();
    //    CTimedListEntry * pEntry;
    //
    //    for (pEntry = (CTimedListEntry *)pList->Next();
    //         pEntry != (CTimedListEntry *)pList->Head();
    //         pEntry = (CTimedListEntry *)pEntry->Next()) {
    //        if (dwExpiryTime < pEntry->ExpiryTime()) {
    //            break;
    //        }
    //    }
    //    InsertTail(pEntry);
    //    return this == pList->Next();
    //}

    BOOL InsertBack(CDoubleLinkedList * pList) {

        DWORD dwExpiryTime = ExpiryTime();
        CTimedListEntry * pEntry;

        for (pEntry = (CTimedListEntry *)pList->Prev();
             pEntry != (CTimedListEntry *)pList->Head();
             pEntry = (CTimedListEntry *)pEntry->Prev()) {
            if (dwExpiryTime >= pEntry->ExpiryTime()) {
                break;
            }
        }
        InsertTail(pEntry);
        return this == pList->Next();
    }
};

//
// CPrioritizedListEntry
//

class CPrioritizedListEntry : public CDoubleLinkedListEntry {

private:

    LONG m_lPriority;

public:

    CPrioritizedListEntry(LONG lPriority) {
        m_lPriority = lPriority;
    }

    LONG GetPriority(VOID) const {
        return m_lPriority;
    }

    VOID SetPriority(LONG lPriority) {
        m_lPriority = lPriority;
    }
};

//
// CPrioritizedList
//

class CPrioritizedList : public CDoubleLinkedList {

    //
    // PERF: this really needs to be a btree of list anchors
    //

public:

    VOID
    insert(
        IN CPrioritizedListEntry * pEntry,
        IN LONG lPriority
        ) {
        pEntry->SetPriority(lPriority);
        insert(pEntry);
    }

    VOID
    insert(
        IN CPrioritizedListEntry * pEntry
        ) {

        CPrioritizedListEntry * pCur;

        for (pCur = (CPrioritizedListEntry *)Next();
             pCur != (CPrioritizedListEntry *)Head();
             pCur = (CPrioritizedListEntry *)pCur->Next()) {

            if (pCur->GetPriority() < pEntry->GetPriority()) {
                break;
            }
        }
        pEntry->InsertHead((CDoubleLinkedListEntry *)pCur->Prev());
    }
};
