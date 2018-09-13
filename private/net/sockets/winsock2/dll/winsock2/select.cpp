/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    select.c

Abstract:

    This module contains the "select" entry points from the winsock
    API. The following functions aare contained in this module.

    select()
    WSAEventSelect()
    WSAAsyncSelect()
    __WSAFDIsSet()

Author:

    Dirk Brandewie dirk@mink.intel.com  14-06-1995

Revision History:

    22-Aug-1995 dirk@mink.intel.com
        Cleanup after code review. Moved includes to precomp.h

    16-Aug-1995  dirk@mink.intel.com
        Added implementation of __WSAFDIsSet

--*/
#include "precomp.h"




int WSAAPI
select (
    IN int nfds,
    IN OUT fd_set FAR *readfds,
    IN OUT fd_set FAR *writefds,
    IN OUT fd_set FAR *exceptfds,
    IN const struct timeval FAR *timeout
    )
/*++
Routine Description:

    Determine the status of one or more sockets, waiting if necessary.

Arguments:

    nfds - This argument is ignored and included only for the sake of
           compatibility.

    readfds - An optional pointer to a set of sockets to be checked
              for readability.

    writefds - An optional pointer to a set of sockets to be checked
               for writability.

    exceptfds - An optional pointer to a set of sockets to be checked
                for errors.

    timeout - The maximum time for select() to wait, or NULL for
              blocking operation.

Returns:
    select() returns the total number of descriptors which are ready
    and contained in the fd_set structures, 0 if the time limit
    expired, or SOCKET_ERROR if an error occurred.  If the return
    value is SOCKET_ERROR, The error code is stored with
    SetLastError().
--*/
{
    INT                ReturnValue;
    INT                ErrorCode;
    PDPROVIDER         Provider;
    PDSOCKET           Socket;
    SOCKET             SocketID;
    BOOL               FoundSocket=FALSE;

    ErrorCode = TURBO_PROLOG();

    if (ErrorCode == ERROR_SUCCESS) {

        __try {
            // Look for a socket in the three fd_sets handed in. The first
            // socket found will be used to select the service provider to
            // service this call
            if (readfds && readfds->fd_count)
                {
                SocketID = readfds->fd_array[0];
                FoundSocket = TRUE;
                } //if

            if (!FoundSocket && writefds && writefds->fd_count )
                {
                SocketID = writefds->fd_array[0];
                FoundSocket = TRUE;
                } //if

            if (!FoundSocket && exceptfds && exceptfds->fd_count )
                {
                SocketID = exceptfds->fd_array[0];
                FoundSocket = TRUE;
                } //if
        }
        __except (WS2_EXCEPTION_FILTER()) {
            ErrorCode = WSAEFAULT;
            goto ReturnError;
        }

        if (FoundSocket) {
            Socket = DSOCKET::GetCountedDSocketFromSocket(SocketID);
            if(Socket != NULL){
                Provider = Socket->GetDProvider();
                ReturnValue = Provider->WSPSelect(
                    nfds,
                    readfds,
                    writefds,
                    exceptfds,
                    timeout,
                    &ErrorCode);
                Socket->DropDSocketReference();
                if (ReturnValue!=SOCKET_ERROR)
                    return ReturnValue;

                assert (ErrorCode!=NO_ERROR);
                if (ErrorCode==NO_ERROR)
                    ErrorCode = WSASYSCALLFAILURE;

            } //if
            else {
                ErrorCode = WSAENOTSOCK;
            }
        } //if
        else {
            ErrorCode = WSAEINVAL;
        } //else
    }

ReturnError:
    SetLastError(ErrorCode);
    return(SOCKET_ERROR);
} //select




int WSAAPI
WSAEventSelect(
    IN SOCKET s,
    IN WSAEVENT hEventObject,
    IN long lNetworkEvents
    )
/*++
Routine Description:

    Specify an event object to be associated with the supplied set of
    FD_XXX network events.

Arguments:

    s - A descriptor identifying the socket.

    hEventObject - A handle identifying the event object to be
                   associated with the supplied set of FD_XXX network
                   events.

    lNetworkEvents - A bitmask which specifies the combination of
                     FD_XXX network events in which the application
                     has interest.

Returns:
    Zero on success else SOCKET_Error. The error code is stored with
    SetLastError().
--*/
{
    INT                ReturnValue;
    INT                ErrorCode;
    PDPROVIDER         Provider;
    PDSOCKET           Socket;

    ErrorCode = TURBO_PROLOG();

    if (ErrorCode == ERROR_SUCCESS) {
        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            Provider = Socket->GetDProvider();
            ReturnValue = Provider->WSPEventSelect(
                s,
                hEventObject,
                lNetworkEvents,
                &ErrorCode);
            Socket->DropDSocketReference();
            if (ReturnValue==ERROR_SUCCESS)
                return ReturnValue;
            assert (ErrorCode!=NO_ERROR);
            if (ErrorCode==NO_ERROR)
                ErrorCode = WSASYSCALLFAILURE;
        }
        else {
            ErrorCode = WSAENOTSOCK;
        }
    }

    SetLastError(ErrorCode);
    return SOCKET_ERROR;
} //WSAEventSelect


int WSAAPI
WSAAsyncSelect(
    IN SOCKET s,
    IN HWND hWnd,
    IN u_int wMsg,
    IN long lEvent
    )
/*++
Routine Description:

    Request event notification for a socket.

Arguments:

    s - A descriptor identifying the socket for which event notification is
        required.

    hWnd - A handle identifying the window which should receive a message when
           a network event occurs.

    wMsg - The message to be received when a network event occurs.

    lEvent - A bitmask which specifies a combination of network events in which
             the application is interested.

Returns:
    The return value is 0 if the application's declaration of interest in the
    network event set was successful.  Otherwise the value SOCKET_ERROR is
    returned, and a specific error number may be retrieved by calling
    WSAGetLastError().
--*/
{
    INT                ReturnValue;
    INT                ErrorCode;
    PDPROVIDER         Provider;
    PDSOCKET           Socket;

    ErrorCode = TURBO_PROLOG();

    if (ErrorCode == ERROR_SUCCESS) {
        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            Provider = Socket->GetDProvider();
            ReturnValue = Provider->WSPAsyncSelect(
                s,
                hWnd,
                wMsg,
                lEvent,
                &ErrorCode);
            Socket->DropDSocketReference();
            if (ReturnValue==ERROR_SUCCESS)
                return ReturnValue;
            assert (ErrorCode!=NO_ERROR);
            if (ErrorCode==NO_ERROR)
                ErrorCode = WSASYSCALLFAILURE;
        }
        else {
            ErrorCode = WSAENOTSOCK;
        }
    }

    SetLastError(ErrorCode);
    return SOCKET_ERROR;
} //WSAAsyncSelect



int FAR PASCAL
__WSAFDIsSet(
    SOCKET fd,
    fd_set FAR *set)
/*++
Routine Description:

    Determines if a specific socket is a contained in an FD_SET.

Arguments:

    s - A descriptor identifying the socket.

    set - A pointer to an FD_SET.
Returns:

    Returns TRUE if socket s is a member of set, otherwise FALSE.

--*/
{
    int i = set->fd_count; // index into FD_SET
    int rc=FALSE; // user return code

    while (i--){
        if (set->fd_array[i] == fd) {
            rc = TRUE;
        } //if
    } //while
    return (rc);
} // __WSAFDIsSet



int FAR PASCAL
WPUFDIsSet(
    SOCKET fd,
    fd_set FAR *set)
/*++
Routine Description:

    Determines if a specific socket is a contained in an FD_SET.

Arguments:

    s - A descriptor identifying the socket.

    set - A pointer to an FD_SET.
Returns:

    Returns TRUE if socket s is a member of set, otherwise FALSE.

--*/
{
    int return_value;

    return_value = __WSAFDIsSet(
        fd,
        set
        );
    return(return_value);
} // WPUFDIsSet

