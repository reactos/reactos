/*++

    Copyright (c) 1996 Microsoft Corporation

Module Name:

    bhook.cpp

Abstract:

    This module contains the winsock API entrypoints for manipulating
    blocking hooks for WinSock 1.x applications.

    The following functions are exported by this module:

        WSACancelBlockingCall()
        WSAIsBlocking()
        WSASetBlockingHook()
        WSAUnhookBlockingHook()

Author:

    Keith Moore keithmo@microsoft.com 10-May-1996

Revision History:

--*/

#include "precomp.h"



int
WSAAPI
WSACancelBlockingCall(
    VOID
    )

/*++

Routine Description:

    This function cancels any outstanding blocking operation for this
    task.  It is normally used in two situations:

        (1) An application is processing a message which has been
          received while a blocking call is in progress.  In this case,
          WSAIsBlocking() will be true.

        (2) A blocking call is in progress, and Windows Sockets has
          called back to the application's "blocking hook" function (as
          established by WSASetBlockingHook()).

    In each case, the original blocking call will terminate as soon as
    possible with the error WSAEINTR.  (In (1), the termination will not
    take place until Windows message scheduling has caused control to
    revert to the blocking routine in Windows Sockets.  In (2), the
    blocking call will be terminated as soon as the blocking hook
    function completes.)

    In the case of a blocking connect() operation, the Windows Sockets
    implementation will terminate the blocking call as soon as possible,
    but it may not be possible for the socket resources to be released
    until the connection has completed (and then been reset) or timed
    out.  This is likely to be noticeable only if the application
    immediately tries to open a new socket (if no sockets are
    available), or to connect() to the same peer.

Arguments:

    None.

Return Value:

    The value returned by WSACancelBlockingCall() is 0 if the operation
    was successfully canceled.  Otherwise the value SOCKET_ERROR is
    returned, and a specific error number may be retrieved by calling
    WSAGetLastError().

--*/

{
    PDPROCESS Process;
    PDTHREAD  Thread;
    INT       ErrorCode;

    ErrorCode = PROLOG(&Process,&Thread);

    if (ErrorCode!=ERROR_SUCCESS)
    {
        if( ErrorCode != WSAEINPROGRESS ) {
            SetLastError(ErrorCode);
            return SOCKET_ERROR;
        }
    } //if

    //
    // Verify this isn't a WinSock 2.x app trying to do something stupid.
    //

    if( Process->GetMajorVersion() >= 2 ) {
        SetLastError( WSAEOPNOTSUPP );
        return SOCKET_ERROR;
    }

    //
    // Let the DTHREAD object cancel the socket I/O initiated in
    // this thread.
    //

    ErrorCode = Thread->CancelBlockingCall();

    if (ErrorCode == ERROR_SUCCESS) {
        return (ERROR_SUCCESS);
    }
    else {
        SetLastError(ErrorCode);
        return (SOCKET_ERROR);
    }

} // WSACancelBlockingCall


BOOL
WSAAPI
WSAIsBlocking(
    VOID
    )

/*++

Routine Description:

    This function allows a task to determine if it is executing while
    waiting for a previous blocking call to complete.

Arguments:

    None.

Return Value:

    The return value is TRUE if there is an outstanding blocking
    function awaiting completion.  Otherwise, it is FALSE.

--*/

{
    PDPROCESS Process;
    PDTHREAD  Thread;
    INT       ErrorCode;

    ErrorCode = PROLOG(&Process,&Thread);

    if (ErrorCode!=ERROR_SUCCESS)
    {
        if( ErrorCode != WSAEINPROGRESS ) {
            return FALSE;
        }
    } //if

    return Thread->IsBlocking();

} // WSAIsBlocking


FARPROC
WSAAPI
WSASetBlockingHook (
    FARPROC lpBlockFunc
    )

/*++

Routine Description:

    This function installs a new function which a Windows Sockets
    implementation should use to implement blocking socket function
    calls.

    A Windows Sockets implementation includes a default mechanism by
    which blocking socket functions are implemented.  The function
    WSASetBlockingHook() gives the application the ability to execute
    its own function at "blocking" time in place of the default
    function.

    When an application invokes a blocking Windows Sockets API
    operation, the Windows Sockets implementation initiates the
    operation and then enters a loop which is equivalent to the
    following pseudocode:

        for(;;) {
             // flush messages for good user response
             while(BlockingHook())
                  ;
             // check for WSACancelBlockingCall()
             if(operation_cancelled())
                  break;
             // check to see if operation completed
             if(operation_complete())
                  break;     // normal completion
        }

    The default BlockingHook() function is equivalent to:

        BOOL DefaultBlockingHook(void) {
             MSG msg;
             BOOL ret;
             // get the next message if any
             ret = (BOOL)PeekMessage(&msg,0,0,PM_REMOVE);
             // if we got one, process it
             if (ret) {
                  TranslateMessage(&msg);
                  DispatchMessage(&msg);
             }
             // TRUE if we got a message
             return ret;
        }

    The WSASetBlockingHook() function is provided to support those
    applications which require more complex message processing - for
    example, those employing the MDI (multiple document interface)
    model.  It is not intended as a mechanism for performing general
    applications functions.  In particular, the only Windows Sockets API
    function which may be issued from a custom blocking hook function is
    WSACancelBlockingCall(), which will cause the blocking loop to
    terminate.

Arguments:

    lpBlockFunc - A pointer to the procedure instance address of the
        blocking function to be installed.

Return Value:

    The return value is a pointer to the procedure-instance of the
    previously installed blocking function.  The application or library
    that calls the WSASetBlockingHook () function should save this
    return value so that it can be restored if necessary.  (If "nesting"
    is not important, the application may simply discard the value
    returned by WSASetBlockingHook() and eventually use
    WSAUnhookBlockingHook() to restore the default mechanism.) If the
    operation fails, a NULL pointer is returned, and a specific error
    number may be retrieved by calling WSAGetLastError().

--*/

{
    PDPROCESS          Process;
    PDTHREAD           Thread;
    INT                ErrorCode;
    FARPROC            PreviousHook;

    ErrorCode = PROLOG(&Process,&Thread);

    if (ErrorCode!=ERROR_SUCCESS)
    {
        SetLastError(ErrorCode);
        return NULL;
    } //if

    //
    // Verify this isn't a WinSock 2.x app trying to do something stupid.
    //

    if( Process->GetMajorVersion() >= 2 ) {
        SetLastError( WSAEOPNOTSUPP );
        return NULL;
    }

    //
    // Validate the blocking hook parameter.
    //

    if( IsBadCodePtr( lpBlockFunc ) ) {
        SetLastError( WSAEFAULT );
        return NULL;
    }

    //
    // Let the DTHREAD object set the blocking hook & return the previous
    // hook.
    //

    PreviousHook = Thread->SetBlockingHook( lpBlockFunc );
    assert( PreviousHook != NULL );

    return PreviousHook;

} // WSASetBlockingHook


int
WSAAPI
WSAUnhookBlockingHook(
    VOID
    )

/*++

Routine Description:

    This function removes any previous blocking hook that has been
    installed and reinstalls the default blocking mechanism.

    WSAUnhookBlockingHook() will always install the default mechanism,
    not the previous mechanism.  If an application wish to nest blocking
    hooks - i.e.  to establish a temporary blocking hook function and
    then revert to the previous mechanism (whether the default or one
    established by an earlier WSASetBlockingHook()) - it must save and
    restore the value returned by WSASetBlockingHook(); it cannot use
    WSAUnhookBlockingHook().

Arguments:

    None.

Return Value:

    The return value is 0 if the operation was successful.  Otherwise
    the value SOCKET_ERROR is returned, and a specific error number may
    be retrieved by calling WSAGetLastError().

--*/

{
    PDPROCESS          Process;
    PDTHREAD           Thread;
    INT                ErrorCode;

    ErrorCode = PROLOG(&Process,&Thread);

    if (ErrorCode!=ERROR_SUCCESS)
    {
        SetLastError(ErrorCode);
        return SOCKET_ERROR;
    } //if

    //
    // Verify this isn't a WinSock 2.x app trying to do something stupid.
    //

    if( Process->GetMajorVersion() >= 2 ) {
        SetLastError( WSAEOPNOTSUPP );
        return SOCKET_ERROR;
    }

    //
    // Let the DTHREAD object unhook the blocking hook.
    //

    ErrorCode = Thread->UnhookBlockingHook();

    if (ErrorCode == ERROR_SUCCESS) {
        return (ERROR_SUCCESS);
    }
    else {
        SetLastError(ErrorCode);
        return (SOCKET_ERROR);
    }
} // WSAUnhookBlockingHook

