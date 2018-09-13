
    PSOCKET_INFORMATION socketInfo;
    INT err;
    INT result;

    SOCK_ENTER( "WSPGeneric", (PVOID)s, ...

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPGeneric", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Attempt to find the socket in our lookup table.
    //

    socketInfo = SockFindAndReferenceWS2Socket( s );

    if( socketInfo == NULL || socketInfo->State == SocketStateClosing ) {

        IF_DEBUG(...) {

            SOCK_PRINT((
                "WSPGeneric failed on %s handle: %lx\n",
                socketInfo == NULL ? "unknown" : "closed",
                s
                ));

        }

        if( socketInfo != NULL ) {

            SockDereferenceSocket( socketInfo );

        }

        *lpErrno = WSAENOTSOCK;
        SOCK_EXIT( "WSPGeneric", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Let the hooker do its thang.
    //

    SockPrepareForBlockingHook( socketInfo );
    SockPreApiCallout();

    result = socketInfo->Hooker->
                 socketInfo->WS1Handle,
                 ...
                 );

    if( result == SOCKET_ERROR ) {

        err = socketInfo->Hooker->WSAGetLastError();
        SOCK_ASSERT( err != NO_ERROR );
        SockPostApiCallout();
        goto exit;

    }

    SockPostApiCallout();

    //
    // Success!
    //

    SOCK_ASSERT( err == NO_ERROR );
    SOCK_ASSERT( result != SOCKET_ERROR );

exit:

    if( err != NO_ERROR ) {

        *lpErrno = err;
        result = SOCKET_ERROR;

    }

    SockDereferenceSocket( socketInfo );

    SOCK_EXIT( "WSPGeneric", result, (result == SOCKET_ERROR) ? lpErrno : NULL );
    return result;


