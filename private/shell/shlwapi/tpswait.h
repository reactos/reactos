/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    tpswait.h

Abstract:

    Wait classes. Moved out of tpsclass.h

    Contents:
        CWait
        CWaitRequest
        CWaitAddRequest
        CWaitRemoveRequest
        CWaitThreadInfo

Author:

    Richard L Firth (rfirth) 08-Aug-1998

Revision History:

    08-Aug-1998 rfirth
        Created

--*/

//
// forward declarations
//

class CWaitThreadInfo;

//
// classes
//

//
// CWait
//

class CWait : public CTimedListEntry {

private:

    HANDLE m_hObject;
    WAITORTIMERCALLBACKFUNC m_pCallback;
    LPVOID m_pContext;
    CWaitThreadInfo * m_pThreadInfo;
    DWORD m_dwFlags;

public:

    CWait(HANDLE hObject,
          WAITORTIMERCALLBACKFUNC pCallback,
          LPVOID pContext,
          DWORD dwWaitTime,
          DWORD dwFlags,
          CWaitThreadInfo * pInfo
          ) : CTimedListEntry(dwWaitTime) {
        m_hObject = hObject;
        m_pCallback = pCallback;
        m_pContext = pContext;
        m_pThreadInfo = pInfo;
        m_dwFlags = dwFlags;
    }

    CWait() {
    }

    CWait * Next(VOID) {
        return (CWait *)CTimedListEntry::Next();
    }

    CWaitThreadInfo * GetThreadInfo(VOID) const {
        return m_pThreadInfo;
    }

    VOID Execute(BOOL bTimeout) {

        //
        // execute function in this thread if required to do so, else we run
        // the callback in a non-I/O worker thread
        //

        //
        // BUGBUG - can't do this: the callback types for Wait & Work requests
        //          are different: one takes 2 parameters, the other one. We
        //          can't make this change until this issue is resolved with
        //          NT guys
        //

        //if (m_dwFlags & WT_EXECUTEINWAITTHREAD) {
            m_pCallback(m_pContext, bTimeout != 0);
        //} else {
        //
        //    //
        //    // BUGBUG - would have to allocate object from heap to hold callback
        //    //          function, context & bTimeout parameters in order to pass
        //    //          them to worker thread (we only have access to one APC
        //    //          parameter and we'd have to nominate different APC)
        //    //
        //
        //    Ie_QueueUserWorkItem((LPTHREAD_START_ROUTINE)m_pCallback,
        //                         m_pContext,
        //                         FALSE
        //                         );
        //}
    }

    HANDLE GetHandle(VOID) const {
        return m_hObject;
    }

    BOOL IsNoRemoveItem(VOID) {
        return (m_dwFlags & SRWSO_NOREMOVE) ? TRUE : FALSE;
    }
};

//
// CWaitRequest
//

class CWaitRequest {

private:

    BOOL m_bCompleted;
    CWait * m_pWait;

public:

    CWaitRequest() {
        m_bCompleted = FALSE;
    }

    CWaitRequest(CWait * pWait) {
        m_bCompleted = FALSE;
        m_pWait = pWait;
    }

    VOID SetComplete(VOID) {
        m_bCompleted = TRUE;
    }

    VOID WaitForCompletion(VOID) {
        while (!m_bCompleted) {
            SleepEx(0, TRUE);
        }
    }

    VOID SetWaitPointer(CWait * pWait) {
        m_pWait = pWait;
    }

    CWait * GetWaitPointer(VOID) const {
        return m_pWait;
    }
};

//
// CWaitAddRequest
//

class CWaitAddRequest : public CWait, public CWaitRequest {

public:

    CWaitAddRequest(HANDLE hObject,
                    WAITORTIMERCALLBACKFUNC pCallback,
                    LPVOID pContext,
                    DWORD dwWaitTime,
                    DWORD dwFlags,
                    CWaitThreadInfo * pInfo
                    ) :
                    CWait(hObject, pCallback, pContext, dwWaitTime, dwFlags, pInfo),
                    CWaitRequest()
    {
    }
};

//
// CWaitRemoveRequest
//

class CWaitRemoveRequest : public CWaitRequest {

public:

    CWaitRemoveRequest(HANDLE hWait) : CWaitRequest((CWait *)hWait) {
    }
};

//
// CWaitThreadInfo
//

class CWaitThreadInfo : public CDoubleLinkedList, public CCriticalSection {

private:

    HANDLE m_hThread;
    DWORD m_dwObjectCount;
    HANDLE m_Objects[MAXIMUM_WAIT_OBJECTS];
    CWait * m_pWaiters[MAXIMUM_WAIT_OBJECTS];
    CWait m_Waiters[MAXIMUM_WAIT_OBJECTS];
    CDoubleLinkedList m_FreeList;
    CDoubleLinkedList m_WaitList;

public:

    CWaitThreadInfo(CDoubleLinkedList * pList) {
        CDoubleLinkedList::Init();
        m_hThread = NULL;
        m_dwObjectCount = 0;
        m_FreeList.Init();
        m_WaitList.Init();
        for (int i = 0; i < ARRAY_ELEMENTS(m_Waiters); ++i) {
            m_Waiters[i].InsertTail(&m_FreeList);
        }
        InsertHead(pList);
    }

    VOID SetHandle(HANDLE hThread) {
        m_hThread = hThread;
    }

    HANDLE GetHandle(VOID) const {
        return m_hThread;
    }

    DWORD GetObjectCount(VOID) const {
        return m_dwObjectCount;
    }

    BOOL IsAvailableEntry(VOID) const {
        return m_dwObjectCount < ARRAY_ELEMENTS(m_Objects);
    }

    BOOL IsInvalidHandle(DWORD dwIndex) {

        ASSERT(dwIndex < m_dwObjectCount);

        //
        // GetHandleInformation() doesn't exist on Win95
        //
        //
        //DWORD dwHandleFlags;
        //
        //return !GetHandleInformation(m_Objects[dwIndex], &dwHandleFlags);

        DWORD status = WaitForSingleObject(m_Objects[dwIndex], 0);

        if ((status == WAIT_FAILED) && (GetLastError() == ERROR_INVALID_HANDLE)) {
//#if DBG
//char buf[128];
//wsprintf(buf, "IsInvalidHandle(%d): handle %#x is invalid\n", dwIndex, m_Objects[dwIndex]);
//OutputDebugString(buf);
//#endif
            return TRUE;
        }
        return FALSE;
    }

    VOID Compress(DWORD dwIndex, DWORD dwCount = 1) {

        ASSERT(dwCount != 0);
        ASSERT((int)m_dwObjectCount > 0);

        if ((dwIndex + dwCount) < m_dwObjectCount) {
            RtlMoveMemory(&m_Objects[dwIndex],
                          &m_Objects[dwIndex + dwCount],
                          sizeof(m_Objects[0]) * (m_dwObjectCount - (dwIndex + dwCount))
                          );
            RtlMoveMemory(&m_pWaiters[dwIndex],
                          &m_pWaiters[dwIndex + dwCount],
                          sizeof(m_pWaiters[0]) * (m_dwObjectCount - (dwIndex + dwCount))
                          );
        }
        m_dwObjectCount -= dwCount;
    }

    VOID Expand(DWORD dwIndex) {
        RtlMoveMemory(&m_Objects[dwIndex],
                      &m_Objects[dwIndex + 1],
                      sizeof(m_Objects[0]) * (m_dwObjectCount - dwIndex)
                      );
        RtlMoveMemory(&m_pWaiters[dwIndex],
                      &m_pWaiters[dwIndex + 1],
                      sizeof(m_pWaiters[0]) * (m_dwObjectCount - dwIndex)
                      );
        ++m_dwObjectCount;

        ASSERT(m_dwObjectCount <= ARRAY_ELEMENTS(m_Objects));
    }

    //DWORD BuildList(VOID) {
    //
    //    //
    //    // PERF: only rebuild from changed index
    //    //
    //
    //    m_dwObjectCount = 0;
    //    for (CWait * pWait = (CWait *)m_WaitList.Next();
    //         pWait = pWait->Next();
    //         !m_WaitList.IsHead(pWait)) {
    //        m_pWaiters[m_dwObjectCount] = pWait;
    //        m_Objects[m_dwObjectCount] = pWait->GetHandle();
    //        ++m_dwObjectCount;
    //    }
    //    return GetWaitTime();
    //}

    DWORD Wait(DWORD dwTimeout = INFINITE) {

        //
        // if no objects in list, sleep alertably for the timeout period
        //

        if (m_dwObjectCount == 0) {
            SleepEx(dwTimeout, TRUE);
            return WAIT_IO_COMPLETION;
        }

        //
        // else wait alertably for the timeout period
        //

        ASSERT(m_dwObjectCount <= ARRAY_ELEMENTS(m_Objects));

        return WaitForMultipleObjectsEx(m_dwObjectCount,
                                        m_Objects,
                                        FALSE,  // fWaitAll
                                        dwTimeout,
                                        TRUE    // fAlertable
                                        );
    }

    DWORD GetWaitTime(VOID) {

        ASSERT(m_dwObjectCount <= ARRAY_ELEMENTS(m_Objects));

        if (m_dwObjectCount != 0) {

            CWait * pWaiter = m_pWaiters[0];
            DWORD dwWaitTime = pWaiter->GetWaitTime();

            if (dwWaitTime != INFINITE) {

                DWORD dwTimeNow = GetTickCount();
                DWORD dwTimeStamp = pWaiter->GetTimeStamp();

                if (dwTimeNow > dwTimeStamp + dwWaitTime) {

                    //
                    // first object expired already
                    //

                    return 0;
                }

                //
                // number of milliseconds until next waiter expires
                //

                return (dwTimeStamp + dwWaitTime) - dwTimeNow;
            }
        }

        //
        // nothing in list
        //

        return INFINITE;
    }

    CWait * GetFreeWaiter(VOID) {
        return (CWait *)m_FreeList.RemoveHead();
    }

    VOID InsertWaiter(CWait * pWait) {

        DWORD dwIndex = 0;
        BOOL bAtEnd = TRUE;
        CDoubleLinkedListEntry * pHead = m_WaitList.Head();

        ASSERT(m_dwObjectCount <= ARRAY_ELEMENTS(m_Objects));

        if ((m_dwObjectCount != 0) && !pWait->IsInfiniteTimeout()) {

            //
            // not infinite timeout. Find place in list to insert this object
            //

            //
            // PERF: typically, new wait will be longer than most currently in
            //       list, so should start from end of non-infinite timeouts
            //       and work backwards
            //

            for (; dwIndex < m_dwObjectCount; ++dwIndex) {
                if (pWait->ExpiryTime() < m_pWaiters[dwIndex]->ExpiryTime()) {
                    pHead = m_pWaiters[dwIndex]->Head();
                    bAtEnd = (dwIndex == (m_dwObjectCount - 1));
                    break;
                }
            }
        }

        //
        // insert the new wait object at the correct location
        //

        pWait->InsertTail(pHead);
        if (!bAtEnd && (m_dwObjectCount != 0)) {
            Expand(dwIndex);
        } else {
            dwIndex = m_dwObjectCount;
            ++m_dwObjectCount;
        }

        //
        // update object list and pointer list
        //

        m_Objects[dwIndex] = pWait->GetHandle();
        m_pWaiters[dwIndex] = pWait;
    }

    VOID RemoveWaiter(CWait * pWait, DWORD dwIndex) {

        //
        // remove the waiter from the wait list and add it back to the
        // free list
        //

        pWait->Remove();
        pWait->InsertTail(&m_FreeList);

        //
        // if the object was not at the end of the list then compress
        // the list
        //

        if (dwIndex != (m_dwObjectCount - 1)) {
            Compress(dwIndex, 1);
        } else {
            --m_dwObjectCount;
        }
    }

    VOID RemoveWaiter(DWORD dwIndex) {

        ASSERT(m_dwObjectCount <= ARRAY_ELEMENTS(m_Objects));

        RemoveWaiter(m_pWaiters[dwIndex], dwIndex);
    }

    BOOL RemoveWaiter(CWait * pWait) {

        ASSERT(m_dwObjectCount <= ARRAY_ELEMENTS(m_Objects));

        for (DWORD dwIndex = 0; dwIndex < m_dwObjectCount; ++dwIndex) {
            if (m_pWaiters[dwIndex] == pWait) {
                RemoveWaiter(pWait, dwIndex);
                return TRUE;
            }
        }
        return FALSE;
    }

    VOID ProcessTimeouts(VOID) {

        DWORD dwTimeNow = GetTickCount();
        DWORD dwCount = 0;

        ASSERT(m_dwObjectCount <= ARRAY_ELEMENTS(m_Objects));

        while (dwCount < m_dwObjectCount) {

            CWait * pWait = m_pWaiters[dwCount];

            //
            // if waiter has expired, invoke its callback then remove it from
            // the wait list and add back to the free list
            //

            if (pWait->IsTimedOut(dwTimeNow)) {
                pWait->Execute(TRUE);
                pWait->Remove();
                pWait->InsertTail(&m_FreeList);
                ++dwCount;
            } else {

                //
                // quit loop at first non-timed-out entry
                //

                break;
            }
        }

        ASSERT(dwCount != 0);

        if (dwCount != 0) {
            Compress(0, dwCount);
        }
    }

    VOID PurgeInvalidHandles(VOID) {

        DWORD dwCount = 0;
        DWORD dwIndex = 0;
        DWORD dwIndexStart = 0;

        ASSERT(m_dwObjectCount <= ARRAY_ELEMENTS(m_Objects));

        while (dwIndex < m_dwObjectCount) {

            CWait * pWait = m_pWaiters[dwIndex];

            //
            // if handle has become invalid, invoke the callback then remove it
            // from the wait list and add back to the free list
            //

            if (IsInvalidHandle(dwIndex)) {
                pWait->Execute(FALSE);
                pWait->Remove();
                pWait->InsertTail(&m_FreeList);
                if (dwIndexStart == 0) {
                    dwIndexStart = dwIndex;
                }
                ++dwCount;
            } else if (dwCount != 0) {
                Compress(dwIndexStart, dwCount);
                dwIndex = dwIndexStart - 1;
                dwIndexStart = 0;
                dwCount = 0;
            }
            ++dwIndex;
        }
        if (dwCount != 0) {
            Compress(dwIndexStart, dwCount);
        }
    }

    VOID ProcessCompletion(DWORD dwIndex) {

        CWait * pWait = m_pWaiters[dwIndex];

        pWait->Execute(FALSE);
        if (!pWait->IsNoRemoveItem()) {
            RemoveWaiter(dwIndex);
        }
    }
};
