/*++

Copyright (c) 1992-1996 Microsoft Corporation

Module Name:

    select.c

Abstract:

    This module contains support for the select( ) and WSASelectWindow
    WinSock APIs.

Author:

    David Treadwell (davidtr)    4-Apr-1992

Revision History:

--*/


#include "winsockp.h"
//
// FD_SET uses the FD_SETSIZE macro, so define it to a huge value here so
// that apps can pass a very large number of sockets to select(), which
// uses the FD_SET macro.
// Have to do this after precompiled include since it will be ignored
// otherwise
//
#undef FD_SETSIZE
#define FD_SETSIZE 65536


#define HANDLES_IN_SET(set) ( (set) == NULL ? 0 : (set->fd_count & 0xFFFF) )
#define IS_EVENT_ENABLED(event, socket)                     \
            ( (socket->DisabledAsyncSelectEvents & event) == 0 && \
              (socket->AsyncSelectlEvent & event) != 0 )

typedef struct _POLL_CONTEXT_BLOCK {
    PSOCKET_INFORMATION SocketInfo;
    DWORD AsyncSelectSerialNumber;
    IO_STATUS_BLOCK IoStatus;
    AFD_POLL_INFO PollInfo;
} POLL_CONTEXT_BLOCK, *PPOLL_CONTEXT_BLOCK;

BOOL
SockCheckAndInitAsyncSelectHelper (
    VOID
    );

VOID
SockProcessAsyncSelect (
    PSOCKET_INFORMATION Socket,
    PPOLL_CONTEXT_BLOCK Context
    );


int
WSPAPI
WSPSelect (
    int nfds,
    fd_set *readfds,
    fd_set *writefds,
    fd_set *exceptfds,
    const struct timeval *timeout,
    LPINT lpErrno
    )

/*++

Routine Description:

    This function is used to determine the status of one or more
    sockets.  For each socket, the caller may request information on
    read, write or error status.  The set of sockets for which a given
    status is requested is indicated by an fd_set structure.  Upon
    return, the structure is updated to reflect the subset of these
    sockets which meet the specified condition, and select() returns the
    number of sockets meeting the conditions.  A set of macros is
    provided for manipulating an fd_set.  These macros are compatible
    with those used in the Berkeley software, but the underlying
    representation is completely different.

    The parameter readfds identifies those sockets which are to be
    checked for readability.  If the socket is currently listen()ing, it
    will be marked as readable if an incoming connection request has
    been received, so that an accept() is guaranteed to complete without
    blocking.  For other sockets, readability means that queued data is
    available for reading, so that a recv() or recvfrom() is guaranteed
    to complete without blocking.  The presence of out-of-band data will
    be checked if the socket option SO_OOBINLINE has been enabled (see
    setsockopt()).

    The parameter writefds identifies those sockets which are to be
    checked for writeability.  If a socket is connect()ing
    (non-blocking), writeability means that the connection establishment
    is complete.  For other sockets, writeability means that a send() or
    sendto() will complete without blocking.  [It is not specified how
    long this guarantee can be assumed to be valid, particularly in a
    multithreaded environment.]

    The parameter exceptfds identifies those sockets which are to be
    checked for the presence of out- of-band data or any exceptional
    error conditions.  Note that out-of-band data will only be reported
    in this way if the option SO_OOBINLINE is FALSE.  For a SOCK_STREAM,
    the breaking of the connection by the peer or due to KEEPALIVE
    failure will be indicated as an exception.  This specification does
    not define which other errors will be included.

    Any of readfds, writefds, or exceptfds may be given as NULL if no
    descriptors are of interest.

    Four macros are defined in the header file winsock.h for
    manipulating the descriptor sets.  The variable FD_SETSIZE
    determines the maximum number of descriptors in a set.  (The default
    value of FD_SETSIZE is 64, which may be modified by #defining
    FD_SETSIZE to another value before #including winsock.h.)
    Internally, an fd_set is represented as an array of SOCKETs; the
    last valid entry is followed by an element set to INVALID_SOCKET.
    The macros are:

    FD_CLR(s, *set)     Removes the descriptor s from set.
    FD_ISSET(s, *set)   Nonzero if s is a member of the set, zero otherwise.
    FD_SET(s, *set)     Adds descriptor s to set.
    FD_ZERO(*set)       Initializes the set to the NULL set.

    The parameter timeout controls how long the select() may take to
    complete.  If timeout is a null pointer, select() will block
    indefinitely until at least one descriptor meets the specified
    criteria.  Otherwise, timeout points to a struct timeval which
    specifies the maximum time that select() should wait before
    returning.  If the timeval is initialized to {0, 0}, select() will
    return immediately; this is used to "poll" the state of the selected
    sockets.

Arguments:

    nfds - This argument is ignored and included only for the sake of
        compatibility.

    readfds - A set of sockets to be checked for readability.

    writefds - A set of sockets to be checked for writeability

    exceptfds -  set of sockets to be checked for errors.

    timeout   The maximum time for select() to wait, or NULL for blocking
        operation.

Return Value:

    select() returns the total number of descriptors which are ready and
    contained in the fd_set structures, or 0 if the time limit expired.

--*/

{

    NTSTATUS status;
	PWINSOCK_TLS_DATA	tlsData;
    int err;
    PAFD_POLL_INFO pollInfo;
    ULONG pollBufferSize;
    PAFD_POLL_HANDLE_INFO pollHandleInfo;
    ULONG handleCount;
    ULONG i;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG handlesReady;
    UCHAR pollBuffer[MAX_FAST_AFD_POLL];

    WS_ENTER( "WSPSelect", readfds, writefds, exceptfds, (PVOID)timeout );

    WS_ASSERT( lpErrno != NULL );

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPSelect", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    //
    // Set up locals so that we know how to clean up on exit.
    //

    pollInfo = NULL;
    handlesReady = 0;

    __try {

        //
        // Determine how many handles we're going to check so that we can
        // allocate a buffer large enough to hold information about all of
        // them.
        //

        handleCount = HANDLES_IN_SET( readfds ) +
                      HANDLES_IN_SET( writefds ) +
                      HANDLES_IN_SET( exceptfds );

        //
        // If there are no handles specified, just return.
        //

        if ( handleCount == 0 ) {

            WS_EXIT( "WSPSelect", 0, FALSE );
            return 0;

        }

        //
        // Allocate space to hold the input buffer for the poll IOCTL.
        //

        pollBufferSize = sizeof(AFD_POLL_INFO) +
                             handleCount * sizeof(AFD_POLL_HANDLE_INFO);

        if( pollBufferSize <= sizeof(pollBuffer) ) {

            pollInfo = (PVOID)pollBuffer;

        } else {

            pollInfo = ALLOCATE_HEAP( pollBufferSize );

            if ( pollInfo == NULL ) {

                err = WSAENOBUFS;
                goto exit;

            }

        }

        //
        // Initialize the poll buffer.
        //

        pollInfo->NumberOfHandles = handleCount;
        pollInfo->Unique = FALSE;

        pollHandleInfo = pollInfo->Handles;

        for ( i = 0; readfds != NULL && i < (readfds->fd_count & 0xFFFF); i++ ) {

            //
            // If the connection is disconnected, either abortively or
            // orderly, then it is considered possible to read immediately
            // on the socket, so include these events in addition to receive.
            //

            pollHandleInfo->Handle = (HANDLE)readfds->fd_array[i];
            pollHandleInfo->PollEvents =
                AFD_POLL_RECEIVE | AFD_POLL_DISCONNECT | AFD_POLL_ABORT;
            pollHandleInfo++;

        }

        for ( i = 0; writefds != NULL && i < (writefds->fd_count & 0xFFFF); i++ ) {

            pollHandleInfo->Handle = (HANDLE)writefds->fd_array[i];
            pollHandleInfo->PollEvents = AFD_POLL_SEND;
            pollHandleInfo++;

        }

        for ( i = 0; exceptfds != NULL && i < (exceptfds->fd_count & 0xFFFF); i++ ) {

            pollHandleInfo->Handle = (HANDLE)exceptfds->fd_array[i];
            pollHandleInfo->PollEvents =
                AFD_POLL_RECEIVE_EXPEDITED | AFD_POLL_CONNECT_FAIL;
            pollHandleInfo++;

        }

        //
        // If a timeout was specified, convert it to NT format.  Since it is
        // a relative time, it must be negative.
        //

        if ( timeout != NULL ) {

            LARGE_INTEGER microseconds;

            pollInfo->Timeout = RtlEnlargedIntegerMultiply(
                                    timeout->tv_sec,
                                    -10*1000*1000
                                    );

            microseconds = RtlEnlargedIntegerMultiply( timeout->tv_usec, -10 );

            pollInfo->Timeout.QuadPart =
                pollInfo->Timeout.QuadPart + microseconds.QuadPart;

        } else {

            //
            // No timeout was specified, just set the timeout value
            // to the largest possible value, in effect using an infinite
            // timeout.
            //

            pollInfo->Timeout.LowPart = 0xFFFFFFFF;
            pollInfo->Timeout.HighPart = 0x7FFFFFFF;

        }

    }
    __except (SOCK_EXCEPTION_FILTER()) {
        err = WSAEFAULT;
        goto exit;
    }
    //
    // Send the IOCTL to AFD.  AFD will complete the request as soon as
    // one or more of the specified handles is ready for the specified
    // operation.
    //
    // Just use the first handle as the handle for the request.  Any
    // handle is fine; we just need a handle to AFD so that it gets to the
    // driver.
    //
    // Note that the same buffer is used for both input and output.
    // Since IOCTL_AFD_POLL is a method 0 (buffered) IOCTL, this
    // shouldn't cause problems.
    //

    WS_ASSERT( (IOCTL_AFD_POLL & 0x03) == METHOD_BUFFERED );

    status = NtDeviceIoControlFile(
                 pollInfo->Handles[0].Handle,
                 tlsData->EventHandle,
                 NULL,                   // APC Routine
                 NULL,                   // APC Context
                 &ioStatusBlock,
                 IOCTL_AFD_POLL,
                 pollInfo,
                 pollBufferSize,
                 pollInfo,
                 pollBufferSize
                 );

    if ( status == STATUS_PENDING ) {

        SockWaitForSingleObject(
            tlsData->EventHandle,
            (SOCKET)pollInfo->Handles[0].Handle,
            pollInfo->Timeout.QuadPart == 0 ?
                SOCK_NEVER_CALL_BLOCKING_HOOK :
                SOCK_ALWAYS_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        status = ioStatusBlock.Status;

    }

    if ( !NT_SUCCESS(status) ) {

        err = SockNtStatusToSocketError( status );
        goto exit;

    }

    __try {

        //
        // Use the information provided by the driver to set up the fd_set
        // structures to return to the caller.  First zero out the structures.
        //

        if ( readfds != NULL ) {

            FD_ZERO( readfds );

        }

        if ( writefds != NULL ) {

            FD_ZERO( writefds );

        }

        if ( exceptfds != NULL ) {

            FD_ZERO( exceptfds );

        }

        //
        // Walk the poll buffer returned by AFD, setting up the fd_set
        // structures as we go.
        //

        pollHandleInfo = pollInfo->Handles;

        for ( i = 0; i < pollInfo->NumberOfHandles; i++ ) {

            WS_ASSERT( pollHandleInfo->PollEvents != 0 );

            if ( (pollHandleInfo->PollEvents & AFD_POLL_RECEIVE) != 0 ) {

                WS_ASSERT( readfds != NULL );

                if ( !FD_ISSET( (SOCKET)pollHandleInfo->Handle, readfds ) ) {

                    FD_SET( (SOCKET)pollHandleInfo->Handle, readfds );
                    handlesReady++;

                }

                IF_DEBUG(SELECT) {

                    WS_PRINT(( "WSPSelect handle %lx ready for reading.\n",
                                   pollHandleInfo->Handle ));

                }

            }

            if ( (pollHandleInfo->PollEvents & AFD_POLL_SEND) != 0 ) {

                WS_ASSERT( writefds != NULL );

                if ( !FD_ISSET( (SOCKET)pollHandleInfo->Handle, writefds ) ) {

                    FD_SET( (SOCKET)pollHandleInfo->Handle, writefds );
                    handlesReady++;

                }

                IF_DEBUG(SELECT) {

                    WS_PRINT(( "WSPSelect handle %lx ready for writing.\n",
                                   pollHandleInfo->Handle ));

                }

            }

            if ( (pollHandleInfo->PollEvents & AFD_POLL_RECEIVE_EXPEDITED) != 0 ) {

                WS_ASSERT( exceptfds != NULL );

                if ( !FD_ISSET( (SOCKET)pollHandleInfo->Handle, exceptfds ) ) {

                    FD_SET( (SOCKET)pollHandleInfo->Handle, exceptfds );
                    handlesReady++;

                }


                IF_DEBUG(SELECT) {

                    WS_PRINT(( "WSPSelect handle %lx ready for expedited reading.\n",
                                   pollHandleInfo->Handle ));

                }

            }

            if ( (pollHandleInfo->PollEvents & AFD_POLL_ACCEPT) != 0 ) {

                WS_ASSERT( readfds != NULL );

                if ( !FD_ISSET( (SOCKET)pollHandleInfo->Handle, readfds ) ) {

                    FD_SET( (SOCKET)pollHandleInfo->Handle, readfds );
                    handlesReady++;

                }


                IF_DEBUG(SELECT) {

                    WS_PRINT(( "WSPSelect handle %lx ready for accept.\n",
                                   pollHandleInfo->Handle ));

                }

            }

            if ( (pollHandleInfo->PollEvents & AFD_POLL_CONNECT) != 0 ) {

                WS_ASSERT( NT_SUCCESS(pollHandleInfo->Status) );
                WS_ASSERT( writefds != NULL );

                if ( !FD_ISSET( (SOCKET)pollHandleInfo->Handle, writefds ) ) {

                    FD_SET( (SOCKET)pollHandleInfo->Handle, writefds );
                    handlesReady++;

                }

                IF_DEBUG(SELECT) {

                    WS_PRINT(( "WSPSelect handle %lx completed connect, status %lx\n",
                                   pollHandleInfo->Handle, pollHandleInfo->Status ));

                }

            }

            if ( (pollHandleInfo->PollEvents & AFD_POLL_CONNECT_FAIL) != 0 ) {

                WS_ASSERT( !NT_SUCCESS(pollHandleInfo->Status) );
                WS_ASSERT( exceptfds != NULL );

                if ( !FD_ISSET( (SOCKET)pollHandleInfo->Handle, exceptfds ) ) {

                    FD_SET( (SOCKET)pollHandleInfo->Handle, exceptfds );
                    handlesReady++;

                }

                IF_DEBUG(SELECT) {

                    WS_PRINT(( "WSPSelect handle %lx completed connect, status %lx\n",
                                   pollHandleInfo->Handle, pollHandleInfo->Status ));

                }

            }

            if ( (pollHandleInfo->PollEvents & AFD_POLL_DISCONNECT) != 0 ) {

                WS_ASSERT( readfds != NULL );

                if ( !FD_ISSET( (SOCKET)pollHandleInfo->Handle, readfds ) ) {

                    FD_SET( (SOCKET)pollHandleInfo->Handle, readfds );
                    handlesReady++;

                }


                IF_DEBUG(SELECT) {

                    WS_PRINT(( "WSPSelect handle %lx disconnected.\n",
                                   pollHandleInfo->Handle ));

                }

            }

            if ( (pollHandleInfo->PollEvents & AFD_POLL_ABORT) != 0 ) {

                WS_ASSERT( readfds != NULL );

                if ( !FD_ISSET( (SOCKET)pollHandleInfo->Handle, readfds ) ) {

                    FD_SET( (SOCKET)pollHandleInfo->Handle, readfds );
                    handlesReady++;

                }


                IF_DEBUG(SELECT) {

                    WS_PRINT(( "WSPSelect handle %lx aborted.\n",
                                   pollHandleInfo->Handle ));

                }

            }

            if ( (pollHandleInfo->PollEvents & AFD_POLL_LOCAL_CLOSE) != 0 ) {

                //
                // If the app does a closesocket() on a handle that has a
                // select() outstanding on it, this event may get set by
                // AFD even though we didn't request notification of it.
                // If exceptfds is NULL, then this is an error condition.
                //

                if ( readfds == NULL ) {

                    handlesReady = 0;
                    err = WSAENOTSOCK;
                    goto exit;

                } else {

                    if ( !FD_ISSET( (SOCKET)pollHandleInfo->Handle, readfds ) ) {

                        FD_SET( (SOCKET)pollHandleInfo->Handle, readfds );
                        handlesReady++;

                    }

                }

                IF_DEBUG(SELECT) {

                    WS_PRINT(( "WSPSelect handle %lx closed locally.\n",
                                   pollHandleInfo->Handle ));

                }

            }

            pollHandleInfo++;
        }
    }
    __except (SOCK_EXCEPTION_FILTER()) {
        err = WSAEFAULT;
        goto exit;
    }


exit:

    IF_DEBUG(SELECT) {

        if ( err != NO_ERROR ) {

            WS_PRINT(( "WSPSelect failed: %ld.\n", err ));

        } else {

            WS_PRINT(( "WSPSelect succeeded, %ld readfds, %ld writefds, "
                       "%ld exceptfds\n",
                           HANDLES_IN_SET( readfds ),
                           HANDLES_IN_SET( writefds ),
                           HANDLES_IN_SET( exceptfds ) ));

        }

    }

    if ( pollInfo != NULL && pollInfo != (PVOID)pollBuffer ) {

        FREE_HEAP( pollInfo );

    }

    if ( err != NO_ERROR ) {

        WS_EXIT( "WSPSelect", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    WS_ASSERT( (ULONG)handlesReady == (ULONG)(HANDLES_IN_SET( readfds ) +
                                              HANDLES_IN_SET( writefds ) +
                                              HANDLES_IN_SET( exceptfds )) );

    WS_EXIT( "WSPSelect", handlesReady, FALSE );
    return handlesReady;

}   // WSPSelect


int
WSPAPI
WSPAsyncSelect (
    SOCKET Handle,
    HWND hWnd,
    unsigned int wMsg,
    long lEvent,
    LPINT lpErrno
    )

/*++

Routine Description:

    This function is used to request that the Windows Sockets DLL should
    send a message to the window hWnd whenever it detects any of the
    network events specified by the lEvent parameter.  The message which
    should be sent is specified by the wMsg parameter.  The socket for
    which notification is required is identified by s.

    The lEvent parameter is constructed by or'ing any of the values
    specified in the following list:

         Value       Meaning

         FD_READ     Want to receive notification of readiness for reading.
         FD_WRITE    Want to receive notification of readiness for writing.
         FD_OOB      Want to receive notification of the arrival of
                     out-of-band data.
         FD_ACCEPT   Want to receive notification of incoming connections.
         FD_CONNECT  Want to receive notification of completed connection.
         FD_CLOSE    Want to receive notification of socket closure.

    Issuing a WSAAsyncSelect() for a socket cancels any previous
    WSAAsyncSelect() for the same socket.  For example, to receive
    notification for both reading and writing, the application must call
    WSAAsyncSelect() with both FD_READ and FD_WRITE, as follows:

        rc = WSAAsyncSelect(s, hWnd, wMsg, FD_READ | FD_WRITE);

    It is not possible to specify different messages for different
    events.  The following code will not work; the second call will
    cancel the effects of the first, and only FD_WRITE events will be
    reported with message wMsg2:

    rc = WSAAsyncSelect(s, hWnd, wMsg1, FD_READ);
    rc = WSAAsyncSelect(s, hWnd, wMsg2, FD_WRITE);

    To cancel all notification i.e., to indicate that the Windows
    Sockets implementation should send no further messages related to
    network events on the socket lEvent should be set to zero.

    rc = WSAAsyncSelect(s, hWnd, 0, 0);

    Although the WSAAsyncSelect() takes effect immediately, it is
    possible that messages may be waiting in the application's message
    queue.  The application must therefore be prepared to receive
    network event messages even after cancellation.

    This function automatically sets socket s to non-blocking mode.

    When one of the nominated network events occurs on the specified
    socket s, the application's window hWnd receives message wMsg.  The
    wParam argument identifies the socket on which a network event has
    occurred.  The low word of lParam specifies the network event that
    has occurred.  The high word of lParam contains any error code.  The
    error code be any error as defined in winsock.h.

    The error and event codes may be extracted from the lParam using the
    macros WSAGETSELECTERROR and WSAGETSELECTEVENT, defined in winsock.h
    as:

    #define WSAGETSELECTERROR(lParam) HIWORD(lParam)

    #define WSAGETSELECTEVENT(lParam) LOWORD(lParam)

    The use of these macros will maximize the portability of the source
    code for the application.

    The possible network event codes which may be returned are as
    follows:

         Value       Meaning

         FD_READ     Socket s ready for reading.
         FD_WRITE    Socket s ready for writing.
         FD_OOB      Out-of-band data ready for reading on socket s.
         FD_ACCEPT   Socket s ready for accepting a new incoming connection.
         FD_CONNECT  Connection on socket s completed.
         FD_CLOSE    Connection identified by socket s has been closed.

Arguments:

    s - A descriptor identifying the socket for which event notification
        is required.

    hWnd - A handle identifying the window which should receive a
        message when a network event occurs.

    wMsg - The message to be received when a network event occurs.

    lEvent - A bitmask which specifies a combination of network events
        in which the application is interested.

Return Value:

    The return value is 0 if the application's declaration of interest
    in the network event set was successful.  Otherwise the value
    SOCKET_ERROR is returned, and a specific error number may be
    retrieved by calling WSAGetLastError().

--*/

{

	PWINSOCK_TLS_DATA	tlsData;
    PSOCKET_INFORMATION socket;
    int err;
    PPOLL_CONTEXT_BLOCK context;
    NTSTATUS    status;
    BOOLEAN nonBlocking;


    WS_ENTER( "WSPAsyncSelect", (PVOID)Handle, (PVOID)hWnd, (PVOID)wMsg, (PVOID)lEvent );

    WS_ASSERT( lpErrno != NULL );

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPSelect", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    //
    // Initialize locals so that we know how to clean up on exit.
    //

    socket = NULL;
    context = NULL;

    //
    // Check that the hWnd passed in is a valid Windows handle.
    //

    if ( !IsWindow( hWnd ) ) {

        err = WSAEINVAL;
        goto errorexit;

    }

    //
    // Make sure that only valid bits are specified in lEvent.
    //
    // !!! should we also make sure that the bits make sense for the
    //     state of the socket, i.e. don't allow FD_ACCEPT on a
    //     connected socket?
    //

    if ( (lEvent & ~FD_ALL_EVENTS) != 0 ) {

        err = WSAEINVAL;
        goto errorexit;

    }

    //
    // Find a pointer to the socket structure corresponding to the
    // passed-in handle.
    //

    socket = SockFindAndReferenceSocket( Handle, TRUE );

    if ( socket == NULL ) {

        err = WSAENOTSOCK;
        goto errorexit;

    }


    //
    // Allocate space to hold the poll context block.
    //

    context = ALLOCATE_HEAP( sizeof(POLL_CONTEXT_BLOCK) );

    if ( context == NULL ) {

        err = WSAENOBUFS;
        goto errorexit;

    }

    //
    // Set the socket to nonblocking.
    //

    nonBlocking = TRUE;

    err = SockSetInformation(
                socket,
                AFD_NONBLOCKING_MODE,
                &nonBlocking,
                NULL,
                NULL
                );

    if ( err != NO_ERROR ) {

        goto errorexit;

    }

    socket->NonBlocking = TRUE;

    //
    // If there's a WSPEventSelect active on this socket, deactivate it.
    //

    if( socket->EventSelectlNetworkEvents ) {

        err = SockEventSelectHelper(
                  socket,
                  NULL,
                  0
                  );

        if( err != NO_ERROR ) {

            goto errorexit;

        }

    }

    //
    // Initialize the async thread if it hasn't already been started.
    //

    if ( !SockCheckAndReferenceAsyncThread( ) ) {
        err = WSAENOBUFS;
        goto errorexit;
    }

    if (!SockCheckAndInitAsyncSelectHelper ()) {
        SockDereferenceAsyncThread ();
        err = WSAENOBUFS;
        goto errorexit;

    }

    //
    // Acquire the lock that protects this socket.  We hold this lock
    // throughout this routine to synchronize against other callers
    // performing operations on the socket we're using.
    //

    SockAcquireSocketLockExclusive( socket );


    //
    // Store the specified window handle, message, and event mask in
    // the socket information structure.
    //

    socket->AsyncSelecthWnd = hWnd;
    socket->AsyncSelectwMsg = wMsg;
    socket->AsyncSelectlEvent = lEvent;
    socket->DisabledAsyncSelectEvents = 0;

    //
    // If the socket is not connected and is not a datagram socket,
    // disable FD_WRITE events.  We'll reenable them when the socket
    // becomes connected.  Disabling them here prevents a race condition
    // between FD_WRITE events and FD_CONNECT events, and we don't want
    // FD_WRITE events to be posted before FD_CONNECT.
    //

    if ( !SockIsSocketConnected( socket ) &&
             !IS_DGRAM_SOCK(socket) ) {

        socket->DisabledAsyncSelectEvents |= FD_WRITE;

    }

    //
    // Bump the async select serial number on this socket.  This field
    // allows us to stop sending a particular message as soon as
    // WSPAsyncSelect is called to change the settings on the socket.
    // When an AFD poll completes for an async select, if the serial
    // number has changed then no message is sent.
    //

    socket->AsyncSelectSerialNumber++;

    //
    // If there are no events that we can poll on, just return.  When a
    // reenabling function or WSPAsyncSelect is called a new request
    // will be queued to the async thread, and a new poll will be initiated.
    //

    if ( (socket->AsyncSelectlEvent &
                       (~socket->DisabledAsyncSelectEvents)) == 0 ) {

        SockReleaseSocketLock (socket);

        SockDereferenceAsyncThread ();
        SockDereferenceSocket( socket );
        FREE_HEAP (context);
        IF_DEBUG(ASYNC_SELECT) {

            WS_PRINT(( "WSPAsyncSelect: no events to poll on, "
                       "socket = %lx, lEvent = %lx, disabled = %lx\n",
                           socket, socket->AsyncSelectlEvent,
                           socket->DisabledAsyncSelectEvents ));

        }
        WS_EXIT( "WSPAsyncSelect", NO_ERROR, FALSE );
        return NO_ERROR;
    }

    //
    // Initialize unchanging part of the context
    // Store referenced pointer to socket info structure, so we can
    // safely access it when poll completes even if socket is closed.
    // This reference will be released when we free the context.
    //

    context->SocketInfo = socket;

    context->AsyncSelectSerialNumber = socket->AsyncSelectSerialNumber;
    SockReleaseSocketLock( socket );


	//
	// Start async select in the context of our thread.
	// Some of the applications tend to exit the thread in which async select
	// was started killing the poll IRP.  Because we are not that much concerned with
	// performance in async select, we'd rather take a hit here than trying to pick up
	// and repost the IRP in our own thread when it gets cancelled.
	//
    status = NtSetIoCompletion (SockAsyncQueuePort,
                        (PVOID)&SockProcessQueuedAsyncSelect,
                        context,
                        0,
                        0
                        );
    if (!NT_SUCCESS (status)) {
        SockDereferenceAsyncThread ();
        err = SockNtStatusToSocketError (status);
        goto errorexit;
    }


    IF_DEBUG(ASYNC_SELECT) {

        WS_PRINT(( "WSPAsyncSelect successfully posted request, "
                   "socket = %lx\n", socket ));

    }

    WS_EXIT( "WSPAsyncSelect", NO_ERROR, FALSE );
    return NO_ERROR;

errorexit:

    if ( socket != NULL ) {

        SockDereferenceSocket( socket );

    }


    if (context!=NULL) {
        FREE_HEAP (context);
    }

    IF_DEBUG(ASYNC_SELECT) {

        WS_PRINT(( "WSPAsyncSelect failed: %ld\n", err ));

    }

    WS_EXIT( "WSPAsyncSelect", SOCKET_ERROR, TRUE );
    *lpErrno = err;
    return SOCKET_ERROR;


} // WSPAsyncSelect



VOID
SockProcessAsyncSelect (
    PSOCKET_INFORMATION Socket,
    PPOLL_CONTEXT_BLOCK Context
    )
{

    ULONG eventsToPoll;
    NTSTATUS status;

    //
    // Initialize the context and poll buffer.  Use an infinite timeout
    // and just the one socket handle.  Also, this should be a unique
    // poll, so if another unique poll is outstanding on this socket
    // it will be canceleld.
    //

    Context->PollInfo.Timeout.HighPart = 0x7FFFFFFF;
    Context->PollInfo.Timeout.LowPart = 0xFFFFFFFF;
    Context->PollInfo.NumberOfHandles = 1;
    Context->PollInfo.Unique = TRUE;

    Context->PollInfo.Handles[0].Handle = Socket->HContext.Handle;
    Context->PollInfo.Handles[0].PollEvents = 0;

    //
    // Determine which events we want to poll.
    //

    eventsToPoll = Socket->AsyncSelectlEvent &
                       (~Socket->DisabledAsyncSelectEvents);

    WS_ASSERT (eventsToPoll!=0);

    //
    // Set up the events to poll on.  Note that we don't put down
    // AFD_POLL_CONNECT or AFD_POLL_CONNECT_FAIL requests even if
    // FD_CONNECT is specified-- this event is handled manually by the
    // connect() routine because of it's inherently different semantics.
    //

    if ( (eventsToPoll & FD_READ) != 0) {

        Context->PollInfo.Handles[0].PollEvents |= AFD_POLL_RECEIVE;

    }

    if ( (eventsToPoll & FD_WRITE) != 0) {

        Context->PollInfo.Handles[0].PollEvents |= AFD_POLL_SEND;

    }

    if ( (eventsToPoll & FD_OOB) != 0 ) {

        Context->PollInfo.Handles[0].PollEvents |= AFD_POLL_RECEIVE_EXPEDITED;

    }

    if ( (eventsToPoll & FD_ACCEPT) != 0 ) {

        Context->PollInfo.Handles[0].PollEvents |= AFD_POLL_ACCEPT;

    }

    if ( (eventsToPoll & FD_CLOSE) != 0) {

        Context->PollInfo.Handles[0].PollEvents |= AFD_POLL_DISCONNECT |
                                                   AFD_POLL_ABORT |
                                                   AFD_POLL_LOCAL_CLOSE;

    }

    if ( (eventsToPoll & FD_QOS) != 0) {

        Context->PollInfo.Handles[0].PollEvents |= AFD_POLL_QOS;

    }

    if ( (eventsToPoll & FD_GROUP_QOS) != 0) {

        Context->PollInfo.Handles[0].PollEvents |= AFD_POLL_GROUP_QOS;

    }

#ifdef _PNP_POWER_
    if ( (eventsToPoll & FD_ROUTING_INTERFACE_CHANGE) != 0 ) {

        Context->PollInfo.Handles[0].PollEvents |= AFD_POLL_ROUTING_IF_CHANGE;

    }

    if ( (eventsToPoll & FD_ADDRESS_LIST_CHANGE) != 0 ) {

        Context->PollInfo.Handles[0].PollEvents |= AFD_POLL_ADDRESS_LIST_CHANGE;

    }

#endif // _PNP_POWER_
    IF_DEBUG(ASYNC_SELECT) {

        WS_PRINT(( "SockProcessAsyncSelect: socket = %lx, handle = %lx, "
                   "lEvent = %lx, disabled = %lx, actual = %lx\n",
                       Socket, Socket->HContext.Handle, Socket->AsyncSelectlEvent,
                       Socket->DisabledAsyncSelectEvents, eventsToPoll ));

    }

    //
    // Start the actual poll.
    //

    WS_ASSERT( (IOCTL_AFD_POLL & 0x03) == METHOD_BUFFERED );

    status = NtDeviceIoControlFile(
                 SockAsyncSelectHelperHandle,
                 NULL,                      // Event
                 NULL,                      // APC
                 Context,
                 &Context->IoStatus,
                 IOCTL_AFD_POLL,
                 &Context->PollInfo,
                 sizeof(Context->PollInfo),
                 &Context->PollInfo,
                 sizeof(Context->PollInfo)
                 );

    if ( NT_ERROR(status) ) {

        //
        // If the request failed, call the completion routine since
        // the system didn't complete it.
        //

        Context->IoStatus.Status = status;

        SockAsyncSelectCompletion( Context, &Context->IoStatus);

    }

    return;

} // SockProcessAsyncSelect



VOID
SockProcessQueuedAsyncSelect (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock
    )
{
    PPOLL_CONTEXT_BLOCK context = ApcContext;
    PSOCKET_INFORMATION socket = context->SocketInfo;

    //
    // Acquire the socket's lock to keep it's state constant while
    // we do the message posts.
    //

    SockAcquireSocketLockExclusive( socket );

    //
    // If the socket has been closed, just return.
    //

    if ( socket->State == SocketStateClosing ) {

        IF_DEBUG(ASYNC_SELECT) {

            WS_PRINT(( "SockProcessQueuedAsyncSelect: tossing request on handle %lx"
                       "--closed\n",
                           socket->HContext.Handle));

        }


        SockReleaseSocketLock( socket );
        goto exit;

    }

    //
    // If the serial number on the socket has changed since this request
    // was initialized, or if the original socket was closed, just bag
    // this notification.  Another poll request should be on the way if the
    // socket is still open.
    //

    if ( context->AsyncSelectSerialNumber != socket->AsyncSelectSerialNumber ){

        IF_DEBUG(ASYNC_SELECT) {

            WS_PRINT(( "SockProcessQueuedAsyncSelect: tossing req on handle %lx, "
                       " ASN %ld req ASN %ld\n",
                           socket->HContext.Handle,
                           socket->AsyncSelectSerialNumber,
                           context->AsyncSelectSerialNumber ));

        }

        SockReleaseSocketLock( socket );
        goto exit;

    }
    //
    // If there are no events that we can poll on, just return.  When a
    // reenabling function or WSPAsyncSelect is called a new request
    // will be queued to the async thread, and a new poll will be initiated.
    //

    if ( (socket->AsyncSelectlEvent &
                       (~socket->DisabledAsyncSelectEvents)) == 0 ) {

        IF_DEBUG(ASYNC_SELECT) {

            WS_PRINT(( "SockProcessQueuedAsyncSelect: no events to poll on, "
                       "socket = %lx, lEvent = %lx, disabled = %lx\n",
                           socket, socket->AsyncSelectlEvent,
                           socket->DisabledAsyncSelectEvents ));

        }


        SockReleaseSocketLock( socket );
        goto exit;
    }

    //
    // Start another poll.
    //

    SockProcessAsyncSelect(
        socket,
        context
        );

    SockReleaseSocketLock( socket );

    return;
exit:

    SockDereferenceSocket( socket );
    FREE_HEAP( context );
    SockDereferenceAsyncThread ();
	return;
}



VOID
SockAsyncSelectCompletion (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock
    )
{

    PPOLL_CONTEXT_BLOCK context = ApcContext;
    PSOCKET_INFORMATION socket = context->SocketInfo;
    BOOL posted = FALSE;
    ULONG error;
    DWORD pollEvents;

    IF_DEBUG(ASYNC_SELECT) {

        WS_PRINT(( "AsyncSelectCompletionApc: socket %lx (h:%lx), status %lx, "
                   "poll events %lx\n" ,
                   context->SocketInfo,
                   socket->HContext.Handle,
                   IoStatusBlock->Status,
                   context->PollInfo.Handles[0].PollEvents ));

    }


    //
    // Acquire the socket's lock to keep it's state constant while
    // we do the message posts.
    //

    SockAcquireSocketLockExclusive( socket );

    //
    // If the socket has been closed, just return.
    //

    if ( IoStatusBlock->Status == STATUS_CANCELLED || socket->State == SocketStateClosing ) {

        IF_DEBUG(ASYNC_SELECT) {

            WS_PRINT(( "AsyncSelectCompletionApc: tossing request on handle %lx"
                       "--%s.\n",
                           socket->HContext.Handle,
						   (socket->State == SocketStateClosing) ? "closed" : "cancelled"));

        }


        SockReleaseSocketLock( socket );
        goto exit;

    }

    //
    // If the serial number on the socket has changed since this request
    // was initialized, or if the original socket was closed, just bag
    // this notification.  Another poll request should be on the way if the
    // socket is still open.
    //

    if ( context->AsyncSelectSerialNumber != socket->AsyncSelectSerialNumber ){

        IF_DEBUG(ASYNC_SELECT) {

            WS_PRINT(( "AsyncSelectCompletionApc: tossing req on handle %lx, "
                       " ASN %ld req ASN %ld\n",
                           socket->HContext.Handle,
                           socket->AsyncSelectSerialNumber,
                           context->AsyncSelectSerialNumber ));

        }

        SockReleaseSocketLock( socket );
        goto exit;

    }


    //
    // If the request failed, post a message indicating the failure.
    //

    if ( !NT_SUCCESS(IoStatusBlock->Status) ) {

        ULONG error = SockNtStatusToSocketError( IoStatusBlock->Status );

        posted = (SockUpcallTable->lpWPUPostMessage)(
                     socket->AsyncSelecthWnd,
                     socket->AsyncSelectwMsg,
                     (WPARAM)socket->Handle,
                     WSAMAKESELECTREPLY( 0, error )
                     );

        //WS_ASSERT( posted );

        IF_DEBUG(POST) {

            WS_PRINT(( "POSTED wMsg %lx hWnd %lx socket %lx event %s err %ld\n",
                           socket->AsyncSelectwMsg,
                           socket->AsyncSelecthWnd,
                           socket->Handle, "0", error ));

        }

        SockReleaseSocketLock( socket );
        goto exit;

    }

    //
    // We should never get AFD_POLL_CONNECT or AFD_POLL_CONNECT_FAIL
    // events, since we never ask for them.  FD_CONNECT is handled
    // manually by the connect() routine.
    //

    pollEvents = context->PollInfo.Handles[0].PollEvents;
    WS_ASSERT( (pollEvents & AFD_POLL_CONNECT) == 0 );
    WS_ASSERT( (pollEvents & AFD_POLL_CONNECT_FAIL) == 0 );

    //
    // Post messages based on the event(s) that occurred.
    //

    if ( IS_EVENT_ENABLED( FD_READ, socket ) &&
             (pollEvents & AFD_POLL_RECEIVE) != 0 ) {

        posted = (SockUpcallTable->lpWPUPostMessage)(
                     socket->AsyncSelecthWnd,
                     socket->AsyncSelectwMsg,
                     (WPARAM)socket->Handle,
                     WSAMAKESELECTREPLY( FD_READ, 0 )
                     );

        //WS_ASSERT( posted );

        IF_DEBUG(POST) {

            WS_PRINT(( "POSTED wMsg %lx hWnd %lx socket %lx event %s err %ld\n",
                           socket->AsyncSelectwMsg,
                           socket->AsyncSelecthWnd,
                           socket->Handle, "FD_READ", 0 ));

        }

        //
        // Disable this event.  It will be reenabled when the app does a
        // recv().
        //

        socket->DisabledAsyncSelectEvents |= FD_READ;

    }

    if ( IS_EVENT_ENABLED( FD_OOB, socket ) &&
             (pollEvents & AFD_POLL_RECEIVE_EXPEDITED) != 0 ) {

        posted = (SockUpcallTable->lpWPUPostMessage)(
                     socket->AsyncSelecthWnd,
                     socket->AsyncSelectwMsg,
                     (WPARAM)socket->Handle,
                     WSAMAKESELECTREPLY( FD_OOB, 0 )
                     );

        //WS_ASSERT( posted );

        IF_DEBUG(POST) {

            WS_PRINT(( "POSTED wMsg %lx hWnd %lx socket %lx event %s err %ld\n",
                           socket->AsyncSelectwMsg,
                           socket->AsyncSelecthWnd,
                           socket->Handle, "FD_OOB", 0 ));

        }

        //
        // Disable this event.  It will be reenabled when the app does a
        // recv( MSG_OOB ).
        //
        // !!! need synchronization?
        //

        socket->DisabledAsyncSelectEvents |= FD_OOB;

    }

    if ( IS_EVENT_ENABLED( FD_WRITE, socket ) &&
             (pollEvents & AFD_POLL_SEND) != 0 ) {

        posted = (SockUpcallTable->lpWPUPostMessage)(
                     socket->AsyncSelecthWnd,
                     socket->AsyncSelectwMsg,
                     (WPARAM)socket->Handle,
                     WSAMAKESELECTREPLY( FD_WRITE, 0 )
                     );

        //WS_ASSERT( posted );

        IF_DEBUG(POST) {

            WS_PRINT(( "POSTED wMsg %lx hWnd %lx socket %lx event %s err %ld\n",
                           socket->AsyncSelectwMsg,
                           socket->AsyncSelecthWnd,
                           socket->Handle, "FD_WRITE", 0 ));

        }

        //
        // Disable this event.  It will be reenabled when the app does a
        // send().
        //

        socket->DisabledAsyncSelectEvents |= FD_WRITE;

    }

    if ( IS_EVENT_ENABLED( FD_ACCEPT, socket ) &&
             (pollEvents & AFD_POLL_ACCEPT) != 0 ) {

        posted = (SockUpcallTable->lpWPUPostMessage)(
                     socket->AsyncSelecthWnd,
                     socket->AsyncSelectwMsg,
                     (WPARAM)socket->Handle,
                     WSAMAKESELECTREPLY( FD_ACCEPT, 0 )
                     );

        //WS_ASSERT( posted );

        IF_DEBUG(POST) {

            WS_PRINT(( "POSTED wMsg %lx hWnd %lx socket %lx event %s err %ld\n",
                           socket->AsyncSelectwMsg,
                           socket->AsyncSelecthWnd,
                           socket->Handle, "FD_ACCEPT", 0 ));

        }

        //
        // Disable this event.  It will be reenabled when the app does an
        // accept().
        //

        socket->DisabledAsyncSelectEvents |= FD_ACCEPT;

    }

    if ( IS_EVENT_ENABLED( FD_CLOSE, socket ) &&
         ((pollEvents & AFD_POLL_DISCONNECT) != 0 ||
          (pollEvents & AFD_POLL_ABORT) != 0 ||
          (pollEvents & AFD_POLL_LOCAL_CLOSE) != 0) ) {

        if ( (pollEvents & AFD_POLL_ABORT) != 0 ) {

            error = WSAECONNABORTED;

        } else {

            error = NO_ERROR;

        }

        posted = (SockUpcallTable->lpWPUPostMessage)(
                     socket->AsyncSelecthWnd,
                     socket->AsyncSelectwMsg,
                     (WPARAM)socket->Handle,
                     WSAMAKESELECTREPLY( FD_CLOSE, error )
                     );

        //WS_ASSERT( posted );

        IF_DEBUG(POST) {

            WS_PRINT(( "POSTED wMsg %lx hWnd %lx socket %lx event %s err %ld\n",
                           socket->AsyncSelectwMsg,
                           socket->AsyncSelecthWnd,
                           socket->Handle, "FD_CLOSE", 0 ));

        }

        //
        // Disable this event.  It will never be reenabled.
        //

        socket->DisabledAsyncSelectEvents |= FD_CLOSE;

    }

    if ( IS_EVENT_ENABLED( FD_QOS, socket ) &&
             (pollEvents & AFD_POLL_QOS) != 0 ) {

        posted = (SockUpcallTable->lpWPUPostMessage)(
                     socket->AsyncSelecthWnd,
                     socket->AsyncSelectwMsg,
                     (WPARAM)socket->Handle,
                     WSAMAKESELECTREPLY( FD_QOS, 0 )
                     );

        //WS_ASSERT( posted );

        IF_DEBUG(POST) {

            WS_PRINT(( "POSTED wMsg %lx hWnd %lx socket %lx event %s err %ld\n",
                           socket->AsyncSelectwMsg,
                           socket->AsyncSelecthWnd,
                           socket->Handle, "FD_QOS", 0 ));

        }

        //
        // Disable this event.  It will be reenabled when the app does an
        // WSAIoctl (SIO_GET_QOS).
        //

        socket->DisabledAsyncSelectEvents |= FD_QOS;

    }


    if ( IS_EVENT_ENABLED( FD_GROUP_QOS, socket ) &&
             (pollEvents & AFD_POLL_GROUP_QOS) != 0 ) {

        posted = (SockUpcallTable->lpWPUPostMessage)(
                     socket->AsyncSelecthWnd,
                     socket->AsyncSelectwMsg,
                     (WPARAM)socket->Handle,
                     WSAMAKESELECTREPLY( FD_GROUP_QOS, 0 )
                     );

        //WS_ASSERT( posted );

        IF_DEBUG(POST) {

            WS_PRINT(( "POSTED wMsg %lx hWnd %lx socket %lx event %s err %ld\n",
                           socket->AsyncSelectwMsg,
                           socket->AsyncSelecthWnd,
                           socket->Handle, "FD_GROUP_QOS", 0 ));

        }

        //
        // Disable this event.  It will be reenabled when the app does an
        // WSAIoctl (SIO_GET_GROUP_QOS).
        //

        socket->DisabledAsyncSelectEvents |= FD_GROUP_QOS;

    }

#ifdef _PNP_POWER_
    if ( IS_EVENT_ENABLED( FD_ROUTING_INTERFACE_CHANGE, socket ) &&
             (pollEvents & AFD_POLL_ROUTING_IF_CHANGE) != 0 ) {

        posted = (SockUpcallTable->lpWPUPostMessage)(
                     socket->AsyncSelecthWnd,
                     socket->AsyncSelectwMsg,
                     (WPARAM)socket->Handle,
                     WSAMAKESELECTREPLY( FD_ROUTING_INTERFACE_CHANGE, 0 )
                     );

        //WS_ASSERT( posted );

        IF_DEBUG(POST) {

            WS_PRINT(( "POSTED wMsg %lx hWnd %lx socket %lx event %s err %ld\n",
                           socket->AsyncSelectwMsg,
                           socket->AsyncSelecthWnd,
                           socket->Handle, "FD_ROUTING_INTERFACE_CHANGE", 0 ));

        }

        //
        // Don't need to disable this event.  It will not occur until application does
        // WSAIoctl (SIO_ROUTING_INTERFACE_CHANGE).
        //

        // socket->DisabledAsyncSelectEvents |= FD_ROUTING_INTERFACE_CHANGE;

    }

    if ( IS_EVENT_ENABLED( FD_ADDRESS_LIST_CHANGE, socket ) &&
             (pollEvents & AFD_POLL_ADDRESS_LIST_CHANGE) != 0 ) {

        posted = (SockUpcallTable->lpWPUPostMessage)(
                     socket->AsyncSelecthWnd,
                     socket->AsyncSelectwMsg,
                     (WPARAM)socket->Handle,
                     WSAMAKESELECTREPLY( FD_ADDRESS_LIST_CHANGE, 0 )
                     );

        //WS_ASSERT( posted );

        IF_DEBUG(POST) {

            WS_PRINT(( "POSTED wMsg %lx hWnd %lx socket %lx event %s err %ld\n",
                           socket->AsyncSelectwMsg,
                           socket->AsyncSelecthWnd,
                           socket->Handle, "FD_ADDRESS_LIST_CHANGE", 0 ));

        }

        //
        // Don't need to disable this event.  It will not occur until application does
        // WSAIoctl (SIO_ADDRESS_LIST_CHANGE).
        //

        // socket->DisabledAsyncSelectEvents |= FD_ADDRESS_LIST_CHANGE;

    }
#endif // _PNP_POWER_


    //
    // If there are no events that we can poll on, just return.  When a
    // reenabling function or WSPAsyncSelect is called a new request
    // will be queued to the async thread, and a new poll will be initiated.
    //

    if ( (socket->AsyncSelectlEvent &
                       (~socket->DisabledAsyncSelectEvents)) == 0 ) {

        IF_DEBUG(ASYNC_SELECT) {

            WS_PRINT(( "SockAsyncSelectCompletion: no events to poll on, "
                       "socket = %lx, lEvent = %lx, disabled = %lx\n",
                           socket, socket->AsyncSelectlEvent,
                           socket->DisabledAsyncSelectEvents ));

        }


        SockReleaseSocketLock( socket );
        goto exit;
    }

    //
    // Start another poll.
    //

    SockProcessAsyncSelect(
        socket,
        context
        );

    SockReleaseSocketLock( socket );

    return;
exit:

    SockDereferenceSocket( socket );
    FREE_HEAP( context );
    SockDereferenceAsyncThread ();
	return;
} // AsyncSelectCompletionApc


VOID
SockReenableAsyncSelectEvent (
    IN PSOCKET_INFORMATION Socket,
    IN ULONG Event
    )
{
    PPOLL_CONTEXT_BLOCK context;
    NTSTATUS    status;
    //
    // Note: caller must have acquired socket lock prior to calling this func
    //

    //
    // Check whether the specified event is in the list of disabled
    // async select events for the socket.  If it isn't, we don't
    // need to do anything.
    //

    if ( (Socket->DisabledAsyncSelectEvents & Event) == 0 ) {

        return;

    }

    //
    // If the socket is closing, don't reenable the select event.
    //

    if ( Socket->State == SocketStateClosing ) {

        return;
    }

    IF_DEBUG(ASYNC_SELECT) {

        WS_PRINT(( "SockReenableSelectEvent: reenabling event %lx for socket "
                   "%lx (%lx)\n", Event, Socket->HContext.Handle, Socket ));
    }

    //
    // The specified event is currently disabled.
    //
    // *** Note that it is assumed that the socket lock is held on entry
    //     to this routine!
    //

    //
    // Reset the bit for this event so that it is no longer disabled.
    //

    Socket->DisabledAsyncSelectEvents &= ~Event;

    //
    // If there are no events that we can poll on, just return.  When a
    // reenabling function or WSPAsyncSelect is called a new request
    // will be queued to the async thread, and a new poll will be initiated.
    //

    if ( (Socket->AsyncSelectlEvent &
                       (~Socket->DisabledAsyncSelectEvents)) == 0 ) {

        IF_DEBUG(ASYNC_SELECT) {

            WS_PRINT(( "SockReenableAsyncSelectEvent: no events to poll on, "
                       "socket = %lx, lEvent = %lx, disabled = %lx\n",
                           Socket, Socket->AsyncSelectlEvent,
                           Socket->DisabledAsyncSelectEvents ));

        }

        return;
    }

    //
    // Allocate space to hold the poll context block.
    //

    context = ALLOCATE_HEAP( sizeof(POLL_CONTEXT_BLOCK) );

    if ( context == NULL ) {
        //
        // BUGBUG Need to report error.
        //

        return ;

    }


    if (!SockCheckAndReferenceAsyncThread ()) {
        //
        // BUGBUG Need to report error.
        //
        FREE_HEAP (context);
        return;
    }

    //
    // Initialize unchanging part of the context
    // Store referenced pointer to socket info structure, so we can
    // safely access it when poll completes even if socket is closed.
    // This reference will be released when we free the context.
    //

    SockReferenceSocket(Socket);

    context->SocketInfo = Socket;

    //
    // We need to bump the number, so that if there is outstaning async
    // select on the socket it will go away
    //

    Socket->AsyncSelectSerialNumber += 1;
    context->AsyncSelectSerialNumber = Socket->AsyncSelectSerialNumber;



	//
	// Start async select in the context of our thread.
	// Some of the applications tend to exit the thread in which async select
	// was started killing the poll IRP.  Because we are not that much concerned with
	// performance in async select, we'd rather take a hit here than trying to pick up
	// and repost the IRP in our own thread when it gets cancelled
	//
    status = NtSetIoCompletion (SockAsyncQueuePort,
                        (PVOID)&SockProcessQueuedAsyncSelect,
                        context,
                        0,
                        0
                        );
    if (!NT_SUCCESS (status)) {
        //
        // BUGBUG Need to report error.
        //
        FREE_HEAP (context);
    }

    return;

} // SockReenableAsyncSelectEvent

BOOL
SockCheckAndInitAsyncSelectHelper (
    VOID
    ) {
    HANDLE  Handle;
    UNICODE_STRING afdName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    FILE_COMPLETION_INFORMATION completionInfo;
    OBJECT_HANDLE_FLAG_INFORMATION HandleInfo;
    NTSTATUS status;

    //
    // If the async select helper handle is already initialized, return.
    //

    if (SockAsyncSelectHelperHandle!=NULL)
        return TRUE;

    //
    // Acquire the global lock to synchronize the creation of the handle.
    //

	SockAcquireRwLockExclusive (&SocketGlobalLock);

    // Check again, in case another thread has already initialized
    // the async helper handle.
    //

    if (SockAsyncSelectHelperHandle!=NULL) {
    	SockReleaseRwLockExclusive (&SocketGlobalLock);
        return TRUE;
    }

    //
    // Set up to open a handle to AFD.
    //

    RtlInitUnicodeString( &afdName, L"\\Device\\Afd\\AsyncSelectHlp" );

    InitializeObjectAttributes(
        &objectAttributes,
        &afdName,
        OBJ_INHERIT | OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Open a handle to AFD.
    //

    status = NtCreateFile(
                 (PHANDLE)&Handle,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &objectAttributes,
                 &ioStatusBlock,
                 NULL,                                     // AllocationSize
                 0L,                                       // FileAttributes
                 FILE_SHARE_READ | FILE_SHARE_WRITE,       // ShareAccess
                 FILE_OPEN_IF,                             // CreateDisposition
                 0,                                        // CreateOptions
                 NULL,                                     // EaBuffer
                 0                                         // EaBufferLength
                 );

    if ( !NT_SUCCESS(status) ) {
        WS_PRINT(( "SockCheckAndInitAsyncSelectHelper: NtCreateFile(helper) failed: %X\n",
                       status ));
        SockReleaseRwLockExclusive (&SocketGlobalLock);
        return FALSE;
    }


    completionInfo.Port = SockAsyncQueuePort;
    completionInfo.Key = SockAsyncSelectCompletion;
    status = NtSetInformationFile (
                Handle,
                &ioStatusBlock,
                &completionInfo,
                sizeof (completionInfo),
                FileCompletionInformation);
    if (!NT_SUCCESS (status)) {
        WS_PRINT(( "SockCheckAndInitAsyncSelectHelper: NtSeInformationFile(completion) failed: %X\n",
                       status ));

        NtClose (Handle);
	    SockReleaseRwLockExclusive (&SocketGlobalLock);
        return FALSE;
    }

    //
    // Protect our handle from being closed by random NtClose call
    // by some messed up dll or application
    //
    HandleInfo.ProtectFromClose = TRUE;
    HandleInfo.Inherit = FALSE;

    status = NtSetInformationObject( Handle,
                    ObjectHandleFlagInformation,
                    &HandleInfo,
                    sizeof( HandleInfo )
                  );
    //
    // This shouldn't fail
    //
    WS_ASSERT (NT_SUCCESS (status));

    SockAsyncSelectHelperHandle = Handle;

    //
    // Remember that WSAAsyncSelect has been called in this process.
    // We'll use this information in the send and receive routines to
    // know if and when to attempt to reenable async select events.
    //

    SockAsyncSelectCalled = TRUE;

	SockReleaseRwLockExclusive (&SocketGlobalLock);
    return TRUE;
}



int PASCAL FAR
__WSAFDIsSet (
    SOCKET fd,
    fd_set FAR *set
    )

/*++

Routine Description:

    This routine is used by the FD_ISSET macro; applications should
    not call it directly.  It determines whether a socket handle is
    included in an fd_set structure.

Arguments:

    fd - The socket handle to look for.

    set - The fd_set structure to examine.

Return Value:

    TRUE if the socket handle is in the set, FALSE if it is not.

--*/

{

    int i = (set->fd_count & 0xFFFF);

    while (i--) {

        if (set->fd_array[i] == fd) {

            return 1;

        }

    }

    return 0;

}   // __WSAFDIsSet
