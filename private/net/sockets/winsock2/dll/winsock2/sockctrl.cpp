/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    SockCtrl.c

Abstract:

    This  module  contains  functions  that control the state of a socket.  The
    following functions are contained in the module.

    bind()
    connect()
    getpeername()
    getsockname()
    listen()
    setsockopt()
    shutdown()
    WSAConnect()
    WSAEnumNetworkEvents()
    WSAGetOverlapedResult()
    WSAJoinLeaf()


Author:

    Dirk Brandewie dirk@mink.intel.com  14-06-1995

Revision History:

    23-Aug-1995 dirk@mink.intel.com
        Cleanup after code review. Moved includes into precomp.h. Reworked all
        functions to remove extra if's and to be consistant with the rest of
        the project.

--*/
#include "precomp.h"



int WSAAPI
bind(
    IN SOCKET s,
    IN const struct sockaddr FAR *name,
    IN int namelen
    )
/*++
Routine Description:

    Associate a local address with a socket.

Arguments:

    s       - A descriptor identifying an unbound socket.

    name    - The address to assign to the socket.

    namelen - The length of the name.

Returns:

    Zero  on  success  else  SOCKET_ERROR.   The  error  code  is  stored  with
    SetLastError().

--*/
{
    INT                ReturnValue;
    PDPROVIDER         Provider;
    INT                ErrorCode;
    PDSOCKET           Socket;

    ErrorCode = TURBO_PROLOG();
    if (ErrorCode==ERROR_SUCCESS)
    {
        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            Provider = Socket->GetDProvider();
            ReturnValue = Provider->WSPBind(s,
                                            name,
                                            namelen,
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
}



int WSAAPI
connect(
    IN SOCKET s,
    IN const struct sockaddr FAR *name,
    IN int namelen
    )
/*++
Routine Description:

    Establish a connection to a peer.

Arguments:

    s       - A descriptor identifying an unconnected socket.

    name    - The name of the peer to which the socket is to be connected.

    namelen - The length of the name.

Returns:

    Zero  on  success  else  SOCKET_ERROR.   The  error  code  is  stored  with
    SetLastError().

--*/
{

    INT                ReturnValue;
    PDPROCESS          Process;
    PDTHREAD           Thread;
    PDPROVIDER         Provider;
    INT                ErrorCode;
    PDSOCKET           Socket;
    BOOL               RetryConnect;
    INT				   SavedErrorCode;


    ErrorCode = PROLOG(&Process, &Thread);
    if (ErrorCode==ERROR_SUCCESS)
    {
        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            Provider = Socket->GetDProvider();
#ifdef RASAUTODIAL
			RetryConnect = FALSE;
        retry:
#endif // RASAUTODIAL
            ReturnValue = Provider->WSPConnect(s,
                                               name,
                                               namelen,
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &ErrorCode);
#ifdef RASAUTODIAL
            if (ReturnValue == SOCKET_ERROR &&
                (ErrorCode == WSAEHOSTUNREACH || ErrorCode == WSAENETUNREACH))
            {
                if (!RetryConnect) {
                    //
                    // We preserve the original error
                    // so we can return it in case the
                    // second call to WSPConnect() fails
                    // also.
                    //
                    SavedErrorCode = ErrorCode;
                    //
                    // Only one retry per connect attempt.
                    //
                    RetryConnect = TRUE;
                    if (WSAttemptAutodialAddr(name, namelen))
                        goto retry;
                }
                else
                    ErrorCode = SavedErrorCode;
            }
#endif // RASAUTODIAL
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

    //
    // If this is a 1.x application and the service provider
    // failed the request with WSAEALREADY, map the error code
    // to WSAEINVAL to be consistent with MS's WinSock 1.1
    // implementations.
    //

    if( ErrorCode == WSAEALREADY &&
        Process->GetMajorVersion() == 1 ) {
        ErrorCode = WSAEINVAL;
    }

    SetLastError(ErrorCode);
    return SOCKET_ERROR;
}



int WSAAPI
getpeername(
    IN SOCKET s,
    OUT struct sockaddr FAR *name,
    OUT int FAR * namelen
    )
/*++
Routine Description:

    Get the address of the peer to which a socket is connected.

Arguments:

    s       - A descriptor identifying a connected socket.

    name    - The structure which is to receive the name of the peer.

    namelen - A pointer to the size of the name structure.

Returns:

    Zero  on  success  else  SOCKET_ERROR.   The  error  code  is  stored  with
    SetLastError().
--*/
{
    INT                 ReturnValue;
    INT                 ErrorCode;
    PDPROVIDER          Provider;
    PDSOCKET            Socket;

    ErrorCode = TURBO_PROLOG();
    if (ErrorCode == ERROR_SUCCESS) {
        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            Provider = Socket->GetDProvider();
            ReturnValue = Provider->WSPGetPeerName(s,
                                                   name,
                                                   namelen,
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
}



int WSAAPI
getsockname(
    IN SOCKET s,
    OUT struct sockaddr FAR *name,
    OUT int FAR * namelen
    )
/*++
Routine Description:

    Get the local name for a socket.

Arguments:

    s       - A descriptor identifying a bound socket.

    name    - Receives the address (name) of the socket.

    namelen - The size of the name buffer.

Returns:

    Zero  on  success  else  SOCKET_ERROR.   The  error  code  is  stored  with
    SetLastError().

--*/
{
    INT                 ReturnValue;
    INT                 ErrorCode;
    PDPROVIDER          Provider;
    PDSOCKET            Socket;

    ErrorCode = TURBO_PROLOG();
    if (ErrorCode == ERROR_SUCCESS) {
        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            Provider = Socket->GetDProvider();
            ReturnValue = Provider->WSPGetSockName(s,
                                                   name,
                                                   namelen,
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
}




int WSAAPI
getsockopt(
    IN SOCKET s,
    IN int level,
    IN int optname,
    OUT char FAR * optval,
    IN OUT int FAR *optlen
    )
/*++
Routine Description:

    Retrieve a socket option.

Arguments:

    s       - A descriptor identifying a socket.

    level   - The  level  at  which the option is defined; the supported levels
              include   SOL_SOCKET   and  IPPROTO_TCP.   (See  annex  for  more
              protocol-specific levels.)

    optname - The socket option for which the value is to be retrieved.

    optval  - A  pointer  to  the  buffer  in which the value for the requested
              option is to be returned.

    optlen  - A pointer to the size of the optval buffer.

Returns:

    Zero  on  success  else  SOCKET_ERROR.   The  error  code  is  stored  with
    SetLastError().

--*/
{
    INT                 ReturnValue;
    PDPROCESS           Process;
    PDTHREAD            Thread;
    INT                 ErrorCode;
    PDPROVIDER          Provider;
    PDSOCKET            Socket;
    WSAPROTOCOL_INFOW   ProtocolInfoW;
    char FAR *          SavedOptionValue = NULL;
    int                 SavedOptionLen = 0;

    ErrorCode = PROLOG(&Process, &Thread);
    if (ErrorCode==ERROR_SUCCESS) {
        //
        // SO_OPENTYPE hack-o-rama.
        //

        if( level == SOL_SOCKET && optname == SO_OPENTYPE ) {
            __try {
                if( optlen == NULL || *optlen < sizeof(INT) ) {
                    SetLastError( WSAEFAULT );
                    return SOCKET_ERROR;
                }

                *((LPINT)optval) = Thread->GetOpenType();
                *optlen = sizeof(INT);
                return ERROR_SUCCESS;
            }
            __except (WS2_EXCEPTION_FILTER()) {
                SetLastError (WSAEFAULT);
                return SOCKET_ERROR;
            }
        }

        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            Provider = Socket->GetDProvider();

            //
            // If we managed to lookup the provider from the socket, and the
            // user is asking for the ANSI WSAPROTOCOL_INFOA information,
            // then validate their option length parameter, remember this fact,
            // and map the option name to SO_PROTOCOL_INFOW.
            //

            if( level == SOL_SOCKET &&
                optname == SO_PROTOCOL_INFOA ) {

                __try {
                    if( optval == NULL ||
                        optlen == NULL ||
                        *optlen < sizeof(WSAPROTOCOL_INFOA) ) {

                        * optlen = sizeof(WSAPROTOCOL_INFOA);
                        Socket->DropDSocketReference();
                        SetLastError (WSAEFAULT);
                        return (SOCKET_ERROR);
                    }


                    SavedOptionLen = *optlen;
                    *optlen = sizeof(WSAPROTOCOL_INFOW);
                    SavedOptionValue = optval;
                    optval = (char FAR *)&ProtocolInfoW;
                    optname = SO_PROTOCOL_INFOW;
                }
                __except (WS2_EXCEPTION_FILTER()) {
                    ErrorCode = WSAEFAULT;
                    Socket->DropDSocketReference();
                    goto ErrorExit;
                }
            }

            ReturnValue = Provider->WSPGetSockOpt(s,
                                                  level,
                                                  optname,
                                                  optval,
                                                  optlen,
                                                  &ErrorCode);

            Socket->DropDSocketReference();
            if( ReturnValue == ERROR_SUCCESS ) {
                if (SavedOptionValue == NULL ) {
                    return ReturnValue;
                }
                else {
                    //
                    // We successfully retrieved the UNICODE WSAPROTOCOL_INFOW
                    // structure. Now just map it to ANSI.
                    //

                    ErrorCode = MapUnicodeProtocolInfoToAnsi(
                        &ProtocolInfoW,
                        (LPWSAPROTOCOL_INFOA)SavedOptionValue
                        );
                    __try {
                        *optlen = SavedOptionLen;
                    }
                    __except (WS2_EXCEPTION_FILTER()) {
                        ErrorCode = WSAEFAULT;
                    }

                    if (ErrorCode==ERROR_SUCCESS) {
                        return ReturnValue;
                    }
                }
            }
        }
        else {
            ErrorCode = WSAENOTSOCK;
        }
    }

ErrorExit:
    SetLastError(ErrorCode);
    return(SOCKET_ERROR);
}



int WSAAPI
listen(
    IN SOCKET s,
    IN int backlog
    )
/*++
Routine Description:

    Establish a socket to listen for incoming connection.

Arguments:

    s       - A descriptor identifying a bound, unconnected socket.

    backlog - The  maximum length to which the queue of pending connections may
              grow.   If  this  value is SOMAXCONN, then the underlying service
              provider  responsible  for  socket  s  will  set the backlog to a
              maximum reasonable value.

Returns:

    Zero  on  success  else  SOCKET_ERROR.   The  error  code  is  stored  with
    SetLastError().

--*/
{
    INT                 ReturnValue;
    INT                 ErrorCode;
    PDPROVIDER          Provider;
    PDSOCKET            Socket;

    ErrorCode = TURBO_PROLOG();
    if (ErrorCode == ERROR_SUCCESS) {

        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            Provider = Socket->GetDProvider();
            ReturnValue = Provider->WSPListen(s,
                                              backlog,
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
}




int WSAAPI
setsockopt(
    IN SOCKET s,
    IN int level,
    IN int optname,
    IN const char FAR * optval,
    IN int optlen
    )
/*++
Routine Description:

    Set a socket option.

Arguments:

    s       - A descriptor identifying a socket.

    level   - The  level  at  which the option is defined; the supported levels
              include SOL_SOCKET and IPPROTO_TCP.

    optname - The socket option for which the value is to be set.

    optval  - A  pointer  to  the  buffer  in which the value for the requested
              option is supplied.

    optlen  - The size of the optval buffer.

Returns:

    Zero  on  success  else  SOCKET_ERROR.   The  error  code  is  stored  with
    SetLastError().

--*/

{
    INT                 ReturnValue;
    PDPROCESS           Process;
    PDTHREAD            Thread;
    INT                 ErrorCode;
    PDPROVIDER          Provider;
    PDSOCKET            Socket;

    ErrorCode = PROLOG(&Process, &Thread);
    if (ErrorCode == ERROR_SUCCESS) {
        //
        // SO_OPENTYPE hack-o-rama.
        //

        if( level == SOL_SOCKET && optname == SO_OPENTYPE ) {
            INT openType;
            if( optlen < sizeof(INT) ) {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }

            __try {
                openType = *((LPINT)optval);
            }
            __except (WS2_EXCEPTION_FILTER()) {
                SetLastError (WSAEFAULT);
                return SOCKET_ERROR;
            }

            Thread->SetOpenType( openType );
            return ERROR_SUCCESS;
        }

        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            Provider = Socket->GetDProvider();
            ReturnValue = Provider->WSPSetSockOpt(s,
                                                  level,
                                                  optname,
                                                  optval,
                                                  optlen,
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
}



int WSAAPI
shutdown(
    IN SOCKET s,
    IN int how
    )
/*++
Routine Description:

    Disable sends and/or receives on a socket.

Arguments:

     s   - A descriptor identifying a socket.

     how - A  flag  that  describes  what  types of operation will no longer be
           allowed.

Returns:

    Zero  on  success  else  SOCKET_ERROR.   The  error  code  is  stored  with
    SetLastError().
--*/
{
    INT                 ReturnValue;
    INT                 ErrorCode;
    PDPROVIDER          Provider;
    PDSOCKET            Socket;

    ErrorCode = TURBO_PROLOG();
    if (ErrorCode==ERROR_SUCCESS) {
        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            Provider = Socket->GetDProvider();
            ReturnValue = Provider->WSPShutdown(s,
                                                how,
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
}



int WSAAPI
WSAConnect(
    IN SOCKET s,
    IN const struct sockaddr FAR *name,
    IN int namelen,
    IN LPWSABUF lpCallerData,
    OUT LPWSABUF lpCalleeData,
    IN LPQOS lpSQOS,
    IN LPQOS lpGQOS
    )
/*++
Routine Description:

    Establish a connection to a peer, exchange connect data, and specify needed
    quality of service based on the supplied flow spec.

Arguments:

    s            - A descriptor identifying an unconnected socket.

    name         - The name of the peer to which the socket is to be connected.

    namelen      - The length of the name.

    lpCallerData - A  pointer to the user data that is to be transferred to the
                   peer during connection establishment.

    lpCalleeData - A  pointer  to  the user data that is to be transferred back
                   from the peer during connection establishment.

    lpSQOS       - A pointer to the flow spec for socket s.

    lpGQOS       - A  pointer  to  the  flow  spec  for  the  socket  group (if
                   applicable).

Returns:

    Zero  on  success  else  SOCKET_ERROR.   The  error  code  is  stored  with
    SetLastError().

--*/
{
    INT                 ReturnValue;
    INT                 ErrorCode;
    PDPROVIDER          Provider;
    PDSOCKET            Socket;

    ErrorCode = TURBO_PROLOG();
    if (ErrorCode==ERROR_SUCCESS) {
        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            Provider = Socket->GetDProvider();
            ReturnValue = Provider->WSPConnect(s,
                                               name,
                                               namelen,
                                               lpCallerData,
                                               lpCalleeData,
                                               lpSQOS,
                                               lpGQOS,
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
}



int WSAAPI
WSAEnumNetworkEvents(
    IN SOCKET s,
    IN WSAEVENT hEventObject,
    IN LPWSANETWORKEVENTS lpNetworkEvents
    )
/*++
Routine Description:

    Discover occurrences of network events for the indicated socket.

Arguments:
     s               - A descriptor identifying the socket.

     hEventObject    - An  optional  handle  identifying  an  associated  event
                       object to be reset.

     lpNetworkEvents - A  pointer  to a WSANETWORKEVENTS struct which is filled
                       with  a  record  of  occurred  network  events  and  any
                       associated error codes.

Returns:

    Zero  on  success  else  SOCKET_ERROR.   The  error  code  is  stored  with
    SetLastError().
--*/
{
    INT                 ReturnValue;
    INT                 ErrorCode;
    PDPROVIDER          Provider;
    PDSOCKET            Socket;

    ErrorCode = TURBO_PROLOG();
    if (ErrorCode==ERROR_SUCCESS) {
        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            Provider = Socket->GetDProvider();
            ReturnValue = Provider->WSPEnumNetworkEvents(s,
                                                         hEventObject,
                                                         lpNetworkEvents,
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
}



BOOL WSAAPI
WSAGetOverlappedResult(
    IN SOCKET s,
    IN LPWSAOVERLAPPED lpOverlapped,
    OUT LPDWORD lpcbTransfer,
    IN BOOL fWait,
    OUT LPDWORD lpdwFlags
    )
/*++
Routine Description:

    Returns the results of an overlapped operation on the specified socket.

Arguments:
    s            - Identifies  the  socket.   This  is the same socket that was
                   specified  when  the  overlapped  operation was started by a
                   call to WSARecv(), WSARecvFrom(), WSASend(), WSASendTo(), or
                   WSAIoctl().

    lpOverlapped - Points  to a WSAOVERLAPPED structure that was specified when
                   the overlapped operation was started.

    lpcbTransfer - Points  to  a  32-bit  variable  that receives the number of
                   bytes  that  were  actually transferred by a send or receive
                   operation, or by WSAIoctl().

    fWait        - Specifies  whether  the function should wait for the pending
                   overlapped  operation  to  complete.   If TRUE, the function
                   does  not return until the operation has been completed.  If
                   FALSE  and  the  operation  is  still  pending, the function
                   returns  FALSE  and  the  WSAGetLastError() function returns
                   WSA_IO_INCOMPLETE.

    lpdwFlags    - Points  to  a  32-bit variable that will receive one or more
                   flags   that  supplement  the  completion  status.   If  the
                   overlapped   operation   was   initiated  via  WSARecv()  or
                   WSARecvFrom(), this parameter will contain the results value
                   for lpFlags parameter.

Returns:

     If  the  function succeeds, the return value is TRUE.  This means that the
     overlapped  operation  has  completed  and  that  the  value pointed to by
     lpcbTransfer    has   been   updated.    The   application   should   call
     WSAGetLastError() to obtain any error status for the overlapped operation.
     If  the function fails, the return value is FALSE.  This means that either
     the overlapped operation has not completed or that completion status could
     not  be  determined  due to errors in one or more parameters.  On failure,
     the   value   pointed  to  by  lpcbTransfer  will  not  be  updated.   Use
     WSAGetLastError() to determine the cause of the failure.

--*/
{
    BOOL                ReturnValue;
    INT                 ErrorCode;
    PDPROVIDER          Provider;
    PDSOCKET            Socket;

    ErrorCode = TURBO_PROLOG();
    if (ErrorCode==ERROR_SUCCESS) {

        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            Provider = Socket->GetDProvider();
            ReturnValue = Provider->WSPGetOverlappedResult(
                s,
                lpOverlapped,
                lpcbTransfer,
                fWait,
                lpdwFlags,
                &ErrorCode);
            Socket->DropDSocketReference();
            if (ReturnValue)
                return ReturnValue;
        }
        else {
            ErrorCode = WSAENOTSOCK;
        }
    }

    SetLastError(ErrorCode);
    return FALSE;
}
