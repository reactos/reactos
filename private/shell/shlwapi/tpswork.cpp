/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    tpswork.cpp

Abstract:

    Contains Win32 thread pool services worker thread functions

    Contents:
        SHSetThreadPoolLimits
        SHTerminateThreadPool
        SHQueueUserWorkItem
        SHCancelUserWorkItems
        TerminateWorkers
        TpsEnter
        (InitializeWorkerThreadPool)
        (StartIOWorkerThread)
        (QueueIOWorkerRequest)
        (IOWorkerThread)
        (ExecuteIOWorkItem)
        (CThreadPool::WorkerThread)
        (CThreadPool::Worker)

Author:

    Richard L Firth (rfirth) 10-Feb-1998

Environment:

    Win32 user-mode

Revision History:

    10-Feb-1998 rfirth
        Created

    12-Aug-1998 rfirth
        Rewritten for DEMANDTHREAD and LONGEXEC work items. Officially
        divergent from original which was based on NT5 base thread pool API

--*/

#include "priv.h"
#include "threads.h"
#include "tpsclass.h"
#include "tpswork.h"

//
// private prototypes
//

DWORD
InitializeWorkerThreadPool(
    VOID
    );

PRIVATE
DWORD
StartIOWorkerThread(
    VOID
    );

PRIVATE
DWORD
QueueIOWorkerRequest(
    IN LPTHREAD_START_ROUTINE pfnCallback,
    IN LPVOID pContext
    );

PRIVATE
VOID
IOWorkerThread(
    IN HANDLE hEvent
    );

PRIVATE
VOID
ExecuteIOWorkItem(
    IN CIoWorkerRequest * pPacket
    );

//
// global data
//

BOOL g_StartedWorkerInitialization = FALSE;
BOOL g_CompletedWorkerInitialization = FALSE;
BOOL g_bTpsTerminating = FALSE;

DWORD g_NumIoWorkerThreads = 0;
DWORD g_NumIoWorkRequests = 0;
DWORD g_LastIoThreadCreationTickCount = 0;
DWORD g_MaximumIoThreads = MAX_IO_WORKER_THREADS;
DWORD g_MaximumIoQueueDepth = NEW_THREAD_THRESHOLD;
DWORD g_ThreadCreationDelta = THREAD_CREATION_DAMPING_TIME;

CDoubleLinkedList g_IoWorkerThreads;
CCriticalSection_NoCtor g_IoWorkerCriticalSection;
CThreadPool g_ThreadPool;
DWORD g_dwWorkItemId = 0;

const char g_cszShlwapi[] = "SHLWAPI.DLL";

DWORD g_ActiveRequests = 0;
DWORD g_dwTerminationThreadId = 0;
BOOL g_bDeferredWorkerTermination = FALSE;

//
// functions
//

LWSTDAPI_(BOOL)
SHSetThreadPoolLimits(
    IN PSH_THREAD_POOL_LIMITS pLimits
    )

/*++

Routine Description:

    Change internal settings

Arguments:

    pLimits - pointer to SH_THREAD_POOL_LIMITS structure containing limits
              to set

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. See GetLastError() for more info

--*/

{
    if (!pLimits || (pLimits->dwStructSize != sizeof(SH_THREAD_POOL_LIMITS))) {
        return ERROR_INVALID_PARAMETER;
    }

    BOOL success = FALSE;
    DWORD error = ERROR_SHUTDOWN_IN_PROGRESS; // BUGBUG - error code?

    if (!g_bTpsTerminating) {
        InterlockedIncrement((LPLONG)&g_ActiveRequests);
        if (!g_bTpsTerminating) {
            error = ERROR_SUCCESS;
            if (!g_CompletedWorkerInitialization) {
                error = InitializeWorkerThreadPool();
            }
            if (error == ERROR_SUCCESS) {
                g_ThreadPool.SetLimits(pLimits->dwMinimumWorkerThreads,
                                       pLimits->dwMaximumWorkerThreads,
                                       pLimits->dwMaximumWorkerQueueDepth,
                                       pLimits->dwWorkerThreadIdleTimeout,
                                       pLimits->dwWorkerThreadCreationDelta
                                       );
                g_MaximumIoThreads = pLimits->dwMaximumIoWorkerThreads;
                g_MaximumIoQueueDepth = pLimits->dwMaximumIoWorkerQueueDepth;
                g_ThreadCreationDelta = pLimits->dwIoWorkerThreadCreationDelta;
                success = TRUE;
            }
        }
        InterlockedDecrement((LPLONG)&g_ActiveRequests);
    }
    if (success) {
        return success;
    }
    SetLastError(error);
    return success;
}

LWSTDAPI_(BOOL)
SHTerminateThreadPool(
    VOID
    )

/*++

Routine Description:

    Required to clean up threads before unloading SHLWAPI

Arguments:

    None.

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE

--*/

{
    if (InterlockedExchange((PLONG)&g_bTpsTerminating, TRUE)) {
        return TRUE;
    }

    //
    // wait until all in-progress requests have finished
    //

    while (g_ActiveRequests != 0) {
        SleepEx(0, FALSE);
    }

    //
    // kill all I/O worker threads. Queued work items will be lost
    //

    TerminateWorkers();

    //
    // kill all timer threads
    //

    TerminateTimers();

    //
    // kill all wait threads
    //

    TerminateWaiters();

    if (!g_bDeferredWorkerTermination
        && !g_bDeferredTimerTermination
        && !g_bDeferredWaiterTermination) {
        g_dwTerminationThreadId = 0;
        g_bTpsTerminating = FALSE;
    } else {
        g_dwTerminationThreadId = GetCurrentThreadId();
    }
    return TRUE;
}

LWSTDAPI_(BOOL)
SHQueueUserWorkItem(
    IN LPTHREAD_START_ROUTINE pfnCallback,
    IN LPVOID pContext,
    IN LONG lPriority,
    IN DWORD_PTR dwTag,
    OUT DWORD_PTR * pdwId OPTIONAL,
    IN LPCSTR pszModule OPTIONAL,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Queues a work item and associates a user-supplied tag for use by
    SHCancelUserWorkItems()

    N.B. IO work items CANNOT be cancelled due to the fact that they are
    queued as APCs and there is no OS support to revoke an APC

Arguments:

    pfnCallback - caller-supplied function to call

    pContext    - caller-supplied context argument to pfnCallback

    lPriority   - relative priority of non-IO work item. Default is 0

    dwTag       - caller-supplied tag for non-IO work item if TPS_TAGGEDITEM

    pdwId       - pointer to returned ID. Pass NULL if not required. ID will be
                  0 for an IO work item

    pszModule   - if specified, name of library (DLL) to load and free so that
                  the dll will reamin in our process for the lifetime of the work
                  item.

    dwFlags     - flags modifying request:

                    TPS_EXECUTEIO
                        - execute work item in I/O thread. If set, work item
                          cannot be tagged (and therefore cannot be subsequently
                          cancelled) and cannot have an associated priority
                          (both tag and priority are ignored for I/O work items)

                    TPS_TAGGEDITEM
                        - the dwTag field is meaningful

                    TPS_DEMANDTHREAD
                        - a thread will be created for this work item if one is
                          not currently available. DEMANDTHREAD work items are
                          queued at the head of the work queue. That is, they
                          get the highest priority

                    TPS_LONGEXECTIME
                        - caller expects this work item to take relatively long
                          time to complete (e.g. it could be in a UI loop). Work
                          items thus described remove a thread from the pool for
                          an indefinite amount of time

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. See GetLastError() for more info

--*/

{
    DWORD error;

    if (dwFlags & TPS_INVALID_FLAGS) {
        error = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    error = TpsEnter();
    if (error != ERROR_SUCCESS) {
        goto exit;
    }

    if (!g_CompletedWorkerInitialization) {
        error = InitializeWorkerThreadPool();
        if (error != ERROR_SUCCESS) {
            goto leave;
        }
    }

    if (!(dwFlags & TPS_EXECUTEIO)) {
        error = g_ThreadPool.QueueWorkItem((FARPROC)pfnCallback,
                                           (ULONG_PTR)pContext,
                                           lPriority,
                                           dwTag,
                                           pdwId,
                                           pszModule,
                                           dwFlags
                                           );

        ASSERT(error == ERROR_SUCCESS);

    } else {

        DWORD threshold = (g_NumIoWorkerThreads < g_MaximumIoThreads)
                        ? (g_MaximumIoQueueDepth * g_NumIoWorkerThreads)
                        : 0xffffffff;

        g_IoWorkerCriticalSection.Acquire();

        if ((g_NumIoWorkerThreads == 0)
            || ((g_NumIoWorkRequests > threshold)
            && (g_LastIoThreadCreationTickCount + g_ThreadCreationDelta
                < GetTickCount()))) {
            error = StartIOWorkerThread();
        }
        if (error == ERROR_SUCCESS) {
            error = QueueIOWorkerRequest(pfnCallback, pContext);
        }

        g_IoWorkerCriticalSection.Release();

        if (pdwId != NULL) {
            *pdwId = (DWORD_PTR)NULL;
        }

        ASSERT(error == ERROR_SUCCESS);

    }

leave:

    TpsLeave();

exit:

    BOOL success = TRUE;

    if (error != ERROR_SUCCESS) {
        SetLastError(error);
        success = FALSE;
    }
    return success;
}

LWSTDAPI_(DWORD)
SHCancelUserWorkItems(
    IN DWORD_PTR dwTagOrId,
    IN BOOL bTag
    )

/*++

Routine Description:

    Cancels one or more queued work items. By default, if ID is supplied, only
    one work item can be cancelled. If tag is supplied, all work items with same
    tag will be deleted

Arguments:

    dwTagOrId   - user-supplied tag or API-supplied ID of work item(s) to
                  cancel. Used as search key

    bTag        - TRUE if dwTagOrId is tag else ID

Return Value:

    DWORD
        Success - Number of work items successfully cancelled (0..0xFFFFFFFE)

        Failure - 0xFFFFFFFF. Use GetLastError() for more info

                    ERROR_SHUTDOWN_IN_PROGRESS
                        - DLL being unloaded/support terminated

--*/

{
    DWORD error = TpsEnter();
    DWORD result = 0xFFFFFFFF;

    if (error == ERROR_SUCCESS) {
        if (g_CompletedWorkerInitialization) {
            result = g_ThreadPool.RemoveTagged(dwTagOrId, bTag);
        }
        TpsLeave();
    }
    if (result != 0xFFFFFFFF) {
        return result;
    }
    SetLastError(error);
    return result;
}

VOID
TerminateWorkers(
    VOID
    )

/*++

Routine Description:

    Terminate worker threads

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (g_CompletedWorkerInitialization) {
        g_IoWorkerCriticalSection.Acquire();
        while (!g_IoWorkerThreads.IsEmpty()) {

            CIoWorkerThreadInfo * pInfo;

            pInfo = (CIoWorkerThreadInfo *)g_IoWorkerThreads.RemoveHead();

            HANDLE hThread = pInfo->GetHandle();

            if ((hThread != NULL) && (hThread != (HANDLE)-1)) {
                pInfo->SetHandle(NULL);
                QueueNullFunc(hThread);
                SleepEx(0, TRUE);
            }
        }
        g_IoWorkerCriticalSection.Release();
        g_IoWorkerCriticalSection.Terminate();

        //
        // protect ourselves against termination from within a thread pool
        // thread
        //

        DWORD_PTR sig = (DWORD_PTR)TlsGetValue(g_TpsTls);
        DWORD limit = (sig == TPS_WORKER_SIGNATURE) ? 1 : 0;

        g_ThreadPool.Terminate(limit);

        g_StartedWorkerInitialization = FALSE;
        g_CompletedWorkerInitialization = FALSE;
        g_NumIoWorkerThreads = 0;
        g_NumIoWorkRequests = 0;
        g_LastIoThreadCreationTickCount = 0;
        g_MaximumIoThreads = MAX_IO_WORKER_THREADS;
        g_MaximumIoQueueDepth = NEW_THREAD_THRESHOLD;
        g_ThreadCreationDelta = THREAD_CREATION_DAMPING_TIME;
        g_dwWorkItemId = 0;

        if ((sig == TPS_IO_WORKER_SIGNATURE) || (sig == TPS_WORKER_SIGNATURE)) {
            g_bDeferredWorkerTermination = TRUE;
        }
    }
}

//
// private functions
//

PRIVATE
DWORD
InitializeWorkerThreadPool(
    VOID
    )

/*++

Routine Description:

    This routine initializes all aspects of the thread pool.

Arguments:

    None.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DWORD error = ERROR_SUCCESS;

    if (!InterlockedExchange((LPLONG)&g_StartedWorkerInitialization, TRUE)) {
        g_IoWorkerCriticalSection.Init();
        g_IoWorkerThreads.Init();
        g_ThreadPool.Init();

        //
        // signal that initialization has completed
        //

        g_CompletedWorkerInitialization = TRUE;
    } else {

        //
        // relinquish the timeslice until the other thread has initialized
        //

        while (!g_CompletedWorkerInitialization) {
            SleepEx(0, FALSE);
        }
    }
    if (error == ERROR_SUCCESS) {
        error = g_ThreadPool.GetError();

        ASSERT(error == ERROR_SUCCESS);
    }
    return error;
}

PRIVATE
DWORD
StartIOWorkerThread(
    VOID
    )

/*++

Routine Description:

    This routine starts an I/O worker thread

Arguments:

    None.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    HANDLE hThread;
    DWORD error = StartThread((LPTHREAD_START_ROUTINE)IOWorkerThread,
                              &hThread,
                              TRUE
                              );

    if (error == ERROR_SUCCESS) {

        //
        // update the time at which the current thread was created
        //

        InterlockedExchange((LPLONG)&g_LastIoThreadCreationTickCount,
                            (LONG)GetTickCount()
                            );

        //
        // we have the g_IoWorkerCriticalSection. We know the CIoWorkerThreadInfo
        // added at the head is the one we just created
        //

        ((CIoWorkerThreadInfo *)g_IoWorkerThreads.Next())->SetHandle(hThread);

        //
        // increment the count of the thread type created
        //

        InterlockedIncrement((LPLONG)&g_NumIoWorkerThreads);
    } else {

        //
        // thread creation failed. If there is even one thread present do not
        // return failure since we can still service the work request.
        //

        if (g_NumIoWorkerThreads != 0) {
            error = ERROR_SUCCESS;
        }
    }
    return error;
}

PRIVATE
DWORD
QueueIOWorkerRequest(
    IN LPTHREAD_START_ROUTINE pfnCallback,
    IN LPVOID pContext
    )

/*++

Routine Description:

    This routine queues up the request to be executed in a IO worker thread.

Arguments:

    pfnCallback - Routine that is called by the worker thread

    pContext    - Opaque pointer passed in as an argument to pfnCallback

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    //
    // since we don't have access to the additional stack parameters of
    // NtQueueApcThread, we must allocate a packet off the heap in which
    // to pass the parameters
    //

    //
    // PERF: use a pre-allocated cache of request packets
    //

    CIoWorkerRequest * pPacket = new CIoWorkerRequest(pfnCallback, pContext);

    if (pPacket == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // increment the outstanding work request counter
    //

    InterlockedIncrement((LPLONG)&g_NumIoWorkRequests);

    //
    // in order to implement "fair" assignment of work items between IO worker
    // threads each time remove from head of list and reinsert at back. Keep
    // the pointer for thread handle
    //

    ASSERT(!g_IoWorkerThreads.IsEmpty());

    CIoWorkerThreadInfo * pInfo = (CIoWorkerThreadInfo *)g_IoWorkerThreads.RemoveHead();
    pInfo->InsertTail(&g_IoWorkerThreads);

    //
    // queue an APC to the IO worker thread. The worker thread will free the
    // request packet
    //

    BOOL bOk = QueueUserAPC((PAPCFUNC)ExecuteIOWorkItem,
                            pInfo->GetHandle(),
                            (ULONG_PTR)pPacket
                            );

    DWORD error = ERROR_SUCCESS;

    if (!bOk) {

        //
        // GetLastError() before ASSERT()!
        //

        error = GetLastError();
    }

    ASSERT(error == ERROR_SUCCESS);

    return error;
}

PRIVATE
VOID
IOWorkerThread(
    IN HANDLE hEvent
    )

/*++

Routine Description:

    All I/O worker threads execute in this routine. All the work requests execute as APCs
    in this thread.

Arguments:

    hEvent  - event handle signalled when initialization complete

Return Value:

    None.

--*/

{
    HMODULE hDll = LoadLibrary(g_cszShlwapi);

    ASSERT(hDll != NULL);
    ASSERT(g_TpsTls != 0xFFFFFFFF);

    TlsSetValue(g_TpsTls, (LPVOID)TPS_IO_WORKER_SIGNATURE);

    CIoWorkerThreadInfo info(g_IoWorkerThreads.Head());

    SetEvent(hEvent);

    while (!g_bTpsTerminating) {
        SleepEx(INFINITE, TRUE);
    }
    InterlockedDecrement((LPLONG)&g_NumIoWorkerThreads);
    while (info.GetHandle() != NULL) {
        SleepEx(0, FALSE);
    }
    if (GetCurrentThreadId() == g_dwTerminationThreadId) {
        g_bTpsTerminating = FALSE;
        g_bDeferredWorkerTermination = FALSE;
        g_dwTerminationThreadId = 0;
    }
    FreeLibraryAndExitThread(hDll, ERROR_SUCCESS);
}

PRIVATE
VOID
ExecuteIOWorkItem(
    IN CIoWorkerRequest * pPacket
    )

/*++

Routine Description:

    Executes an IO Work function. Runs in a APC in the IO Worker thread

Arguments:

    pPacket - pointer to CIoWorkerRequest allocated by the requesting
              thread. We need to free it/return it to packet cache

Return Value:

    None.

--*/

{
    LPTHREAD_START_ROUTINE fn = pPacket->GetCallback();
    LPVOID ctx = pPacket->GetContext();

    delete pPacket;

    if (!g_bTpsTerminating) {
        fn(ctx);
        InterlockedDecrement((LPLONG)&g_NumIoWorkRequests);
    }
}

VOID
CThreadPool::WorkerThread(
    VOID
    )

/*++

Routine Description:

    Static thread function. Instantiates the thread pool object pointer and
    calls the non-IO worker member function

Arguments:

    None.

Return Value:

    None.

--*/

{
    HMODULE hDll = LoadLibrary(g_cszShlwapi);

    ASSERT(hDll != NULL);
    ASSERT(g_TpsTls != 0xFFFFFFFF);

    TlsSetValue(g_TpsTls, (LPVOID)TPS_WORKER_SIGNATURE);

    g_ThreadPool.Worker();

    if (GetCurrentThreadId() == g_dwTerminationThreadId) {
        g_dwTerminationThreadId = 0;
        g_bDeferredWorkerTermination = FALSE;
        g_bTpsTerminating = FALSE;
    }
    FreeLibraryAndExitThread(hDll, ERROR_SUCCESS);
}

VOID
CThreadPool::Worker(
    VOID
    )

/*++

Routine Description:

    All non I/O worker threads execute in this routine. This function will
    terminate when it has not serviced a request for m_workerIdleTimeout mSec

Arguments:

    None.

Return Value:

    None.

--*/

{
    while (!g_bTpsTerminating)
    {
        FARPROC fn;
        ULONG_PTR ctx;
        DWORD flags;
        HMODULE hMouduleToFree = NULL;
        DWORD error = RemoveWorkItem(&fn, &ctx, &hMouduleToFree, &flags, m_workerIdleTimeout);

        ASSERT(error != ERROR_SUCCESS || !(flags & TPS_INVALID_FLAGS));

        if (g_bTpsTerminating)
        {
            break;
        }
        if (error == ERROR_SUCCESS)
        {
            // call the work function
            ((LPTHREAD_START_ROUTINE)fn)((LPVOID)ctx);

            if (hMouduleToFree)
            {
                // we completed the task, so free the hmodule associated with it
                FreeLibrary(hMouduleToFree);
            }

            if (flags & TPS_LONGEXECTIME)
            {
                MakeAvailable();
            }
        }
        else if (error == WAIT_TIMEOUT)
        {
            m_qlock.Acquire();

            if ((m_queueSize == 0) && (m_availableWorkerThreads > m_minWorkerThreads))
            {
                RemoveWorker();
                m_qlock.Release();

                //#if DBG
                //char buf[256];
                //wsprintf(buf, ">>>> terminating worker thread. Total = %d/%d. Avail = %d. Factor = %d/%d\n",
                //         m_totalWorkerThreads,
                //         m_maxWorkerThreadsCreated,
                //         m_availableWorkerThreads,
                //         m_qFactor,
                //         m_qFactorMax
                //         );
                //OutputDebugString(buf);
                //#endif

                return;
            }
            m_qlock.Release();
        }
    }

    //#if DBG
    //char buf[256];
    //wsprintf(buf, ">>>> terminating worker thread. Total = %d/%d. Avail = %d. Factor = %d/%d\n",
    //         m_totalWorkerThreads,
    //         m_maxWorkerThreadsCreated,
    //         m_availableWorkerThreads,
    //         m_qFactor,
    //         m_qFactorMax
    //         );
    //OutputDebugString(buf);
    //#endif

    RemoveWorker();
}
