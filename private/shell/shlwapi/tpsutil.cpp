/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    tpsutil.cpp

Abstract:

    Comtains common utility functions for Win32 thread pool services

    Contents:
        StartThread
        TpsEnter
        QueueNullFunc
        (NullFunc)

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

//
// private prototypes
//

PRIVATE
VOID
NullFunc(
    IN LPVOID pUnused
    );

//
// functions
//

DWORD
StartThread(
    IN LPTHREAD_START_ROUTINE pfnFunction,
    OUT PHANDLE phThread,
    IN BOOL fSynchronize
    )

/*++

Routine Description:

    This routine is used start a new thread in the pool. If required, we
    synchronize with the new thread using an auto-reset event that the new
    thread must signal once it has completed its initialization

Arguments:

    pfnFunction     - pointer to thread function to start

    phThread        - pointer to returned thread handle

    fSynchronize    - used to indicate if we need to synchronize with the new
                      thread before returning

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY
                    Out of memory

--*/

{
    HANDLE hSyncEvent = NULL;

    if (fSynchronize) {
        hSyncEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (hSyncEvent == NULL) {
            return GetLastError();
        }
    }

    DWORD dwThreadId;
    HANDLE hThread;
    DWORD error = ERROR_SUCCESS;

    hThread = CreateThread(NULL,        // lpSecurityAttributes
                           0,           // dwStackSize (0 == same as init thread)
                           pfnFunction,
                           (LPVOID)hSyncEvent,
                           0,           // dwCreationFlags
                           &dwThreadId  // throw away
                           );
    if (hThread == NULL) {
        error = GetLastError();
    }
    if (hSyncEvent != NULL) {
        if (hThread != NULL) {

            DWORD status = WaitForSingleObject(hSyncEvent, INFINITE);

            if (status == WAIT_FAILED) {
                error = GetLastError();
            } else if (status == WAIT_TIMEOUT) {
                error = WAIT_TIMEOUT;
            } else if (status != WAIT_OBJECT_0) {
                error = ERROR_GEN_FAILURE; // ?
            }
        }
        CloseHandle(hSyncEvent);
    }
    *phThread = hThread;
    return error;
}

DWORD
TpsEnter(
    VOID
    )

/*++

Routine Description:

    synchronize with thread shutting down via SHTerminateThreadPool(). If
    terminating because DLL is unloaded, return error else wait until
    termination completed

Arguments:

    None.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_SHUTDOWN_IN_PROGRESS

--*/

{
    for (; ; ) {
        while (g_bTpsTerminating) {
            if (g_bDllTerminating) {
                return ERROR_SHUTDOWN_IN_PROGRESS; // BUGBUG - error code?
            }
            SleepEx(0, TRUE);
        }
        InterlockedIncrement((LPLONG)&g_ActiveRequests);
        if (!g_bTpsTerminating) {
            return ERROR_SUCCESS;
        }
        InterlockedDecrement((LPLONG)&g_ActiveRequests);
    }
}

VOID
QueueNullFunc(
    IN HANDLE hThread
    )

/*++

Routine Description:

    Queues NullFunc as an APC to hThread

Arguments:

    hThread - thread to queue for

Return Value:

    None.

--*/

{
    QueueUserAPC((PAPCFUNC)NullFunc, hThread, NULL);
}

PRIVATE
VOID
NullFunc(
    IN LPVOID pUnused
    )

/*++

Routine Description:

    NULL APC function. Used to allow TerminateThreadPool() to wake up dormant
    APC threads

Arguments:

    pUnused - unused argument pointer

Return Value:

    None.

--*/

{
}
