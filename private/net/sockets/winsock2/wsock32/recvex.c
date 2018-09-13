/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    recvex.c

Abstract:

    This module contains support for the WSARecvEx() WinSock API.

Author:

    Keith Moore (keithmo)        10-Oct-1995

Revision History:

--*/

#include "winsockp.h"


int PASCAL
WSARecvEx (
    IN SOCKET Handle,
    IN char *Buffer,
    IN int BufferLength,
    IN OUT int *ReceiveFlags
    )

/*++

Routine Description:

    This is an extended API to allow better Windows Sockets support over
    message-based transports.  It is identical to recv() except that the
    ReceiveFlags parameter is an IN-OUT parameter that sets MSG_PARTIAL
    is the TDI provider returns STATUS_RECEIVE_PARTIAL,
    STATUS_RECEIVE_PARTIAL_EXPEDITED or STATUS_BUFFER_OVERFLOW.

Arguments:

    s - A descriptor identifying a connected socket.

    buf - A buffer for the incoming data.

    len - The length of buf.

    flags - Specifies the way in which the call is made.

Return Value:

    If no error occurs, recv() returns the number of bytes received.  If
    the connection has been closed, it returns 0.  Otherwise, a value of
    SOCKET_ERROR is returned, and a specific error code may be retrieved
    by calling WSAGetLastError().

--*/

{

    WSABUF wsaBuf;
    DWORD bytesRead;
    int result;

    //
    // Build a WSABUF describing the (single) recv buffer.
    //

    wsaBuf.len = BufferLength;
    wsaBuf.buf = Buffer;

    //
    // Setup.
    //

    *ReceiveFlags = 0;
    bytesRead = 0;

    //
    // Let WSARecv() do the dirty work.
    //

    result = WSARecv(
                 Handle,
                 &wsaBuf,
                 1,
                 &bytesRead,
                 ReceiveFlags,
                 NULL,
                 NULL
                 );

    if( result == 0 ) {

        //
        // Success.
        //

        result = (int)bytesRead;
        WS_ASSERT( result >= 0 );
        WS_ASSERT( result <= BufferLength );

    } else if( GetLastError() == WSAEMSGSIZE ) {

        //
        // Partial message received. The provider will return a
        // negative bytesRead length. Not anymore.
        //

        result = (int)bytesRead;
        *ReceiveFlags |= MSG_PARTIAL;
        WS_ASSERT( result >= 0 );
        WS_ASSERT( result <= BufferLength );

    }

    return result;

} // WSARecvEx

