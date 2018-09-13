/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    recv.cpp

Abstract:

    This module contains the data recption API functions for winsock2 DLL

Author:

    Dirk Brandewie dirk@mink.intel.com  14-06-1995

Revision History:

    22-Aug-1995 dirk@mink.intel.com
        Cleanup after code review. Moved includes to precomp.h. Reworked whole
        file. recv and recvform are now just calls to the WSA versions of the
        calls.

    dirk@mink.intel.com 21-Jul-1995
       Added warnoff.h to includes. Moved assignment statements
       outside of confditionals

    Mark Hamilton mark_hamilton@ccm.jf.intel.com 18-06-1995
       Implemented all of the functions.
--*/


#include "precomp.h"


int WSAAPI
recv(
     IN SOCKET s,
     OUT char FAR * buf,
     IN int len,
     IN int flags
     )
/*++
Routine Description:

    Receive data from a socket.

Arguments:

    s     - A descriptor identifying a connected socket.

    buf   - A buffer for the incoming data.

    len   - The length of buf.

    flags - Specifies the way in which the call is made.

Returns:

    The  number  of  bytes  received.   If  the  connection has been gracefully
    closed,  the  return  value  is  0.   Otherwise, a value of SOCKET_ERROR is
    returned, and a specific error code is stored with SetErrorCode().

--*/

{
    INT             ErrorCode;
    PDSOCKET        Socket;
    LPWSATHREADID   ThreadId;


	ErrorCode = TURBO_PROLOG_OVLP(&ThreadId);
    if (ErrorCode==ERROR_SUCCESS)
    {
        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            INT             ReturnValue;
            PDPROVIDER      Provider;
            DWORD           BytesReceived;
            WSABUF          Buffers;

            Buffers.len = len;
            Buffers.buf = buf;

            Provider = Socket->GetDProvider();
            ReturnValue = Provider->WSPRecv(s,
                              &Buffers,
                              1,
                              &BytesReceived,
                              (LPDWORD)&flags,
                              NULL,                 // lpOverlapped
                              NULL,                 // lpCompletionRoutine
                              ThreadId,
                              &ErrorCode);
            Socket->DropDSocketReference();
            if (ReturnValue==ERROR_SUCCESS) {
                if ((flags & MSG_PARTIAL)==0) {
                    return (INT)BytesReceived;
                }
                ErrorCode = WSAEMSGSIZE;
            }

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
recvfrom(
    IN SOCKET s,
    OUT char FAR * buf,
    IN int len,
    IN int flags,
    OUT struct sockaddr FAR *from,
    IN OUT int FAR * fromlen
    )
/*++
Routine Description:

    Receive a datagram and store the source address.

Arguments:

    s       - A descriptor identifying a bound socket.

    buf     - A buffer for the incoming data.

    len     - The length of buf.

    flags   - Specifies the way in which the call is made.

    from    - An  optional  pointer  to  a  buffer  which  will hold the source
              address upon return.

    fromlen - An optional pointer to the size of the from buffer.

Returns:

    The  number  of  bytes  received.   If  the  connection has been gracefully
    closed,  the  return  value  is  0.   Otherwise, a value of SOCKET_ERROR is
    returned, and a specific error code is stored with SetErrorCode().

--*/

{
    INT                ErrorCode;
    PDSOCKET           Socket;
    LPWSATHREADID      ThreadId;

	ErrorCode = TURBO_PROLOG_OVLP(&ThreadId);
    if (ErrorCode==ERROR_SUCCESS)
    {

        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            INT             ReturnValue;
            PDPROVIDER      Provider;
            DWORD           BytesReceived;
            WSABUF          Buffers;

            Buffers.len = len;
            Buffers.buf = buf;

            Provider = Socket->GetDProvider();
            ReturnValue = Provider->WSPRecvFrom(s,
                                  &Buffers,
                                  1,
                                  &BytesReceived,
                                  (LPDWORD)&flags,
                                  from,
                                  fromlen,
                                  NULL,                 // lpOverlapped
                                  NULL,                 // lpCompletionRoutine
                                  ThreadId,
                                  &ErrorCode);
            Socket->DropDSocketReference();
            if (ReturnValue==ERROR_SUCCESS) {
                if ((flags & MSG_PARTIAL)==0) {
                    return (INT)BytesReceived;
                }
                ErrorCode = WSAEMSGSIZE;
            }

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
WSARecv(
    IN SOCKET s,
    OUT LPWSABUF lpBuffers,
    IN DWORD dwBufferCount,
    OUT LPDWORD lpNumberOfBytesRecvd,
    IN OUT LPDWORD lpFlags,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )
/*++
Routine Description:

    Receive data from a socket.

Arguments:

    s                    - A descriptor identifying a connected socket.

    lpBuffers            - A  pointer  to  an array of WSABUF structures.  Each
                           WSABUF  structure contains a pointer to a buffer and
                           the length of the buffer.

    dwBufferCount        - The  number  of  WSABUF  structures in the lpBuffers
                           array.

    lpNumberOfBytesRecvd - A  pointer  to  the number of bytes received by this
                           call if the receive operation completes immediately.

    lpFlags              - A pointer to flags.

    lpOverlapped         - A  pointer to a WSAOVERLAPPED structure (ignored for
                           non-overlapped sockets).

    lpCompletionRoutine  - A  pointer to the completion routine called when the
                           receive  operation  has  been completed (ignored for
                           non-overlapped sockets).

Returns:

    If  no  error  occurs  and the receive operation has completed immediately,
    WSARecv() returns the number of bytes received.  If the connection has been
    closed,  it returns 0.  Otherwise, a value of SOCKET_ERROR is returned, and
    the  specific  error  code  is  stored with SetErrorCode().  The error code
    WSA_IO_PENDING   indicates   that   the   overlapped   operation  has  been
    successfully  initiated  and  that  completion will be indicated at a later
    time.  Any other error code indicates that the overlapped operation was not
    successfully initiated and no completion indication will occur.

    If  the  MSG_INTERRUPT  flag  is  set,  the  meaning of the return value is
    changed.  A value of zero indicates success and is interpreted as described
    above.   Otherwise,  the return value will directly contain the appropriate
    error  code  as  shown  below.   Note that this is applicable only to Win16
    environments  and only for protocols that have the XP1_INTERRUPT bit set in
    the WSAPROTOCOL_INFO struct.

--*/
{
    INT                 ErrorCode;
    PDSOCKET            Socket;
    LPWSATHREADID       ThreadId;

	ErrorCode = TURBO_PROLOG_OVLP(&ThreadId);
    if (ErrorCode==ERROR_SUCCESS)
    {
        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            INT                 ReturnValue;
            PDPROVIDER          Provider;

            Provider = Socket->GetDProvider();
            ReturnValue = Provider->WSPRecv(s,
                              lpBuffers,
                              dwBufferCount,
                              lpNumberOfBytesRecvd,
                              lpFlags,
                              lpOverlapped,
                              lpCompletionRoutine,
                              ThreadId,
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
WSARecvDisconnect(
    IN SOCKET s,
    OUT LPWSABUF lpInboundDisconnectData
    )
/*++
Routine Description:

    Terminate  reception  on  a socket, and retrieve the disconnect data if the
    socket is connection-oriented.

Arguments:

    s                       - A descriptor identifying a socket.

    lpInboundDisconnectData - A pointer to the incoming disconnect data.

Returns:

    Zero on success else SOCKET_ERROR. The error code is stored with
    SetErrorCode().

--*/

{
    PDPROCESS           Process;
    PDTHREAD            Thread;
    INT                 ErrorCode;
    PDSOCKET            Socket;

    ErrorCode = PROLOG (&Process,&Thread);
    if (ErrorCode==ERROR_SUCCESS) {

        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            INT                 ReturnValue;
            PDPROVIDER          Provider;

            Provider = Socket->GetDProvider();
            ReturnValue = Provider->WSPRecvDisconnect(
                s,
                lpInboundDisconnectData,
                &ErrorCode);
            Socket->DropDSocketReference();
            if (ReturnValue==ERROR_SUCCESS)
                return ERROR_SUCCESS;
        }
        else {
            ErrorCode = WSAENOTSOCK;
        }
    }

    SetLastError(ErrorCode);
    return(SOCKET_ERROR);
}



int WSAAPI
WSARecvFrom(
    IN SOCKET s,
    OUT LPWSABUF lpBuffers,
    IN DWORD dwBufferCount,
    OUT LPDWORD lpNumberOfBytesRecvd,
    IN OUT LPDWORD lpFlags,
    OUT struct sockaddr FAR *  lpFrom,
    IN OUT LPINT lpFromlen,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )
/*++
Routine Description:

    Receive a datagram and store the source address.

Arguments:

    s                    - A descriptor identifying a socket

    lpBuffers            - A  pointer  to  an array of WSABUF structures.  Each
                           WSABUF  structure contains a pointer to a buffer and
                           the length of the buffer.

    dwBufferCount        - The  number  of  WSABUF  structures in the lpBuffers
                           array.

    lpNumberOfBytesRecvd - A  pointer  to  the number of bytes received by this
                           call if the receive operation completes immediately.

    lpFlags              - A pointer to flags.

    lpFrom               - An  optional pointer to a buffer which will hold the
                           source address upon the completion of the overlapped
                           operation.

    lpFromlen            - A  pointer  to the size of the from buffer, required
                           only if lpFrom is specified.

    lpOverlapped         - A  pointer to a WSAOVERLAPPED structure (ignored for
                           non- overlapped sockets).

    lpCompletionRoutine  - A  pointer to the completion routine called when the
                           receive  operation  has  been completed (ignored for
                           non-overlapped sockets).

Returns:

    Zero  on  success  else  SOCKET_ERROR.   The  error  code  is  stored  with
    SetErrorCode().

--*/

{
    INT                ErrorCode;
    PDSOCKET           Socket;
    LPWSATHREADID      ThreadId;

	ErrorCode = TURBO_PROLOG_OVLP(&ThreadId);
    if (ErrorCode==ERROR_SUCCESS)
    {

        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            INT             ReturnValue;
            PDPROVIDER      Provider;

            Provider = Socket->GetDProvider();
            ReturnValue = Provider->WSPRecvFrom(s,
                                  lpBuffers,
                                  dwBufferCount,
                                  lpNumberOfBytesRecvd,
                                  lpFlags,
                                  lpFrom,
                                  lpFromlen,
                                  lpOverlapped,
                                  lpCompletionRoutine,
                                  ThreadId,
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







