/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    tpstimer.cpp

Abstract:

    Contains Win32 thread pool services timer functions

    Contents:
        TerminateTimers
        SHCreateTimerQueue
        SHDeleteTimerQueue
        SHSetTimerQueueTimer
        SHChangeTimerQueueTimer
        SHCancelTimerQueueTimer
        (InitializeTimerThread)
        (TimerCleanup)
        (CreateDefaultTimerQueue)
        (DeleteDefaultTimerQueue)
        (CleanupDefaultTimerQueue)
        (TimerThread)
        (DeleteTimerQueue)
        (AddTimer)
        (ChangeTimer)
        (CancelTimer)

Author:

    Richard L Firth (rfirth) 10-Feb-1998

Environment:

    Win32 user-mode

Notes:

    Code reworked in C++ from NT-specific C code written by Gurdeep Singh Pall
    (gurdeep)

Revision History:

    10-Feb-1998 rfirth
        Created

--*/

#include "priv.h"
#include "threads.h"
#include "tpsclass.h"
#include "tpstimer.h"

//
// private prototypes
//

PRIVATE
DWORD
InitializeTimerThread(
    VOID
    );

PRIVATE
VOID
TimerCleanup(
    VOID
    );

PRIVATE
HANDLE
CreateDefaultTimerQueue(
    VOID
    );

PRIVATE
VOID
DeleteDefaultTimerQueue(
    VOID
    );

PRIVATE
VOID
CleanupDefaultTimerQueue(
    VOID
    );

PRIVATE
VOID
TimerThread(
    VOID
    );

PRIVATE
VOID
DeleteTimerQueue(
    IN CTimerQueueDeleteRequest * pRequest
    );

PRIVATE
VOID
AddTimer(
    IN CTimerAddRequest * pRequest
    );

PRIVATE
VOID
ChangeTimer(
    IN CTimerChangeRequest * pRequest
    );

PRIVATE
VOID
CancelTimer(
    IN CTimerCancelRequest * pRequest
    );

//
// global data
//

CTimerQueueList g_TimerQueueList;
HANDLE g_hDefaultTimerQueue = NULL;
HANDLE g_hTimerThread = NULL;
DWORD g_dwTimerId = 0;
LONG g_UID = 0;
BOOL g_bTimerInit = FALSE;
BOOL g_bTimerInitDone = FALSE;
BOOL g_bDeferredTimerTermination = FALSE;

//
// functions
//

VOID
TerminateTimers(
    VOID
    )

/*++

Routine Description:

    Terminate timer thread and global variables

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (g_bTimerInitDone) {

        DWORD threadId = GetCurrentThreadId();

        if ((g_hTimerThread != NULL) && (threadId != g_dwTimerId)) {
            QueueNullFunc(g_hTimerThread);

            DWORD ticks = GetTickCount();

            while (g_hTimerThread != NULL) {
                SleepEx(0, TRUE);
                if (GetTickCount() - ticks > 10000) {
                    CloseHandle(g_hTimerThread);
                    g_hTimerThread = NULL;
                    break;
                }
            }
        }
        if (g_dwTimerId == threadId) {
            g_bDeferredTimerTermination = TRUE;
        } else {
            TimerCleanup();
        }
    }
}

LWSTDAPI_(HANDLE)
SHCreateTimerQueue(
    VOID
    )

/*++

Routine Description:

    Creates a timer queue

Arguments:

    None.

Return Value:

    HANDLE
        Success - non-NULL pointer to CTimerQueue object

        Failure - NULL. GetLastError() for more info

--*/

{
    InterlockedIncrement((LPLONG)&g_ActiveRequests);

    HANDLE hResult = NULL;
    DWORD error = ERROR_SUCCESS;

    if (!g_bTpsTerminating) {
        if (g_hTimerThread == NULL) {
            error = InitializeTimerThread();
        }
        if (error == ERROR_SUCCESS) {

            //
            // timer queue handle is just pointer to timer queue object
            //

            hResult = (HANDLE) new CTimerQueue(&g_TimerQueueList);
        } else {
            SetLastError(error);
        }
    } else {
        SetLastError(ERROR_SHUTDOWN_IN_PROGRESS); // BUGBUG - error code?
    }
    InterlockedDecrement((LPLONG)&g_ActiveRequests);
    return hResult;
}

LWSTDAPI_(BOOL)
SHDeleteTimerQueue(
    IN HANDLE hQueue
    )

/*++

Routine Description:

    Deletes the specified timer queue

Arguments:

    hQueue  - handle of queue to delete; NULL for default timer queue

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    InterlockedIncrement((LPLONG)&g_ActiveRequests);

    BOOL bSuccess = FALSE;

    if (!g_bTpsTerminating) {
        if (hQueue == NULL) {
            hQueue = g_hDefaultTimerQueue;
        }
        if ((hQueue != NULL) && (g_hTimerThread != NULL)) {

            CTimerQueueDeleteRequest request(hQueue);

            if (QueueUserAPC((PAPCFUNC)DeleteTimerQueue,
                             g_hTimerThread,
                             (ULONG_PTR)&request)) {
                request.WaitForCompletion();
                bSuccess = request.SetThreadStatus();
            } else {
#if DBG
                DWORD error = GetLastError();

                ASSERT(error == ERROR_SUCCESS);
#endif
            }
        } else {
            SetLastError(ERROR_INVALID_PARAMETER); // BUGBUG - correct error code?
        }
    } else {
        SetLastError(ERROR_SHUTDOWN_IN_PROGRESS); // BUGBUG - error code?
    }
    InterlockedDecrement((LPLONG)&g_ActiveRequests);
    return bSuccess;
}

LWSTDAPI_(HANDLE)
SHSetTimerQueueTimer(
    IN HANDLE hQueue,
    IN WAITORTIMERCALLBACKFUNC pfnCallback,
    IN LPVOID pContext,
    IN DWORD dwDueTime,
    IN DWORD dwPeriod,
    IN LPCSTR lpszLibrary OPTIONAL,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Add a timer to a queue

Arguments:

    hQueue      - handle of timer queue; NULL for default queue

    pfnCallback - function to call when timer triggers

    pContext    - parameter to pfnCallback

    dwDueTime   - initial firing time in milliseconds from now

    dwPeriod    - repeating period. 0 for one-shot

    lpszLibrary - if specified, name of library (DLL) to reference

    dwFlags     - flags controlling function:

                    TPS_EXECUTEIO   - Execute callback in I/O thread

Return Value:

    HANDLE
        Success - non-NULL handle

        Failure - NULL. Call GetLastError() for more info

--*/

{
    InterlockedIncrement((LPLONG)&g_ActiveRequests);

    HANDLE hTimer = NULL;

    if (!g_bTpsTerminating) {

        DWORD error = ERROR_SUCCESS;

        if (g_hTimerThread == NULL) {
            error = InitializeTimerThread();
        }

        ASSERT(g_hTimerThread != NULL);

        if (error == ERROR_SUCCESS) {
            if (hQueue == NULL) {
                hQueue = CreateDefaultTimerQueue();
            }
            if (hQueue != NULL) {

                CTimerAddRequest * pRequest = new CTimerAddRequest(hQueue,
                                                                   pfnCallback,
                                                                   pContext,
                                                                   dwDueTime,
                                                                   dwPeriod,
                                                                   dwFlags
                                                                   );

                if (pRequest != NULL) {
                    hTimer = pRequest->GetHandle();
                    if (QueueUserAPC((PAPCFUNC)AddTimer,
                                     g_hTimerThread,
                                     (ULONG_PTR)pRequest
                                     )) {
                    } else {
#if DBG
                        error = GetLastError();

                        ASSERT(GetLastError() != ERROR_SUCCESS);
#endif
                        delete pRequest;
                        hTimer = NULL;
#if DBG
                        SetLastError(error);
#endif
                    }
                }
            }
        } else {
            SetLastError(error);
        }
    } else {
        SetLastError(ERROR_SHUTDOWN_IN_PROGRESS); // BUGBUG - error code?
    }
    InterlockedDecrement((LPLONG)&g_ActiveRequests);
    return hTimer;
}

LWSTDAPI_(BOOL)
SHChangeTimerQueueTimer(
    IN HANDLE hQueue,
    IN HANDLE hTimer,
    IN DWORD dwDueTime,
    IN DWORD dwPeriod
    )

/*++

Routine Description:

    Change the due time or periodicity of a timer

Arguments:

    hQueue      - handle of queue on which timer resides. NULL for default queue

    hTimer      - handle of timer to change

    dwDueTime   - new due time

    dwPeriod    - new period

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    InterlockedIncrement((LPLONG)&g_ActiveRequests);

    BOOL bSuccess = FALSE;
    DWORD error = ERROR_SHUTDOWN_IN_PROGRESS; // BUGBUG - error code?

    if (!g_bTpsTerminating) {
        error = ERROR_OBJECT_NOT_FOUND;
        if (g_hTimerThread != NULL) {
            if (hQueue == NULL) {
                hQueue = g_hDefaultTimerQueue;
            }
            if (hQueue != NULL) {

                CTimerChangeRequest request(hQueue, hTimer, dwDueTime, dwPeriod);

                error = ERROR_SUCCESS; // both paths call SetLastError() if reqd
                if (QueueUserAPC((PAPCFUNC)ChangeTimer,
                                 g_hTimerThread,
                                 (ULONG_PTR)&request
                                 )) {
                    request.WaitForCompletion();
                    bSuccess = request.SetThreadStatus();
                } else {
#if DBG
                    DWORD error = GetLastError();

                    ASSERT(error == ERROR_SUCCESS);
#endif
                }
            }
        }
    }
    InterlockedDecrement((LPLONG)&g_ActiveRequests);
    if (error != ERROR_SUCCESS) {
        SetLastError(error);
    }
    return bSuccess;
}

LWSTDAPI_(BOOL)
SHCancelTimerQueueTimer(
    IN HANDLE hQueue,
    IN HANDLE hTimer
    )

/*++

Routine Description:

    Cancels a timer

Arguments:

    hQueue  - handle to queue on which timer resides

    hTimer  - handle of timer to cancel

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    InterlockedIncrement((LPLONG)&g_ActiveRequests);

    BOOL bSuccess = FALSE;

    if (!g_bTpsTerminating) {
        if (hQueue == NULL) {
            hQueue = g_hDefaultTimerQueue;
        }
        if ((hQueue != NULL) && (g_hTimerThread != NULL)) {

            CTimerCancelRequest request(hQueue, hTimer);

            if (QueueUserAPC((PAPCFUNC)CancelTimer,
                             g_hTimerThread,
                             (ULONG_PTR)&request
                             )) {
                request.WaitForCompletion();
                bSuccess = request.SetThreadStatus();
            } else {
#if DBG
                DWORD error = GetLastError();

                ASSERT(error == ERROR_SUCCESS);
#endif
            }
        } else {
            SetLastError(ERROR_INVALID_HANDLE);
        }
    } else {
        SetLastError(ERROR_SHUTDOWN_IN_PROGRESS); // BUGBUG - error code?
    }
    InterlockedDecrement((LPLONG)&g_ActiveRequests);
    return bSuccess;
}

//
// private functions
//

PRIVATE
DWORD
InitializeTimerThread(
    VOID
    )
{
    DWORD error = ERROR_SUCCESS;

    while (!g_bTimerInitDone) {
        if (!InterlockedExchange((LPLONG)&g_bTimerInit, TRUE)) {

            //
            // N.B. if CTimerQueueList::Init() does anything more than just
            // initialize lists then add a Deinit()
            //

            g_TimerQueueList.Init();

            ASSERT(g_hTimerThread == NULL);

            error = StartThread((LPTHREAD_START_ROUTINE)TimerThread,
                                &g_hTimerThread,
                                FALSE
                                );
            if (error == ERROR_SUCCESS) {
                g_bTimerInitDone = TRUE;
            } else {
                InterlockedExchange((LPLONG)&g_bTimerInit, FALSE);
            }
            break;
        } else {
            SleepEx(0, FALSE);
        }
    }
    return error;
}

PRIVATE
VOID
TimerCleanup(
    VOID
    )
{
    while (!g_TimerQueueList.QueueListHead()->IsEmpty()) {

        CTimerQueueDeleteRequest request((CTimerQueue *)
                                    g_TimerQueueList.QueueListHead()->Next());

        DeleteTimerQueue(&request);
    }
    DeleteDefaultTimerQueue();
    g_UID = 0;
    g_bTimerInit = FALSE;
    g_bTimerInitDone = FALSE;
}

BOOL bDefaultQueueInit = FALSE;
BOOL bDefaultQueueInitDone = FALSE;
BOOL bDefaultQueueInitFailed = FALSE;

PRIVATE
HANDLE
CreateDefaultTimerQueue(
    VOID
    )
{
    do {
        if ((g_hDefaultTimerQueue != NULL) || bDefaultQueueInitFailed) {
            return g_hDefaultTimerQueue;
        }
        if (!InterlockedExchange((LPLONG)&bDefaultQueueInit, TRUE)) {
            InterlockedExchange((LPLONG)&bDefaultQueueInitDone, FALSE);
            g_hDefaultTimerQueue = SHCreateTimerQueue();
            if (g_hDefaultTimerQueue == NULL) {
                bDefaultQueueInitFailed = TRUE;
                InterlockedExchange((LPLONG)&bDefaultQueueInit, FALSE);
            }
            InterlockedExchange((LPLONG)&bDefaultQueueInitDone, TRUE);
        } else {
            do {
                SleepEx(0, FALSE);
            } while (!bDefaultQueueInitDone);
        }
    } while (TRUE);
}

PRIVATE
VOID
DeleteDefaultTimerQueue(
    VOID
    )
{
    if (g_hDefaultTimerQueue != NULL) {

        CTimerQueueDeleteRequest request((CTimerQueue *)g_hDefaultTimerQueue);

        DeleteTimerQueue(&request);
        g_hDefaultTimerQueue = NULL;
    }
    CleanupDefaultTimerQueue();
}

PRIVATE
VOID
CleanupDefaultTimerQueue(
    VOID
    )
{
    g_hDefaultTimerQueue = NULL;
    bDefaultQueueInit = FALSE;
    bDefaultQueueInitDone = FALSE;
    bDefaultQueueInitFailed = FALSE;
}

PRIVATE
VOID
TimerThread(
    VOID
    )
{
    g_dwTimerId = GetCurrentThreadId();

    HMODULE hDll = LoadLibrary(g_cszShlwapi);

    ASSERT(hDll != NULL);
    ASSERT(g_TpsTls != 0xFFFFFFFF);

    TlsSetValue(g_TpsTls, (LPVOID)TPS_TIMER_SIGNATURE);

    while (!g_bTpsTerminating || (g_ActiveRequests != 0)) {
        if (g_TimerQueueList.Wait()) {
            if (g_bTpsTerminating && (g_ActiveRequests == 0)) {
                break;
            }
            g_TimerQueueList.ProcessCompletions();
        }
    }

    ASSERT(g_hTimerThread != NULL);

    CloseHandle(g_hTimerThread);
    g_hTimerThread = NULL;
    if (g_dwTimerId == g_dwTerminationThreadId) {
        TimerCleanup();
        g_bTpsTerminating = FALSE;
        g_dwTerminationThreadId = 0;
        g_bDeferredTimerTermination = FALSE;
    }
    g_dwTimerId = 0;
    FreeLibraryAndExitThread(hDll, ERROR_SUCCESS);
}

PRIVATE
VOID
DeleteTimerQueue(
    IN CTimerQueueDeleteRequest * pRequest
    )
{
    CTimerQueue * pQueue = (CTimerQueue *)pRequest->GetQueue();
    DWORD dwStatus = ERROR_INVALID_PARAMETER;

    if (g_TimerQueueList.FindQueue((CDoubleLinkedListEntry *)pQueue) != NULL) {
        pQueue->DeleteTimers();
        if (pQueue == g_hDefaultTimerQueue) {
            CleanupDefaultTimerQueue();
        }
        delete pQueue;
        dwStatus = ERROR_SUCCESS;
    }
    pRequest->SetCompletionStatus(dwStatus);
}

PRIVATE
VOID
AddTimer(
    IN CTimerAddRequest * pRequest
    )
{
    CTimerQueue * pQueue = pRequest->GetQueue();

    //
    // add timer object to global list of timer objects, in expiration time
    // order
    //

    pRequest->InsertBack(g_TimerQueueList.TimerListHead());

    //
    // add timer object to end of timer queue list in no particular order. Only
    // used to delete all objects belonging to queue when queue is deleted
    //

    pRequest->TimerListHead()->InsertTail(pQueue->TimerListHead());
    pRequest->SetComplete();
}

PRIVATE
VOID
ChangeTimer(
    IN CTimerChangeRequest * pRequest
    )
{
    CTimerQueue * pQueue = (CTimerQueue *)pRequest->GetQueue();
    CTimerQueueEntry * pTimer = pQueue->FindTimer(pRequest->GetTimer());
    DWORD dwStatus = ERROR_INVALID_PARAMETER;

    if (pTimer != NULL) {
        pTimer->SetPeriod(pRequest->GetPeriod());
        pTimer->SetExpirationTime(pRequest->GetDueTime());
        dwStatus = ERROR_SUCCESS;
    }
    pRequest->SetCompletionStatus(dwStatus);
}

PRIVATE
VOID
CancelTimer(
    IN CTimerCancelRequest * pRequest
    )
{
    CTimerQueue * pQueue = (CTimerQueue *)pRequest->GetQueue();
    CTimerQueueEntry * pTimer = pQueue->FindTimer(pRequest->GetTimer());
    DWORD dwStatus = ERROR_INVALID_PARAMETER;

    if (pTimer != NULL) {
        if (pTimer->IsInUse()) {
            pTimer->SetCancelled();
        } else {
            pTimer->Remove();
            delete pTimer;
        }
        dwStatus = ERROR_SUCCESS;
    }
    pRequest->SetCompletionStatus(dwStatus);
}
