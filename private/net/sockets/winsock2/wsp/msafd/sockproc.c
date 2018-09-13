/*++

Copyright (c) 1992-1996 Microsoft Corporation

Module Name:

    sockproc.c

Abstract:

    This module contains support routines for the WinSock DLL.

Author:

    David Treadwell (davidtr)    20-Feb-1992

Revision History:

--*/

#include "winsockp.h"
#if DBG
	#include <ctype.h>
	#include <stdarg.h>
	#include <wincon.h>
#endif

PSOCKET_INFORMATION
SockpImportHandle (
    IN SOCKET Handle
    );

//
// The (PCHAR) casts in the following macro force the compiler to assume
// only BYTE alignment.
//

#define SockCopyMemory(d,s,l) RtlCopyMemory( (PCHAR)(d), (PCHAR)(s), (l) )


INT
SockBuildSockaddr (
    OUT PSOCKADDR Sockaddr,
    OUT PINT SockaddrLength,
    IN PTRANSPORT_ADDRESS TdiAddress
    )
{
    __try {
        WS_ASSERT( sizeof(TdiAddress->Address[0].AddressType) ==
                    sizeof(Sockaddr->sa_family) );
        WS_ASSERT( FIELD_OFFSET( TA_ADDRESS, AddressLength ) == 0 );
        WS_ASSERT( FIELD_OFFSET( TA_ADDRESS, AddressType ) == sizeof(USHORT) );
        WS_ASSERT( FIELD_OFFSET( TRANSPORT_ADDRESS, Address[0] ) == sizeof(int) );
        WS_ASSERT( FIELD_OFFSET( SOCKADDR, sa_family ) == 0 );

        //
        // Convert the specified TDI address to a sockaddr.
        //

        *SockaddrLength = TdiAddress->Address[0].AddressLength +
                              sizeof(Sockaddr->sa_family);

        RtlCopyMemory(
            Sockaddr,
            &TdiAddress->Address[0].AddressType,
            *SockaddrLength
            );

        return NO_ERROR;
    }
    __except (SOCK_EXCEPTION_FILTER()) {
        return WSAEFAULT;
    }

} // SockBuildSockaddr


INT
SockBuildTdiAddress (
    OUT PTRANSPORT_ADDRESS TdiAddress,
    IN PSOCKADDR Sockaddr,
    IN INT SockaddrLength
    )
{
    __try {
        WS_ASSERT( sizeof(TdiAddress->Address[0].AddressType) ==
                    sizeof(Sockaddr->sa_family) );
        WS_ASSERT( FIELD_OFFSET( TA_ADDRESS, AddressLength ) == 0 );
        WS_ASSERT( FIELD_OFFSET( TA_ADDRESS, AddressType ) == sizeof(USHORT) );
        WS_ASSERT( FIELD_OFFSET( TRANSPORT_ADDRESS, Address[0] ) == sizeof(int) );
        WS_ASSERT( FIELD_OFFSET( SOCKADDR, sa_family ) == 0 );

        //
        // Convert the specified sockaddr to a TDI address.
        //

        TdiAddress->TAAddressCount = 1;
        TdiAddress->Address[0].AddressLength =
            (USHORT)(SockaddrLength - sizeof(Sockaddr->sa_family)) ;

        RtlCopyMemory(
            &TdiAddress->Address[0].AddressType,
            Sockaddr,
            SockaddrLength
            );

        return NO_ERROR;
    }
    __except (SOCK_EXCEPTION_FILTER()) {
        return WSAEFAULT;
    }

} // SockBuildTdiAddress


VOID
SockDestroySocket (
    IN PSOCKET_INFORMATION Socket
    )

/*++

Routine Description:

    Destroys the socket structure and associated resources.
	This is done when last reference to thes socket is released

Arguments:

    Socket - a pointer to the socket to destroy.

Return Value:

    None.

--*/

{
    WS_ASSERT( Socket->HContext.RefCount == 0 );
    WS_ASSERT( Socket->HelperDll!=NULL);
    SockDereferenceHelper (Socket->HelperDll);
#if DBG
    {
        LPWSHANDLE_CONTEXT  ctx = WahReferenceContextByHandle (
                                        SockContextTable,
                                        Socket->HContext.Handle
                                        );
        if (ctx!=NULL) {
            PSOCKET_INFORMATION aSocket = CONTAINING_RECORD (
                                                ctx,
                                                SOCKET_INFORMATION,
                                                HContext
                                                );
            WS_ASSERT (aSocket!=Socket);
            SockDereferenceSocket (aSocket);
        }
    }
#endif

#ifdef _AFD_SAN_SWITCH_
	{
		BOOLEAN deleteCS = FALSE;
		PSOCK_SAN_INFORMATION switchSocket = Socket->SanSocket;
		
		if (switchSocket && (switchSocket->RefSocket == Socket)) {
#if DBG
			{
				PLIST_ENTRY listEntry;
				BOOL found;
				DWORD outOfOrderRecvs = 0;

				EnterCriticalSection(&SockSanListCritSec);

				if (switchSocket->ReceivedOutOfOrderQueue.Flink) {
					for (listEntry = switchSocket->ReceivedOutOfOrderQueue.Flink;
						 listEntry != &switchSocket->ReceivedOutOfOrderQueue;
						 listEntry = listEntry->Flink, outOfOrderRecvs++);
					WS_ASSERT(switchSocket->CloseCount == 2);
				}
				else
					WS_ASSERT(switchSocket->CloseCount == 1);

		
				//
				// We don't decrement NumberReceivesPosted upon out-of-order
				// recv, so take that into account
				//
				WS_ASSERT(switchSocket->NumberReceivesPosted == outOfOrderRecvs);

				WS_ASSERT(switchSocket->SanSocket == INVALID_SOCKET);
				WS_ASSERT(switchSocket->SanContext == 0xf321f321);
		
				//
				// Verify this socket is on SockSanClosingList
				//
				listEntry = SockSanClosingList.Flink;
				found = FALSE;
				while ( listEntry != &SockSanClosingList && !found ) {
					if ( listEntry == &switchSocket->SockListEntry) {
						found = TRUE;
					}
					
					listEntry = listEntry->Flink;
				}
				
				if (!found) {
					WS_PRINT(("SwSock=%p not in SockSanClosingList\n",
							  switchSocket));
					WS_ASSERT(FALSE);
				}
				LeaveCriticalSection(&SockSanListCritSec);
			}
#endif

			SockSanCleanupFlowControl( switchSocket );

			if (switchSocket->NormalDataRegBuf) {
				FREE_HEAP (switchSocket->NormalDataRegBuf);
			}
			SockSanDeallocateFCMemory(switchSocket);

			//
			// Synchronize deletion of tracker with CheckForHungLargeSends. Must
			// do before removing socket from SockSanOpen/ClosingList
			//
			if (switchSocket->Tracker &&
				InterlockedIncrement(&switchSocket->Tracker->TrackerDeletionCount) == 2) {
				FREE_HEAP(switchSocket->Tracker);
			}

			//
			// Remove from SockSanClosingList. Even if mltiple listening
			// sockets, only one is on the SockSanClosingList
			//
			EnterCriticalSection(&SockSanListCritSec);
			RemoveEntryList(&switchSocket->SockListEntry);
			//
			// Delete SockSanListCritSec if this is last SAN socket
			// and WSPCleanup has already been called
			//
			if (IsListEmpty(&SockSanOpenList) &&
				IsListEmpty(&SockSanClosingList) &&
				SockSanDeleteListCritSec) {
				deleteCS = TRUE;
			}
			LeaveCriticalSection(&SockSanListCritSec);
						
			if (deleteCS) {
				DeleteCriticalSection(&SockSanListCritSec);
			}
			
			//
			// Handle case of listening socket with multiple providers.
			// Delete each one. Note: listening sockets do not have
			// flow control initialized, so there's no need to call
			// any of the other cleanup calls above.
			//
			while (switchSocket) {
				PSOCK_SAN_INFORMATION next = switchSocket->Next;

				SockSanDereferenceProvider (switchSocket->SanProvider);
	
				SockSanCleanupListeningContext(switchSocket);
				DeleteCriticalSection(&switchSocket->CritSec);
				FREE_HEAP (switchSocket);
				switchSocket = next;
			}
		}
	}
#endif //_AFD_SAN_SWITCH_
	

    DeleteCriticalSection( &Socket->Lock );
    FREE_HEAP( Socket );
} // SockDestroySocket


PSOCKET_INFORMATION
SockFindAndReferenceSocket (
    IN SOCKET Handle,
    IN BOOLEAN AttemptImport
    )

/*++

Routine Description:

    Looks up a socket in the global socket table, and references
    it if found.

Arguments:

    Handle - NT system handle of the socket to locate.

    AttemptImport - if the socket isn't currently valid in this
        process, this parameter specifies whether we should attempt
        to import the handle into this process.

Return Value:

    PSOCKET_INFORMATION - a referenced pointer to a socket structure,
        or NULL if none was found that matched the specified handle.

--*/

{
    LPWSHANDLE_CONTEXT  ctx;
    ctx = WahReferenceContextByHandle (SockContextTable, (HANDLE)Handle);
    if (ctx) {
        return CONTAINING_RECORD (ctx, SOCKET_INFORMATION, HContext);
    }
    else if (AttemptImport)
        return SockpImportHandle (Handle);
    else
        return NULL;

} // SockFindAndReferenceSocket


PSOCKET_INFORMATION
SockpImportHandle (
    IN SOCKET Handle
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG contextLength;
    PVOID context;
    PSOCKET_INFORMATION newSocket;
    UNICODE_STRING transportDeviceName;
    BOOLEAN succeeded;
    ULONG helperDllContextLength;
    PCHAR contextPtr;
    INT error;
    BOOLEAN resourceInitialized;
    PVOID helperDllContext;
    PWINSOCK_HELPER_DLL_INFO helperDll;
    DWORD helperDllNotificationEvents;
    ULONG newSocketLength;
    INT addressFamily;
    INT socketType;
    INT protocol;
    UCHAR contextBuffer[MAX_FAST_HANDLE_CONTEXT];
    LPWSHANDLE_CONTEXT  ctx;
	PWINSOCK_TLS_DATA	tlsData;

    tlsData = GET_THREAD_DATA ();


    //
    // Take the import lock, so no other thread can do the
    // same operation while we are at it.
    //

	SockAcquireRwLockExclusive (&SocketGlobalLock);

    //
    // Recheck under the lock if other thread did this for us
    //
    ctx = WahReferenceContextByHandle (SockContextTable, (HANDLE)Handle);
    if (ctx) {
	    SockReleaseRwLockExclusive (&SocketGlobalLock);

        return CONTAINING_RECORD (ctx, SOCKET_INFORMATION, HContext);
    }

    //
    // Initialize locals so that we know how to clean up on exit.
    //

	helperDll = NULL;
    context = NULL;
    newSocket = NULL;
    succeeded = FALSE;
    resourceInitialized = FALSE;

    RtlInitUnicodeString( &transportDeviceName, NULL );

    //
    // Call AFD to determine the length of context info for the socket.
    // If this succeeds, then it is most likely true that the handle
    // is valid for this process.
    //

    status = NtDeviceIoControlFile(
                 (HANDLE)Handle,
                 tlsData->EventHandle,
                 NULL,                   // APC Routine
                 NULL,                   // APC Context
                 &ioStatusBlock,
                 IOCTL_AFD_GET_CONTEXT_LENGTH,
                 NULL,
                 0,
                 &contextLength,
                 sizeof(contextLength)
                 );

    if ( status == STATUS_PENDING ) {
        SockWaitForSingleObject(
            tlsData->EventHandle,
            Handle,
            SOCK_NEVER_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS(status) || contextLength < sizeof(SOCK_SHARED_INFO) ) {
        goto exit;
    }

    //
    // Now allocate memory to hold the socket context and get the actual
    // context for the socket.
    //

    if( contextLength <= sizeof(contextBuffer) ) {
        context = contextBuffer;
    } else {
        context = ALLOCATE_HEAP( contextLength );
        if ( context == NULL ) {
            goto exit;
        }
    }

    status = NtDeviceIoControlFile(
                 (HANDLE)Handle,
                 tlsData->EventHandle,
                 NULL,                   // APC Routine
                 NULL,                   // APC Context
                 &ioStatusBlock,
                 IOCTL_AFD_GET_CONTEXT,
                 NULL,
                 0,
                 context,
                 contextLength
                 );

    if ( status == STATUS_PENDING ) {
        SockWaitForSingleObject(
            tlsData->EventHandle,
            Handle,
            SOCK_NEVER_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS(status) ) {
        goto exit;
    }

    //
    // We have obtained the necessary context for the socket.  The context
    // information is structured as follows:
    //
    //     SOCKET_SHARED_INFO structure
    //     Helper DLL Context Length
    //     Local Address
    //     Remote Address
    //     Helper DLL Context
    //

    //
    // Grab some parameters from the context structure.
    //

    addressFamily = ((PSOCK_SHARED_INFO)context)->AddressFamily;
    socketType = ((PSOCK_SHARED_INFO)context)->SocketType;
    protocol = ((PSOCK_SHARED_INFO)context)->Protocol;

    //
    // Get the helper DLL for the socket loaded.
    //

    error = SockGetTdiName(
                &addressFamily,
                &socketType,
                &protocol,
                0,
                0,
                &transportDeviceName,
                &helperDllContext,
                &helperDll,
                &helperDllNotificationEvents
                );
    if ( error != NO_ERROR ) {
        goto exit;
    }

    //
    // Allocate a socket information structure for this socket.
    //

    newSocketLength = (ULONG)(ALIGN_8(sizeof(*newSocket)) +
                      (ALIGN_8(helperDll->MaxSockaddrLength) * 2));

    newSocket = ALLOCATE_HEAP( newSocketLength );
    if ( newSocket == NULL ) {
        goto exit;
    }

    //
    // Copy in to the new socket information structure the initial context.
    //

    RtlCopyMemory( &newSocket->SharedInfo, context, sizeof(SOCK_SHARED_INFO) );

    IF_DEBUG(SOCKET) {
        WS_PRINT(( "Retreived socket info from AFD, handle %lx of type %s\n",
                       newSocket->Handle,
                       ( newSocket->SocketType == SOCK_DGRAM ?
                             "SOCK_DGRAM" :
                             (newSocket->SocketType == SOCK_STREAM ?
                                 "SOCK_STREAM" : "SOCK_RAW")) ));
    }
    //
    // Initialize various fields in the socket information structure.
    //
    // Note that the reference count is initialized to 2 to account for
    // base reference and reference requested by the caller
    //

    newSocket->HContext.Handle = (HANDLE)Handle;
    newSocket->HContext.RefCount = 2;

    newSocket->HelperDllContext = helperDllContext;
    newSocket->HelperDll = helperDll;
    newSocket->HelperDllNotificationEvents = helperDllNotificationEvents;


    newSocket->LocalAddress = (PVOID)ALIGN_8(newSocket + 1);
    // get the real size from the context
    // newSocket->LocalAddressLength = helperDll->MaxSockaddrLength;

    newSocket->RemoteAddress = (PVOID)ALIGN_8((PUCHAR)newSocket->LocalAddress +
                                    helperDll->MaxSockaddrLength);
    // newSocket->RemoteAddressLength = helperDll->MaxSockaddrLength;

    newSocket->TdiAddressHandle = NULL;
    newSocket->TdiConnectionHandle = NULL;

    newSocket->AsyncConnectContext = NULL;

    newSocket->EventSelectEventObject = NULL;
    newSocket->EventSelectlNetworkEvents = 0;

#ifdef _AFD_SAN_SWITCH_
	newSocket->SanSocket = NULL;
	newSocket->DontTrySAN = FALSE;
#endif //_AFD_SAN_SWITCH_


    __try {

        InitializeCriticalSection( &newSocket->Lock );

    } 
    __except( SOCK_EXCEPTION_FILTER() ) {

        error = SockNtStatusToSocketError (GetExceptionCode());
        goto exit;

    }

    resourceInitialized = TRUE;


    //
    // Determine the length of the helper DLL's context information.
    //

    contextPtr = (PCHAR)context + sizeof(SOCK_SHARED_INFO);
    helperDllContextLength = *(PULONG)contextPtr;
    contextPtr += sizeof(ULONG);

    //
    // Copy in information from the context buffer retrieved from AFD.
    //

    WS_ASSERT( newSocket->HelperDll != NULL );

    RtlCopyMemory(
        newSocket->LocalAddress,
        contextPtr,
        helperDll->MaxSockaddrLength
        );
    contextPtr += helperDll->MaxSockaddrLength;

    RtlCopyMemory(
        newSocket->RemoteAddress,
        contextPtr,
        helperDll->MaxSockaddrLength
        );
    contextPtr += helperDll->MaxSockaddrLength;

    //
    // Get TDI handles for this socket.
    //

    error = SockGetTdiHandles( newSocket );
    if ( error != NO_ERROR ) {
        goto exit;
    }

    //
    // Set the helper's context.
    //

    error = newSocket->HelperDll->WSHSetSocketInformation (
                newSocket->HelperDllContext,
                newSocket->Handle,
                newSocket->TdiAddressHandle,
                newSocket->TdiConnectionHandle,
                SOL_INTERNAL,
                SO_CONTEXT,
                contextPtr,
                helperDllContextLength
                );
    if ( error != NO_ERROR ) {
        goto exit;
    }

#ifdef _AFD_SAN_SWITCH_
	//
	// Do before WahInsertHandleContext
	//
	if (SockSanEnabled && newSocket->IsSANSocket) {
		error = SockSanImportSocket(newSocket);
		
		if (error) {
			WS_PRINT(("SockpImportSocket: SockSanImportSocket err=%d\n",
					  error));
			goto exit;
		}	
	}
#endif

    ctx = WahInsertHandleContext (SockContextTable, &newSocket->HContext);
    if (ctx==NULL) {
        error = WSAENOBUFS;
        goto exit;
    }
    else if (ctx!=&newSocket->HContext) {
        WS_PRINT(( "SockpImportHandle: handle %lx must have been closed with"
                    " CloseHandle, not closesocket, cleaning up for it!\n",
                    newSocket->Handle));

        //
        // Just ged rid of the stale context
        //
        SockDereferenceSocket (CONTAINING_RECORD (
                                        ctx, 
                                        SOCKET_INFORMATION,
                                        HContext));
        ctx = &newSocket->HContext;
    }

    //
    // If the socket has AsyncSelect events set up, set them up for this
    // process.
    // BUGBUG
    // IS THIS REALLY POSSIBLE AND USED BY A REAL APPLICATION(S) ????
    // (it's been that way since NT3.51 though).
    // EVEN WHEN HANDLE IS DUPLICATED INSIDE ONE PROCESS THE SPEC SAYS
    // THE WSASYNCSELECT NEEDS TO BE CALLED AGAIN ON THE NEW HANDLE.
    //

    if ( newSocket->AsyncSelectlEvent ) {

        INT result;

        result = WSPAsyncSelect(
                     newSocket->Handle,
                     newSocket->AsyncSelecthWnd,
                     newSocket->AsyncSelectwMsg,
                     newSocket->AsyncSelectlEvent,
                     &error
                     );

        if( result == SOCKET_ERROR ) {

            goto exit;

        }

    }

    succeeded = TRUE;

exit:
	SockReleaseRwLockExclusive (&SocketGlobalLock);


    if ((transportDeviceName.Buffer != NULL) && (socketType == SOCK_RAW)) {

        RtlFreeHeap( RtlProcessHeap(), 0, transportDeviceName.Buffer );

    }

    if ( succeeded) {
        IF_DEBUG(SOCKET) {
            WS_PRINT(( "Imported socket %lx (%lx) of type %s\n",
                           newSocket->Handle, newSocket,
                           ( newSocket->SocketType == SOCK_DGRAM ?
                                 "SOCK_DGRAM" :
                                 (newSocket->SocketType == SOCK_STREAM ?
                                     "SOCK_STREAM" : "SOCK_RAW")) ));
        }

    } else {

        if (newSocket != NULL ) {

            if ( resourceInitialized ) {
                DeleteCriticalSection( &newSocket->Lock );
            }

            if ( newSocket->TdiAddressHandle != NULL ) {
                status = NtClose( newSocket->TdiAddressHandle );
                WS_ASSERT( NT_SUCCESS(status) );
            }

            if ( newSocket->TdiConnectionHandle != NULL ) {
                status = NtClose( newSocket->TdiConnectionHandle );
                WS_ASSERT( NT_SUCCESS(status) );
            }

            WS_ASSERT ( newSocket->HelperDll != NULL );
            SockNotifyHelperDll( newSocket, WSH_NOTIFY_CLOSE );

            FREE_HEAP( newSocket );

            newSocket = NULL;


        }

	    if (helperDll!=NULL) {
		    SockDereferenceHelper (helperDll);
	    }

        IF_DEBUG(SOCKET) {
            WS_PRINT(( "SockGetHandleContext: failed to import socket "
                       "handle %lx\n", Handle ));
        }
    }

    if ( context != NULL && context != contextBuffer ) {
        FREE_HEAP( context );
    }

    return newSocket;

} // SockpImportHandle


INT
SockSetHandleContext (
    IN PSOCKET_INFORMATION Socket
    )
{
    NTSTATUS status;
    PVOID context;
    PCHAR contextPtr;
    ULONG contextLength;
    ULONG helperDllContextLength;
    IO_STATUS_BLOCK ioStatusBlock;
    INT error;
    UCHAR contextBuffer[MAX_FAST_HANDLE_CONTEXT];
	PWINSOCK_TLS_DATA	tlsData;
	tlsData = GET_THREAD_DATA ();

    //
    // Determine how much space we need for the helper DLL context.
    //

    error = Socket->HelperDll->WSHGetSocketInformation (
                Socket->HelperDllContext,
                Socket->Handle,
                Socket->TdiAddressHandle,
                Socket->TdiConnectionHandle,
                SOL_INTERNAL,
                SO_CONTEXT,
                NULL,
                (PINT)&helperDllContextLength
                );
    if ( error != NO_ERROR ) {
        return NO_ERROR;  // !!!
        //return error;
    }

    //
    // Allocate a buffer to hold all context information.
    //

    contextLength = sizeof(SOCK_SHARED_INFO) + 
                        Socket->HelperDll->MaxSockaddrLength +
                        Socket->HelperDll->MaxSockaddrLength +
                        sizeof(helperDllContextLength) + helperDllContextLength;

    if( contextLength <= sizeof(contextBuffer) ) {
        context = (PVOID)contextBuffer;
    } else {
        context = ALLOCATE_HEAP( contextLength );
        if ( context == NULL ) {
            error = WSAENOBUFS;
            return error;
        }
    }

    //
    // Copy over information to the context buffer.  The context buffer
    // has the following format:
    //
    //     SOCKET_SHARED_INFO structure
    //     Helper DLL Context Length
    //     Local Address
    //     Remote Address
    //     Helper DLL Context
    //

    contextPtr = context;

    RtlCopyMemory( contextPtr, &Socket->SharedInfo, sizeof(SOCK_SHARED_INFO) );
    contextPtr += sizeof(SOCK_SHARED_INFO);

    *(PULONG)contextPtr = helperDllContextLength;
    contextPtr += sizeof(helperDllContextLength);

    RtlCopyMemory(contextPtr, Socket->LocalAddress, Socket->HelperDll->MaxSockaddrLength );
    contextPtr += Socket->HelperDll->MaxSockaddrLength;

    RtlCopyMemory(contextPtr, Socket->RemoteAddress, Socket->HelperDll->MaxSockaddrLength );
    contextPtr += Socket->HelperDll->MaxSockaddrLength;

    //
    // Get the context from the helper DLL.
    //

    error = Socket->HelperDll->WSHGetSocketInformation (
                Socket->HelperDllContext,
                Socket->Handle,
                Socket->TdiAddressHandle,
                Socket->TdiConnectionHandle,
                SOL_INTERNAL,
                SO_CONTEXT,
                contextPtr,
                (PINT)&helperDllContextLength
                );
    if ( error != NO_ERROR ) {
        if( context != (PVOID)contextBuffer ) {
            FREE_HEAP( context );
        }
        return error;
    }

    //
    // Now give all this information to AFD to hold on to.
    //

    status = NtDeviceIoControlFile(
                 Socket->HContext.Handle,
                 tlsData->EventHandle,
                 NULL,                   // APC Routine
                 NULL,                   // APC Context
                 &ioStatusBlock,
                 IOCTL_AFD_SET_CONTEXT,
                 context,
                 contextLength,
                 NULL,
                 0
                 );

    if ( status == STATUS_PENDING ) {
        SockWaitForSingleObject(
            tlsData->EventHandle,
            Socket->Handle,
            SOCK_NEVER_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        status = ioStatusBlock.Status;
    }

    if( context != (PVOID)contextBuffer ) {
        FREE_HEAP( context );
    }

    if ( !NT_SUCCESS(status) ) {
        error = SockNtStatusToSocketError( status );
        return error;
    }

    return NO_ERROR;

} // SockSetHandleContext



BOOL
SockWaitForSingleObject (
    IN HANDLE Handle,
    IN SOCKET SocketHandle,
    IN DWORD BlockingHookUsage,
    IN DWORD TimeoutUsage
    )

/*++

Routine Description:

    Does an alertable wait on the specified handle.  If the wait completes
    due to an alert, it rewaits.

Arguments:

    Handle - NT system handle to wait on.

    SocketHandle - the socket handle on which we're performing the IO
        we're waiting for.  This is necessary to support
        WSACancelBlockingCall().

    BlockingHookUsage - indicates whether to call the thread's blocking
        hook.  Possible values are:

            SOCK_ALWAYS_CALL_BLOCKING_HOOK - blocking hook is always
                called if blocking is necessary.

            SOCK_CONDITIONALLY_CALL_BLOCKING_HOOK - blocking hook is
                called if the socket is blocking (i.e. not a nonblocking
                socket).

            SOCK_NEVER_CALL_BLOCKING_HOOK - blocking hook is never
                called.

    TimeoutUsage - determines whether to wait infinitely or for a
        timeout.  Possible values are:

            SOCK_NO_TIMEOUT - wait forever for the handle to be
                signalled.

            SOCK_SEND_TIMEOUT - use the socket's send timeout value
                as a timeout.

            SOCK_RECEIVE_TIMEOUT - use the socket's receive timeout
                value as a timeout.

Return Value:

    BOOL - TRUE if the object was signalled within the appropriate
        timeout, and FALSE if the timeout occurred first.

--*/

{
    NTSTATUS status;
    LARGE_INTEGER timeout;
    BOOLEAN callBlockingHook;
    BOOLEAN useTimeout;
    LARGE_INTEGER endTime;
    LARGE_INTEGER currentTime;
    PSOCKET_INFORMATION socket = NULL;
    LPBLOCKINGCALLBACK blockingCallback;
    DWORD_PTR blockingContext;
	PWINSOCK_TLS_DATA	tlsData;
	tlsData = GET_THREAD_DATA ();

    //
    // First wait for the object for a little while.  This handles the
    // usual case where the object is already signalled or is signalled
    // shortly into the wait.  We'll only go through the longer, more
    // complex path if we're going to have to wait longer.
    //

    timeout.HighPart = 0xFFFFFFFF;
    timeout.LowPart = (ULONG)(-1 * (10*1000*500));     // 0.5 seconds

    status = NtWaitForSingleObject( Handle, TRUE, &timeout );
    if ( status == STATUS_SUCCESS ) {
        return TRUE;
    }

    //
    // If we need to extract information from the socket, get a pointer
    // to the socket information structure.
    //

    if ( BlockingHookUsage == SOCK_CONDITIONALLY_CALL_BLOCKING_HOOK ||
             BlockingHookUsage == SOCK_ALWAYS_CALL_BLOCKING_HOOK ||
             TimeoutUsage == SOCK_SEND_TIMEOUT ||
             TimeoutUsage == SOCK_RECEIVE_TIMEOUT ) {

        socket = SockFindAndReferenceSocket( SocketHandle, FALSE );
        if ( socket == NULL ) {
            NtWaitForSingleObject( Handle, TRUE, NULL );
            return TRUE;
        }
    }

    //
    // Determine whether we need to call the blocking hook while
    // we're waiting.
    //

    switch ( BlockingHookUsage ) {

    case SOCK_ALWAYS_CALL_BLOCKING_HOOK:

        //
        // We'll assume (for now) that we'll need to call the blocking
        // hook. If we later determine that there is no blocking hook
        // installed, then we obviously cannot call it...
        //

        callBlockingHook = TRUE;
        break;

    case SOCK_CONDITIONALLY_CALL_BLOCKING_HOOK:

        //
        // We'll try to call the blocking hook if this is a blocking socket.
        // (Later we'll determine if there is really a blocking hook
        // installed.)
        //

        callBlockingHook = !socket->NonBlocking;
        break;

    case SOCK_NEVER_CALL_BLOCKING_HOOK:

        callBlockingHook = FALSE;
        break;

    default:

        WS_ASSERT( FALSE );
        break;
    }

    //
    // Determine if there's really a blocking hook installed.  If the
    // upcall fails, we'll just press on regardless.
    //

    if( callBlockingHook == TRUE ) {

        INT result;
        INT error;

        ASSERT( socket != NULL );

        blockingCallback = NULL;

        result = (SockUpcallTable->lpWPUQueryBlockingCallback)(
                     socket->CatalogEntryId,
                     &blockingCallback,
                     &blockingContext,
                     &error
                     );

        if( result == SOCKET_ERROR ) {

            WS_PRINT((
                "SockWaitForSingleObject: WPUQueryBlockingCallback failed %d\n",
                error
                ));

        }

        callBlockingHook = ( blockingCallback != NULL );

    }

    //
    // Determine what our timeout should be, if any.
    //

    switch ( TimeoutUsage ) {

    case SOCK_NO_TIMEOUT:

        useTimeout = FALSE;
        break;

    case SOCK_SEND_TIMEOUT:

        if ( socket->SendTimeout != 0 ) {
            useTimeout = TRUE;
            timeout = RtlEnlargedIntegerMultiply( socket->SendTimeout, 10*1000 );
        } else {
            useTimeout = FALSE;
        }

        break;

    case SOCK_RECEIVE_TIMEOUT:

        if ( socket->ReceiveTimeout != 0 ) {
            useTimeout = TRUE;
            timeout = RtlEnlargedIntegerMultiply( socket->ReceiveTimeout, 10*1000 );
        } else {
            useTimeout = FALSE;
        }

        break;

    default:

        WS_ASSERT( FALSE );
        break;
    }

    //
    // Dereference the socket if we got a pointer to the socket
    // information structure.
    //

    if ( socket != NULL ) {
        SockDereferenceSocket( socket );
#if DBG
        socket = NULL;
#endif
    }

    //
    // Calculate the end time we'll use when waiting on the handle.  The
    // end time is the time at which we must quit waiting on the handle
    // and must instead return from this function.
    //

    if ( useTimeout ) {

        //
        // The end time if the current time plus the timeout.  Query
        // the current time.
        //

        status = NtQuerySystemTime( &currentTime );
        WS_ASSERT( NT_SUCCESS(status) );

        endTime.QuadPart = currentTime.QuadPart + timeout.QuadPart;

    } else {

        //
        // We need an infinite timeout.  Set the end time to the largest
        // possible time in NT format.
        //

        endTime.LowPart = 0xFFFFFFFF;
        endTime.HighPart = 0x7FFFFFFF;
    }

    //
    // If we're going to be calling a blocking hook, set up a minimal
    // timeout since we have to call the blocking hook instead of idly
    // waiting.  If we won't be calling the blocking hook, then we'll
    // wait until the end time.
    //

    if ( callBlockingHook ) {
        timeout.LowPart = 0xFFFFFFFF;
        timeout.HighPart = 0xFFFFFFFF;
    } else {
        timeout = endTime;
    }

    //
    // Initialize the thread's cancel
    // Boolean so that we can tell whether the IO has been cancelled,
    // and remember the socket handle on which we're doing the IO.
    //

    tlsData->IoCancelled = FALSE;
    tlsData->SocketHandle = SocketHandle;

    do {

        //
        // If necessary, call the blocking hook function until it
        // returns FALSE.  This gives the routine the oppurtunity
        // to process all the available messages before we complete
        // the wait.
        //

        if ( callBlockingHook ) {

            ASSERT( blockingCallback != NULL );

            if( !(blockingCallback)( blockingContext ) ) {

                ASSERT( tlsData->IoCancelled == TRUE );

            }

        }

        //
        // If the operation was cancelled, reset the timeout to infinite
        // and wait for the cancellation.  We don't want to call the
        // blocking hook after the IO is cancelled.
        //

        if ( tlsData->IoCancelled ) {

            timeout.LowPart = 0xFFFFFFFF;
            timeout.HighPart = 0x7FFFFFFF;

        } else {

            //
            // Determine whether we have exceeded the end time.  If we
            // have exceeded the end time then we must not wait any
            // longer.
            //

            status = NtQuerySystemTime( &currentTime );
            WS_ASSERT( NT_SUCCESS(status) );

            if ( currentTime.QuadPart > endTime.QuadPart ) {
                status = STATUS_TIMEOUT;
                break;
            }
        }

        //
        // Perform the actual wait on the object handle.
        //

        status = NtWaitForSingleObject( Handle, TRUE, &timeout );
        WS_ASSERT( NT_SUCCESS(status) );
        WS_ASSERT( status != STATUS_TIMEOUT || !tlsData->IoCancelled );

    } while ( status == STATUS_USER_APC ||
              status == STATUS_ALERTED ||
              status == STATUS_TIMEOUT );

    //
    // Reset thread variables.
    //

    tlsData->SocketHandle = INVALID_SOCKET;

    //
    // Return TRUE if the wait's return code was success; otherwise, we
    // had to timeout the wait so return FALSE.
    //

    if ( status == STATUS_SUCCESS ) {
        return TRUE;
    } else {
        return FALSE;
    }

} // SockWaitForSingleObject


BOOL
SockDefaultBlockingHook (
    VOID
    )
{

    MSG msg;
    BOOL retrievedMessage;

    //
    // Get the next message for this thread, if any.
    //

    retrievedMessage = PeekMessage( &msg, NULL, 0, 0, PM_REMOVE );

    //
    // Process the message if we got one.
    //

    if ( retrievedMessage ) {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }

    //
    // If we got a message, indicate that we want to be called again.
    //

    return retrievedMessage;

} // SockDefaultBlockingHook


BOOLEAN
SockIsSocketConnected (
    IN PSOCKET_INFORMATION Socket
    )
{
    static const LONGLONG   Wait0 = 0;
    PVOID                   context;
    PASYNC_COMPLETION_PROC  proc;
    IO_STATUS_BLOCK         ioStatus;
    NTSTATUS                status;

    //
    // If there is a connect in progress, check if it has already
    // completed but has not been processed by async thread and
    // attempt to process it here
    //

    while ((Socket->AsyncConnectContext!=NULL) &&
            (Socket->AsyncConnectContext->IoStatusBlock.Status!=STATUS_PENDING)){
        SockReleaseSocketLock (Socket);

        status = NtRemoveIoCompletion (
                SockAsyncQueuePort,
                (PVOID *)&proc,
                &context,
                &ioStatus,
                (PLARGE_INTEGER)&Wait0
                );
        WS_ASSERT (NT_SUCCESS (status));

        if (status==STATUS_SUCCESS) {
            if (proc!=ASYNC_TERMINATION_PROC) {
				SockHandleAsyncIndication(proc, context, &ioStatus);
            }
            else {
                status = NtSetIoCompletion (SockAsyncQueuePort,
                                    ASYNC_TERMINATION_PROC,
                                    (PVOID)-1,
                                    0,
                                    0);
                WS_ASSERT (NT_SUCCESS (status));
                SockAcquireSocketLockExclusive (Socket);
                break;
            }
        }

        SockAcquireSocketLockExclusive (Socket);
    }

    //
    // Check whether the socket is already connected.
    //

    if ( Socket->State == SocketStateConnected ) {
        return TRUE;
    }

    return FALSE;

} // SockIsSocketConnected


INT
SockGetTdiHandles (
    IN PSOCKET_INFORMATION Socket
    )
{
    AFD_HANDLE_INFO handleInfo;
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG getHandleInfo;
	PWINSOCK_TLS_DATA	tlsData;
	tlsData = GET_THREAD_DATA ();

    //
    // Determine which handles we need to get.
    //

    getHandleInfo = 0;

    if ( Socket->TdiAddressHandle == NULL ) {
        getHandleInfo |= AFD_QUERY_ADDRESS_HANDLE;
    }

    if ( Socket->TdiConnectionHandle == NULL ) {
        getHandleInfo |= AFD_QUERY_CONNECTION_HANDLE;
    }

    //
    // If we already have both TDI handles for the socket, just return.
    //

    if ( getHandleInfo == 0 ) {
        return NO_ERROR;
    }

    //
    // Call AFD to retrieve the TDI handles for the socket.
    //

    status = NtDeviceIoControlFile(
                 Socket->HContext.Handle,
                 tlsData->EventHandle,
                 NULL,                   // APC Routine
                 NULL,                   // APC Context
                 &ioStatusBlock,
                 IOCTL_AFD_QUERY_HANDLES,
                 &getHandleInfo,
                 sizeof(getHandleInfo),
                 &handleInfo,
                 sizeof(handleInfo)
                 );

    // *** Because this routine can be called at APC level from
    //     ConnectCompletionApc(), IOCTL_AFD_QUERY_HANDLES must
    //     never pend.

    WS_ASSERT( status != STATUS_PENDING );

    if ( status == STATUS_PENDING ) {
        SockWaitForSingleObject(
            tlsData->EventHandle,
            Socket->Handle,
            SOCK_NEVER_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS(status) ) {
        return SockNtStatusToSocketError( status );
    }

    //
    // Set up the handles that we were returned.
    //

    if ( Socket->TdiAddressHandle == NULL ) {
        Socket->TdiAddressHandle = handleInfo.TdiAddressHandle;
    }

    if ( Socket->TdiConnectionHandle == NULL ) {
        Socket->TdiConnectionHandle = handleInfo.TdiConnectionHandle;
    }

    return NO_ERROR;

} // SockGetTdiHandles


INT
SockGetInformation (
    IN PSOCKET_INFORMATION Socket,
    IN ULONG InformationType,
    IN PVOID AdditionalInputInfo OPTIONAL,
    IN ULONG AdditionalInputInfoLength,
    IN OUT PBOOLEAN Boolean OPTIONAL,
    IN OUT PULONG Ulong OPTIONAL,
    IN OUT PLARGE_INTEGER LargeInteger OPTIONAL
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    PAFD_INFORMATION afdInfo;
    AFD_INFORMATION localInfo;
    ULONG afdInfoLength;
	PWINSOCK_TLS_DATA	tlsData;
	tlsData = GET_THREAD_DATA ();


    if (ARGUMENT_PRESENT( AdditionalInputInfo )
         && (AdditionalInputInfoLength>0)) {
        //
        // Allocate space for the I/O buffer.
        //

        afdInfoLength = sizeof(*afdInfo) + AdditionalInputInfoLength;
        afdInfo = ALLOCATE_HEAP( afdInfoLength );
        if ( afdInfo == NULL ) {
            return WSAENOBUFS;
        }

        //
        // If there is additional input information, copy it to the input
        // buffer.
        //

        RtlCopyMemory( afdInfo + 1, AdditionalInputInfo, AdditionalInputInfoLength );
    }
    else {
        afdInfo = &localInfo;
        afdInfoLength = sizeof(*afdInfo);
    }
    //
    // Set up the AFD information block.
    //

    afdInfo->InformationType = InformationType;


    //
    // Set the blocking mode to AFD.
    //

    status = NtDeviceIoControlFile(
                 Socket->HContext.Handle,
                 tlsData->EventHandle,
                 NULL,                      // APC Routine
                 NULL,                      // APC Context
                 &ioStatusBlock,
                 IOCTL_AFD_GET_INFORMATION,
                 afdInfo,
                 afdInfoLength,
                 afdInfo,
                 sizeof(*afdInfo)
                 );

    //
    // Wait for the operation to complete.
    //

    if ( status == STATUS_PENDING ) {
        SockWaitForSingleObject(
            tlsData->EventHandle,
            Socket->Handle,
            SOCK_NEVER_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS(status) ) {
        if (afdInfo!=&localInfo) {
            FREE_HEAP( afdInfo );
        }
        return SockNtStatusToSocketError( status );
    }

    //
    // Put the return info in the requested parameter.
    //

    if ( ARGUMENT_PRESENT( Boolean ) ) {
        *Boolean = afdInfo->Information.Boolean;
    } else if ( ARGUMENT_PRESENT( Ulong ) ) {
        *Ulong = afdInfo->Information.Ulong;
    } else {
        WS_ASSERT( ARGUMENT_PRESENT( LargeInteger ) );
        *LargeInteger = afdInfo->Information.LargeInteger;
    }

    if (afdInfo!=&localInfo) {
        FREE_HEAP( afdInfo );
    }

    return NO_ERROR;

} // SockGetInformation


INT
SockSetInformation (
    IN PSOCKET_INFORMATION Socket,
    IN ULONG InformationType,
    IN PBOOLEAN Boolean OPTIONAL,
    IN PULONG Ulong OPTIONAL,
    IN PLARGE_INTEGER LargeInteger OPTIONAL
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    AFD_INFORMATION afdInfo;
	PWINSOCK_TLS_DATA	tlsData;
	tlsData = GET_THREAD_DATA ();

    //
    // Set up the AFD information block.
    //

    afdInfo.InformationType = InformationType;

    if ( ARGUMENT_PRESENT( Boolean ) ) {
        afdInfo.Information.Boolean = *Boolean;
    } else if ( ARGUMENT_PRESENT( Ulong ) ) {
        afdInfo.Information.Ulong = *Ulong;
    } else {
        ASSERT( ARGUMENT_PRESENT( LargeInteger ) );
        afdInfo.Information.LargeInteger = *LargeInteger;
    }

    //
    // Set the blocking mode to AFD.
    //

    status = NtDeviceIoControlFile(
                 Socket->HContext.Handle,
                 tlsData->EventHandle,
                 NULL,                      // APC Routine
                 NULL,                      // APC Context
                 &ioStatusBlock,
                 IOCTL_AFD_SET_INFORMATION,
                 &afdInfo,
                 sizeof(afdInfo),
                 NULL,
                 0
                 );

    //
    // Wait for the operation to complete.
    //

    if ( status == STATUS_PENDING ) {
        SockWaitForSingleObject(
            tlsData->EventHandle,
            Socket->Handle,
            SOCK_NEVER_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS(status) ) {
        return SockNtStatusToSocketError( status );
    }

    return NO_ERROR;

} // SockSetInformation


int
SockEnterApiSlow (
	OUT PWINSOCK_TLS_DATA	*tlsData
    )
{

    //
    // Bail if we're already detached from the process.
    //

    if( SockProcessTerminating ) {

        IF_DEBUG(ENTER) {
            WS_PRINT(( "SockEnterApi: process terminating\n" ));
        }

        return WSANOTINITIALISED;
    }

    //
    // Make sure that WSAStartup has been called, if necessary.
    //

    if (SockWspStartupCount <= 0) {
		WS_ASSERT (SockWspStartupCount==0);

        IF_DEBUG(ENTER) {
            WS_PRINT(( "SockEnterApi: WSAStartup() not called!\n" ));
        }

        return WSANOTINITIALISED;
    }

    //
    // If this thread has not been initialized, do it now.
    //

    *tlsData = GET_THREAD_DATA();

    if ( *tlsData == NULL ) {

        if ( !SockThreadInitialize() ) {

            IF_DEBUG(ENTER) {
                WS_PRINT(( "SockEnterApi: SockThreadInitialize failed.\n" ));
            }

            return WSAENOBUFS;
        }

        *tlsData = GET_THREAD_DATA();
    }


    //
    // Everything's cool.  Proceed.
    //

    return NO_ERROR;

} // SockEnterApi

#if DBG

VOID
WsAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber
    )
{
    BOOL ok;
    CHAR choice[16];
    DWORD bytes;
    DWORD error;

    IF_DEBUG(CONSOLE) {
        WS_PRINT(( "\n failed: %s\n  at line %ld of %s\n",
                    FailedAssertion, LineNumber, FileName ));
        do {
            WS_PRINT(( "[B]reak/[I]gnore? " ));
            bytes = sizeof(choice);
            ok = ReadFile(
                    GetStdHandle(STD_INPUT_HANDLE),
                    &choice,
                    bytes,
                    &bytes,
                    NULL
                    );
            if ( ok ) {
                if ( toupper(choice[0]) == 'I' ) {
                    break;
                }
                if ( toupper(choice[0]) == 'B' ) {
                    DbgUserBreakPoint( );
                }
            } else {
                error = GetLastError( );
            }
        } while ( TRUE );

        return;
    }

    RtlAssert( FailedAssertion, FileName, LineNumber, NULL );

} // WsAssert

BOOLEAN ConsoleInitialized = FALSE;

HANDLE DebugFileHandle = INVALID_HANDLE_VALUE;
PCHAR DebugFileName = "msafd.log";


VOID
WsPrintf (
    char *Format,
    ...
    )

{
    va_list arglist;
    char OutputBuffer[1024];
    ULONG length;
    BOOL ret;

    length = (ULONG)wsprintfA( OutputBuffer, "MSAFD: " );

    va_start( arglist, Format );

    wvsprintfA( OutputBuffer + length, Format, arglist );

    va_end( arglist );

    IF_DEBUG(DEBUGGER) {
        DbgPrint( "%s", OutputBuffer );
    }

    IF_DEBUG(CONSOLE) {

        if ( !ConsoleInitialized ) {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            COORD coord;

            ConsoleInitialized = TRUE;
            (VOID)AllocConsole( );
            (VOID)GetConsoleScreenBufferInfo(
                    GetStdHandle(STD_OUTPUT_HANDLE),
                    &csbi
                    );
            coord.X = (SHORT)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
            coord.Y = (SHORT)((csbi.srWindow.Bottom - csbi.srWindow.Top + 1) * 20);
            (VOID)SetConsoleScreenBufferSize(
                    GetStdHandle(STD_OUTPUT_HANDLE),
                    coord
                    );
        }

        length = strlen( OutputBuffer );

        ret = WriteFile(
                  GetStdHandle(STD_OUTPUT_HANDLE),
                  (LPVOID )OutputBuffer,
                  length,
                  &length,
                  NULL
                  );
        if ( !ret ) {
            DbgPrint( "WsPrintf: console WriteFile failed: %ld\n",
                          GetLastError( ) );
        }
    }

    IF_DEBUG(FILE) {

        if ( DebugFileHandle == INVALID_HANDLE_VALUE ) {
            DebugFileHandle = CreateFile(
                                  DebugFileName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ,
                                  NULL,
                                  CREATE_ALWAYS,
                                  0,
                                  NULL
                                  );
        }

        if ( DebugFileHandle == INVALID_HANDLE_VALUE ) {

            DbgPrint( "WsPrintf: Failed to open winsock debug log file %s: %ld\n",
                          DebugFileName, GetLastError( ) );

        } else {

            length = strlen( OutputBuffer );

            ret = WriteFile(
                      DebugFileHandle,
                      (LPVOID )OutputBuffer,
                      length,
                      &length,
                      NULL
                      );
            if ( !ret ) {
                DbgPrint( "WsPrintf: file WriteFile failed: %ld\n",
                              GetLastError( ) );
            }
        }
    }

} // WsPrintf

#endif


VOID
WsPrintSockaddr (
    IN PSOCKADDR Sockaddr,
    IN PINT SockaddrLength
    )
{

#if DBG

    if ( Sockaddr == NULL ) {
        DbgPrint( " NULL addr pointer.\n" );
        return;
    }

    if ( SockaddrLength == NULL ) {
        DbgPrint( " NULL addrlen pointer.\n" );
        return;
    }

    switch ( Sockaddr->sa_family) {

    case AF_INET: {

        PSOCKADDR_IN sockaddrIn = (PSOCKADDR_IN)Sockaddr;

        if ( *SockaddrLength < sizeof(SOCKADDR_IN) ) {
            WS_PRINT(( " SHORT AF_INET: len %ld\n", *SockaddrLength ));
            return;
        }

        DbgPrint( " IP %ld.%ld.%ld.%ld port %ld\n",
                       sockaddrIn->sin_addr.S_un.S_un_b.s_b1,
                       sockaddrIn->sin_addr.S_un.S_un_b.s_b2,
                       sockaddrIn->sin_addr.S_un.S_un_b.s_b3,
                       sockaddrIn->sin_addr.S_un.S_un_b.s_b4,
                       sockaddrIn->sin_port );
        return;

    }

    case AF_IPX: {
#include <wsipx.h>
        PSOCKADDR_IPX sockaddrIpx = (PSOCKADDR_IPX)Sockaddr;

        if ( *SockaddrLength < sizeof(SOCKADDR_IPX) ) {
            WS_PRINT(( " SHORT AF_IPX: len %ld\n", *SockaddrLength ));
            return;
        }

        DbgPrint( " IPX %2.2x%2.2x%2.2x%2.2x:%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x: socket %lx\n",
                       sockaddrIpx->sa_netnum[0],
                       sockaddrIpx->sa_netnum[1],
                       sockaddrIpx->sa_netnum[2],
                       sockaddrIpx->sa_netnum[3],
                       sockaddrIpx->sa_nodenum[0],
                       sockaddrIpx->sa_nodenum[1],
                       sockaddrIpx->sa_nodenum[2],
                       sockaddrIpx->sa_nodenum[3],
                       sockaddrIpx->sa_nodenum[4],
                       sockaddrIpx->sa_nodenum[5],
                       sockaddrIpx->sa_socket );
        return;
    }
    case AF_NETBIOS: {
#include <wsnetbs.h>
        PSOCKADDR_NB sockaddrNb = (PSOCKADDR_NB)Sockaddr;
        INT     i;

        if ( *SockaddrLength < sizeof(PSOCKADDR_NB) ) {
            WS_PRINT(( " SHORT AF_NETBIOS: len %ld\n", *SockaddrLength ));
            return;
        }
        DbgPrint (" NETBIOS ");
        for (i=0;i<NETBIOS_NAME_LENGTH-1;i++) {
            if (isprint (sockaddrNb->snb_name[i])) {
                DbgPrint ("%c", sockaddrNb->snb_name[i]);
            }
            else {
                DbgPrint (".");
            }
        }

        DbgPrint ("(");
        for (i=0;i<NETBIOS_NAME_LENGTH-1;i++) {
            DbgPrint ("%2.2lx ", sockaddrNb->snb_name[i]);
        }

        DbgPrint ("), port 0x%lx, type 0x%lx\n",
            sockaddrNb->snb_name[NETBIOS_NAME_LENGTH-1],
            sockaddrNb->snb_type);
        return;
    }
    default: {
        INT i;
        WS_PRINT(( " Family %ld, ", Sockaddr->sa_family ));
        for (i=0; i<*SockaddrLength-FIELD_OFFSET(SOCKADDR, sa_data); i++) {
            DbgPrint ("%2.2lx ", Sockaddr->sa_data[i]);
        }
        DbgPrint ("\n");

        break;
    }
    }

    return;

#endif
} // WsPrintSockaddr

#if DBG

VOID
WsEnterApiCall (
    IN PCHAR RoutineName,
    IN PVOID Arg1,
    IN PVOID Arg2,
    IN PVOID Arg3,
    IN PVOID Arg4
    )
{
    ULONG i;
	PWINSOCK_TLS_DATA	tlsData;
	tlsData = GET_THREAD_DATA();

    CHECK_HEAP;

    //
    // If this thread has not been initialized, do it now.  This is
    // duplicated in SockEnterApi(), but we need it here to
    // access SockIndentLevel below.
    //

    if ( tlsData == NULL ) {
        if ( SockProcessTerminating ||
             !SockThreadInitialize() ) {
            return;
        }
		tlsData = GET_THREAD_DATA();
    }

    IF_DEBUG(ENTER) {
        for ( i = 0; i < tlsData->IndentLevel; i++ ) {
            //WS_PRINT(( "    " ));
        }
        WS_PRINT(( "[%d]--> %s() args 0x%lx 0x%lx 0x%lx 0x%lx\n",
                      tlsData->IndentLevel, RoutineName, Arg1, Arg2, Arg3, Arg4 ));
    }

    tlsData->IndentLevel++;

    return;

} // WsEnter

struct _ERROR_STRINGS {
    INT ErrorCode;
    PCHAR ErrorString;
} ErrorStrings[] = {
    (WSABASEERR+4),   "WSAEINTR",
    (WSABASEERR+9),   "WSAEBADF",
    (WSABASEERR+13),  "WSAEACCES",
    (WSABASEERR+14),  "WSAEFAULT",
    (WSABASEERR+22),  "WSAEINVAL",
    (WSABASEERR+24),  "WSAEMFILE",
    (WSABASEERR+35),  "WSAEWOULDBLOCK",
    (WSABASEERR+36),  "WSAEINPROGRESS",
    (WSABASEERR+37),  "WSAEALREADY",
    (WSABASEERR+38),  "WSAENOTSOCK",
    (WSABASEERR+39),  "WSAEDESTADDRREQ",
    (WSABASEERR+40),  "WSAEMSGSIZE",
    (WSABASEERR+41),  "WSAEPROTOTYPE",
    (WSABASEERR+42),  "WSAENOPROTOOPT",
    (WSABASEERR+43),  "WSAEPROTONOSUPPORT",
    (WSABASEERR+44),  "WSAESOCKTNOSUPPORT",
    (WSABASEERR+45),  "WSAEOPNOTSUPP",
    (WSABASEERR+46),  "WSAEPFNOSUPPORT",
    (WSABASEERR+47),  "WSAEAFNOSUPPORT",
    (WSABASEERR+48),  "WSAEADDRINUSE",
    (WSABASEERR+49),  "WSAEADDRNOTAVAIL",
    (WSABASEERR+50),  "WSAENETDOWN",
    (WSABASEERR+51),  "WSAENETUNREACH",
    (WSABASEERR+52),  "WSAENETRESET",
    (WSABASEERR+53),  "WSAECONNABORTED",
    (WSABASEERR+54),  "WSAECONNRESET",
    (WSABASEERR+55),  "WSAENOBUFS",
    (WSABASEERR+56),  "WSAEISCONN",
    (WSABASEERR+57),  "WSAENOTCONN",
    (WSABASEERR+58),  "WSAESHUTDOWN",
    (WSABASEERR+59),  "WSAETOOMANYREFS",
    (WSABASEERR+60),  "WSAETIMEDOUT",
    (WSABASEERR+61),  "WSAECONNREFUSED",
    (WSABASEERR+62),  "WSAELOOP",
    (WSABASEERR+63),  "WSAENAMETOOLONG",
    (WSABASEERR+64),  "WSAEHOSTDOWN",
    (WSABASEERR+65),  "WSAEHOSTUNREACH",
    (WSABASEERR+66),  "WSAENOTEMPTY",
    (WSABASEERR+67),  "WSAEPROCLIM",
    (WSABASEERR+68),  "WSAEUSERS",
    (WSABASEERR+69),  "WSAEDQUOT",
    (WSABASEERR+70),  "WSAESTALE",
    (WSABASEERR+71),  "WSAEREMOTE",
    (WSABASEERR+101), "WSAEDISCON",
    (WSABASEERR+91),  "WSASYSNOTREADY",
    (WSABASEERR+92),  "WSAVERNOTSUPPORTED",
    (WSABASEERR+93),  "WSANOTINITIALISED",
    NO_ERROR,         "NO_ERROR"
};


PCHAR
WsGetErrorString (
    IN INT Error
    )
{
    INT i;

    for ( i = 0; ErrorStrings[i].ErrorCode != NO_ERROR; i++ ) {
        if ( ErrorStrings[i].ErrorCode == Error ) {
            return ErrorStrings[i].ErrorString;
        }
    }

    return "Unknown";

} // WsGetErrorString


VOID
WsExitApiCall (
    IN PCHAR RoutineName,
    IN LONG_PTR ReturnValue,
    IN BOOLEAN Failed,
    IN INT error

    )
{
    ULONG i;
	PWINSOCK_TLS_DATA	tlsData;
	tlsData = GET_THREAD_DATA();

    if( SockProcessTerminating ||
         tlsData== NULL ) {
        SetLastError( error );
        return;
    }

    CHECK_HEAP;

    tlsData->IndentLevel--;

    IF_DEBUG(EXIT) {
        for ( i = 0; i < tlsData->IndentLevel; i++ ) {
            //WS_PRINT(( "    " ));
        }
        if ( !Failed ) {
            WS_PRINT(( "[%d]<-- %s() returning %ld (0x%lx)\n",
                           tlsData->IndentLevel, RoutineName, ReturnValue, ReturnValue ));
        } else {

            PSZ errorString = WsGetErrorString( error );

            WS_PRINT(( "[%d]<-- %s() FAILED--error %ld (0x%lx) == %s\n",
                           tlsData->IndentLevel, RoutineName, error, error, errorString ));
        }
    }

    SetLastError( error );

    return;

} // WsExitApiCall

LIST_ENTRY SockHeapListHead;
ULONG SockTotalAllocations = 0;
ULONG SockTotalFrees = 0;
ULONG SockTotalBytesAllocated = 0;
RTL_RESOURCE SocketHeapLock;
BOOLEAN SockHeapDebugInitialized = FALSE;
BOOLEAN SockDebugHeap = FALSE;

PVOID SockHeap = NULL;
PVOID SockCaller1;
PVOID SockCaller2;
BOOLEAN SockDoHeapCheck = TRUE;
BOOLEAN SockDoubleHeapCheck = FALSE;

#define WINSOCK_HEAP_CODE_1 0xabcdef00
#define WINSOCK_HEAP_CODE_2 0x12345678
#define WINSOCK_HEAP_CODE_3 0x87654321
#define WINSOCK_HEAP_CODE_4 0x00fedcba
#define WINSOCK_HEAP_CODE_5 0xa1b2c3d4

typedef struct _SOCK_HEAP_HEADER {
    ULONG HeapCode1;
    ULONG HeapCode2;
    LIST_ENTRY GlobalHeapListEntry;
    PCHAR FileName;
    ULONG LineNumber;
    ULONG Size;
    ULONG Pad;
} SOCK_HEAP_HEADER, *PSOCK_HEAP_HEADER;

typedef struct _SOCK_HEAP_TAIL {
    PSOCK_HEAP_HEADER Header;
    ULONG HeapCode3;
    ULONG HeapCode4;
    ULONG HeapCode5;
} SOCK_HEAP_TAIL, *PSOCK_HEAP_TAIL;

#define FREE_LIST_SIZE 64
SOCK_HEAP_HEADER SockRecentFreeList[FREE_LIST_SIZE];
ULONG SockRecentFreeListIndex = 0;


VOID
SockInitializeDebugData (
    VOID
    )
{
    RtlInitializeResource( &SocketHeapLock );
    InitializeListHead( &SockHeapListHead );

} // SockInitializeDebugData


PVOID
SockAllocateHeap (
    IN ULONG NumberOfBytes,
    PCHAR FileName,
    ULONG LineNumber
    )
{
    PSOCK_HEAP_HEADER header;
    SOCK_HEAP_TAIL UNALIGNED *tail;
    SOCK_HEAP_TAIL localTail;

    //WS_ASSERT( !SockProcessTerminating );
    WS_ASSERT( (NumberOfBytes & 0xF0000000) == 0 );
    WS_ASSERT( SockPrivateHeap != NULL );

    SockCheckHeap( );

    RtlAcquireResourceExclusive( &SocketHeapLock, TRUE );

    header = RtlAllocateHeap( SockPrivateHeap, 0,
                              NumberOfBytes + sizeof(*header) + sizeof(*tail) );
    if ( header == NULL ) {
        RtlReleaseResource( &SocketHeapLock );

        if( SockDoubleHeapCheck ) {
            SockCheckHeap();
        }

        return NULL;
    }

    header->HeapCode1 = WINSOCK_HEAP_CODE_1;
    header->HeapCode2 = WINSOCK_HEAP_CODE_2;
    header->FileName = FileName;
    header->LineNumber = LineNumber;
    header->Size = NumberOfBytes;

    tail = (SOCK_HEAP_TAIL UNALIGNED *)( (PCHAR)(header + 1) + NumberOfBytes );

    localTail.Header = header;
    localTail.HeapCode3 = WINSOCK_HEAP_CODE_3;
    localTail.HeapCode4 = WINSOCK_HEAP_CODE_4;
    localTail.HeapCode5 = WINSOCK_HEAP_CODE_5;

    SockCopyMemory(
        tail,
        &localTail,
        sizeof(localTail)
        );

    InsertTailList( &SockHeapListHead, &header->GlobalHeapListEntry );
    SockTotalAllocations++;
    SockTotalBytesAllocated += header->Size;

    RtlReleaseResource( &SocketHeapLock );

    if( SockDoubleHeapCheck ) {
        SockCheckHeap();
    }

    return (PVOID)(header + 1);

} // SockAllocateHeap


VOID
SockFreeHeap (
    IN PVOID Pointer
    )
{
    PSOCK_HEAP_HEADER header = (PSOCK_HEAP_HEADER)Pointer - 1;
    SOCK_HEAP_TAIL UNALIGNED * tail;
    SOCK_HEAP_TAIL localTail;

    //WS_ASSERT( !SockProcessTerminating );
    WS_ASSERT( SockPrivateHeap != NULL );

    SockCheckHeap( );

    tail = (SOCK_HEAP_TAIL UNALIGNED *)( (PCHAR)(header + 1) + header->Size );

    if ( !SockHeapDebugInitialized ) {
        SockInitializeDebugData( );
        SockHeapDebugInitialized = TRUE;
    }

    RtlAcquireResourceExclusive( &SocketHeapLock, TRUE );

    SockCopyMemory(
        &localTail,
        tail,
        sizeof(localTail)
        );

    WS_ASSERT( header->HeapCode1 == WINSOCK_HEAP_CODE_1 );
    WS_ASSERT( header->HeapCode2 == WINSOCK_HEAP_CODE_2 );
    WS_ASSERT( localTail.HeapCode3 == WINSOCK_HEAP_CODE_3 );
    WS_ASSERT( localTail.HeapCode4 == WINSOCK_HEAP_CODE_4 );
    WS_ASSERT( localTail.HeapCode5 == WINSOCK_HEAP_CODE_5 );
    WS_ASSERT( localTail.Header == header );

    RemoveEntryList( &header->GlobalHeapListEntry );
    SockTotalFrees++;
    SockTotalBytesAllocated -= header->Size;

    //RtlMoveMemory( &SockRecentFreeList[SockRecentFreeListIndex], header, sizeof(*header ) );
    //SockRecentFreeListIndex++;
    //if ( SockRecentFreeListIndex >= FREE_LIST_SIZE ) {
    //    SockRecentFreeListIndex = 0;
    //}

    RtlZeroMemory( header, sizeof(*header) );

    header->HeapCode1 = (ULONG)~WINSOCK_HEAP_CODE_1;
    header->HeapCode2 = (ULONG)~WINSOCK_HEAP_CODE_2;
    localTail.HeapCode3 = (ULONG)~WINSOCK_HEAP_CODE_3;
    localTail.HeapCode4 = (ULONG)~WINSOCK_HEAP_CODE_4;
    localTail.HeapCode5 = (ULONG)~WINSOCK_HEAP_CODE_5;
    localTail.Header = NULL;

    SockCopyMemory(
        tail,
        &localTail,
        sizeof(localTail)
        );

    RtlReleaseResource( &SocketHeapLock );

    RtlFreeHeap( SockPrivateHeap, 0, (PVOID)header );

    if( SockDoubleHeapCheck ) {
        SockCheckHeap();
    }

} // SockFreeHeap


VOID
SockCheckHeap (
    VOID
    )
{
    PLIST_ENTRY listEntry;
    PLIST_ENTRY lastListEntry = NULL;
    PSOCK_HEAP_HEADER header;
    SOCK_HEAP_TAIL UNALIGNED *tail;
    SOCK_HEAP_TAIL localTail;

    if ( !SockHeapDebugInitialized ) {
        SockInitializeDebugData( );
        SockHeapDebugInitialized = TRUE;
        //SockHeap = RtlCreateHeap( HEAP_GROWABLE, 0, 0, 0, 0, NULL );
        //WS_ASSERT( SockHeap != NULL );
    }

    if ( !SockDoHeapCheck ) {
        return;
    }

    RtlValidateHeap( SockPrivateHeap, 0, NULL );

    RtlAcquireResourceExclusive( &SocketHeapLock, TRUE );

    for ( listEntry = SockHeapListHead.Flink;
          listEntry != &SockHeapListHead;
          listEntry = listEntry->Flink ) {

        if ( listEntry == NULL ) {
            DbgPrint( "listEntry == NULL, lastListEntry == %lx\n", lastListEntry );
            DbgBreakPoint( );
        }

        header = CONTAINING_RECORD( listEntry, SOCK_HEAP_HEADER, GlobalHeapListEntry );
        tail = (SOCK_HEAP_TAIL UNALIGNED *)( (PCHAR)(header + 1) + header->Size );

        SockCopyMemory(
            &localTail,
            tail,
            sizeof(localTail)
            );

        if ( header->HeapCode1 != WINSOCK_HEAP_CODE_1 ) {
            DbgPrint( "SockCheckHeap, fail 1, header %lx tail %lx\n", header, tail );
            DbgBreakPoint( );
        }

        if ( header->HeapCode2 != WINSOCK_HEAP_CODE_2 ) {
            DbgPrint( "SockCheckHeap, fail 2, header %lx tail %lx\n", header, tail );
            DbgBreakPoint( );
        }

        if ( localTail.HeapCode3 != WINSOCK_HEAP_CODE_3 ) {
            DbgPrint( "SockCheckHeap, fail 3, header %lx tail %lx\n", header, tail );
            DbgBreakPoint( );
        }

        if ( localTail.HeapCode4 != WINSOCK_HEAP_CODE_4 ) {
            DbgPrint( "SockCheckHeap, fail 4, header %lx tail %lx\n", header, tail );
            DbgBreakPoint( );
        }

        if ( localTail.HeapCode5 != WINSOCK_HEAP_CODE_5 ) {
            DbgPrint( "SockCheckHeap, fail 5, header %lx tail %lx\n", header, tail );
            DbgBreakPoint( );
        }

        if ( localTail.Header != header ) {
            DbgPrint( "SockCheckHeap, fail 6, header %lx tail %lx\n", header, tail );
            DbgBreakPoint( );
        }

        lastListEntry = listEntry;
    }

    RtlGetCallersAddress( &SockCaller1, &SockCaller2 );

    RtlReleaseResource( &SocketHeapLock );

} // SockCheckHeap

LONG
SockExceptionFilter(
    LPEXCEPTION_POINTERS ExceptionPointers,
    LPSTR SourceFile,
    LONG LineNumber
    )
{

    LPSTR fileName;

    //
    // Protect ourselves in case the process is totally screwed.
    //

    __try {

        //
        // Exceptions should never be thrown in a properly functioning
        // system, so this is bad. To ensure that someone will see this,
        // forcibly enable debugger output if none of the output bits are
        // enabled.
        //

        if( ( WsDebug & ( WINSOCK_DEBUG_CONSOLE |
                          WINSOCK_DEBUG_FILE |
                          WINSOCK_DEBUG_DEBUGGER ) ) == 0 ) {

            WsDebug |= WINSOCK_DEBUG_DEBUGGER;

        }

        //
        // Strip off the path from the source file.
        //

        fileName = strrchr( SourceFile, '\\' );

        if( fileName == NULL ) {
            fileName = SourceFile;
        } else {
            fileName++;
        }

        //
        // Whine about the exception.
        //

        WS_PRINT((
            "SockExceptionFilter: exception %08lx @ %08lx, caught in %s:%d\n",
            ExceptionPointers->ExceptionRecord->ExceptionCode,
            ExceptionPointers->ExceptionRecord->ExceptionAddress,
            fileName,
            LineNumber
            ));

    }
    __except( EXCEPTION_EXECUTE_HANDLER ) {

        //
        // Not much we can do here...
        //

        NOTHING;

    }

    return EXCEPTION_EXECUTE_HANDLER;

}   // SockExceptionFilter

#endif // if DBG

VOID
WINAPI
SockIoCompletion (
    PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    DWORD Reserved
    )

/*++

Routine Description:

    This procedure is called to complete WSARecv, WSARecvFrom, WSASend,
    WSASendTo, and WSAIoctl asynchronous I/O operations. Its primary
    function is to extract the appropriate information from the passed
    IoStatusBlock and call the user's completion routine.

    The users completion routine is called as:

        Routine Description:

            When an outstanding I/O completes with a callback, this
            function is called.  This function is only called while the
            thread is in an alertable wait (SleepEx,
            WaitForSingleObjectEx, or WaitForMultipleObjectsEx with the
            bAlertable flag set to TRUE).  Returning from this function
            allows another pendiong I/O completion callback to be
            processed.  If this is the case, this callback is entered
            before the termination of the thread's wait with a return
            code of WAIT_IO_COMPLETION.

            Note that each time your completion routine is called, the
            system uses some of your stack.  If you code your completion
            logic to do additional ReadFileEx's and WriteFileEx's within
            your completion routine, AND you do alertable waits in your
            completion routine, you may grow your stack without ever
            trimming it back.

        Arguments:

            dwErrorCode - Supplies the I/O completion status for the
                related I/O.  A value of 0 indicates that the I/O was
                successful.  Note that end of file is indicated by a
                non-zero dwErrorCode value of ERROR_HANDLE_EOF.

            dwNumberOfBytesTransfered - Supplies the number of bytes
                transfered during the associated I/O.  If an error
                occured, a value of 0 is supplied.

            lpOverlapped - Supplies the address of the WSAOVERLAPPED
                structure used to initiate the associated I/O.  The
                hEvent field of this structure is not used by the system
                and may be used by the application to provide additional
                I/O context.  Once a completion routine is called, the
                system will not use the WSAOVERLAPPED structure.  The
                completion routine is free to deallocate the overlapped
                structure.

Arguments:

    ApcContext - Supplies the users completion routine. The format of
        this routine is an LPWSAOVERLAPPED_COMPLETION_ROUTINE.

    IoStatusBlock - Supplies the address of the IoStatusBlock that
        contains the I/O completion status. The IoStatusBlock is
        contained within the WSAOVERLAPPED structure.

    Reserved - Not used; reserved for future use.

Return Value:

    None.

--*/

{

    LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine;
    DWORD dwErrorCode;
    DWORD dwNumberOfBytesTransfered;
    DWORD dwFlags;
    LPWSAOVERLAPPED lpOverlapped;
	PWINSOCK_TLS_DATA	tlsData;
	tlsData = GET_THREAD_DATA ();

    UNREFERENCED_PARAMETER( Reserved);

    dwErrorCode = 0;
    dwFlags = 0;

    __try {
        if( NT_ERROR(IoStatusBlock->Status) ) {

            if (IoStatusBlock->Status != STATUS_CANCELLED) {

                dwErrorCode = SockNtStatusToSocketError(IoStatusBlock->Status);

            } else {

                dwErrorCode = WSA_OPERATION_ABORTED;
            }

            dwNumberOfBytesTransfered = 0;

        } else {

            dwErrorCode = 0;
            dwNumberOfBytesTransfered = (DWORD)IoStatusBlock->Information;

            //
            // Set up the ReceiveFlags output parameter based on the type
            // of receive.
            //

            switch( IoStatusBlock->Status ) {

            case STATUS_BUFFER_OVERFLOW:
                dwErrorCode = WSAEMSGSIZE;
                break;

            case STATUS_RECEIVE_PARTIAL:
                dwFlags = MSG_PARTIAL;
                break;

            case STATUS_RECEIVE_EXPEDITED:
                dwFlags = MSG_OOB;
                break;

            case STATUS_RECEIVE_PARTIAL_EXPEDITED:
                dwFlags = MSG_PARTIAL | MSG_OOB;
                break;

            }

        }
    }
    __except (SOCK_EXCEPTION_FILTER()) {
        dwErrorCode = WSAEFAULT;
        dwNumberOfBytesTransfered = 0;
        dwFlags = 0;
    }

    CompletionRoutine = (LPWSAOVERLAPPED_COMPLETION_ROUTINE)ApcContext;

    lpOverlapped = (LPWSAOVERLAPPED)CONTAINING_RECORD(
                        IoStatusBlock,
                        WSAOVERLAPPED,
                        Internal
                        );

    (CompletionRoutine)(
        dwErrorCode,
        dwNumberOfBytesTransfered,
        lpOverlapped,
        dwFlags
        );

    tlsData->PendingAPCCount--;
    InterlockedDecrement( &SockProcessPendingAPCCount );

} // SockIoCompletion

BOOL
SockThreadInitialize(
    VOID
    )
{

    PWINSOCK_TLS_DATA tlsData;
    HANDLE threadEvent;
    NTSTATUS status;

    IF_DEBUG(INIT) {
        WS_PRINT(( "SockThreadInitialize: TEB = %lx\n",
                       NtCurrentTeb( ) ));
    }

    //
    // Create the thread's event.
    //

    status = NtCreateEvent(
                 &threadEvent,
                 EVENT_ALL_ACCESS,
                 NULL,
                 NotificationEvent,
                 FALSE
                 );
    if ( !NT_SUCCESS(status) ) {
        WS_PRINT(( "SockThreadInitialize: NtCreateEvent failed: %X\n", status ));
        return FALSE;
    }

    //
    // Allocate space for per-thread data the DLL will have.
    //

    tlsData = ALLOCATE_THREAD_DATA( sizeof(*tlsData) );
    if ( tlsData == NULL ) {
        WS_PRINT(( "SockThreadInitialize: unable to allocate thread data.\n" ));
        return FALSE;
    }

    //
    // Store a pointer to this data area in TLS.
    //

    if( !SET_THREAD_DATA(tlsData) ) {

        WS_PRINT(( "SockThreadInitialize: TlsSetValue failed: %ld\n", GetLastError( ) ));
#if !defined(USE_TEB_FIELD)
        SockTlsSlot = 0xFFFFFFFF;
#endif  // !USE_TEB_FIELD
        //
        // Should we free the TLS data here (e.g.: FREE_THREAD_DATA(tlsData))?
        // If TlsSetValue failed, we are in big trouble anyway since this can
        // only occur if someone hajacked and freed our TLS slot.  In that
        // case we leak all the thread slots.  One more won't make a difference.
        //
        //
        return FALSE;
    }

    //
    // Initialize the thread data.
    //

    RtlZeroMemory( tlsData, sizeof(*tlsData) );

#if DBG
    tlsData->IndentLevel = 0;
#endif
    tlsData->SocketHandle = INVALID_SOCKET;
    tlsData->EventHandle = threadEvent;

#ifdef _AFD_SAN_SWITCH_
	tlsData->UpcallLevel = 0;
	tlsData->UpcallOvListHead = NULL;
	tlsData->UpcallOvListTail = NULL;
#endif //_AFD_SAN_SWITCH_
	
    return TRUE;

}   // SockThreadInitialize


INT
SockIsAddressConsistentWithConstrainedGroup(
    IN PSOCKET_INFORMATION Socket,
    IN GROUP Group,
    IN PSOCKADDR SocketAddress,
    IN INT SocketAddressLength
    )

/*++

Routine Description:

    Searches all open sockets, validating that the specified address is
    consistent with all sockets associated to the specified constrained
    group identifier.

Arguments:

    Socket - An open socket handle. Used just to get use "into" AFD
        where the real work is done.

    Group - The constrained group identifier.

    SocketAddress - The socket address to check.

    SocketAddressLength - The length of SocketAddress.

Return Value:

    INT - NO_ERROR if the address is consistent, Winsock error otherwise.

--*/

{

    INT err;
    NTSTATUS status;
	PWINSOCK_TLS_DATA	tlsData;
    IO_STATUS_BLOCK ioStatusBlock;
    PAFD_VALIDATE_GROUP_INFO validateInfo;
    ULONG validateInfoLength;
    UCHAR validateInfoBuffer[sizeof(AFD_VALIDATE_GROUP_INFO) + MAX_FAST_TDI_ADDRESS];

	tlsData = GET_THREAD_DATA ();
    WS_ASSERT( Socket != NULL );
    WS_ASSERT( Group != 0 );
    WS_ASSERT( Group != SG_UNCONSTRAINED_GROUP );
    WS_ASSERT( Group != SG_CONSTRAINED_GROUP );
    WS_ASSERT( SocketAddress != NULL );
    WS_ASSERT( SocketAddressLength > 0 );

    //
    // Allocate enough space to hold the TDI address structure we'll pass
    // to AFD.  Note that is the address is small enough, we just use
    // an automatic in order to improve performance.
    //

    validateInfo = (PAFD_VALIDATE_GROUP_INFO)validateInfoBuffer;

    validateInfoLength = sizeof(AFD_VALIDATE_GROUP_INFO) -
                             sizeof(TRANSPORT_ADDRESS) +
                             Socket->HelperDll->MaxTdiAddressLength;

    if( validateInfoLength > sizeof(validateInfoBuffer) ) {

        validateInfo = ALLOCATE_HEAP( validateInfoLength );

        if( validateInfo == NULL ) {

            return WSAENOBUFS;

        }

    }

    //
    // Convert the address from the sockaddr structure to the appropriate
    // TDI structure.
    //

    err = SockBuildTdiAddress(
        &validateInfo->RemoteAddress,
        SocketAddress,
        SocketAddressLength
        );

    if (err!=NO_ERROR)
        return err;

    //
    // Let AFD do the dirty work.
    //

    validateInfo->GroupID = (LONG)Group;

    status = NtDeviceIoControlFile(
                 Socket->HContext.Handle,
                 tlsData->EventHandle,
                 NULL,                  // APC Routine
                 NULL,                  // APC Context
                 &ioStatusBlock,
                 IOCTL_AFD_VALIDATE_GROUP,
                 validateInfo,
                 validateInfoLength,
                 NULL,
                 0
                 );

    if( status == STATUS_PENDING ) {
        SockWaitForSingleObject(
            tlsData->EventHandle,
            Socket->Handle,
            SOCK_NEVER_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        status = ioStatusBlock.Status;
    }

    if( validateInfo != (PAFD_VALIDATE_GROUP_INFO)validateInfoBuffer ) {

        FREE_HEAP( validateInfo );

    }

    if( NT_SUCCESS(status) ) {

        return NO_ERROR;

    }


    return SockNtStatusToSocketError (status);

}   // SockIsAddressConsistentWithConstrainedGroup


VOID
SockCancelIo(
    IN SOCKET Socket
    )

/*++

Routine Description:

    Cancels all IO on the specific socket initiated by the current thread.

Arguments:

    Socket - The socket to cancel.

Return Value:

    None.

--*/

{

    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS status;

    WS_ASSERT( Socket != (SOCKET)NULL );
    WS_ASSERT( Socket != INVALID_SOCKET );

    status = NtCancelIoFile(
                 (HANDLE)Socket,
                 &ioStatusBlock
                 );

    WS_ASSERT( status != STATUS_PENDING );

    if( !NT_SUCCESS(status) || !NT_SUCCESS(ioStatusBlock.Status) ) {

        WS_PRINT((
            "SockCancelIo: NtCancelIoFile() failed, %08lx:%08lx\n",
            status,
            ioStatusBlock.Status
            ));

    }

}   // SockCancelIo

INT
SockpGetProtocolInfoArray (
    VOID
    )
{
    DWORD               sz;
    INT                 err,res;
    LPWSAPROTOCOL_INFOW info;

    sz = 0;
    res = WSCEnumProtocols (NULL, NULL, &sz, &err);
    if ((res==SOCKET_ERROR) && (err==WSAENOBUFS)) {
        info = ALLOCATE_HEAP (sz);
        if (info!=NULL) {
            res = WSCEnumProtocols (NULL, info, &sz, &err);
            if (res!=SOCKET_ERROR) {
                err = 0;

                WS_ASSERT (res>0);
                
                if (SockProtocolInfoArray!=NULL) {
                    WS_ASSERT (SockProtocolInfoCount>0);
                    FREE_HEAP (SockProtocolInfoArray);
                }

                SockProtocolInfoArray = info;
                SockProtocolInfoCount = res;

            }
            else {
                FREE_HEAP (info);
                WS_ASSERT (err!=NO_ERROR);
            }
        }
        else {
            err = WSA_NOT_ENOUGH_MEMORY;
        }
    }
    else {
        if (res!=SOCKET_ERROR) {
            err = WSASYSCALLFAILURE;
        }
        else {
            WS_ASSERT (err!=NO_ERROR);
        }
    }

    return err;
}



INT
SockBuildProtocolInfoForSocket(
    IN PSOCKET_INFORMATION Socket,
    OUT LPWSAPROTOCOL_INFOW ProtocolInfo
    )
{
    INT     i, err;
    BOOL    resourceShared = TRUE;

    WS_ASSERT( Socket != NULL );
    WS_ASSERT( ProtocolInfo != NULL );

    SockAcquireRwLockShared (&SocketGlobalLock);

    do {
        for (i=0; i<SockProtocolInfoCount; i++) {
            if (SockProtocolInfoArray[i].dwCatalogEntryId==Socket->CatalogEntryId) {
                __try {
                    *ProtocolInfo = SockProtocolInfoArray[i];
                    err = NO_ERROR;
                }
                __except (SOCK_EXCEPTION_FILTER()) {
                    err = WSAEFAULT;
                }
                if (resourceShared) {
                    SockReleaseRwLockShared (&SocketGlobalLock);
                }
                else {
                    SockReleaseRwLockExclusive (&SocketGlobalLock);
                }
                return err;
            }
        }
        if (resourceShared) {
            SockReleaseRwLockShared (&SocketGlobalLock);
            resourceShared = FALSE;
	        SockAcquireRwLockExclusive (&SocketGlobalLock);
            err = SockpGetProtocolInfoArray ();
            if (err==NO_ERROR) {
                continue;
            }
        }
        else
            err = WSASYSCALLFAILURE;
        break;
    }
    while (1);

    WS_ASSERT (resourceShared==FALSE);
    WS_ASSERT (err!=NO_ERROR);
    SockReleaseRwLockExclusive (&SocketGlobalLock);

    return err;
}   // SockBuildProtocolInfoForSocket


//
// Read-Write lock routines
// Allow for recursive acquires by writers and readers.
// Do not allow for changing reader lock to writer.
//
// Lock (critical section) provides mutual exclusion for writers
// ReaderCount reflects the state of the lock:
//      0 - lock is unowned
//      >0 - lock is owned by one or more readers
//      <0 - lock is owned by a writer or writer is waiting to own it
//             (the value reflects the number of recursive acquires + 1 in the
//              first case or number of remaining readers + 2 in the second).
// WriterWaitEvent is used to unwait the writer by the last
//              reader who owned the lock.

NTSTATUS
SockInitializeRwLockAndSpinCount (
    PSOCK_RW_LOCK   Lock,
    ULONG           SpinCount
    )
/*++

Routine Description:

    Initialize socket RW lock.

Arguments:

    Lock    - pointer to SOCK_RW_LOCK structure.

Return Value:

    Status of the operation (event allocation or undelying critical section
    initialization status).

--*/
{
    NTSTATUS    status;

    if (SpinCount & 0x80000000) { 
        status = NtCreateEvent (&Lock->WriterWaitEvent,
                        EVENT_ALL_ACCESS,
                        NULL,
                        SynchronizationEvent,
                        FALSE
                        );
        if (!NT_SUCCESS (status)) {
            return status;
        }
    }

    status = RtlInitializeCriticalSectionAndSpinCount (&Lock->Lock, SpinCount);
    if (NT_SUCCESS (status)) {
        Lock->ReaderCount = 0;
#if DBG
        Lock->WriterId = NULL;
#endif
#if DEBUG_LOCKS
        Lock->DebugInfo = RtlAllocateHeap (RtlProcessHeap (),
                                            0, 
                                            sizeof (SOCK_RW_LOCK_DEBUG_INFO)*MAX_RW_LOCK_DEBUG);
        Lock->DebugInfoIdx = -1;
#endif
    }
    else if (Lock->WriterWaitEvent!=NULL) {
        (VOID)NtClose (Lock->WriterWaitEvent);
        Lock->WriterWaitEvent = NULL;
    }

    return status;
}

NTSTATUS
SockDeleteRwLock (
    PSOCK_RW_LOCK   Lock
    )
/*++

Routine Description:

    Release resources used by socket RW lock.

Arguments:

    Lock    - pointer to initialized SOCK_RW_LOCK structure.

Return Value:

    Status of the operation.

--*/
{
    if (Lock->WriterWaitEvent!=NULL) {
#if DBG
        NTSTATUS    status =
#else
        (VOID)
#endif
        NtClose (Lock->WriterWaitEvent);
        WS_ASSERT (NT_SUCCESS (status));
        Lock->WriterWaitEvent = NULL;
    }

#if DEBUG_LOCKS
    if (Lock->DebugInfo!=NULL) {
        RtlFreeHeap (RtlProcessHeap (), 0, Lock->DebugInfo);
    }
#endif
    return RtlDeleteCriticalSection (&Lock->Lock);
}

VOID
SockAcquireRwLockShared(
    PSOCK_RW_LOCK   Lock
    )
/*++

Routine Description:

    Acquire socket RW lock in shared mode.
    Allows for recursive acquire if lock is already owned shared or
    exclusive by the current thread.

Arguments:

    Lock    - pointer to initialized SOCK_RW_LOCK structure.

Return Value:

    None.

--*/
{
    BOOL    lockAcquired = FALSE;
    LONG    count; 
    LONG    newCount;

    do {
        count = Lock->ReaderCount;
        if (count<0) {
            //
            // Writer is active, wait for it by acquiring
            // critical section. This will also prevent us
            // from starvation if another writer comes after us.
            //

            EnterCriticalSection( &Lock->Lock );
            lockAcquired = TRUE;

            //
            // Re-read the counter, it is likely that it has changed
            // while we were waiting for critical section
            //
            count = Lock->ReaderCount;
            
            if (count<0) {
                //
                // This is a recursive acquire while holding writer
                // lock in the same thread.
                //
                WS_ASSERT (count<-1);
                WS_ASSERT (Lock->WriterId==NtCurrentTeb ()->ClientId.UniqueThread);
                newCount = count - 1;
            }
            else {
                //
                // Normal acquire, writer is gone.
                //
                newCount = count + 1;
            }
        }
        else {
            //
            // No active writers, get it.
            newCount = count + 1;
        }

        //
        // Change reader count.
        //
        newCount = InterlockedCompareExchange ((LPLONG)&Lock->ReaderCount,
                                            newCount,
                                            count);
        //
        // Release critical section if we acquired it
        //
        if (lockAcquired) {
            LeaveCriticalSection (&Lock->Lock);
            lockAcquired = FALSE;
        }

    }
    while (newCount!=count);
#if DEBUG_LOCKS
    if (Lock->DebugInfo!=NULL) {
        INT idx = InterlockedIncrement (&Lock->DebugInfoIdx) % MAX_RW_LOCK_DEBUG;
        Lock->DebugInfo[idx].Operation = RW_LOCK_READ_ACQUIRE;
        Lock->DebugInfo[idx].Count = newCount;
        Lock->DebugInfo[idx].ThreadId = NtCurrentTeb ()->ClientId.UniqueThread;
        RtlGetCallersAddress (&Lock->DebugInfo[idx].Caller,
                                &Lock->DebugInfo[idx].CallersCaller);
    }
#endif
} // SockAcquireRwLockShared

VOID
SockReleaseRwLockShared(
    PSOCK_RW_LOCK   Lock
    )
/*++

Routine Description:

    Release shared ownership of the socket RW lock.

Arguments:

    Lock    - pointer to initialized SOCK_RW_LOCK structure previously
                acquired in shared mode

Return Value:

    None.

--*/
{
    while (1) {
        LONG    count = Lock->ReaderCount;
        LONG    newCount;

        if (count>0) {
            //
            // No writers active, decrement the count
            //
            newCount = count-1;
        }
        else {
            WS_ASSERT (count!=0);
            //
            // Writer is active, increment count.
            //
            newCount = count+1;
        }

        if (InterlockedCompareExchange ((LPLONG)&Lock->ReaderCount,
                                            newCount,
                                            count)==count) {
#if DEBUG_LOCKS
            if (Lock->DebugInfo!=NULL) {
                INT idx = InterlockedIncrement (&Lock->DebugInfoIdx) % MAX_RW_LOCK_DEBUG;
                Lock->DebugInfo[idx].Operation = RW_LOCK_READ_RELEASE;
                Lock->DebugInfo[idx].Count = newCount;
                Lock->DebugInfo[idx].ThreadId = NtCurrentTeb ()->ClientId.UniqueThread;
                RtlGetCallersAddress (&Lock->DebugInfo[idx].Caller,
                                        &Lock->DebugInfo[idx].CallersCaller);
            }
#endif
            if (newCount==-1) {
                //
                // This is the last reader, signal to waiting writer.
                //
#if DBG
                NTSTATUS    status;
                LONG        prevState;
                status = NtSetEvent (Lock->WriterWaitEvent, &prevState);
                WS_ASSERT (NT_SUCCESS (status));
                WS_ASSERT (prevState==0);
#else
                (VOID)NtSetEvent (Lock->WriterWaitEvent, NULL);
#endif
            }
            break;
        }
    }
}

VOID
SockpWaitForReaders (
    PSOCK_RW_LOCK   Lock
    )
/*++

Routine Description:

    Wait for readers to release the lock.

Arguments:

    Lock    - pointer to initialized SOCK_RW_LOCK structure 

Return Value:

    None.
    This routine raises an exception if it cannot allocate
    event or wait on event fails.

NOTE:
    Lock's critical section must be held when calling this routine.


--*/
{
    NTSTATUS    status;
    LARGE_INTEGER   Wait0;

    //
    // First see if context switch does the job.
    //

    Wait0.QuadPart = 0;
    status = NtDelayExecution (FALSE, &Wait0);
    if (Lock->ReaderCount==-2) {
        return;
    }

    //
    // We must be dealing with stuburn reader or priority inversion
    // problem.
    //

    if (Lock->WriterWaitEvent==NULL) {
        //
        // This is the first time here, need to allocate event.
        //
        status = NtCreateEvent (&Lock->WriterWaitEvent,
                                    EVENT_ALL_ACCESS,
                                    NULL,
                                    SynchronizationEvent,
                                    FALSE);
        if (!NT_SUCCESS (status)) {
            LARGE_INTEGER   Wait10ms;

            //
            // Failed to allocate an event, loop using simple sleep.
            // Use non-0 timeout to let even lower priority threads run.
            //
            Wait10ms.QuadPart = -10*1000*10;
            while (Lock->ReaderCount!=-2) {
                WS_ASSERT (Lock->ReaderCount<=-2);
                NtDelayExecution (FALSE, &Wait10ms);
            }
        }
    }

    //
    // Some readers are still in, tell them we need
    // a signal and wait.
    //
    if (InterlockedIncrement ((PLONG)&Lock->ReaderCount)!=-1) {
        WS_ASSERT (Lock->ReaderCount<=-1);
        WS_ASSERT (Lock->WriterWaitEvent!=NULL);
        status = NtWaitForSingleObject (Lock->WriterWaitEvent, FALSE, NULL);
        if (!NT_SUCCESS (status)) {
            RtlRaiseStatus (status);
        }
    }
    WS_ASSERT (Lock->ReaderCount==-1);
    Lock->ReaderCount = -2;
}

VOID
SockAcquireRwLockExclusive(
    PSOCK_RW_LOCK   Lock
    )
/*++

Routine Description:

    Acquire socket RW lock exclusively.
    Allows for recursive acquire if lock is already owned exclusively
    by the current thread.

Arguments:

    Lock    - pointer to initialized SOCK_RW_LOCK structure.

Return Value:

    None.
    This routine raises an exception if it cannot allocate
    event or wait on event fails.

--*/
{
    LONG    count, newCount;

    //
    // Guarantee mutual exclusion
    //
    EnterCriticalSection( &Lock->Lock );

    if (Lock->ReaderCount>=0) {
        //
        // Lock is not owned or owned in shared mode
        //
        WS_ASSERT (Lock->WriterId==NULL);
#if DBG
        Lock->WriterId = NtCurrentTeb()->ClientId.UniqueThread;
#endif
        do {
            count = Lock->ReaderCount;
            WS_ASSERT (count>=0);
            //
            // Negate the count to indicate that writer is waiting.
            // Readers expect the counter to be negative when writer is 
            // waiting, hence -1 to cover the case of 0 readers and -1 
            // to have the room to decrement before reaching 0.
            //
            newCount = -count-2;

        }
        while (InterlockedCompareExchange ((LPLONG)&Lock->ReaderCount,
                                                newCount,
                                                count)!=count);

        if (newCount!=-2) {
            //
            // Not all readers exited, need to wait.
            //
            ULONG_PTR   spinCount = Lock->Lock.SpinCount;
            while (Lock->ReaderCount!=-2) {
                WS_ASSERT (Lock->ReaderCount<=-2);
                if (spinCount>0) {
                    //
                    // Spin if we have set spin count.
                    //
                    spinCount--;
                }
                else {
                    //
                    // Some readers are still in, use the slow path.
                    //
                    SockpWaitForReaders (Lock);
                    break;
                }
            }
        }

        WS_ASSERT (Lock->ReaderCount==-2);
    }
    else {
        //
        // Recursive acquire.  Make sure we already own it.
        //
        WS_ASSERT (Lock->ReaderCount<-1);
        WS_ASSERT (Lock->WriterId==NtCurrentTeb()->ClientId.UniqueThread);
        //
        // Decrement to account for recursive acquires
        //
        Lock->ReaderCount -= 1;
    }


#if DEBUG_LOCKS
    if (Lock->DebugInfo!=NULL) {
        INT idx = InterlockedIncrement (&Lock->DebugInfoIdx) % MAX_RW_LOCK_DEBUG;
        Lock->DebugInfo[idx].Operation = RW_LOCK_WRITE_ACQUIRE;
        Lock->DebugInfo[idx].Count = Lock->ReaderCount;
        Lock->DebugInfo[idx].ThreadId = NtCurrentTeb ()->ClientId.UniqueThread;
        RtlGetCallersAddress (&Lock->DebugInfo[idx].Caller,
                                &Lock->DebugInfo[idx].CallersCaller);
    }
#endif
} // SockAcquireRwLockExclusive

VOID
SockReleaseRwLockExclusive(
    PSOCK_RW_LOCK   Lock
    )
/*++

Routine Description:

    Release exclusive ownership of the socket RW lock.

Arguments:

    Lock    - pointer to initialized SOCK_RW_LOCK structure previously
                acquired in shared mode

Return Value:

    None.

--*/
{
    WS_ASSERT (Lock->WriterId==NtCurrentTeb()->ClientId.UniqueThread);
	WS_ASSERT (Lock->ReaderCount<-1);
#if DEBUG_LOCKS
    if (Lock->DebugInfo!=NULL) {
        INT idx = InterlockedIncrement (&Lock->DebugInfoIdx) % MAX_RW_LOCK_DEBUG;
        Lock->DebugInfo[idx].Operation = RW_LOCK_WRITE_RELEASE;
        Lock->DebugInfo[idx].Count = Lock->ReaderCount;
        Lock->DebugInfo[idx].ThreadId = NtCurrentTeb ()->ClientId.UniqueThread;
        RtlGetCallersAddress (&Lock->DebugInfo[idx].Caller,
                                &Lock->DebugInfo[idx].CallersCaller);
    }
#endif
    if (++Lock->ReaderCount==-1) {
        //
        // Last recursive lock is being released
        // Indicate that lock is no longer owned.
        //
#if DBG
        Lock->WriterId = NULL;
#endif
        Lock->ReaderCount = 0;
    }

    //
    // Release the mutual exclusion (critical section maintains its
    // own recursion count).
    //
    LeaveCriticalSection( &Lock->Lock );
} // SockReleaseRwLockExclusive

