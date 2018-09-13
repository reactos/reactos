/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    tpswork.h

Abstract:

    Worker thread classes. Moved out of tpsclass.h

    Contents:
        CIoWorkerThreadInfo
        CIoWorkerRequest
        CThreadPool

Author:

    Richard L Firth (rfirth) 08-Aug-1998

Revision History:

    08-Aug-1998 rfirth
        Created

--*/

//
// manifests
//

#define THREAD_CREATION_DAMPING_TIME    5000
#define NEW_THREAD_THRESHOLD            10
#define MIN_WORKER_THREADS              1
#define MAX_WORKER_THREADS              128
#define MAX_IO_WORKER_THREADS           256
#define MAX_QUEUE_DEPTH                 0
#define THREAD_IDLE_TIMEOUT             60000

#define TPS_ID                          0x80000000

//
// external data
//

extern DWORD g_dwWorkItemId;

//
// classes
//

//
// CIoWorkerThreadInfo
//

class CIoWorkerThreadInfo : public CDoubleLinkedListEntry {

private:

    HANDLE m_hThread;

public:

    CIoWorkerThreadInfo(CDoubleLinkedList * pList) {
        m_hThread = (HANDLE)-1;
        InsertHead(pList);
    }

    ~CIoWorkerThreadInfo() {

        ASSERT(m_hThread == NULL);

    }

    VOID SetHandle(HANDLE hThread) {
        m_hThread = hThread;
    }

    HANDLE GetHandle(VOID) const {
        return m_hThread;
    }
};

//
// CIoWorkerRequest
//

class CIoWorkerRequest {

private:

    LPTHREAD_START_ROUTINE m_pfnCallback;
    LPVOID m_pContext;

public:

    CIoWorkerRequest(LPTHREAD_START_ROUTINE pfnCallback, LPVOID pContext) {
        m_pfnCallback = pfnCallback;
        m_pContext = pContext;
    }

    LPTHREAD_START_ROUTINE GetCallback(VOID) const {
        return m_pfnCallback;
    }

    LPVOID GetContext(VOID) const {
        return m_pContext;
    }
};

//
// CThreadPool - maintains lists of work items, non-IO worker threads and
// IO worker threads
//

class CThreadPool {

private:

    //
    // private classes
    //

    //
    // CWorkItem - queued app-supplied functions, ordered by priority
    //

    class CWorkItem : public CPrioritizedListEntry {

    public:

        FARPROC m_function;
        ULONG_PTR m_context;
        DWORD_PTR m_tag;
        DWORD_PTR m_id;
        DWORD m_flags;
        HINSTANCE m_hInstModule;

        CWorkItem(FARPROC lpfn,
                  ULONG_PTR context,
                  LONG priority,
                  DWORD_PTR tag,
                  DWORD_PTR * pid,
                  LPCSTR pszModule,
                  DWORD flags
                  ) : CPrioritizedListEntry(priority)
        {
            m_function = lpfn;
            m_context = context;
            m_tag = tag;
            m_id = (DWORD_PTR)0;
            m_flags = flags;

            if (pszModule && *pszModule)
            {
                m_hInstModule = LoadLibrary(pszModule);

                if (!m_hInstModule)
                {
                    ASSERTMSG(FALSE, TEXT("CWorkItem::CWorkItem  - faild to load %hs (error = %d), worker thread could be abanonded!!"), pszModule, GetLastError());
                }
            }
            else
            {
                m_hInstModule = NULL;
            }

            if (pid) {
                m_id = (DWORD_PTR)++g_dwWorkItemId;
                *pid = m_id;
                m_flags |= TPS_ID;
            }
        }

        ~CWorkItem()
        {
            // we used to call FreeLibrary(m_hInstModule) here but we delete the workitem
            // when we grab it off of the queue (in RemoveWorkItem). so we have to wait until
            // we are actually done running the task before we call FreeLibaray()
        }

        BOOL Match(DWORD_PTR Tag, BOOL IsTag) {
            return IsTag
                ? ((m_flags & TPS_TAGGEDITEM) && (m_tag == Tag))
                : ((m_flags & TPS_ID) && (m_id == Tag));
        }

        BOOL IsLongExec(VOID) {
            return (m_flags & TPS_LONGEXECTIME) ? TRUE : FALSE;
        }
    };

    //
    // work item queue variables
    //

    CPrioritizedList m_queue;
    CCriticalSection_NoCtor m_qlock;
    HANDLE m_event;
    DWORD m_error;
    DWORD m_queueSize;
    DWORD m_qFactor;
    DWORD m_minWorkerThreads;
    DWORD m_maxWorkerThreads;
    DWORD m_maxQueueDepth;
    DWORD m_workerIdleTimeout;
    DWORD m_creationDelta;
    DWORD m_totalWorkerThreads;
    DWORD m_availableWorkerThreads;

#if DBG
    DWORD m_queueSizeMax;
    DWORD m_qFactorMax;
    DWORD m_maxWorkerThreadsCreated;
#endif

    //
    // private member functions
    //

    CWorkItem * DequeueWorkItem(VOID) {

        CWorkItem * pItem = NULL;

        if (!m_queue.IsEmpty()) {
            pItem = (CWorkItem *)m_queue.RemoveHead();
            --m_queueSize;
        }
        return pItem;
    }

    VOID
    Worker(
        VOID
        );

public:

    static
    VOID
    WorkerThread(
        VOID
        );

    BOOL Init(VOID) {
        m_queue.Init();
        m_qlock.Init();

        //
        // create auto-reset, initially unsignalled event
        //

        m_event = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_error = (m_event != NULL) ? ERROR_SUCCESS : GetLastError();
        m_queueSize = 0;
        m_qFactor = 0;
        m_minWorkerThreads = MIN_WORKER_THREADS;
        m_maxWorkerThreads = MAX_WORKER_THREADS;
        m_maxQueueDepth = MAX_QUEUE_DEPTH;
        m_workerIdleTimeout = THREAD_IDLE_TIMEOUT;
        m_creationDelta = THREAD_CREATION_DAMPING_TIME;
        m_totalWorkerThreads = 0;
        m_availableWorkerThreads = 0;

#if DBG
        m_queueSizeMax = 0;
        m_qFactorMax = 0;
        m_maxWorkerThreadsCreated = 0;
#endif

        return m_error == ERROR_SUCCESS;
    }

    VOID Terminate(DWORD Limit) {
        PurgeWorkItems();
        TerminateThreads(Limit);
        if (m_event != NULL) {

            BOOL bOk = CloseHandle(m_event);

            ASSERT(bOk);

            m_event = NULL;
        }
        m_qlock.Terminate();

        ASSERT(m_queue.IsEmpty());

//#if DBG
//char buf[256];
//wsprintf(buf,
//         "CThreadPool::Terminate(): m_queueSizeMax = %d, m_maxWorkerThreadsCreated = %d, m_qFactorMax = %d\n",
//         m_queueSizeMax,
//         m_maxWorkerThreadsCreated,
//         m_qFactorMax
//         );
//OutputDebugString(buf);
//#endif
    }

    DWORD GetError() const {
        return m_error;
    }

    VOID
    SetLimits(
        IN DWORD dwMinimumThreads,
        IN DWORD dwMaximumThreads,
        IN DWORD dwMaximumQueueDepth,
        IN DWORD dwThreadIdleTimeout,
        IN DWORD dwThreadCreationDelta
        )
    {
        m_minWorkerThreads = dwMinimumThreads;
        m_maxWorkerThreads = dwMaximumThreads;
        m_maxQueueDepth = dwMaximumQueueDepth;
        m_workerIdleTimeout = dwThreadIdleTimeout;
        m_creationDelta = dwThreadCreationDelta;
    }

    VOID MakeAvailable(VOID) {
        InterlockedIncrement((LPLONG)&m_availableWorkerThreads);
        if (m_qFactor == 0) {
            m_qFactor = 1;
        } else {
            m_qFactor <<= 1;
        }
#if DBG
        if (m_qFactor > m_qFactorMax) {
            m_qFactorMax = m_qFactor;
        }
#endif
    }

    VOID MakeUnavailable(VOID) {
        InterlockedDecrement((LPLONG)&m_availableWorkerThreads);
        m_qFactor >>= 1;
        if ((m_qFactor == 0) && (m_availableWorkerThreads != 0)) {
            m_qFactor = 1;
        }
    }

    DWORD
    QueueWorkItem(
        FARPROC pfnFunction,
        ULONG_PTR pContext,
        LONG lPriority,
        DWORD_PTR dwTag,
        DWORD_PTR * pdwId,
        LPCSTR pszModule,
        DWORD dwFlags
        )
    {
        //
        // add a work item to the queue at the appropriate place and create a
        // thread to handle it if necessary
        //

        CWorkItem * pItem = new CWorkItem(pfnFunction,
                                          pContext,
                                          lPriority,
                                          dwTag,
                                          pdwId,
                                          pszModule,
                                          dwFlags
                                          );

        if (pItem == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        m_qlock.Acquire();

        //
        // demand-thread work-items have the highest priority. Put at head of
        // queue, else insert based on priority
        //

        if (dwFlags & TPS_DEMANDTHREAD) {
            pItem->InsertHead(&m_queue);
        } else {
            m_queue.insert(pItem);
        }
        ++m_queueSize;
#if DBG
        if (m_queueSize > m_queueSizeMax) {
            m_queueSizeMax = m_queueSize;
        }
#endif

        //
        // determine whether we need to create a new thread:
        //
        //  * no available threads
        //  * work queue growing too fast
        //  * all available threads about to be taken by long-exec work items
        //

        BOOL bCreate = FALSE;
        DWORD error = ERROR_SUCCESS;

        if (m_queueSize > (m_availableWorkerThreads * m_qFactor)) {
            bCreate = TRUE;
        } else {

            DWORD i = 0;
            DWORD n = 0;
            CWorkItem * pItem = (CWorkItem *)m_queue.Next();

            while ((pItem != m_queue.Head()) && (i < m_availableWorkerThreads)) {
                if (pItem->IsLongExec()) {
                    ++n;
                }
                pItem = (CWorkItem *)pItem->Next();
                ++i;
            }
            if (n == m_availableWorkerThreads) {
                bCreate = TRUE;
            }
        }
        m_qlock.Release();
        if (bCreate) {
            // if the CreateWorkerThread fails, do NOT pass back an error code to the caller
            // since we've already added the workitem to the queue.  An error code will
            // likely result in the caller freeing the data for the work item. (saml 081799)
            CreateWorkerThread();
        }
        SetEvent(m_event);
        return error;
    }

    DWORD
    RemoveWorkItem(
        FARPROC * ppfnFunction,
        ULONG_PTR * pContext,
        HMODULE* hModuleToFree,
        DWORD * pdwFlags,
        DWORD dwTimeout
        )
    {
        BOOL bFirstTime = TRUE;
        DWORD dwWaitTime = dwTimeout;

        while (TRUE) {

            CWorkItem * pItem;

            //
            // first test the FIFO state without waiting for the event
            //

            if (!m_queue.IsEmpty())
            {
                m_qlock.Acquire();
                pItem = DequeueWorkItem();

                if (pItem != NULL)
                {
                    if (pItem->m_flags & TPS_LONGEXECTIME)
                    {
                        MakeUnavailable();
                    }

                    m_qlock.Release();
                    *ppfnFunction = pItem->m_function;
                    *pContext = pItem->m_context;
                    *pdwFlags = pItem->m_flags & ~TPS_RESERVED_FLAGS;
                    *hModuleToFree = pItem->m_hInstModule;
                    delete pItem;
                    
                    return ERROR_SUCCESS;
                }
                m_qlock.Release();
            }

            DWORD dwStartTime;

            if ((dwTimeout != INFINITE) && bFirstTime) {
                dwStartTime = GetTickCount();
            }

            //
            // if dwTimeout is 0 (poll) and we've already waited unsuccessfully
            // then we're done: we timed out
            //

            if ((dwTimeout == 0) && !bFirstTime) {
                break;
            }

            //
            // wait alertably: process I/O completions while we wait
            //
            // BUGBUG - we want MsgWaitForMultipleObjectsEx() here, but Win95
            //          doesn't support it
            //

            DWORD status = MsgWaitForMultipleObjects(1,
                                                     &m_event,
                                                     FALSE,
                                                     dwWaitTime,
                                                     //QS_ALLINPUT
                                                     QS_SENDMESSAGE | QS_KEY
                                                     );

            //
            // quit now if thread pool is terminating
            //

            if (g_bTpsTerminating) {
                break;
            }
            bFirstTime = FALSE;
            if ((status == WAIT_OBJECT_0) || (status == WAIT_IO_COMPLETION)) {

                //
                // we think there is something to remove from the FIFO or I/O
                // completed. If we're not waiting forever, update the time to
                // wait on the next iteration based on the time we started
                //

                if (dwTimeout != INFINITE) {

                    DWORD dwElapsedTime = GetTickCount() - dwStartTime;

                    if (dwElapsedTime > dwTimeout) {

                        //
                        // waited longer than requested. Don't wait again if
                        // we find there's nothing in the FIFO
                        //

                        dwWaitTime = 0;
                    } else {

                        //
                        // amount of time to wait next iteration is amount of
                        // time until expiration of originally specified period
                        //

                        dwWaitTime = dwTimeout - dwElapsedTime;
                    }
                }
                continue;
            } else if (status == WAIT_OBJECT_0 + 1) {

                MSG msg;

                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                    if (msg.message == WM_QUIT) {
                         return WAIT_ABANDONED;
                    } else {
                        DispatchMessage(&msg);
                    }
                }
                continue;
            }

            //
            // WAIT_TIMEOUT (or WAIT_ABANDONED (?))
            //

            break;
        }
        return WAIT_TIMEOUT;
    }

    DWORD RemoveTagged(DWORD_PTR Tag, BOOL IsTag) {

        DWORD count = 0;

        m_qlock.Acquire();

        CPrioritizedListEntry * pEntry = (CPrioritizedListEntry *)m_queue.Next();
        CPrioritizedListEntry * pPrev = (CPrioritizedListEntry *)m_queue.Head();

        while (pEntry != m_queue.Head()) {

            CWorkItem * pItem = (CWorkItem *)pEntry;

            if (pItem->Match(Tag, IsTag)) {
                pItem->Remove();
                --m_queueSize;
                delete pItem;
                ++count;
                if (!IsTag) {
                    break;
                }
            } else {
                pPrev = pEntry;
            }
            pEntry = (CPrioritizedListEntry *)pPrev->Next();
        }
        m_qlock.Release();
        return count;
    }

    DWORD GetQueueSize(VOID) const {
        return m_queueSize;
    }

    VOID PurgeWorkItems(VOID) {
        m_qlock.Acquire();

        CWorkItem * pItem;

        while ((pItem = DequeueWorkItem()) != NULL) {
            delete pItem;
        }
        m_qlock.Release();
    }

    VOID Signal(VOID) {
        if (m_event != NULL) {
            SetEvent(m_event);
        }
    }

    DWORD CreateWorkerThread(VOID) {

        HANDLE hThread;
        DWORD error = ERROR_SUCCESS;

        error = StartThread((LPTHREAD_START_ROUTINE)WorkerThread,
                            &hThread,
                            FALSE
                            );
        if (error == ERROR_SUCCESS) {
            AddWorker();
#if DBG
            if (m_totalWorkerThreads > m_maxWorkerThreadsCreated) {
                m_maxWorkerThreadsCreated = m_totalWorkerThreads;
            }
//char buf[256];
//wsprintf(buf, ">>>> started worker thread. Total = %d/%d. Avail = %d. Factor = %d/%d\n",
//         m_totalWorkerThreads,
//         m_maxWorkerThreadsCreated,
//         m_availableWorkerThreads,
//         m_qFactor,
//         m_qFactorMax
//         );
//OutputDebugString(buf);
#endif
            CloseHandle(hThread); // thread handle not required
            return ERROR_SUCCESS;
        }

        ASSERT(error == ERROR_SUCCESS);

        return error;
    }

    VOID TerminateThreads(DWORD Limit) {
        while (m_totalWorkerThreads > Limit) {
            Signal();
            SleepEx(0, FALSE);
        }
    }

    VOID AddWorker(VOID) {
        InterlockedIncrement((LPLONG)&m_totalWorkerThreads);
        MakeAvailable();
    }

    VOID RemoveWorker(VOID) {
        MakeUnavailable();
        InterlockedDecrement((LPLONG)&m_totalWorkerThreads);
    }
};
