/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    blocklst.cxx

Abstract:

    Contains WinInet async support to allow blocking of unknown threads, on auto-proxy events.

    Contents:
        BlockThreadOnEvent
        SignalThreadOnEvent
        AcquireBlockedRequestQueue
        ReleaseBlockedRequestQueue
        (DestroyBlockedThreadEvent)


Author:

    Arthur L Bierer (arthurbi) 15-Feb-1998

Environment:

    Win32 user-mode DLL

Revision History:

    15-Feb-1998 arthurbi
        Created

--*/

#include <wininetp.h>
#include <perfdiag.hxx>

//
// private data
//

//
// BlockedRequestQueue - when threads need to block on an event they get
// placed in here until their event is signaled. This is needed so threads
// can block on internally kept events without having to be tied
// to keeping track of event handles.
//
// Blocked threads must not be also waiting for a socket to become unblocked
//

GLOBAL SERIALIZED_LIST BlockedRequestQueue = {0};

//
// ARB - information common to all asynchronous blocked events
//

typedef struct {

    //
    // List - requests are queued on doubly-linked list. N.B. Code that deals
    // in ARBs implicitly assumes that List is at offset zero in the ARB. Move
    // this and pick up the pieces...
    //

    LIST_ENTRY List;

    //
    // hEvent - handle to Event that we are blocked on.
    //

    HANDLE      hEvent;

    //
    // dwBlockedOnEvent - contains the event this ARB may be blocked
    // on.  This allows a FIBER to block itself on an Internally kept
    // event. When the event is signalled, it will wakeup, and moved
    // to the Active Pool of fibers.
    //
    // A ZERO value means there is NO event that is being blocked on.
    //

    DWORD_PTR   dwBlockedOnEvent;

    //
    // dwBlockedOnEventReturnCode - contains error code returned from
    //  fiber or main thread that is doing the unblocking.
    //

    DWORD       dwBlockedOnEventReturnCode;

#if INET_DEBUG

    //
    // dwSignature - in the debug version, we maintain a signature in the ARB
    // for sanity checking
    //

    DWORD dwSignature;

#endif // INET_DEBUG

} ARB, * LPARB;

//
// functions...
//

PRIVATE
VOID
DestroyBlockedThreadEvent(
    IN LPARB lpArb
    )

/*++

Routine Description:

    Removes lpArb from the blocked request queue if its still there, and
    destroys it

Arguments:

    lpArb   - pointer to AR_SYNC_EVENT ARB

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 None,
                 "DestroyBlockedThreadEvent",
                 "%#x",
                 lpArb
                 ));

    AcquireBlockedRequestQueue();

    if (lpArb->hEvent != NULL) {
        CloseHandle(lpArb->hEvent);
    }

    if (IsOnSerializedList(&BlockedRequestQueue, &lpArb->List)) {
        RemoveFromSerializedList(&BlockedRequestQueue, &lpArb->List);
    }

    ReleaseBlockedRequestQueue();

    DEBUG_LEAVE(0);
}


DWORD
BlockThreadOnEvent(
    IN DWORD_PTR dwEventId,
    IN DWORD dwTimeout,
    IN BOOL bReleaseLock
    )

/*++

Routine Description:

    Waits for an async event if called in the context of a fiber, else waits for
    an event if called in the context of a sync request

Arguments:

    dwEventId       - event id to wait on

    dwTimeout       - amount of time to wait

    bReleaseLock    - TRUE if we need to release the blocked request queue

Return Value:

    DWORD
        Success - ERROR_SUCCESS
                    The request/wait completed successfully

        Failure - ERROR_INTERNET_TIMEOUT
                    The request timed out

                  ERROR_INTERNET_INTERNAL_ERROR
                    We couldn't get the INTERNET_THREAD_INFO

--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 Dword,
                 "BlockThreadOnEvent",
                 "%#x, %d, %B",
                 dwEventId,
                 dwTimeout,
                 bReleaseLock
                 ));

    DWORD error;
    LPARB lpArb;

    lpArb = (LPARB)ALLOCATE_FIXED_MEMORY(sizeof(ARB));
    if (lpArb == NULL) {

        DEBUG_PRINT(ASYNC,
                    ERROR,
                    ("Out of memory allocating ARB\n"
                    ));

        error = GetLastError();
        goto quit;
    }

    //
    // set up the remaining fields in the ARB - initialize the list pointer,
    // set the priority (to default), and the event id
    //

    lpArb->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (lpArb->hEvent == NULL) {
        error = GetLastError();
        goto quit;
    }
    lpArb->dwBlockedOnEvent = dwEventId;

    //
    // add the request to the blocked request queue
    //

    InsertAtTailOfSerializedList(&BlockedRequestQueue, &lpArb->List);

    //
    // if we acquired the blocked request queue before calling this function
    // then we need to release it
    //

    if (bReleaseLock) {
        ReleaseBlockedRequestQueue();
    }

    //
    // now wait here for the event to become signalled
    //

    error = PERF_WaitForSingleObject(lpArb->hEvent,
                                     dwTimeout
                                     );

    //
    // if we timed out then we will remove and destroy the ARB, else the thread
    // which signalled the request will have done so
    //

    if (error == WAIT_TIMEOUT) {
        error = ERROR_INTERNET_TIMEOUT;
    } else {
        error = lpArb->dwBlockedOnEventReturnCode;
    }

quit:

    //
    // remove the request from the blocked request queue and destroy it
    //

    if (lpArb != NULL)
    {
        DestroyBlockedThreadEvent(lpArb);

        lpArb = (LPARB)FREE_MEMORY((HLOCAL)lpArb);
        if (lpArb != NULL) {
            INET_ASSERT(FALSE);
        }
    }

    DEBUG_LEAVE(error);

    return error;
}



DWORD
SignalThreadOnEvent(
    IN DWORD_PTR dwEventId,
    IN DWORD dwNumberOfWaiters,
    IN DWORD dwReturnCode
    )

/*++

Routine Description:

    Unblocks a number of fibers that may be waiting for an event to be signalled.
    When the fibers unblock they will be rescheduled to the Active Request Queue.
    The 'event' is reset automatically back to unsignalled state.

    If called outside of the worker thread, this function also handles
    interupting the blocked worked thread so it can resume requests.

Arguments:

    dwEventId           - Event ID to wake up on.

    dwNumberOfWaiters   - number of waiters to unblock. Choose a large number
                          to mean 'all'

    dwReturnCode        - Upon waking up fibers, their blocked called will return
                          with this error code.

Return Value:

    DWORD
        Number of waiters unblocked

--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 Int,
                 "SignalThreadOnEvent",
                 "%#x, %d, %d (%s)",
                 dwEventId,
                 dwNumberOfWaiters,
                 dwReturnCode,
                 InternetMapError(dwReturnCode)
                 ));

    INET_ASSERT(dwNumberOfWaiters > 0);

    DWORD dwUnblocked = 0;

    AcquireBlockedRequestQueue();

    LPARB lpArb = (LPARB)HeadOfSerializedList(&BlockedRequestQueue);
    LPARB lpArbPrevious = (LPARB)SlSelf(&BlockedRequestQueue);

    while (lpArb != (LPARB)SlSelf(&BlockedRequestQueue)) {
        if (lpArb->dwBlockedOnEvent == dwEventId) {

            lpArb->dwBlockedOnEventReturnCode = dwReturnCode;

            //
            // if the ARB is really an async request then add it to the end of
            // the async request queue else if it is a sync request then just
            // signal the event. The waiter will free the ARB
            //

            SetEvent(lpArb->hEvent);

            DEBUG_PRINT(ASYNC,
                        INFO,
                        ("signalled sync request %#x, on %#x\n",
                        lpArb,
                        lpArb->dwBlockedOnEvent
                        ));


            //
            // if we've hit the number of waiters we were to unblock then
            // quit
            //

            ++dwUnblocked;
            if (dwUnblocked == dwNumberOfWaiters) {
                break;
            }

            //
            // we moved the ARB
            //

            lpArb = lpArbPrevious;
        }
        lpArbPrevious = lpArb;
        lpArb = (LPARB)lpArb->List.Flink;
    }

    ReleaseBlockedRequestQueue();

    DEBUG_LEAVE(dwUnblocked);

    return dwUnblocked;
}



VOID
AcquireBlockedRequestQueue(
    VOID
    )

/*++

Routine Description:

    Synchronizes access to the blocked request queue

Arguments:

    None.

Return Value:

    None.

--*/

{
    LockSerializedList(&BlockedRequestQueue);
}


VOID
ReleaseBlockedRequestQueue(
    VOID
    )

/*++

Routine Description:

    Releases the lock acquired with AcquireBlockedRequestQueue

Arguments:

    None.

Return Value:

    None.

--*/

{
    UnlockSerializedList(&BlockedRequestQueue);
}
