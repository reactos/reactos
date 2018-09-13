/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    tpswait.cpp

Abstract:

    Contains Win32 thread pool services wait functions

    Contents:
        TerminateWaiters
        SHRegisterWaitForSingleObject
        SHUnregisterWait
        (InitializeWaitThreadPool)
        (FindWaitThreadInfo)
        (AddWait)
        (RemoveWait)
        (WaitThread)

Author:

    Richard L Firth (rfirth) 10-Feb-1998

Environment:

    Win32 user-mode

Notes:

    Taken from NT-specific code written by Gurdeep Singh Pall (gurdeep)

Revision History:

    10-Feb-1998 rfirth
        Created

--*/

#include "priv.h"
#include "threads.h"
#include "tpsclass.h"
#include "tpswait.h"

//
// private prototypes
//

PRIVATE
DWORD
InitializeWaitThreadPool(
    VOID
    );

PRIVATE
DWORD
FindWaitThreadInfo(
    OUT CWaitThreadInfo * * pInfo
    );

PRIVATE
VOID
AddWait(
    IN OUT CWaitAddRequest * pRequest
    );

PRIVATE
VOID
RemoveWait(
    IN CWaitRemoveRequest * pRequest
    );

PRIVATE
VOID
WaitThread(
    IN HANDLE hEvent
    );

//
// global data
//

CDoubleLinkedList g_WaitThreads;
CCriticalSection_NoCtor g_WaitCriticalSection;
BOOL g_StartedWaitInitialization = FALSE;
BOOL g_CompletedWaitInitialization = FALSE;
BOOL g_bDeferredWaiterTermination = FALSE;

//
// functions
//

VOID
TerminateWaiters(
    VOID
    )

/*++

Routine Description:

    Terminate waiter threads and global variables

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (g_CompletedWaitInitialization) {
        g_WaitCriticalSection.Acquire();
        while (!g_WaitThreads.IsEmpty()) {

            CWaitThreadInfo * pInfo;

            pInfo = (CWaitThreadInfo *)g_WaitThreads.RemoveHead();

            HANDLE hThread = pInfo->GetHandle();

            pInfo->SetHandle(NULL);
            QueueNullFunc(hThread);
            SleepEx(0, FALSE);
        }
        g_WaitCriticalSection.Release();
        g_WaitCriticalSection.Terminate();
        g_StartedWaitInitialization = FALSE;
        g_CompletedWaitInitialization = FALSE;
    }
    if (TlsGetValue(g_TpsTls) == (LPVOID)TPS_WAITER_SIGNATURE) {
        g_bDeferredWaiterTermination = TRUE;
    }
}

LWSTDAPI_(HANDLE)
SHRegisterWaitForSingleObject(
    IN HANDLE hObject,
    IN WAITORTIMERCALLBACKFUNC pfnCallback,
    IN LPVOID pContext,
    IN DWORD dwWaitTime,
    IN LPCSTR lpszLibrary OPTIONAL,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    This routine adds a new wait request to the pool of objects being waited on.

Arguments:

    hObject     - handle to the object to be waited on

    pfnCallback - routine called when the wait completes or a timeout occurs

    pContext    - opaque pointer passed in as an argument to pfnCallback

    dwWaitTime  - Timeout for the wait in milliseconds. 0 means dont timeout.

    lpszLibrary - if specified, name of library (DLL) to reference

    dwFlags     - flags modifying request:

                    SRWSO_NOREMOVE
                        - once the handle becomes signalled, do not remove it
                          from the handle array. Intended to be used with
                          auto-reset events which become unsignalled again as
                          soon as the waiting thread is made runnable

Return Value:

    HANDLE
        Success - Non-NULL handle of created wait object

        Failure - NULL. Call GetLastError() for error code

--*/

{
    InterlockedIncrement((LPLONG)&g_ActiveRequests);

    HANDLE hWait = NULL;
    DWORD error = ERROR_SUCCESS;

    if (g_bTpsTerminating) {
        error = ERROR_SHUTDOWN_IN_PROGRESS; // BUGBUG - error code?
        goto exit;
    }

    if (dwFlags & SRWSO_INVALID_FLAGS) {
        error = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    //DWORD dwHandleFlags;
    //
    //if (!GetHandleInformation(hObject, &dwHandleFlags)) {
    //
    //    //
    //    // error == ERROR_SUCCESS returns GetHandleInformation() last error
    //    //
    //
    //    ASSERT(error == ERROR_SUCCESS);
    //
    //    goto exit;
    //}

    //
    // GetHandleInformation() doesn't work on Win95
    //

    if (WaitForSingleObject(hObject, 0) == WAIT_FAILED) {

        //
        // error == ERROR_SUCCESS returns WaitForSingleObject() last error
        //

        ASSERT(error == ERROR_SUCCESS);

        goto exit;
    }

    if (IsBadCodePtr((FARPROC)pfnCallback)) {
        error = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    //
    // initialize wait thread pool if it isn't already done
    //

    if (!g_CompletedWaitInitialization) {
        error = InitializeWaitThreadPool();
        if (error != ERROR_SUCCESS) {
            goto exit;
        }
    }

    //
    // find or create a wait thread that can accomodate another wait request
    //

    CWaitThreadInfo * pInfo;

    error = FindWaitThreadInfo(&pInfo);
    if (error == ERROR_SUCCESS) {

        CWaitAddRequest request(hObject,
                                pfnCallback,
                                pContext,
                                dwWaitTime,
                                dwFlags,
                                pInfo
                                );

        //
        // queue an APC to the wait thread
        //

        BOOL bSuccess = QueueUserAPC((PAPCFUNC)AddWait,
                                     pInfo->GetHandle(),
                                     (ULONG_PTR)&request
                                     );

        ASSERT(bSuccess);

        if (bSuccess) {

            //
            // relinquish the timeslice until the other thread has initialized
            //

            request.WaitForCompletion();

            //
            // the returned handle is the address of the wait object copied to
            // the wait thread's stack
            //

            hWait = request.GetWaitPointer();
        }
        pInfo->Release();
    }

exit:

    if (error != ERROR_SUCCESS) {
        SetLastError(error);
    }
    InterlockedDecrement((LPLONG)&g_ActiveRequests);
    return hWait;
}

LWSTDAPI_(BOOL)
SHUnregisterWait(
    IN HANDLE hWait
    )

/*++

Routine Description:

    This routine removes the specified wait from the pool of objects being waited
    on. This routine will block until all callbacks invoked as a result of this
    wait have been executed. This function MUST NOT be invoked inside the
    callback routines.

Arguments:

    hWait   - 'handle' indentifying the wait request

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for error code

--*/

{
    InterlockedIncrement((LPLONG)&g_ActiveRequests);

    BOOL bSuccess = FALSE;
    DWORD error = ERROR_SUCCESS;

    if (hWait) {
        if (!g_bTpsTerminating) {

            CWaitThreadInfo * pInfo = ((CWait *)hWait)->GetThreadInfo();
            CWaitRemoveRequest request(hWait);

            //
            // lock the thread control block
            //

            pInfo->Acquire();

            //
            // queue an APC to the wait thread
            //

            if (QueueUserAPC((PAPCFUNC)RemoveWait,
                             pInfo->GetHandle(),
                             (ULONG_PTR)&request
                             )) {

                //
                // relinquish the timeslice until the other thread has initialized
                //

                request.WaitForCompletion();
                if (!(bSuccess = (request.GetWaitPointer() != NULL))) {
                    error = ERROR_OBJECT_NOT_FOUND; // BUGBUG - error code?
                }
            }

            //
            // release lock to the thread control block
            //

            pInfo->Release();
        } else {
            error = ERROR_SHUTDOWN_IN_PROGRESS; // BUGBUG - error code?
        }
    } else {
        error = ERROR_INVALID_PARAMETER;
    }
    if (error != ERROR_SUCCESS) {
        SetLastError(error);
    }
    InterlockedDecrement((LPLONG)&g_ActiveRequests);
    return bSuccess;
}

//
// private functions
//

PRIVATE
DWORD
InitializeWaitThreadPool(
    VOID
    )

/*++

Routine Description:

    This routine initializes all aspects of the thread pool.

Arguments:

    None

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DWORD error = ERROR_SUCCESS;

    if (!InterlockedExchange((LPLONG)&g_StartedWaitInitialization, TRUE)) {
        g_WaitCriticalSection.Init();
        g_WaitThreads.Init();
        g_CompletedWaitInitialization = TRUE;
    } else {

        //
        // relinquish the timeslice until the other thread has initialized
        //

        while (!g_CompletedWaitInitialization) {
            SleepEx(0, FALSE);  // Sleep(0) without an additional call/return
        }
    }
    return error;
}

PRIVATE
DWORD
FindWaitThreadInfo(
    OUT CWaitThreadInfo * * ppInfo
    )

/*++

Routine Description:

    Walks thru the list of wait threads and finds one which can accomodate
    another wait. If one is not found then a new thread is created.

    This routine returns with the thread's WaitThreadCriticalSecton owned if it
    is successful.

Arguments:

    ppInfo  - pointer to pointer to returned control block

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    HANDLE hThread = NULL;

    //
    // take exclusive lock to the wait threads list
    //

    g_WaitCriticalSection.Acquire();

    do {

        DWORD error;

        //
        // walk thru the list of Wait Threads and find a Wait thread that can
        // accomodate a new wait request
        //

        //
        // *Consider* finding a wait thread with least # of waits to facilitate
        // better load balancing of waits
        //

        for (CWaitThreadInfo * pInfo = (CWaitThreadInfo *)g_WaitThreads.Next();
             !g_WaitThreads.IsHead(pInfo);
             pInfo = (CWaitThreadInfo *)pInfo->Next()) {


            //
            // slight cheese: if hThread is not NULL then its because we just
            // created a new thread. We know we have g_WaitCriticalSection held
            // and no other thread can be accessing the new thread's control
            // block, so we can now write in the handle to be used in future
            // calls to QueueUserAPC. This saves us having to duplicate the
            // thread handle in the new thread
            //

            if (hThread != NULL) {
                pInfo->SetHandle(hThread);
            }

            //
            // take exclusive lock to the wait thread control block
            //

            pInfo->Acquire();

            //
            // wait threads can accomodate up to MAX_WAITS (WaitForMultipleObject
            // limit)
            //

            if (pInfo->IsAvailableEntry()) {

                //
                // found a thread with some wait slots available. Release lock
                // on the wait threads list
                //

                *ppInfo = pInfo;
                g_WaitCriticalSection.Release();
                return ERROR_SUCCESS;
            }

            //
            // release lock to thread control block
            //

            pInfo->Release();
        }

        //
        // if we reach here, we don't have any more wait threads so create more
        //

        error = StartThread((LPTHREAD_START_ROUTINE)WaitThread, &hThread, TRUE);

        //
        // if thread creation fails then return the failure to caller
        //

        if (error != ERROR_SUCCESS) {

            ASSERT(FALSE);

            g_WaitCriticalSection.Release();
            return error;
        }

        //
        // loop back now that we have created another thread and put new wait
        // request in new thread
        //

    } while(TRUE);
}

PRIVATE
VOID
AddWait(
    IN OUT CWaitAddRequest * pRequest
    )

/*++

Routine Description:

    This routine is used for adding waits to the wait thread. It is executed in
    an APC.

Arguments:

    pRequest    - pointer to request object

Return Value:

    None.

--*/

{
    if (!g_bTpsTerminating) {

        CWaitThreadInfo * pInfo = pRequest->GetThreadInfo();
        CWait * pWait = pInfo->GetFreeWaiter();

        //
        // copy relevant fields from request object. C++ knows how to pull CWait
        // object out of CWaitAddRequest object. Insert the wait request object in
        // the list of active waits, in increasing expiration time order
        //

        *pWait = *pRequest;
        pInfo->InsertWaiter(pWait);

        //
        // return to the caller the address of the wait object on the wait thread's
        // stack and indicate to the calling thread that this request is complete
        //

        pRequest->SetWaitPointer(pWait);
    }
    pRequest->SetComplete();
}

PRIVATE
VOID
RemoveWait(
    IN CWaitRemoveRequest * pRequest
    )

/*++

Routine Description:

    This routine is used for deleting the specified wait. It is executed in an
    APC.

Arguments:

    pRequest    - pointer to request object

Return Value:

    None.

--*/

{
    if (!g_bTpsTerminating) {
        if (!pRequest->GetWaitPointer()->GetThreadInfo()->RemoveWaiter(pRequest->GetWaitPointer())) {
            pRequest->SetWaitPointer(NULL);
        }
    }
    pRequest->SetComplete();
}

PRIVATE
VOID
WaitThread(
    IN HANDLE hEvent
    )

/*++

Routine Description:

    This routine is used for all waits in the wait thread pool

Arguments:

    hEvent  - event handle to signal once initialization is complete

Return Value:

    None.

--*/

{
    HMODULE hDll = LoadLibrary(g_cszShlwapi);

    ASSERT(hDll != NULL);
    ASSERT(g_TpsTls != 0xFFFFFFFF);

    TlsSetValue(g_TpsTls, (LPVOID)TPS_WAITER_SIGNATURE);

    CWaitThreadInfo waitInfo(&g_WaitThreads);

    SetEvent(hEvent);

    while (!g_bTpsTerminating || (g_ActiveRequests != 0)) {

        DWORD dwIndex = waitInfo.Wait(waitInfo.GetWaitTime());

        if (g_bTpsTerminating && (g_ActiveRequests == 0)) {
            break;
        }
        if (dwIndex == WAIT_TIMEOUT) {
            waitInfo.ProcessTimeouts();
        } else if ((dwIndex >= WAIT_OBJECT_0)
                   && (dwIndex < (WAIT_OBJECT_0 + waitInfo.GetObjectCount()))) {
            waitInfo.ProcessCompletion(dwIndex);
        } else if ((dwIndex == 0xFFFFFFFF) && GetLastError() == ERROR_INVALID_HANDLE) {
            waitInfo.PurgeInvalidHandles();
        } else {

            ASSERT(dwIndex == WAIT_IO_COMPLETION);

        }
    }
    while (waitInfo.GetHandle() != NULL) {
        SleepEx(0, FALSE);
    }
    if (GetCurrentThreadId() == g_dwTerminationThreadId) {
        g_bTpsTerminating = FALSE;
        g_bDeferredWaiterTermination = FALSE;
        g_dwTerminationThreadId = 0;
    }
    FreeLibraryAndExitThread(hDll, ERROR_SUCCESS);
}
