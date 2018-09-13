/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    tpstimer.h

Abstract:

    Timer classes. Moved out of tpsclass.h

    Contents:
        CTimer
        CTimerQueueEntry
        CTimerQueueList
        CTimerQueue
        CTimerRequest
        CTimerQueueDeleteRequest
        CTimerAddRequest
        CTimerChangeRequest
        CTimerCancelRequest

Author:

    Richard L Firth (rfirth) 08-Aug-1998

Revision History:

    08-Aug-1998 rfirth
        Created

--*/

//
// manifests
//

#define TPS_TIMER_IN_USE    0x80000000
#define TPS_TIMER_CANCELLED 0x40000000

//
// external data
//

extern LONG g_UID;

//
// classes
//

//
// CTimer
//

class CTimer {

private:

    HANDLE m_hQueue;    // address of owning queue
    HANDLE m_hTimer;    // timer ordinal
    WAITORTIMERCALLBACKFUNC m_pfnCallback;
    LPVOID m_pContext;
    DWORD m_dwPeriod;
    DWORD m_dwFlags;

public:

    CTimer(HANDLE hQueue,
           WAITORTIMERCALLBACKFUNC pfnCallback,
           LPVOID pContext,
           DWORD dwPeriod,
           DWORD dwFlags
           )
    {
        m_hQueue = hQueue;

        //
        // BUGBUG - not industrial-strength: can have 2 timers with same ID
        //

        m_hTimer = (HANDLE)InterlockedIncrement(&g_UID);
        m_pfnCallback = pfnCallback;
        m_pContext = pContext;
        m_dwPeriod = dwPeriod;
        m_dwFlags = dwFlags;
    }

    HANDLE GetHandle(VOID) const {
        return m_hTimer;
    }

    HANDLE GetQueue(VOID) const {
        return m_hQueue;
    }

    VOID Execute(VOID) {
        if (m_dwFlags & TPS_EXECUTEIO) {

            //
            // BUGBUG - NT code does nothing with this flag. We should queue
            //          request to I/O worker thread
            //

            ASSERT(!(m_dwFlags & TPS_EXECUTEIO));

        }
        m_pfnCallback(m_pContext, TRUE);
    }

    VOID SetPeriod(DWORD dwPeriod) {
        m_dwPeriod = dwPeriod;
    }

    DWORD GetPeriod(VOID) const {
        return m_dwPeriod;
    }

    BOOL IsOneShot(VOID) {
        return GetPeriod() == 0;
    }

    VOID SetInUse(VOID) {
        m_dwFlags |= TPS_TIMER_IN_USE;
    }

    VOID ResetInUse(VOID) {
        m_dwFlags &= ~TPS_TIMER_IN_USE;
    }

    BOOL IsInUse(VOID) {
        return (m_dwFlags & TPS_TIMER_IN_USE) ? TRUE : FALSE;
    }

    VOID SetCancelled(VOID) {
        m_dwFlags |= TPS_TIMER_CANCELLED;
    }

    BOOL IsCancelled(VOID) {
        return (m_dwFlags & TPS_TIMER_CANCELLED) ? TRUE : FALSE;
    }
};

//
// CTimerQueueEntry
//

class CTimerQueueEntry : public CTimedListEntry, public CTimer {

private:

public:

    CDoubleLinkedList m_TimerList;

    CTimerQueueEntry(HANDLE hQueue,
                     WAITORTIMERCALLBACKFUNC pfnCallback,
                     LPVOID pContext,
                     DWORD dwDueTime,
                     DWORD dwPeriod,
                     DWORD dwFlags
                     ) :
                     CTimedListEntry(dwDueTime),
                     CTimer(hQueue,
                            pfnCallback,
                            pContext,
                            dwPeriod,
                            dwFlags
                            )
    {
        CDoubleLinkedListEntry::Init();
        m_TimerList.Init();
    }

    ~CTimerQueueEntry() {
        m_TimerList.Remove();
    }

    VOID SetPeriodicTime(VOID) {
        SetTimeStamp(ExpiryTime());
        SetWaitTime(GetPeriod());
    }

    CDoubleLinkedList * TimerListHead(VOID) {
        return m_TimerList.Head();
    }
};

//
// CTimerQueueList
//

class CTimerQueueList {

private:

    CDoubleLinkedList m_QueueList;
    CDoubleLinkedList m_TimerList;

public:

    VOID Init(VOID) {
        m_QueueList.Init();
        m_TimerList.Init();
    }

    CDoubleLinkedList * QueueListHead(VOID) {
        return m_QueueList.Head();
    }

    CDoubleLinkedList * TimerListHead(VOID) {
        return m_TimerList.Head();
    }

    CDoubleLinkedListEntry * FindQueue(CDoubleLinkedListEntry * pEntry) {
        return m_QueueList.FindEntry(pEntry);
    }

    BOOL Wait(VOID) {

        DWORD dwWaitTime = INFINITE;
        CTimedListEntry * pTimer = (CTimedListEntry * )m_TimerList.Next();

        ASSERT(pTimer != NULL);

        if (pTimer != (CTimedListEntry * )m_TimerList.Head()) {
            dwWaitTime = pTimer->TimeToWait();
        }

        //
        //  HACKHACK (tnoonan):  Can't just check for 0 since
        //  Win95 will always return WAIT_TIMEOUT (despite what
        //  the docs say).
        //

        DWORD dwResult = SleepEx(dwWaitTime, TRUE);

        return (dwResult == 0) || (dwResult == WAIT_TIMEOUT);
    }

    VOID ProcessCompletions(VOID) {

        //
        // run down list of all timers; for each expired timer, execute its
        // completion handler. If one-shot timer, delete it, else reset the
        // timer and re-insert it in the list
        //
        // If a timer is re-inserted further down the list, we may visit it
        // again before we have completed the traversal. This is OK: either it
        // has already expired, in which case we execute again, or it hasn't
        // expired, in which case we terminate the traversal
        //

        CTimerQueueEntry * pTimer;
        CTimerQueueEntry * pNext = (CTimerQueueEntry *)m_TimerList.Next();

        do {
            pTimer = pNext;
            if ((pTimer == (CTimerQueueEntry *)m_TimerList.Head())
            || !pTimer->IsTimedOut()) {
                break;
            }
            pNext = (CTimerQueueEntry * )pTimer->Next();
            pTimer->Remove();
            pTimer->SetInUse();
            pTimer->Execute();
            if (pTimer->IsOneShot() || pTimer->IsCancelled()) {
                delete pTimer;
            } else {
                pTimer->SetPeriodicTime();
                pTimer->ResetInUse();
                pTimer->InsertBack(m_TimerList.Head());
            }
        } while (TRUE);
    }
};

//
// CTimerQueue
//

class CTimerQueue : public CDoubleLinkedList {

private:

    CDoubleLinkedList m_TimerList;

public:

    CTimerQueue(CTimerQueueList * pList) {
        CDoubleLinkedList::Init();
        m_TimerList.Init();
        InsertTail(pList->QueueListHead());
    }

    ~CTimerQueue() {
        Remove();
    }

    CDoubleLinkedList * TimerListHead(VOID) {
        return m_TimerList.Head();
    }

    CTimerQueueEntry * FindTimer(HANDLE hTimer) {

        CDoubleLinkedListEntry * pEntry;

        for (pEntry = m_TimerList.Next();
             pEntry != m_TimerList.Head();
             pEntry = pEntry->Next()) {

            CTimerQueueEntry * pTimer;

            pTimer = CONTAINING_RECORD(pEntry, CTimerQueueEntry, m_TimerList);
            if (pTimer->GetHandle() == hTimer) {
                return pTimer;
            }
        }
        return NULL;
    }

    VOID DeleteTimers(VOID) {

        CDoubleLinkedListEntry * pEntry;

        for (pEntry = m_TimerList.Next();
             pEntry != m_TimerList.Head();
             pEntry = m_TimerList.Next()) {

            CTimerQueueEntry * pTimer;

            pTimer = CONTAINING_RECORD(pEntry, CTimerQueueEntry, m_TimerList);

            //
            // remove timer from global timer list (linked on CDoubleLinkedList)
            //

            pTimer->Remove();

            //
            // timer will be removed from m_TimerList by its destructor
            //

            delete pTimer;
        }
    }
};

//
// CTimerRequest
//

class CTimerRequest {

private:

    BOOL m_bCompleted;
    DWORD m_dwStatus;

public:

    CTimerRequest() {
        m_bCompleted = FALSE;
        m_dwStatus = ERROR_SUCCESS;
    }

    VOID SetComplete(VOID) {
        m_bCompleted = TRUE;
    }

    VOID WaitForCompletion(VOID) {
        while (!m_bCompleted) {
            SleepEx(0, TRUE);
        }
    }

    VOID SetStatus(DWORD dwStatus) {
        m_dwStatus = dwStatus;
    }

    VOID SetCompletionStatus(DWORD dwStatus) {
        SetStatus(dwStatus);
        SetComplete();
    }

    BOOL SetThreadStatus(VOID) {
        if (m_dwStatus == ERROR_SUCCESS) {
            return TRUE;
        }
        SetLastError(m_dwStatus);
        return FALSE;
    }
};

//
// CTimerQueueDeleteRequest
//

class CTimerQueueDeleteRequest : public CTimerRequest {

private:

    HANDLE m_hQueue;

public:

    CTimerQueueDeleteRequest(HANDLE hQueue) : CTimerRequest() {
        m_hQueue = hQueue;
    }

    HANDLE GetQueue(VOID) const {
        return m_hQueue;
    }
};

//
// CTimerAddRequest
//

class CTimerAddRequest : public CTimerQueueEntry, public CTimerRequest {

public:

    CTimerAddRequest(HANDLE hQueue,
                     WAITORTIMERCALLBACKFUNC pfnCallback,
                     LPVOID pContext,
                     DWORD dwDueTime,
                     DWORD dwPeriod,
                     DWORD dwFlags
                     ) :
                     CTimerQueueEntry(hQueue,
                                      pfnCallback,
                                      pContext,
                                      dwDueTime,
                                      dwPeriod,
                                      dwFlags
                                      ),
                     CTimerRequest()
    {
    }

    HANDLE GetHandle(VOID) const {
        return CTimer::GetHandle();
    }

    CTimerQueue * GetQueue(VOID) const {
        return (CTimerQueue *)CTimer::GetQueue();
    }
};

//
// CTimerChangeRequest
//

class CTimerChangeRequest : public CTimerRequest {

private:

    HANDLE m_hQueue;
    HANDLE m_hTimer;
    DWORD m_dwDueTime;
    DWORD m_dwPeriod;

public:

    CTimerChangeRequest(HANDLE hQueue,
                        HANDLE hTimer,
                        DWORD dwDueTime,
                        DWORD dwPeriod
                        ) :
                        CTimerRequest()
    {
        m_hQueue = hQueue;
        m_hTimer = hTimer;
        m_dwDueTime = dwDueTime;
        m_dwPeriod = dwPeriod;
    }

    HANDLE GetQueue(VOID) const {
        return m_hQueue;
    }

    HANDLE GetTimer(VOID) const {
        return m_hTimer;
    }

    DWORD GetDueTime(VOID) const {
        return m_dwDueTime;
    }

    DWORD GetPeriod(VOID) const {
        return m_dwPeriod;
    }
};

//
// CTimerCancelRequest
//

class CTimerCancelRequest : public CTimerRequest {

private:

    HANDLE m_hQueue;
    HANDLE m_hTimer;

public:

    CTimerCancelRequest(HANDLE hQueue, HANDLE hTimer) : CTimerRequest() {
        m_hQueue = hQueue;
        m_hTimer = hTimer;
    }

    HANDLE GetQueue(VOID) const {
        return m_hQueue;
    }

    HANDLE GetTimer(VOID) const {
        return m_hTimer;
    }
};
