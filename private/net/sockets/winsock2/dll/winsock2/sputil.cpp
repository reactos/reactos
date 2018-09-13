/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    sputil.cpp

Abstract:

    This  module  contains the implementation of the utility functions provided
    to   winsock   service  providers.   This  module  contains  the  following
    functions.

    WPUCloseEvent
    WPUCreateEvent
    WPUResetEvent
    WPUSetEvent
    WPUQueryBlockingCallback
    WSCGetProviderPath

Author:

    Dirk Brandewie (dirk@mink.intel.com) 20-Jul-1995

Notes:

    $Revision:   1.21  $

    $Modtime:   08 Mar 1996 00:45:22  $


Revision History:

    22-Aug-1995 dirk@mink.intel.com
        Cleanup after code review. Moved includes to precomp.h. Added
        some trace code
--*/


#include "precomp.h"


PWINSOCK_POST_ROUTINE SockPostRoutine = NULL;


BOOL WSPAPI
WPUCloseEvent(
    IN WSAEVENT hEvent,
    OUT LPINT lpErrno
    )
/*++
Routine Description:

    Closes an open event object handle.

Arguments:

    hEvent  - Identifies an open event object handle.

    lpErrno - A pointer to the error code.

Returns:

    If the function succeeds, the return value is TRUE.

--*/
{
    BOOL ReturnCode;

    ReturnCode = CloseHandle(hEvent);
    if (!ReturnCode) {
        *lpErrno = GetLastError();
    } //if
    return(ReturnCode);
}



WSAEVENT WSPAPI
WPUCreateEvent(
    OUT LPINT lpErrno
    )
/*++
Routine Description:

    Create a new event object.

Arguments:

    lpErrno - A pointer to the error code.

Returns:

    If  the  function  succeeds,  the  return  value is the handle of the event
    object.

    If the function fails, the return value is WSA_INVALID_EVENT and a specific
    error code is available in lpErrno.

--*/
{
    HANDLE ReturnValue;

    ReturnValue = CreateEvent(NULL, // default security
                              TRUE, // manual reset
                              FALSE, // nonsignalled state
                              NULL); // anonymous
    if (NULL == ReturnValue) {
        *lpErrno = GetLastError();
    } //if
    return(ReturnValue);
}




int WSPAPI
WPUQueryBlockingCallback(
    IN DWORD dwCatalogEntryId,
    OUT LPBLOCKINGCALLBACK FAR * lplpfnCallback,
    OUT PDWORD_PTR lpdwContext,
    OUT LPINT lpErrno
    )
/*++
Routine Description:

    Returns a pointer to a callback function the service probider should invoke
    periodically while servicing blocking operations.

Arguments:

    dwCatalogEntryId - Identifies the calling service provider.

    lplpfnCallback   - Receives a pointer to the blocking callback function.

    lpdwContext      - Receives  a context value the service provider must pass
                       into the blocking callback.

    lpErrno          - A pointer to the error code.

Returns:

    If  the function succeeds, it returns ERROR_SUCCESS.  Otherwise, it returns
    SOCKET_ERROR and a specific error code is available in the location pointed
    to by lpErrno.
--*/
{
    int                  ReturnValue;
    INT                  ErrorCode;
    LPBLOCKINGCALLBACK   callback_func = NULL;
    PDTHREAD             Thread;
    PDPROCESS            Process;
    DWORD_PTR            ContextValue  = 0;
    PDCATALOG            Catalog;

    assert(lpdwContext);
    assert(lpErrno);

    ErrorCode = PROLOG(&Process,
           &Thread);
    if (ErrorCode == ERROR_SUCCESS) {
        callback_func = Thread->GetBlockingCallback();

        if( callback_func != NULL ) {
            PPROTO_CATALOG_ITEM  CatalogItem;
            PDPROVIDER           Provider;

            Catalog = Process->GetProtocolCatalog();
            assert(Catalog);
            ErrorCode = Catalog->GetCountedCatalogItemFromCatalogEntryId(
                dwCatalogEntryId,  // CatalogEntryId
                & CatalogItem);    // CatalogItem
            if (ERROR_SUCCESS == ErrorCode) {

                Provider = CatalogItem->GetProvider();
                assert(Provider);
                ContextValue = Provider->GetCancelCallPtr();
                CatalogItem->Dereference ();
            } //if
        } //if
    } //if

    if (ERROR_SUCCESS == ErrorCode) {
        ReturnValue = ERROR_SUCCESS;
    }
    else {
        ReturnValue = SOCKET_ERROR;
        callback_func = NULL;
    } //if

    // Set the out parameters.
    *lpdwContext = ContextValue;
    *lpErrno = ErrorCode;
    *lplpfnCallback = callback_func;

    return(ReturnValue);
}




int WSPAPI
WPUQueueApc(
    IN LPWSATHREADID lpThreadId,
    IN LPWSAUSERAPC lpfnUserApc,
    IN DWORD_PTR dwContext,
    OUT LPINT lpErrno
    )
/*++
Routine Description:

    Queues  a  user-mode  APC  to  the  specified thread in order to facilitate
    invocation of overlapped I/O completion routines.

Arguments:

    lpThreadId  - A  pointer  to  a  WSATHREADID  structure that identifies the
                  thread  context.   This  is typically supplied to the service
                  provider  by  the  WinSock  DLL  as  in input parameter to an
                  overlapped operation.

    lpfnUserApc - Points to the APC function to be called.


    dwContext   - A  32  bit context value which is subsequently supplied as an
                  input parameter to the APC function.

    lpErrno     - A pointer to the error code.

Returns:

    If  no  error  occurs,  WPUQueueApc()  returns  0 and queues the completion
    routine  for the specified thread.  Otherwise, it returns SOCKET_ERROR, and
    a specific error code is available in lpErrno.

--*/
{
    HANDLE HelperHandle;
    PDPROCESS Process;
    PDTHREAD Thread;
    INT      ErrorCode;

    // Use ProLog to fill in Process and thread pointers. Only fail if
    // there is no valid process context
    ErrorCode = PROLOG(&Process,
           &Thread);
    if (ErrorCode == WSANOTINITIALISED) {
        *lpErrno = ErrorCode;
        return(SOCKET_ERROR);
    } //if
    assert (Process!=NULL);

    ErrorCode = Process->GetAsyncHelperDeviceID(&HelperHandle);
    if (ERROR_SUCCESS == ErrorCode )
    {
        ErrorCode = (INT) WahQueueUserApc(HelperHandle,
                                           lpThreadId,
                                           lpfnUserApc,
                                           dwContext);

    } //if

    if( ErrorCode == NO_ERROR ) {
        return ERROR_SUCCESS;
    }
    else {
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }
}


int
WSPAPI
WPUCompleteOverlappedRequest (
    SOCKET s, 	
    LPWSAOVERLAPPED lpOverlapped, 	
    DWORD dwError, 	
    DWORD cbTransferred, 	
    LPINT lpErrno
)
/*++
Routine Description:

    This function simmulates completion of overlapped IO request
    on socket handle created for non-IFS providers

Arguments:

    s            - socket handle to complete request on
    lpOverlapped - pointer to overlapped structure
    dwError      - WinSock 2.0 error code for opreation being completed
    cbTransferred- number of bytes transferred to/from user buffers as the
                    result of the operation being completed
    lpErrno     - A pointer to the error code.

Returns:

    If  no  error  occurs,  WPUCompleteOverlappedRequest()  returns  0 and
    completes the overlapped request as request by the application.
    Otherwise, it returns SOCKET_ERROR, and a specific error code is available
    in lpErrno.

--*/
{
    HANDLE HelperHandle;
    PDPROCESS Process;
    PDTHREAD Thread;
    INT      ErrorCode;

    // Use ProLog to fill in Process and thread pointers. Only fail if
    // there is no valid process context
    ErrorCode = PROLOG(&Process,
           &Thread);
    if (ErrorCode == WSANOTINITIALISED) {
        *lpErrno = ErrorCode;
        return(SOCKET_ERROR);
    } //if
    assert (Process!=NULL);

    ErrorCode = Process->GetHandleHelperDeviceID(&HelperHandle);
    if (ERROR_SUCCESS == ErrorCode )
    {
        ErrorCode = (INT) WahCompleteRequest (HelperHandle,
                                           s,
                                           lpOverlapped,
                                           dwError,
                                           cbTransferred);

    } //if

    if( ErrorCode == NO_ERROR ) {
        return ERROR_SUCCESS;
    }
    else {
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }
}


int WSPAPI
WPUOpenCurrentThread(
    OUT LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno
    )
/*++
Routine Description:

    Opens the current thread. This is intended to be used by layered service
    providers that wish to initiate overlapped IO from non-application threads.

Arguments:

    lpThreadId  - A pointer to a WSATHREADID structure that will receive the
                  thread data.

    lpErrno     - A pointer to the error code.

Returns:

    If no error occurs, WPUOpenCurrentThread() returns 0 and the caller is
    responsible for (eventually) closing the thread by calling WPUCloseThread().
    Otherwise, WPUOpenCurrentThread() returns SOCKET_ERROR and a specific
    error code is available in lpErrno.

--*/
{
    HANDLE HelperHandle;
    PDPROCESS Process;
    PDTHREAD Thread;
    INT      ErrorCode;

    // Use ProLog to fill in Process and thread pointers. Only fail if
    // there is no valid process context
    ErrorCode = PROLOG(&Process,
           &Thread);
    if (ErrorCode == WSANOTINITIALISED) {
        *lpErrno = ErrorCode;
        return(SOCKET_ERROR);
    } //if
    assert (Process!=NULL);

    ErrorCode = Process->GetAsyncHelperDeviceID(&HelperHandle);
    if (ERROR_SUCCESS == ErrorCode )
    {
        ErrorCode = (INT) WahOpenCurrentThread(HelperHandle,
                                                lpThreadId);

    } //if

    if( ErrorCode == NO_ERROR ) {
        return ERROR_SUCCESS;
    }
    else {
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }
}


int WSPAPI
WPUCloseThread(
    IN LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno
    )
/*++
Routine Description:

    Closes a thread opened via WPUOpenCurrentThread().

Arguments:

    lpThreadId  - A pointer to a WSATHREADID structure that identifies the
                  thread context.  This structure must have been initialized
                  by a previous call to WPUOpenCurrentThread().

    lpErrno     - A pointer to the error code.

Returns:

    If no error occurs, WPUCloseThread() returns 0.  Otherwise, it returns
    SOCKET_ERROR, and a specific error code is available in lpErrno.

--*/
{
    INT ReturnCode= SOCKET_ERROR;
    HANDLE HelperHandle;
    PDPROCESS Process;
    PDTHREAD Thread;
    INT      ErrorCode=0;

    // Use ProLog to fill in Process and thread pointers. Only fail if
    // there is no valid process context
    ErrorCode = PROLOG(&Process,
           &Thread);
    if (ErrorCode == WSANOTINITIALISED) {
        *lpErrno = ErrorCode;
        return(SOCKET_ERROR);
    } //if
    assert (Process!=NULL);

    ErrorCode = Process->GetAsyncHelperDeviceID(&HelperHandle);
    if (ERROR_SUCCESS == ErrorCode )
        {
        ErrorCode = (INT) WahCloseThread(HelperHandle,
                                          lpThreadId);

    } //if

    if( ErrorCode == NO_ERROR ) {
        return ERROR_SUCCESS;
    }
    else {
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }
}



BOOL WSPAPI
WPUResetEvent(
    IN WSAEVENT hEvent,
    OUT LPINT lpErrno
    )
/*++
Routine Description:

    Resets the state of the specified event object to nonsignaled.

Arguments:

    hEvent  - Identifies an open event object handle.

    lpErrno - A pointer to the error code.

Returns:

    If the function succeeds, the return value is TRUE.  If the function fails,
    the  return  value  is  FALSE  and  a  specific  error code is available in
    lpErrno.
--*/
{
    BOOL ReturnCode;

    ReturnCode = ResetEvent(hEvent);
    if (FALSE == ReturnCode) {
        *lpErrno = GetLastError();
    } //if
    return(ReturnCode);
}




BOOL WSPAPI
WPUSetEvent(
    IN WSAEVENT hEvent,
    OUT LPINT lpErrno
    )
/*++
Routine Description:

    Sets the state of the specified event object to signaled.

Arguments:

    hEvent  - Identifies an open event object handle.

    lpErrno - A pointer to the error code.

Returns:

    If the function succeeds, the return value is TRUE.  If the function fails,
    the  return  value  is  FALSE  and  a  specific  error code is available in
    lpErrno.
--*/
{
    BOOL ReturnCode;

    ReturnCode = SetEvent(hEvent);
    if (FALSE == ReturnCode) {
        *lpErrno = GetLastError();
    } //if
    return(ReturnCode);
}


BOOL
WINAPI
WPUPostMessage(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PWINSOCK_POST_ROUTINE   sockPostRoutine;
    sockPostRoutine = GET_SOCK_POST_ROUTINE ();
    if (sockPostRoutine==NULL)
        return FALSE;

    //
    // Special post routine works only for 16 bit apps.
    // It assumes that message is in HIWORD of Msg and LOWORD
    // is an index that tells it what post routine to call
    // (so it can properly map parameters for async name resolution).
    // If layered provider did its own async select and used its own
    // message and window in the context of 16 bit process it will simply
    // crash when processing message posted by the base provider. Grrr...
    //
    // So in the code below we try our best to figure out if message is
    // directed not to application but to layered provider window.
    //
    if (sockPostRoutine!=PostMessage) {
        PDSOCKET    Socket = DSOCKET::GetCountedDSocketFromSocketNoExport((SOCKET)wParam);
        BOOL        apiSocket;
        if (Socket!=NULL) {
            apiSocket = Socket->IsApiSocket();
            Socket->DropDSocketReference ();
            if (!apiSocket) {
                //
                // We use delayload option with user32.dll, hence
                // the exception handler here.
                //
                __try {
                    return PostMessage (hWnd, Msg, wParam, lParam);
                }
                __except (WS2_EXCEPTION_FILTER()) {
                    return FALSE;
                }
            }
        }
    }

    return (sockPostRoutine)( hWnd, Msg, wParam, lParam );
}   // WPUPostMessage

