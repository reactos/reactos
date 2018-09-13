/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    thrdinfo.cxx

Abstract:

    Functions to manipulate an INTERNET_THREAD_INFO

    Contents:
        InternetCreateThreadInfo
        InternetDestroyThreadInfo
        InternetTerminateThreadInfo
        InternetGetThreadInfo
        InternetSetThreadInfo
        InternetIndicateStatusAddress
        InternetIndicateStatusString
        InternetIndicateStatusNewHandle
        InternetIndicateStatus
        InternetSetLastError
        _InternetSetLastError
        InternetLockErrorText
        InternetUnlockErrorText
        InternetSetContext
        InternetSetObjectHandle
        InternetGetObjectHandle
        InternetFreeThreadInfo

Author:

    Richard L Firth (rfirth) 16-Feb-1995

Environment:

    Win32 user-level DLL

Revision History:

    16-Feb-1995 rfirth
        Created

--*/

#include <wininetp.h>
#include <perfdiag.hxx>

//
// manifests
//

#define BAD_TLS_INDEX   0xffffffff  // according to online win32 SDK documentation
#ifdef SPX_SUPPORT
#define GENERIC_SPX_NAME   "SPX Server"
#endif //SPX_SUPPORT
//
// macros
//

#ifdef ENABLE_DEBUG

#define InitializeInternetThreadInfo(lpThreadInfo) \
    InitializeListHead(&lpThreadInfo->List); \
    lpThreadInfo->Signature = INTERNET_THREAD_INFO_SIGNATURE; \
    lpThreadInfo->ThreadId = GetCurrentThreadId();

#else

#define InitializeInternetThreadInfo(threadInfo) \
    InitializeListHead(&lpThreadInfo->List); \
    lpThreadInfo->ThreadId = GetCurrentThreadId();

#endif // ENABLE_DEBUG

//
// private data
//

PRIVATE DWORD InternetTlsIndex = BAD_TLS_INDEX;
PRIVATE SERIALIZED_LIST ThreadInfoList;



INTERNETAPI
BOOL
WINAPI
ResumeSuspendedDownload(
    IN HINTERNET hRequest,
    IN DWORD dwResultCode
    )
/*++

Routine Description:

    Attempts to restart a stalled FSM that is blocked on UI interaction,

Arguments:

    hRequest - handle to open HTTP request

    dwResultCode  - the result of InternetErrorDlg, passed back into Wininet via this API

Return Value:

    DWORD
        Success - ERROR_SUCCESS
                  ERROR_IO_PENDING

        Failure - 

--*/

{
    HINTERNET hRequestMapped = NULL;
    BOOL fResumed = FALSE;
    DWORD error;

    //
    // map the handle
    //

    error = MapHandleToAddress(hRequest, (LPVOID *)&hRequestMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hRequestMapped == NULL)) {
        goto quit;
    }

    //
    // Call internal to do the work
    //

    error = ResumeAfterUserInput(
        hRequestMapped,
        dwResultCode,
        &fResumed
        );

    if ( error != ERROR_SUCCESS) 
    {
        goto quit;
    }

    if ( error == ERROR_SUCCESS )
    {
        error = ERROR_IO_PENDING; // remap to pending
    }


    //
    // If we failed to resume, the handle must have been canceled
    //

    if (!fResumed)
    {
        error = ERROR_INTERNET_OPERATION_CANCELLED;
        goto quit;
    }

quit:

    if (hRequestMapped != NULL) {
        DereferenceObject((LPVOID)hRequestMapped);
    }

    SetLastError(error);

    return (error == ERROR_SUCCESS) ? TRUE : FALSE;
}


//
// functions
//


DWORD 
ResumeAfterUserInput(
    IN HINTERNET hRequestMapped,
    IN DWORD     dwResultCode,
    OUT LPBOOL   pfItemResumed
    )
/*++

Routine Description:

    
     Internal version of Resume API that 
       unblocks an FSM after the UI has been shown, user prompted, and the FSM should now continue.

Arguments:

    hRequestMapped - 
    dwResultCode -
    pfItemResumed - 

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - 

--*/
{
    HTTP_REQUEST_HANDLE_OBJECT *pRequest;
    INTERNET_CONNECT_HANDLE_OBJECT * pConnect;
    INTERNET_HANDLE_OBJECT * pInternet;
    DWORD error = ERROR_SUCCESS;

    DEBUG_ENTER((DBG_THRDINFO,
                Dword,
                "ResumeAfterUserInput",
                "%x, %u, %x",
                hRequestMapped,
                dwResultCode,
                pfItemResumed
                ));

    *pfItemResumed = FALSE;

    //
    // Now round up the objects that are involved 
    //

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *) hRequestMapped;

    pConnect = (INTERNET_CONNECT_HANDLE_OBJECT *)pRequest->GetParent();

    INET_ASSERT(pConnect != NULL);
    INET_ASSERT(pConnect->IsValid(TypeHttpConnectHandle) == ERROR_SUCCESS);

    pInternet = (INTERNET_HANDLE_OBJECT *)pConnect->GetParent();

    INET_ASSERT(pInternet != NULL);
    INET_ASSERT(pInternet->IsValid(TypeInternetHandle) == ERROR_SUCCESS);

    //
    // Lock us while we attempt to unblock the FSM
    //
        
    pInternet->LockPopupInfo();      

    //
    // Can only resume if we're blocked on both request and internet handles
    // 

    if ( pInternet->IsBlockedOnUserInput() && 
         pRequest->IsBlockedOnUserInput() ) 
    {
        DWORD dwCntUnBlocked;

        INET_ASSERT(pInternet->GetBlockId() == pRequest->GetBlockId());

        dwCntUnBlocked = UnblockWorkItems(
                            pInternet->GetBlockedUiCount(),
                            (DWORD) pRequest->GetBlockId(), // blocked on FSM
                            ERROR_SUCCESS,
                            TP_NO_PRIORITY_CHANGE
                            );

        if ( dwCntUnBlocked > 0 )
        {
            *pfItemResumed = TRUE;
            pInternet->SetBlockedResultCode(dwResultCode);
        }

    }

    INET_ASSERT( ! (pRequest->IsBlockedOnUserInput() && !pInternet->IsBlockedOnUserInput()) );

//quit: -- not used

    pInternet->UnlockPopupInfo();

    DEBUG_LEAVE(error);

    return error;
}




DWORD
ChangeUIBlockingState(
    IN HINTERNET hRequestMapped,
    IN DWORD     dwError,
    OUT LPDWORD  lpdwActionTaken,
    OUT LPDWORD  lpdwResultCode,
    IN OUT LPVOID * lplpResultData
    )

/*++

Routine Description:

    Attempts to determine the best way of putting up UI for a given error.  This allows 
        us to back out of the Asyncronous FSM thread while popping up UI.

    How it works: 
        We attempt to prevent more than one dialog per InternetOpen handle

Arguments:

  hRequestHandle - mapped handle to open request

    dwError - error code to pass back to client and ultimately to InternetErrorDlg to generate UI

    lpdwActionTaken - returns one of several values, used to tell caller what UI action has been taken
                        UI_ACTION_CODE_NONE_TAKEN                   0
                        UI_ACTION_CODE_BLOCKED_FOR_INTERNET_HANDLE  1
                        UI_ACTION_CODE_BLOCKED_FOR_USER_INPUT       2
                        UI_ACTION_CODE_USER_ACTION_COMPLETED        3

    lpdwResultCode  - returns the result of InternetErrorDlg, passed through ResumeSuspendedDownload

    lplpResultData  - a void pointer allocated and owned by callee until after a thread has been resumed
                        used to pass extra data through client to InternetErrorDlg

Return Value:

    DWORD
        Success - ERROR_SUCCESS
                  ERROR_IO_PENDING

        Failure - 

--*/

{
    HTTP_REQUEST_HANDLE_OBJECT *pRequest;
    INTERNET_CONNECT_HANDLE_OBJECT * pConnect;

    DEBUG_ENTER((DBG_THRDINFO,
                Dword,
                "ChangeUIBlockingState",
                "%x, %u, %x, %x [%x]",
                hRequestMapped,
                dwError,
                lpdwActionTaken,
                lpdwResultCode,
                lplpResultData,
                (lplpResultData ? *lplpResultData : NULL )
                ));


    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    DWORD error = ERROR_SUCCESS;
    DWORD_PTR dwBlockId;
    BOOL fLocked = FALSE;
    BOOL fDoAsyncCallback = FALSE; // TRUE if we need to callback to the app
    LPVOID lpResultData = NULL;
   
    *lpdwActionTaken = UI_ACTION_CODE_NONE_TAKEN;

    if ( lplpResultData ) {
        lpResultData = *lplpResultData; // save off
    }

    //
    // Gather various sundry elements, objects, thread info, etc,
    //  validate, and if proper continue with the process
    //

    if (lpThreadInfo != NULL) {

        INET_ASSERT(lpThreadInfo->hObject != NULL);
        INET_ASSERT(lpThreadInfo->hObjectMapped != NULL);

        if ( lpThreadInfo->Fsm == NULL )
        {
            goto quit;
        }

        if ( ! lpThreadInfo->IsAsyncWorkerThread )
        {
            goto quit;
        }

        if ( hRequestMapped == NULL )
        {
            hRequestMapped = lpThreadInfo->Fsm->GetMappedHandle();
        }

        INET_ASSERT(hRequestMapped == lpThreadInfo->Fsm->GetMappedHandle());

        //
        // if the context value in the thread info block is 0 then we use the
        // context from the handle object
        //

        DWORD_PTR context;

        context = _InternetGetContext(lpThreadInfo);
        if (context == INTERNET_NO_CALLBACK) {
            context = ((INTERNET_HANDLE_OBJECT *)lpThreadInfo->hObjectMapped)->GetContext();
        }

        INTERNET_STATUS_CALLBACK appCallback;

        appCallback = ((INTERNET_HANDLE_OBJECT *)lpThreadInfo->hObjectMapped)->GetStatusCallback();

        //
        // No callback, no FSM means no special UI callback can be done.
        //

        if ((appCallback == NULL) || (context == INTERNET_NO_CALLBACK)) 
        {
            //
            // For the sync return error
            //

            error = dwError;
            goto quit;
        }

    }
    else
    {
        INET_ASSERT(FALSE);
        goto quit;
    }


    //
    // Now get the objects that are involved 
    //

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *) hRequestMapped;

    pConnect = (INTERNET_CONNECT_HANDLE_OBJECT *)pRequest->GetParent();

    INET_ASSERT(pConnect != NULL);
    INET_ASSERT(pConnect->IsValid(TypeHttpConnectHandle) == ERROR_SUCCESS);

    INTERNET_HANDLE_OBJECT * pInternet;

    pInternet = (INTERNET_HANDLE_OBJECT *)pConnect->GetParent();

    INET_ASSERT(pInternet != NULL);
    INET_ASSERT(pInternet->IsValid(TypeInternetHandle) == ERROR_SUCCESS);
    
    pInternet->LockPopupInfo();      
    fLocked = TRUE;

    //
    // We check whether we're blocked on the HTTP handle and the 
    //  Internet handle.   Basically we have a little matrix here
    //  based on which one is blocked (or if neither is blocked)
    //
    //  So: 
    //      InternetHandle(blocked)/RequestHandle(not-blocked) -
    //          indicates we are not the request blocking the UI,
    //          we will need to block the FSM until the other dialog
    //          has completed
    //
    //      InternetHandle(blocked)/RequestHandle(blocked) - 
    //          indicates that we have blocked and have now been 
    //          woken up, we need return the result of the UI
    //          to the caller.
    //
    //      InternetHandle(not-blocked)/RequestHandle(blocked) - 
    //          ASSERT, we shouldn't have this happen
    //
    //      InternetHandle(not-blocked)/RequestHandle(not-blocked) - 
    //          We've just entered with no-blocking handles,
    //          so we block both and wait till UI has completed
    //

    if (!pInternet->IsBlockedOnUserInput())
    {
        //
        // Indicate to the caller via callback
        //   that we need to generate UI, then block
        //

        DEBUG_PRINT(THRDINFO,
                    INFO,
                    ("Blocking on UI, notifing client via callback\n"
                    ));

        fDoAsyncCallback = TRUE;

        INET_ASSERT(!pRequest->IsBlockedOnUserInput());

        dwBlockId = (DWORD_PTR) lpThreadInfo->Fsm;
        pInternet->BlockOnUserInput(dwError, dwBlockId, lpResultData); // IncrementBlockedUiCount() is implied here.
        pRequest->BlockOnUserInput(dwBlockId);
        *lpdwActionTaken = UI_ACTION_CODE_BLOCKED_FOR_USER_INPUT;
    }
    else if (pRequest->IsBlockedOnUserInput() )
    {

        DEBUG_PRINT(THRDINFO,
                    INFO,
                    ("UnBlocking on UI, returning UI result\n"
                    ));

        INET_ASSERT(pInternet->IsBlockedOnUserInput());

        //
        // Retrieve the result of the UI and return
        //   to caller.
        //

        
        INET_ASSERT(pInternet->GetBlockId() == pRequest->GetBlockId());

        pInternet->UnBlockOnUserInput(lpdwResultCode, &lpResultData);
        pRequest->UnBlockOnUserInput();

        if ( lplpResultData ) {
            *lplpResultData = lpResultData;
        }

        *lpdwActionTaken = UI_ACTION_CODE_USER_ACTION_COMPLETED;

        //
        // If others are still blocked, then wake them up too
        //

        if (pInternet->IsBlockedOnUserInput())
        {
            DWORD dwCntUnBlocked;

            dwCntUnBlocked = UnblockWorkItems(
                                pInternet->GetBlockedUiCount(),
                                (DWORD) pRequest->GetBlockId(), // blocked on FSM
                                ERROR_SUCCESS,
                                TP_NO_PRIORITY_CHANGE
                                );

            INET_ASSERT(pInternet->GetBlockedUiCount() == dwCntUnBlocked);

            pInternet->ClearBlockedUiCount();
        }

        goto quit;
    }
    else
    {
        DEBUG_PRINT(THRDINFO,
                    INFO,
                    ("Blocking on another FSM (that is busy doing UI) until their completeion\n"
                    ));

        INET_ASSERT(pInternet->IsBlockedOnUserInput());
        INET_ASSERT(!pRequest->IsBlockedOnUserInput());

        //
        // not blocked on the request handle
        // but blocked on internet handle, so we need
        //  to wait until this internet handle is ours
        //   so we do nothing
        //

        pInternet->IncrementBlockedUiCount();

        dwBlockId = pInternet->GetBlockId();        
        *lpdwActionTaken = UI_ACTION_CODE_BLOCKED_FOR_INTERNET_HANDLE;
    }

    //
    // Now do the actual blocking of the FSM here.
    //

    lpThreadInfo->Fsm->SetState(FSM_STATE_CONTINUE);
    lpThreadInfo->Fsm->SetNextState(FSM_STATE_CONTINUE);

    error = BlockWorkItem(
        lpThreadInfo->Fsm,
        dwBlockId,               // block the FSM on FSM that created this
        INFINITE                 // we block foreever
        );

    if ( error == ERROR_SUCCESS )
    { 
        error = ERROR_IO_PENDING;
        //goto quit;
    }

quit:

    if (fLocked)
    {
        INET_ASSERT(hRequestMapped);

        pInternet->UnlockPopupInfo();
    }

    if (fDoAsyncCallback)
    {
        INTERNET_ASYNC_RESULT asyncResult;

        //
        // Pass the result Data pointer back to the caller,
        //  this is needed to pass extra info to InternetErrorDlg,
        //  once this pointer is passed, the caller must not free,
        //  until his/her FSM is restarted.
        //

        INET_ASSERT(*lpdwActionTaken == UI_ACTION_CODE_BLOCKED_FOR_USER_INPUT);

        // SUNDOWN: typecast problem
        asyncResult.dwResult = GuardedCast((DWORD_PTR)lpResultData);
        asyncResult.dwError  = dwError;

        SetLastError(dwError);

        error = InternetIndicateStatus(
                    INTERNET_STATUS_USER_INPUT_REQUIRED,
                    (LPVOID)&asyncResult,
                    sizeof(asyncResult)
                    );
        if ( error == ERROR_SUCCESS )
        {
            error = ERROR_IO_PENDING;
        }
        else
        {
            INET_ASSERT(FALSE);
        }
    }

    DEBUG_LEAVE(error);

    return error;
}


LPINTERNET_THREAD_INFO
InternetCreateThreadInfo(
    IN BOOL SetTls
    )

/*++

Routine Description:

    Creates, initializes an INTERNET_THREAD_INFO. Optionally (allocates and)
    sets this thread's Internet TLS

    Assumes: 1. The first time this function is called is in the context of the
                process attach library call, so we allocate the TLS index once

Arguments:

    SetTls  - TRUE if we are to set the INTERNET_THREAD_INFO TLS for this thread

Return Value:

    LPINTERNET_THREAD_INFO
        Success - pointer to allocated INTERNET_THREAD_INFO structure which has
                  been set as this threads value in its InternetTlsIndex slot

        Failure - NULL

--*/

{
    LPINTERNET_THREAD_INFO lpThreadInfo = NULL;
    BOOL ok = FALSE;

    if (InDllCleanup) {
        goto quit;
    }
    if (InternetTlsIndex == BAD_TLS_INDEX) {

        //
        // first time through, initialize serialized list
        //

        InitializeSerializedList(&ThreadInfoList);

        //
        // we assume that if we are allocating the TLS index, then this is the
        // one and only thread in this process that can call into this DLL
        // right now - i.e. this thread is loading the DLL
        //

        InternetTlsIndex = TlsAlloc();
    }
    if (InternetTlsIndex != BAD_TLS_INDEX) {
        lpThreadInfo = NEW(INTERNET_THREAD_INFO);
        if (lpThreadInfo != NULL) {
            InitializeInternetThreadInfo(lpThreadInfo);
            if (SetTls) {
                ok = TlsSetValue(InternetTlsIndex, (LPVOID)lpThreadInfo);
                if (!ok) {

                    DEBUG_PUT(("InternetCreateThreadInfo(): TlsSetValue(%d, %#x) returns %d\n",
                             InternetTlsIndex,
                             lpThreadInfo,
                             GetLastError()
                             ));

                    DEBUG_BREAK(THRDINFO);

                }
            } else {
                ok = TRUE;
            }
        } else {

            DEBUG_PUT(("InternetCreateThreadInfo(): NEW(INTERNET_THREAD_INFO) returned NULL\n"));

            DEBUG_BREAK(THRDINFO);

        }
    } else {

        DEBUG_PUT(("InternetCreateThreadInfo(): TlsAlloc() returns %#x, error %d\n",
                 BAD_TLS_INDEX,
                 GetLastError()
                 ));

        DEBUG_BREAK(THRDINFO);
    }
    if (ok) {
        InsertAtHeadOfSerializedList(&ThreadInfoList, &lpThreadInfo->List);
    } else {
        if (lpThreadInfo != NULL) {
            DEL(lpThreadInfo);
            lpThreadInfo = NULL;
        }
        if (InternetTlsIndex != BAD_TLS_INDEX) {
            TlsFree(InternetTlsIndex);
            InternetTlsIndex = BAD_TLS_INDEX;
        }
    }

quit:

    return lpThreadInfo;
}


VOID
InternetDestroyThreadInfo(
    VOID
    )

/*++

Routine Description:

    Cleans up the INTERNET_THREAD_INFO - deletes any memory it owns and deletes
    it

Arguments:

    None.

Return Value:

    None.

--*/

{
    LPINTERNET_THREAD_INFO lpThreadInfo;

    IF_DEBUG(NOTHING) {
        DEBUG_PUT(("InternetDestroyThreadInfo(): Thread %#x: Deleting INTERNET_THREAD_INFO\n",
                    GetCurrentThreadId()
                    ));
    }

    //
    // don't call InternetGetThreadInfo() - we don't need to create the
    // INTERNET_THREAD_INFO if it doesn't exist in this case
    //

    lpThreadInfo = (LPINTERNET_THREAD_INFO)TlsGetValue(InternetTlsIndex);
    if (lpThreadInfo != NULL) {

#if INET_DEBUG

        //
        // there shouldn't be anything in the debug record stack. On Win95, we
        // ignore this check if this is the async scheduler (nee worker) thread
        // AND there are entries in the debug record stack. The async thread
        // gets killed off before it has chance to DEBUG_LEAVE, then comes here,
        // causing this assert to be over-active
        //

        if (IsPlatformWin95() && lpThreadInfo->IsAsyncWorkerThread) {
            if (lpThreadInfo->CallDepth != 0) {

                DEBUG_PUT(("InternetDestroyThreadInfo(): "
                            "Thread %#x: "
                            "%d records in debug stack\n",
                            lpThreadInfo->CallDepth
                            ));
            }
        } else {

            INET_ASSERT(lpThreadInfo->Stack == NULL);

        }

#endif // INET_DEBUG

        InternetFreeThreadInfo(lpThreadInfo);

        INET_ASSERT(InternetTlsIndex != BAD_TLS_INDEX);

        TlsSetValue(InternetTlsIndex, NULL);
    } else {

        DEBUG_PUT(("InternetDestroyThreadInfo(): Thread %#x: no INTERNET_THREAD_INFO\n",
                    GetCurrentThreadId()
                    ));

    }
}


VOID
InternetFreeThreadInfo(
    IN LPINTERNET_THREAD_INFO lpThreadInfo
    )

/*++

Routine Description:

    Removes the INTERNET_THREAD_INFO from the list and frees all allocated
    blocks

Arguments:

    lpThreadInfo    - pointer to INTERNET_THREAD_INFO to remove and free

Return Value:

    None.

--*/

{
    RemoveFromSerializedList(&ThreadInfoList, &lpThreadInfo->List);

    if (lpThreadInfo->hErrorText != NULL) {
        FREE_MEMORY(lpThreadInfo->hErrorText);
    }

    //if (lpThreadInfo->lpResolverInfo != NULL) {
    //    if (lpThreadInfo->lpResolverInfo->DnrSocketHandle != NULL) {
    //        lpThreadInfo->lpResolverInfo->DnrSocketHandle->Dereference();
    //    }
    //    DEL(lpThreadInfo->lpResolverInfo);
    //}

    DEL(lpThreadInfo);
}


VOID
InternetTerminateThreadInfo(
    VOID
    )

/*++

Routine Description:

    Destroy all INTERNET_THREAD_INFO structures and terminate the serialized
    list. This funciton called at process detach time.

    At DLL_PROCESS_DETACH time, there may be other threads in the process for
    which we created an INTERNET_THREAD_INFO that aren't going to get the chance
    to delete the structure, so we do it here.

    Code in this module assumes that it is impossible for a new thread to enter
    this DLL while we are terminating in DLL_PROCESS_DETACH

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // get rid of this thread's info structure. No more debug output after this!
    //

    InternetDestroyThreadInfo();

    //
    // get rid of the thread info structures left by other threads
    //

    LockSerializedList(&ThreadInfoList);

    LPINTERNET_THREAD_INFO lpThreadInfo;

    while (lpThreadInfo = (LPINTERNET_THREAD_INFO)SlDequeueHead(&ThreadInfoList)) {

        //
        // already dequeued, no need to call InternetFreeThreadInfo()
        //

        FREE_MEMORY(lpThreadInfo);
    }

    UnlockSerializedList(&ThreadInfoList);

    //
    // no more need for list
    //

    TerminateSerializedList(&ThreadInfoList);

    //
    // or TLS index
    //

    TlsFree(InternetTlsIndex);
    InternetTlsIndex = BAD_TLS_INDEX;
}


LPINTERNET_THREAD_INFO
InternetGetThreadInfo(
    VOID
    )

/*++

Routine Description:

    Gets the pointer to the INTERNET_THREAD_INFO for this thread and checks
    that it still looks good.

    If this thread does not have an INTERNET_THREAD_INFO then we create one,
    presuming that this is a new thread

Arguments:

    None.

Return Value:

    LPINTERNET_THREAD_INFO
        Success - pointer to INTERNET_THREAD_INFO block

        Failure - NULL

--*/

{
    LPINTERNET_THREAD_INFO lpThreadInfo = NULL;
    DWORD lastError;

    //
    // this is pretty bad - TlsGetValue() can destroy the per-thread last error
    // variable if it returns NULL (to indicate that NULL was actually set, and
    // that NULL does not indicate an error). So we have to read it before it is
    // potentially destroyed, and reset it before we quit.
    //
    // We do this here because typically, other functions will be completely
    // unsuspecting of this behaviour, and it is better to fix it once here,
    // than in several dozen other places, even though it is slightly
    // inefficient
    //

    lastError = GetLastError();
    if (InternetTlsIndex != BAD_TLS_INDEX) {
        lpThreadInfo = (LPINTERNET_THREAD_INFO)TlsGetValue(InternetTlsIndex);
    }

    //
    // we may be in the process of creating the INTERNET_THREAD_INFO, in
    // which case its okay for this to be NULL. According to online SDK
    // documentation, a threads TLS value will be initialized to NULL
    //

    if (lpThreadInfo == NULL) {

        //
        // we presume this is a new thread. Create an INTERNET_THREAD_INFO
        //

        IF_DEBUG(NOTHING) {
            DEBUG_PUT(("InternetGetThreadInfo(): Thread %#x: Creating INTERNET_THREAD_INFO\n",
                      GetCurrentThreadId()
                      ));
        }

        lpThreadInfo = InternetCreateThreadInfo(TRUE);
    }
    if (lpThreadInfo != NULL) {

        INET_ASSERT(lpThreadInfo->Signature == INTERNET_THREAD_INFO_SIGNATURE);
        INET_ASSERT(lpThreadInfo->ThreadId == GetCurrentThreadId());

    } else {

        DEBUG_PUT(("InternetGetThreadInfo(): Failed to get/create INTERNET_THREAD_INFO\n"));

    }

    //
    // as above - reset the last error variable in case TlsGetValue() trashed it
    //

    SetLastError(lastError);

    //
    // actual success/failure indicated by non-NULL/NULL pointer resp.
    //

    return lpThreadInfo;
}


VOID
InternetSetThreadInfo(
    IN LPINTERNET_THREAD_INFO lpThreadInfo
    )

/*++

Routine Description:

    Sets lpThreadInfo as the current thread's INTERNET_THREAD_INFO. Used within
    fibers

Arguments:

    lpThreadInfo    - new INTERNET_THREAD_INFO to set

Return Value:

    None.

--*/

{
    if (InternetTlsIndex != BAD_TLS_INDEX) {
        if (!TlsSetValue(InternetTlsIndex, (LPVOID)lpThreadInfo)) {

            DEBUG_PUT(("InternetSetThreadInfo(): TlsSetValue(%d, %#x) returns %d\n",
                     InternetTlsIndex,
                     lpThreadInfo,
                     GetLastError()
                     ));

            INET_ASSERT(FALSE);

        }
    } else {

        DEBUG_PUT(("InternetSetThreadInfo(): InternetTlsIndex = %d\n",
                 InternetTlsIndex
                 ));

        INET_ASSERT(FALSE);
    }
}


DWORD
InternetIndicateStatusAddress(
    IN DWORD dwInternetStatus,
    IN LPSOCKADDR lpSockAddr,
    IN DWORD dwSockAddrLength
    )

/*++

Routine Description:

    Make a status callback to the app. The data is a network address that we
    need to convert to a string

Arguments:

    dwInternetStatus    - INTERNET_STATUS_ value

    lpSockAddr          - pointer to full socket address

    dwSockAddrLength    - length of lpSockAddr in bytes

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_OPERATION_CANCELLED
                    The app closed the object handle during the callback

--*/

{
    LPSTR lpAddress;

    INET_ASSERT(lpSockAddr != NULL);

    switch (lpSockAddr->sa_family) {
    case AF_INET:
        lpAddress = _I_inet_ntoa(
                        ((struct sockaddr_in*)lpSockAddr)->sin_addr
                        );
        break;

    case AF_IPX:

        //
        // BUGBUG - this should be a call to WSAAddressToString, but that's not implemented yet
        //
#ifdef SPX_SUPPORT
        lpAddress = GENERIC_SPX_NAME;
#else
        lpAddress = NULL;
#endif //SPX_SUPPORT
        break;

    default:
        lpAddress = NULL;
        break;
    }
    return InternetIndicateStatusString(dwInternetStatus, lpAddress);
}


DWORD
InternetIndicateStatusString(
    IN DWORD dwInternetStatus,
    IN LPSTR lpszStatusInfo OPTIONAL
    )

/*++

Routine Description:

    Make a status callback to the app. The data is a string

Arguments:

    dwInternetStatus    - INTERNET_STATUS_ value

    lpszStatusInfo      - string status data

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_OPERATION_CANCELLED
                    The app closed the object handle during the callback

--*/

{
    DEBUG_ENTER((DBG_THRDINFO,
                Dword,
                "InternetIndicateStatusString",
                "%d, %q",
                dwInternetStatus,
                lpszStatusInfo
                ));

    DWORD length;

    if (ARGUMENT_PRESENT(lpszStatusInfo)) {
        length = strlen(lpszStatusInfo) + 1;
    } else {
        length = 0;
    }

    DWORD error;

    error = InternetIndicateStatus(dwInternetStatus, lpszStatusInfo, length);

    DEBUG_LEAVE(error);

    return error;
}


DWORD
InternetIndicateStatusNewHandle(
    IN LPVOID hInternetMapped
    )

/*++

Routine Description:

    Indicates to the app a new handle

Arguments:

    hInternetMapped - mapped address of new handle being indicated

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_OPERATION_CANCELLED
                    The app closed the either the new object handle or the
                    parent object handle during the callback

--*/

{
    DEBUG_ENTER((DBG_THRDINFO,
                Dword,
                "InternetIndicateStatusNewHandle",
                "%#x",
                hInternetMapped
                ));

    HANDLE_OBJECT * hObject = (HANDLE_OBJECT *)hInternetMapped;

    //
    // reference the new request handle, in case the app closes it in the
    // callback. The new handle now has a reference count of 2
    //

    hObject->Reference();

    INET_ASSERT(hObject->ReferenceCount() == 2);

    //
    // we indicate the pseudo handle to the app
    //

    HINTERNET hInternet = hObject->GetPseudoHandle();

    DWORD error = InternetIndicateStatus(INTERNET_STATUS_HANDLE_CREATED,
                                         (LPVOID)&hInternet,
                                         sizeof(hInternet)
                                         );

    //
    // dereference the new request handle. If this returns TRUE then the new
    // handle has been deleted (the app called InternetCloseHandle() against
    // it which dereferenced it to 1, and now we've dereferenced it to zero)
    //

    if (hObject->Dereference()) {
        error = ERROR_INTERNET_OPERATION_CANCELLED;
    } else if (error == ERROR_INTERNET_OPERATION_CANCELLED) {

        //
        // the parent handle was deleted. Kill off the new handle too
        //

        BOOL ok;

        ok = hObject->Dereference();

        INET_ASSERT(ok);

    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
InternetIndicateStatus(
    IN DWORD dwStatus,
    IN LPVOID lpBuffer,
    IN DWORD dwLength
    )

/*++

Routine Description:

    If the app has registered a callback function for the object that this
    thread is operating on, call it with the arguments supplied

Arguments:

    dwStatus    - INTERNET_STATUS_ value

    lpBuffer    - pointer to variable data buffer

    dwLength    - length of *lpBuffer in bytes

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_OPERATION_CANCELLED
                    The app closed the object handle during the callback

--*/

{
    DEBUG_ENTER((DBG_THRDINFO,
                Dword,
                "InternetIndicateStatus",
                "%s, %#x, %d",
                InternetMapStatus(dwStatus),
                lpBuffer,
                dwLength
                ));

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    DWORD error = ERROR_SUCCESS;

    //
    // the app can affect callback operation by specifying a zero context value
    // meaning no callbacks will be generated for this API
    //

    if (lpThreadInfo != NULL) {

        INET_ASSERT(lpThreadInfo->hObject != NULL);
        INET_ASSERT(lpThreadInfo->hObjectMapped != NULL);

        //
        // if the context value in the thread info block is 0 then we use the
        // context from the handle object
        //

        DWORD_PTR context;

        context = _InternetGetContext(lpThreadInfo);
        if (context == INTERNET_NO_CALLBACK) {
            context = ((INTERNET_HANDLE_OBJECT *)lpThreadInfo->hObjectMapped)->GetContext();
        }

        INTERNET_STATUS_CALLBACK appCallback;

        appCallback = ((INTERNET_HANDLE_OBJECT *)lpThreadInfo->hObjectMapped)->GetStatusCallback();

        IF_DEBUG(THRDINFO) {

            if (dwStatus == INTERNET_STATUS_REQUEST_COMPLETE) {

                DEBUG_PRINT(THRDINFO,
                            INFO,
                            ("REQUEST_COMPLETE: dwResult = %#x, dwError = %d [%s]\n",
                            ((LPINTERNET_ASYNC_RESULT)lpBuffer)->dwResult,
                            ((LPINTERNET_ASYNC_RESULT)lpBuffer)->dwError,
                            InternetMapError(((LPINTERNET_ASYNC_RESULT)lpBuffer)->dwError)
                            ));

            }
        }

        if ((appCallback != NULL) && (context != INTERNET_NO_CALLBACK)) {

            LPVOID pInfo;
            DWORD infoLength;
            BOOL isAsyncWorkerThread;
            BYTE buffer[256];

            //
            // we make a copy of the info to remove the app's opportunity to
            // change it. E.g. if we were about to resolve host name "foo" and
            // passed the pointer to our buffer containing "foo", the app could
            // change the name to "bar", changing the intended server
            //

            if (lpBuffer != NULL) {
                if (dwLength <= sizeof(buffer)) {
                    pInfo = buffer;
                } else {
                    pInfo = (LPVOID)ALLOCATE_FIXED_MEMORY(dwLength);
                }

                if (pInfo != NULL) {
                    memcpy(pInfo, lpBuffer, dwLength);
                    infoLength = dwLength;
                } else {
                    infoLength = 0;

                    DEBUG_PRINT(THRDINFO,
                                ERROR,
                                ("Failed to allocate %d bytes for info\n",
                                dwLength
                                ));

                }
            } else {
                pInfo = NULL;
                infoLength = 0;
            }

            //
            // we're about to call into the app. We may be in the context of an
            // async worker thread, and if the callback submits an async request
            // then we'll execute it synchronously. To avoid this, we will reset
            // the async worker thread indicator in the INTERNET_THREAD_INFO and
            // restore it when the app returns control to us. This way, if the
            // app makes an API request during the callback, on a handle that
            // has async I/O semantics, then we will simply queue it, and not
            // try to execute it synchronously
            //

            isAsyncWorkerThread = lpThreadInfo->IsAsyncWorkerThread;
            lpThreadInfo->IsAsyncWorkerThread = FALSE;

            BOOL bInCallback = lpThreadInfo->InCallback;

            lpThreadInfo->InCallback = TRUE;

            INET_ASSERT(!IsBadCodePtr((FARPROC)appCallback));

            DEBUG_ENTER((DBG_THRDINFO,
                         None,
                         "(*callback)",
                         "%#x, %#x, %s (%d), %#x [%#x], %d",
                         lpThreadInfo->hObject,
                         context,
                         InternetMapStatus(dwStatus),
                         dwStatus,
                         pInfo,
                         ((dwStatus == INTERNET_STATUS_HANDLE_CREATED)
                         || (dwStatus == INTERNET_STATUS_HANDLE_CLOSING))
                            ? (DWORD_PTR)*(LPHINTERNET)pInfo
                            : (((dwStatus == INTERNET_STATUS_REQUEST_SENT)
                            || (dwStatus == INTERNET_STATUS_RESPONSE_RECEIVED)
                            || (dwStatus == INTERNET_STATUS_INTERMEDIATE_RESPONSE)
                            || (dwStatus == INTERNET_STATUS_STATE_CHANGE))
                                ? *(LPDWORD)pInfo
                                : 0),
                         infoLength
                         ));

            PERF_LOG(PE_APP_CALLBACK_START,
                     dwStatus,
                     lpThreadInfo->ThreadId,
                     lpThreadInfo->hObject
                     );

            HINTERNET hObject = lpThreadInfo->hObject;
            LPVOID hObjectMapped = lpThreadInfo->hObjectMapped;

            appCallback(lpThreadInfo->hObject,
                        context,
                        dwStatus,
                        pInfo,
                        infoLength
                        );

            lpThreadInfo->hObject = hObject;
            lpThreadInfo->hObjectMapped = hObjectMapped;

            PERF_LOG(PE_APP_CALLBACK_END,
                     dwStatus,
                     lpThreadInfo->ThreadId,
                     lpThreadInfo->hObject
                     );

            DEBUG_LEAVE(0);

            lpThreadInfo->InCallback = bInCallback;
            lpThreadInfo->IsAsyncWorkerThread = isAsyncWorkerThread;

            //
            // free the buffer
            //

            if (pInfo != NULL) {
                if (dwLength > sizeof(buffer))
                    FREE_FIXED_MEMORY(pInfo);
            }

            //
            // if the object is now invalid then the app closed the handle in
            // the callback, and the entire operation is cancelled
            //

            if (((HANDLE_OBJECT *)lpThreadInfo->hObjectMapped)->IsInvalidated()) {
                error = ERROR_INTERNET_OPERATION_CANCELLED;
            }
        } else {

            DEBUG_PRINT(THRDINFO,
                        ERROR,
                        ("%#x: callback = %#x, context = %#x\n",
                        lpThreadInfo->hObject,
                        appCallback,
                        context
                        ));

            //
            // if we're completing a request then we shouldn't be here - it
            // means we lost the context or callback address somewhere along the
            // way
            //

            INET_ASSERT(dwStatus != INTERNET_STATUS_REQUEST_COMPLETE);

#ifdef DEBUG
            if ( dwStatus == INTERNET_STATUS_REQUEST_COMPLETE)
            {
                INET_ASSERT(appCallback != NULL);
                INET_ASSERT(context != INTERNET_NO_CALLBACK);
                INET_ASSERT(_InternetGetContext(lpThreadInfo) != INTERNET_NO_CALLBACK);
            }
#endif


        }
    } else {

        //
        // this is catastrophic if the indication was async request completion
        //

        DEBUG_PUT(("InternetIndicateStatus(): no INTERNET_THREAD_INFO?\n"));

    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
InternetSetLastError(
    IN DWORD ErrorNumber,
    IN LPSTR ErrorText,
    IN DWORD ErrorTextLength,
    IN DWORD Flags
    )

/*++

Routine Description:

    Copies the error text to the per-thread error buffer (moveable memory)

Arguments:

    ErrorNumber     - protocol-specific error code

    ErrorText       - protocol-specific error text (from server). The buffer is
                      NOT zero-terminated

    ErrorTextLength - number of characters in ErrorText

    Flags           - Flags that control how this function operates:

                        SLE_APPEND          TRUE if ErrorText is to be appended
                                            to the text already in the buffer

                        SLE_ZERO_TERMINATE  TRUE if ErrorText must have a '\0'
                                            appended to it

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - Win32 error

--*/

{
    DEBUG_ENTER((DBG_THRDINFO,
                Dword,
                "InternetSetLastError",
                "%d, %.80q, %d, %#x",
                ErrorNumber,
                ErrorText,
                ErrorTextLength,
                Flags
                ));

    DWORD error;
    LPINTERNET_THREAD_INFO lpThreadInfo;

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo != NULL) {
        error = _InternetSetLastError(lpThreadInfo,
                                      ErrorNumber,
                                      ErrorText,
                                      ErrorTextLength,
                                      Flags
                                      );
    } else {

        DEBUG_PUT(("InternetSetLastError(): no INTERNET_THREAD_INFO\n"));

        error = ERROR_INTERNET_INTERNAL_ERROR;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
_InternetSetLastError(
    IN LPINTERNET_THREAD_INFO lpThreadInfo,
    IN DWORD ErrorNumber,
    IN LPSTR ErrorText,
    IN DWORD ErrorTextLength,
    IN DWORD Flags
    )

/*++

Routine Description:

    Sets or resets the last error text in an INTERNET_THREAD_INFO block

Arguments:

    lpThreadInfo    - pointer to INTERNET_THREAD_INFO

    ErrorNumber     - protocol-specific error code

    ErrorText       - protocol-specific error text (from server). The buffer is
                      NOT zero-terminated

    ErrorTextLength - number of characters in ErrorText

    Flags           - Flags that control how this function operates:

                        SLE_APPEND          TRUE if ErrorText is to be appended
                                            to the text already in the buffer

                        SLE_ZERO_TERMINATE  TRUE if ErrorText must have a '\0'
                                            appended to it

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - Win32 error

--*/

{
    DEBUG_ENTER((DBG_THRDINFO,
                Dword,
                "_InternetSetLastError",
                "%#x, %d, %.80q, %d, %#x",
                lpThreadInfo,
                ErrorNumber,
                ErrorText,
                ErrorTextLength,
                Flags
                ));

    DWORD currentLength;
    DWORD newTextLength;
    DWORD error;

    newTextLength = ErrorTextLength;

    //
    // if we are appending text, then account for the '\0' currently at the end
    // of the buffer (if it exists)
    //

    if (Flags & SLE_APPEND) {
        currentLength = lpThreadInfo->ErrorTextLength;
        if (currentLength != 0) {
            --currentLength;
        }
        newTextLength += currentLength;
    }

    if (Flags & SLE_ZERO_TERMINATE) {
        ++newTextLength;
    }

    //
    // expect success (and why not?)
    //

    error = ERROR_SUCCESS;

    //
    // allocate, grow or shrink the buffer to fit. The buffer is moveable. If
    // the buffer is being shrunk to zero size then NULL will be returned as
    // the buffer handle from ResizeBuffer()
    //

    lpThreadInfo->hErrorText = ResizeBuffer(lpThreadInfo->hErrorText,
                                            newTextLength,
                                            FALSE
                                            );
    if (lpThreadInfo->hErrorText != NULL) {

        LPSTR lpErrorText;

        lpErrorText = (LPSTR)LOCK_MEMORY(lpThreadInfo->hErrorText);

        INET_ASSERT(lpErrorText != NULL);

        if (lpErrorText != NULL) {
            if (Flags & SLE_APPEND) {
                lpErrorText += currentLength;
            }
            memcpy(lpErrorText, ErrorText, ErrorTextLength);
            if (Flags & SLE_ZERO_TERMINATE) {
                lpErrorText[ErrorTextLength++] = '\0';
            }

            //
            // the text should always be zero-terminated. We expect this in
            // InternetGetLastResponseInfo()
            //

            INET_ASSERT(lpErrorText[ErrorTextLength - 1] == '\0');

            UNLOCK_MEMORY(lpThreadInfo->hErrorText);

        } else {

            //
            // real error occurred - failed to lock memory?
            //

            error = GetLastError();
        }
    } else {

        INET_ASSERT(newTextLength == 0);

        newTextLength = 0;
    }

    //
    // set the error code and text length
    //

    lpThreadInfo->ErrorTextLength = newTextLength;
    lpThreadInfo->ErrorNumber = ErrorNumber;

    DEBUG_LEAVE(error);

    return error;
}


LPSTR
InternetLockErrorText(
    VOID
    )

/*++

Routine Description:

    Returns a pointer to the locked per-thread error text buffer

Arguments:

    None.

Return Value:

    LPSTR
        Success - pointer to locked buffer

        Failure - NULL

--*/

{
    LPINTERNET_THREAD_INFO lpThreadInfo;

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo != NULL) {

        HLOCAL lpErrorText;

        lpErrorText = lpThreadInfo->hErrorText;
        if (lpErrorText != (HLOCAL)NULL) {
            return (LPSTR)LOCK_MEMORY(lpErrorText);
        }
    }
    return NULL;
}

//
//VOID
//InternetUnlockErrorText(
//    VOID
//    )
//
///*++
//
//Routine Description:
//
//    Unlocks the per-thread error text buffer locked by InternetLockErrorText()
//
//Arguments:
//
//    None.
//
//Return Value:
//
//    None.
//
//--*/
//
//{
//    LPINTERNET_THREAD_INFO lpThreadInfo;
//
//    lpThreadInfo = InternetGetThreadInfo();
//
//    //
//    // assume that if we locked the error text, there must be an
//    // INTERNET_THREAD_INFO when we come to unlock it
//    //
//
//    INET_ASSERT(lpThreadInfo != NULL);
//
//    if (lpThreadInfo != NULL) {
//
//        HLOCAL hErrorText;
//
//        hErrorText = lpThreadInfo->hErrorText;
//
//        //
//        // similarly, there must be a handle to the error text buffer
//        //
//
//        INET_ASSERT(hErrorText != NULL);
//
//        if (hErrorText != (HLOCAL)NULL) {
//            UNLOCK_MEMORY(hErrorText);
//        }
//    }
//}


VOID
InternetSetContext(
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Sets the context value in the INTERNET_THREAD_INFO for status callbacks

Arguments:

    dwContext   - context value to remember

Return Value:

    None.

--*/

{
    LPINTERNET_THREAD_INFO lpThreadInfo;

    lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo != NULL) {
        _InternetSetContext(lpThreadInfo, dwContext);
    }
}


VOID
InternetSetObjectHandle(
    IN HINTERNET hInternet,
    IN HINTERNET hInternetMapped
    )

/*++

Routine Description:

    Sets the hObject field in the INTERNET_THREAD_INFO structure so we can get
    at the handle contents, even when we're in a function that does not take
    the hInternet as a parameter

Arguments:

    hInternet       - handle of object we may need info from

    hInternetMapped - mapped handle of object we may need info from

Return Value:

    None.

--*/

{
    LPINTERNET_THREAD_INFO lpThreadInfo;

    lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo != NULL) {
        _InternetSetObjectHandle(lpThreadInfo, hInternet, hInternetMapped);
    }
}


HINTERNET
InternetGetObjectHandle(
    VOID
    )

/*++

Routine Description:

    Just returns the hObject value stored in our INTERNET_THREAD_INFO

Arguments:

    None.

Return Value:

    HINTERNET
        Success - non-NULL handle value

        Failure - NULL object handle (may not have been set)

--*/

{
    LPINTERNET_THREAD_INFO lpThreadInfo;
    HINTERNET hInternet;

    lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo != NULL) {
        hInternet = lpThreadInfo->hObject;
    } else {
        hInternet = NULL;
    }
    return hInternet;
}


HINTERNET
InternetGetMappedObjectHandle(
    VOID
    )

/*++

Routine Description:

    Just returns the hObjectMapped value stored in our INTERNET_THREAD_INFO

Arguments:

    None.

Return Value:

    HINTERNET
        Success - non-NULL handle value

        Failure - NULL object handle (may not have been set)

--*/

{
    LPINTERNET_THREAD_INFO lpThreadInfo;
    HINTERNET hInternet;

    lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo != NULL) {
        hInternet = lpThreadInfo->hObjectMapped;
    } else {
        hInternet = NULL;
    }
    return hInternet;
}
