/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Wsraw.h

Abstract:

    Support for extended winsock calls for WOW.

Author:

    David Treadwell (davidtr)    02-Oct-1992

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "wsdynmc.h"

DLLENTRYPOINTS  wsockapis[WOW_WSOCKAPI_COUNT] = {
                        (char *) 1,     NULL,
                        (char *) 2,     NULL,
                        (char *) 3,     NULL,
                        (char *) 4,     NULL,
                        (char *) 5,     NULL,
                        (char *) 6,     NULL,
                        (char *) 7,     NULL,
                        (char *) 8,     NULL,
                        (char *) 9,     NULL,
                        (char *) 10,    NULL,
                        (char *) 11,    NULL,
                        (char *) 12,    NULL,
                        (char *) 13,    NULL,
                        (char *) 14,    NULL,
                        (char *) 15,    NULL,
                        (char *) 16,    NULL,
                        (char *) 17,    NULL,
                        (char *) 18,    NULL,
                        (char *) 19,    NULL,
                        (char *) 20,    NULL,
                        (char *) 21,    NULL,
                        (char *) 22,    NULL,
                        (char *) 23,    NULL,
                        (char *) 51,    NULL,
                        (char *) 52,    NULL,
                        (char *) 53,    NULL,
                        (char *) 54,    NULL,
                        (char *) 55,    NULL,
                        (char *) 56,    NULL,
                        (char *) 57,    NULL,
                        (char *) 101,   NULL,
                        (char *) 102,   NULL,
                        (char *) 103,   NULL,
                        (char *) 104,   NULL,
                        (char *) 105,   NULL,
                        (char *) 106,   NULL,
                        (char *) 107,   NULL,
                        (char *) 108,   NULL,
                        (char *) 109,   NULL,
                        (char *) 110,   NULL,
                        (char *) 111,   NULL,
                        (char *) 112,   NULL,
                        (char *) 113,   NULL,
                        (char *) 114,   NULL,
                        (char *) 115,   NULL,
                        (char *) 116,   NULL,
                        (char *) 151,   NULL,
                        (char *) 1000,  NULL,
                        (char *) 1107,  NULL};



DWORD WWS32TlsSlot = 0xFFFFFFFF;
RTL_CRITICAL_SECTION WWS32CriticalSection;
BOOL WWS32Initialized = FALSE;
LIST_ENTRY WWS32AsyncContextBlockListHead;
WORD WWS32AsyncTaskHandleCounter;
DWORD WWS32ThreadSerialNumberCounter;
HINSTANCE   hInstWSOCK32;


DWORD
WWS32CallBackHandler (
    VOID
    );

BOOL
WWS32DefaultBlockingHook (
    VOID
    );

/*++

 GENERIC FUNCTION PROTOTYPE:
 ==========================

ULONG FASTCALL WWS32<function name>(PVDMFRAME pFrame)
{
    ULONG ul;
    register P<function name>16 parg16;

    GETARGPTR(pFrame, sizeof(<function name>16), parg16);

    <get any other required pointers into 16 bit space>

    ALLOCVDMPTR
    GETVDMPTR
    GETMISCPTR
    et cetera

    <copy any complex structures from 16 bit -> 32 bit space>
    <ALWAYS use the FETCHxxx macros>

    ul = GET<return type>16(<function name>(parg16->f1,
                                                :
                                                :
                                            parg16->f<n>);

    <copy any complex structures from 32 -> 16 bit space>
    <ALWAYS use the STORExxx macros>

    <free any pointers to 16 bit space you previously got>

    <flush any areas of 16 bit memory if they were written to>

    FLUSHVDMPTR

    FREEARGPTR(parg16);
    RETURN(ul);
}

NOTE:

  The VDM frame is automatically set up, with all the function parameters
  available via parg16->f<number>.

  Handles must ALWAYS be mapped for 16 -> 32 -> 16 space via the mapping tables
  laid out in WALIAS.C.

  Any storage you allocate must be freed (eventually...).

  Further to that - if a thunk which allocates memory fails in the 32 bit call
  then it must free that memory.

  Also, never update structures in 16 bit land if the 32 bit call fails.

  Be aware that the GETxxxPTR macros return the CURRENT selector-to-flat_memory
  mapping.  Calls to some 32-bit functions may indirectly cause callbacks into
  16-bit code.  These may cause 16-bit memory to move due to allocations
  made in 16-bit land.  If the 16-bit memory does move, the corresponding 32-bit
  ptr in WOW32 needs to be refreshed to reflect the NEW selector-to-flat_memory
  mapping.

--*/

ULONG FASTCALL WWS32WSAAsyncSelect(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWSAASYNCSELECT16 parg16;
    SOCKET s32;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    GETARGPTR(pFrame, sizeof(WSAASYNCSELECT16), parg16);

    //
    // Find the 32-bit socket handle.
    //

    s32 = GetWinsock32( parg16->hSocket );

    if ( s32 == INVALID_SOCKET ) {

        (*wsockapis[WOW_WSASETLASTERROR].lpfn)( WSAENOTSOCK );
        ul = (ULONG)GETWORD16( SOCKET_ERROR );

    } else {

        ul = GETWORD16( (*wsockapis[WOW_WSAASYNCSELECT].lpfn)(
                            s32,
                            (HWND)HWND32(parg16->hWnd),
                            (parg16->wMsg << 16) | WWS32_MESSAGE_ASYNC_SELECT,
                            parg16->lEvent
                            ));
    }

    FREEARGPTR(parg16);

    RETURN(ul);

} // WWS32WSAAsyncSelect

ULONG FASTCALL WWS32WSASetBlockingHook(PVDMFRAME pFrame)
{
    ULONG ul;
    VPWNDPROC  vpBlockFunc;

    //FARPROC previousHook;
    register PWSASETBLOCKINGHOOK16 parg16;

    GETARGPTR(pFrame, sizeof(WSASETBLOCKINGHOOK16), parg16);
    vpBlockFunc = parg16->lpBlockFunc;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)NULL);
    }

    if ( (*wsockapis[WOW_WSAISBLOCKING].lpfn)( ) ) {
        SetLastError( WSAEINPROGRESS );
        RETURN((ULONG)NULL);
    }

    ul = WWS32vBlockingHook;
    WWS32vBlockingHook = vpBlockFunc;

    FREEARGPTR( parg16 );

    RETURN(ul);

} // WWS32WSASetBlockingHook

ULONG FASTCALL WWS32WSAUnhookBlockingHook(PVDMFRAME pFrame)
{

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    if ( (*wsockapis[WOW_WSAISBLOCKING].lpfn)() ) {
        SetLastError( WSAEINPROGRESS );
        RETURN((ULONG)SOCKET_ERROR);
    }

    WWS32vBlockingHook = WWS32_DEFAULT_BLOCKING_HOOK;

    RETURN(0);

} // WWS32WSAUnhookBlockingHook

ULONG FASTCALL WWS32WSAGetLastError(PVDMFRAME pFrame)
{
    ULONG ul;

    ul = GETWORD16( (*wsockapis[WOW_WSAGETLASTERROR].lpfn)( ) );

    RETURN(ul);

} // WWS32WSAGetLastError

ULONG FASTCALL WWS32WSASetLastError(PVDMFRAME pFrame)
{
    register PWSASETLASTERROR16 parg16;

    GETARGPTR(pFrame, sizeof(WSASETLASTERROR16), parg16);

    (*wsockapis[WOW_WSASETLASTERROR].lpfn)( FETCHWORD( parg16->Error ) );

    FREEARGPTR(parg16);

    RETURN(0);

} // WWS32WSASetLastError

ULONG FASTCALL WWS32WSACancelBlockingCall(PVDMFRAME pFrame)
{
    ULONG ul;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    ul = GETWORD16((*wsockapis[WOW_WSACANCELBLOCKINGCALL].lpfn)( ));

    RETURN(ul);

} // WWS32WSACancelBlockingCall

ULONG FASTCALL WWS32WSAIsBlocking(PVDMFRAME pFrame)
{
    ULONG ul;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)FALSE);
    }

    ul = GETWORD16((*wsockapis[WOW_WSAISBLOCKING].lpfn)( ));

    RETURN(ul);

} // WWS32WSAIsBlocking

ULONG FASTCALL WWS32WSAStartup(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    register PWSASTARTUP16 parg16;
    PWSADATA16 wsaData16;
    PWINSOCK_THREAD_DATA data;
    NTSTATUS status;
    FARPROC previousHook;
    PSZ description;
    PSZ systemStatus;
    WORD versionRequested;
    VPWSADATA16 vpwsaData16;

    GETARGPTR(pFrame, sizeof(WSASTARTUP16), parg16);

    vpwsaData16 = parg16->lpWSAData;

    versionRequested = INT32(parg16->wVersionRequired);

    //
    // If winsock has not yet been initialized, initialize data structures
    // now.
    //

    if ( !WWS32Initialized ) {

        WSADATA wsaData;

        InitializeListHead( &WWS32AsyncContextBlockListHead );
        InitializeListHead( &WWS32SocketHandleListHead );

        WWS32AsyncTaskHandleCounter = 1;
        WWS32SocketHandleCounter = 1;
        WWS32SocketHandleCounterWrapped = FALSE;
        WWS32ThreadSerialNumberCounter = 1;

        //
        // Load WSOCK32.DLL and initialize all the entry points.
        //

        if (!LoadLibraryAndGetProcAddresses ("WSOCK32.DLL", wsockapis, WOW_WSOCKAPI_COUNT)) {
            LOGDEBUG (LOG_ALWAYS, ("WOW::WWS32WSAStartup: LoadLibrary failed\n"));
            ul = GETWORD16(WSAENOBUFS);
            return (ul);
        }

        //
        // Initialize the ntvdm process to the 32-bit Windows Sockets
        // DLL.
        //

        ul = (*wsockapis[WOW_WSASTARTUP].lpfn)( MAKEWORD( 1, 1 ), &wsaData );
        if ( ul != NO_ERROR ) {
            RETURN(ul);
        }

        //
        // Initialize the critical section we'll use for synchronizing
        // async requests.
        //

        status = RtlInitializeCriticalSection( &WWS32CriticalSection );
        if ( !NT_SUCCESS(status) ) {
            ul = GETWORD16(WSAENOBUFS);
            RETURN(ul);
        }

        //
        // Get a slot in TLS.
        //

        WWS32TlsSlot = TlsAlloc( );
        if ( WWS32TlsSlot == 0xFFFFFFFF ) {
            RtlDeleteCriticalSection( &WWS32CriticalSection );
            ul = GETWORD16(WSAENOBUFS);
            RETURN(ul);
        }

        WWS32Initialized = TRUE;
    }

    //
    // Make sure that we're not in a blocking call.
    //

    if ( (*wsockapis[WOW_WSAISBLOCKING].lpfn)( ) ) {
        RETURN((ULONG)WSAEINPROGRESS);
    }

    //
    // If this thread has not already had a WSAStartup() call, allocate
    // and initialize per-thread data.
    //

    if ( !WWS32IsThreadInitialized ) {

        //
        // We support versions 1.0 and 1.1 of the Windows Sockets
        // specification.  If the requested version is below that, fail.
        //

        if ( LOBYTE(versionRequested) < 1 ) {
            ul = WSAVERNOTSUPPORTED;
            RETURN(ul);
        }

        //
        // Allocate space for the per-thread data that we need.  Note that
        // we set the value in the TSL slot regardless of whether we actually
        // managed to allocate the memory--this is because we want NULL
        // in the TLS slot if we couldn't properly allocate the storage.
        //

        data = malloc_w( sizeof(*data) );

        if ( !TlsSetValue( WWS32TlsSlot, (LPVOID)data ) || data == NULL ) {

            ul = GETWORD16(WSAENOBUFS);
            if ( data != NULL ) {
                free_w( (PVOID)data );
            }
            FREEARGPTR( parg16 );
            RETURN(ul);
        }

        //
        // Initialize the blocking hook.
        //

        WWS32vBlockingHook = WWS32_DEFAULT_BLOCKING_HOOK;

        //
        // Allocate the individual data objects we need for this task.
        //

        data->vIpAddress = GlobalAllocLock16( GMEM_MOVEABLE, 256, NULL );
        if ( data->vIpAddress == 0 ) {
            free_w( (PVOID)data );
            TlsSetValue( WWS32TlsSlot, NULL );
            FREEARGPTR( parg16 );
            RETURN(ul);
        }

        data->vHostent = GlobalAllocLock16( GMEM_MOVEABLE, MAXGETHOSTSTRUCT, NULL );
        if ( data->vHostent == 0 ) {
            GlobalUnlockFree16( data->vIpAddress );
            free_w( (PVOID)data );
            TlsSetValue( WWS32TlsSlot, NULL );
            FREEARGPTR( parg16 );
            RETURN(ul);
        }

        data->vServent = GlobalAllocLock16( GMEM_MOVEABLE, MAXGETHOSTSTRUCT, NULL );
        if ( data->vServent == 0 ) {
            GlobalUnlockFree16( data->vIpAddress );
            GlobalUnlockFree16( data->vHostent );
            free_w( (PVOID)data );
            TlsSetValue( WWS32TlsSlot, NULL );
            FREEARGPTR( parg16 );
            RETURN(ul);
        }

        data->vProtoent = GlobalAllocLock16( GMEM_MOVEABLE, MAXGETHOSTSTRUCT, NULL );
        if ( data->vProtoent == 0 ) {
            GlobalUnlockFree16( data->vIpAddress );
            GlobalUnlockFree16( data->vHostent );
            GlobalUnlockFree16( data->vServent );
            free_w( (PVOID)data );
            TlsSetValue( WWS32TlsSlot, NULL );
            FREEARGPTR( parg16 );
            RETURN(ul);
        }

        //
        // Initialize other per-thread data.
        //

        WWS32ThreadSerialNumber = WWS32ThreadSerialNumberCounter++;
        WWS32ThreadStartupCount = 1;

        //
        // If they requested version 1.0, give them 1.0.  If they
        // requested anything else (has to be higher than 1.0 due to
        // above test), then give them 1.1.  We only support 1.0
        // and 1.1.  If they can't handle 1.1, they will call
        // WSAStartup() and fail.
        //

        if ( versionRequested == 0x0001 ) {
            WWS32ThreadVersion = 0x0001;
        } else {
            WWS32ThreadVersion = 0x0101;
        }

        //
        // Set up the blocking hook.  We always use this blocking hook,
        // even for the default case.
        //

        previousHook = (FARPROC) (*wsockapis[WOW_WSASETBLOCKINGHOOK].lpfn)( (FARPROC)WWS32CallBackHandler );

        //
        // Set up the routine we'll use in leiu of wsock32.dll posting
        // messages directly to the application.  We need to intervene
        // on all posts because we need to convert 32-bit arguments to
        // 16-bit.
        //

        (*wsockapis[WOW_WSAPSETPOSTROUTINE].lpfn)( WWS32DispatchPostMessage );

    } else {

        //
        // This thread has already had a WSAStartup() call.  Make sure
        // that they're requesting the same version as before.
        //

        if ( versionRequested != WWS32ThreadVersion ) {
            ul = WSAVERNOTSUPPORTED;
            RETURN(ul);
        }

        //
        // Increment the count of WSAStartup() calls for the thread.
        //

        WWS32ThreadStartupCount++;
    }

    //
    // Get a 32-bit pointer to the 16-bit WSADATA structure and
    // initialize the caller's WSAData structure.
    //

    GETVDMPTR( vpwsaData16, sizeof(WSADATA16), wsaData16 );

    STOREWORD( wsaData16->wVersion, WWS32ThreadVersion );
    STOREWORD( wsaData16->wHighVersion, MAKEWORD(1, 1) );

    description = "16-bit Windows Sockets";
    RtlCopyMemory( wsaData16->szDescription,
                   description,
                   strlen(description) + 1 );

    systemStatus = "Running.";
    RtlCopyMemory( wsaData16->szSystemStatus,
                   systemStatus,
                   strlen(systemStatus) + 1 );

    STOREWORD( wsaData16->iMaxSockets, 0xFFFF );
    STOREWORD( wsaData16->iMaxUdpDg, 8096 );
    STOREDWORD( wsaData16->lpVendorInfo, 0 );

    FLUSHVDMPTR( vpwsaData16, sizeof(WSADATA16), wsaData16 );
    FREEVDMPTR( wsaData16 );

    FREEARGPTR( parg16 );

    RETURN(ul);

} // WWS32WSAStartup

ULONG FASTCALL WWS32WSACleanup(PVDMFRAME pFrame)
{
    ULONG ul = 0;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)SOCKET_ERROR);
    }

    if ( (*wsockapis[WOW_WSAISBLOCKING].lpfn)( ) ) {
        SetLastError( WSAEINPROGRESS );
        RETURN((ULONG)SOCKET_ERROR);
    }

    WWS32ThreadStartupCount--;

    if ( WWS32ThreadStartupCount == 0 ) {

        WWS32TaskCleanup( );

    }

    RETURN(ul);

} // WWS32WSACleanup

VOID
WWS32TaskCleanup(
    VOID
    )
{
    LIST_ENTRY listHead;
    PWINSOCK_THREAD_DATA data;
    PLIST_ENTRY listEntry;
    PWINSOCK_SOCKET_INFO socketInfo;
    struct linger lingerInfo;
    int err;

    //
    // Get a pointer to the thread's data and set the TLS slot for
    // this thread to NULL so that we know that the thread is no
    // longer initialized.
    //

    data = TlsGetValue( WWS32TlsSlot );
    ASSERT( data != NULL );

    TlsSetValue( WWS32TlsSlot, NULL );

    //
    // Free thread data user for the database calls.
    //

    GlobalUnlockFree16( data->vIpAddress );
    GlobalUnlockFree16( data->vHostent );
    GlobalUnlockFree16( data->vServent );
    GlobalUnlockFree16( data->vProtoent );

    //
    // Close all sockets that the thread has opened.  We first find
    // all the sockets for this thread, remove them from the global
    // list, and place them onto a local list.  Then we close each
    // socket.  We do this as two steps because we can't hold the
    // critical section while calling wsock32 in order to avoid
    // deadlocks.
    //

    RtlEnterCriticalSection( &WWS32CriticalSection );

    InitializeListHead( &listHead );

    for ( listEntry = WWS32SocketHandleListHead.Flink;
          listEntry != &WWS32SocketHandleListHead;
          listEntry = listEntry->Flink ) {

        socketInfo = CONTAINING_RECORD(
                         listEntry,
                         WINSOCK_SOCKET_INFO,
                         GlobalSocketListEntry
                         );

        if ( socketInfo->ThreadSerialNumber == data->ThreadSerialNumber ) {

            //
            // The socket was opened by this thread.  We need to
            // first remove the entry from the global list, but
            // maintain the listEntry local variable so that we can
            // still walk the list.
            //

            listEntry = socketInfo->GlobalSocketListEntry.Blink;
            RemoveEntryList( &socketInfo->GlobalSocketListEntry );

            //
            // Now insert the entry on our local list.
            //

            InsertTailList( &listHead, &socketInfo->GlobalSocketListEntry );
        }
    }

    RtlLeaveCriticalSection( &WWS32CriticalSection );

    //
    // Walk through the sockets opened by this thread and close them
    // abortively.
    //

    for ( listEntry = listHead.Flink;
          listEntry != &listHead;
          listEntry = listEntry->Flink ) {

        //
        // Close it abortively and free the handle.
        //

        socketInfo = CONTAINING_RECORD(
                         listEntry,
                         WINSOCK_SOCKET_INFO,
                         GlobalSocketListEntry
                         );

        lingerInfo.l_onoff = 1;
        lingerInfo.l_linger = 0;

        err = (*wsockapis[WOW_SETSOCKOPT].lpfn)(
                  socketInfo->SocketHandle32,
                  SOL_SOCKET,
                  SO_LINGER,
                  (char *)&lingerInfo,
                  sizeof(lingerInfo)
                  );
        //ASSERT( err == NO_ERROR );

        err = (*wsockapis[WOW_CLOSESOCKET].lpfn)( socketInfo->SocketHandle32 );
        ASSERT( err == NO_ERROR );

        //
        // When we free the handle the socketInfo structure will
        // also be freed.  Set the list pointer to the entry
        // prior to this one so that we can successfully walk
        // the list.
        //

        listEntry = socketInfo->GlobalSocketListEntry.Blink;

        RemoveEntryList( &socketInfo->GlobalSocketListEntry );
        free_w( (PVOID)socketInfo );
    }

    //
    // Set the TLS slot for this thread to NULL so that we know
    // that the thread is not initialized.
    //

    err = TlsSetValue( WWS32TlsSlot, NULL );
    ASSERT( err );

    //
    // Free the structure that holds thread information.
    //

    free_w( (PVOID)data );

} // WWS32TaskCleanup

ULONG FASTCALL WWS32__WSAFDIsSet(PVDMFRAME pFrame)
{
    ULONG ul;
    register P__WSAFDISSET16 parg16;
    PFD_SET16 fdSet16;
    PFD_SET fdSet32;

    if ( !WWS32IsThreadInitialized ) {
        SetLastError( WSANOTINITIALISED );
        RETURN((ULONG)FALSE);
    }

    GETARGPTR( pFrame, sizeof(__WSAFDISSET16), parg16 );

    GETVDMPTR( parg16->Set, sizeof(FD_SET16), fdSet16 );

    fdSet32 = AllocateFdSet32( fdSet16 );

    if ( fdSet32 != NULL ) {

        ConvertFdSet16To32( fdSet16, fdSet32 );

        ul = (*wsockapis[WOW_WSAFDISSET].lpfn)( GetWinsock32( parg16->hSocket ), fdSet32 );

        free_w( (PVOID)fdSet32 );

    } else {

        ul = 0;
    }

    FREEARGPTR(parg16);

    RETURN(ul);

} // WWS32__WSAFDIsSet


PWINSOCK_POST_ROUTINE WWS32PostDispatchTable[] =
{
    WWS32PostAsyncSelect,
    WWS32PostAsyncGetHost,
    WWS32PostAsyncGetProto,
    WWS32PostAsyncGetServ
};

BOOL
WWS32DispatchPostMessage (
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
{

    ASSERT( WWS32PostDispatchTable[WWS32_MESSAGE_ASYNC_SELECT] ==
                WWS32PostAsyncSelect );
    ASSERT( WWS32PostDispatchTable[WWS32_MESSAGE_ASYNC_GETHOST] ==
                WWS32PostAsyncGetHost );
    ASSERT( WWS32PostDispatchTable[WWS32_MESSAGE_ASYNC_GETPROTO] ==
                WWS32PostAsyncGetProto );
    ASSERT( WWS32PostDispatchTable[WWS32_MESSAGE_ASYNC_GETSERV] ==
                WWS32PostAsyncGetServ );
    ASSERT( (Msg & 0xFFFF) <= WWS32_MESSAGE_ASYNC_GETSERV );

    //
    // Call the routine that will handle the message.  The low word
    // of Msg specifies the routine, the high word of Msg is the
    // 16-bit message that that routine will post.
    //

    return WWS32PostDispatchTable[Msg & 0xFFFF](
               hWnd,
               Msg,
               wParam,
               lParam
               );

} // WWS32DispatchPostMessage


BOOL
WWS32PostAsyncSelect (
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
{

    HAND16 h16;

    h16 = GetWinsock16( wParam, 0 );

    if( h16 == 0 ) {
        return TRUE;
    }

    return PostMessage(
               hWnd,
               Msg >> 16,
               h16,
               lParam
               );

} // WWS32PostAsyncSelect


DWORD
WWS32CallBackHandler (
    VOID
    )
{

    VPVOID ret;

    //
    // If the default blocking hook is in force, use it.  Otherwise,
    // call back into the application's blocking hook.
    //

    if ( WWS32vBlockingHook == WWS32_DEFAULT_BLOCKING_HOOK ) {
        return WWS32DefaultBlockingHook( );
    }

    (VOID)CallBack16( RET_WINSOCKBLOCKHOOK, NULL, WWS32vBlockingHook, &ret );

    return ret & 0xFF;

} // WWS32CallBackHandler


BOOL
WWS32DefaultBlockingHook (
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
        TranslateMessage( (CONST MSG *)&msg );
        DispatchMessage( (CONST MSG *)&msg );
    }

    //
    // If we got a message, indicate that we want to be called again.
    //

    return retrievedMessage;

} // WWS32DefaultBlockingHook
